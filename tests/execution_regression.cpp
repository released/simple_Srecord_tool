#include "../src/core/conversion_job.h"
#include "../src/core/process_runner.h"
#include "../src/core/text_utils.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <io.h>

namespace fs = std::filesystem;
using namespace srecord_tool::core;

namespace {

void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Write(const fs::path& path, const std::string& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(bytes.data(), std::streamsize(bytes.size()));
    file.close();
    Require(bool(file), "Fixture write failed");
}
std::string Read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    Require(bool(file), "Fixture read failed");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
std::string Encode(const std::vector<std::wstring>& values) {
    std::ostringstream text;
    for (const auto& value : values) {
        text << value.size() << ':';
        for (wchar_t ch : value) text << std::hex << std::setw(4) << std::setfill('0') << unsigned(ch);
        text << '\n' << std::dec;
    }
    return text.str();
}
bool WaitForFile(const fs::path& file) {
    for (int i = 0; i < 200; ++i) {
        if (fs::exists(file)) return true;
        Sleep(20);
    }
    return false;
}
ConversionPlan Plan(const SRecordOptions& options) {
    ConversionPlan plan;
    std::wstring error;
    if (!BuildConversionPlan(options, &plan, &error)) {
        std::wcerr << error << '\n';
        throw std::runtime_error("Plan preparation failed");
    }
    return plan;
}
ConversionResult Execute(const SRecordOptions& options) {
    std::atomic_bool cancel{false};
    auto result = ExecuteConversion(Plan(options), cancel);
    if (result.outcome != ConversionOutcome::Succeeded) std::wcerr << result.error << '\n';
    return result;
}
void NoStages(const fs::path& root) {
    for (const auto& item : fs::directory_iterator(root))
        Require(item.path().filename().wstring().rfind(L".srecord-", 0) != 0, "Staging file leaked");
}

int Child(int argc, wchar_t** argv) {
    if (std::wstring(argv[1]) == L"--echo") {
        _setmode(_fileno(stdout), _O_BINARY);
        std::cout << Encode({argv + 2, argv + argc});
        return 0;
    }
    if (std::wstring(argv[1]) == L"--linger") { Sleep(60000); return 0; }
    if (std::wstring(argv[1]) == L"--closed-output") {
        CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
        CloseHandle(GetStdHandle(STD_ERROR_HANDLE));
        Sleep(150);
        return 23;
    }
    if (std::wstring(argv[1]) == L"--flood") {
        for (int i = 0; i < 1024; ++i) std::cout << std::string(1024, 'X');
        return 0;
    }
    if (std::wstring(argv[1]) == L"--tree") {
        auto command = JoinCommandLine({argv[0], L"--linger"});
        STARTUPINFOW startup = {}; startup.cb = sizeof(startup);
        PROCESS_INFORMATION process = {};
        if (!CreateProcessW(argv[0], command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, nullptr, &startup, &process)) return 2;
        Write(argv[2], std::to_string(process.dwProcessId));
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
        Sleep(60000);
        return 0;
    }
    // Synthetic native engine: only test-owned inputs select failure/wait behavior.
    const std::wstring input = argv[1];
    for (int i = 2; i + 1 < argc; ++i) {
        if (std::wstring(argv[i]) != L"-o") continue;
        if (std::wstring(argv[i + 1]) == L"NUL") return 0;
        if (input.find(L"empty.bin") == std::wstring::npos) Write(argv[i + 1], "staged-payload");
        if (input.find(L"wait.bin") != std::wstring::npos) {
            Write(input + L".ready", "ready"); Sleep(60000);
        }
        if (i + 2 < argc && std::wstring(argv[i + 2]) == L"-Intel") return 17;
        return 0;
    }
    return 3;
}

void CheckProcess(const std::wstring& self, const fs::path& root) {
    std::vector<std::wstring> literals = {L"", L"a b", L"trailing space\\", L"a\\\"b", L"\\\\",
        L"& | > < ^ ( ) %PATH% !literal!", L"\x6e2c\x8a66", L"-option", L"@file"};
    std::vector<std::wstring> args = {self, L"--echo"};
    args.insert(args.end(), literals.begin(), literals.end());
    ProcessResult result;
    std::wstring error;
    Require(RunProcessCapture(args, root.wstring(), &result, &error) && result.exit_code == 0, "Literal child failed");
    const auto expected = Encode(literals);
    if (result.output != std::wstring(expected.begin(), expected.end())) {
        std::cerr << "Expected argv encoding:\n" << expected;
        std::wcerr << L"Observed argv encoding:\n" << result.output << L"\n";
    }
    Require(result.output == std::wstring(expected.begin(), expected.end()), "Literal argv changed");
    Require(!RunProcessCapture({self, std::wstring(L"a\0b", 3)}, root.wstring(), &result, &error), "Embedded NUL accepted");
    Require(!RunProcessCapture({(root / L"missing.exe").wstring()}, root.wstring(), &result, &error), "Missing EXE accepted");
    Require(RunProcessCapture({self, L"--flood"}, root.wstring(), &result, &error) &&
        result.output_truncated && result.output.size() < 270000, "Capture bound failed");
    Require(RunProcessCapture({self, L"--closed-output"}, root.wstring(), &result, &error) &&
        result.exit_code == 23, "Closed stdout was mistaken for process completion");

    std::atomic_bool cancel{false};
    const auto pid_file = root / L"descendant.pid";
    bool ran = false;
    std::thread worker([&]() { ran = RunProcessCapture({self, L"--tree", pid_file.wstring()},
        root.wstring(), &result, &error, &cancel); });
    const bool ready = WaitForFile(pid_file);
    const auto start = std::chrono::steady_clock::now();
    cancel.store(true);
    worker.join();
    Require(ready && ran && result.cancelled, "Process tree cancellation failed");
    Require(std::chrono::steady_clock::now() - start < std::chrono::seconds(3), "Cancellation stalled");
    const DWORD pid = DWORD(std::stoul(Read(pid_file)));
    HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (child) {
        const DWORD wait = WaitForSingleObject(child, 3000);
        CloseHandle(child);
        Require(wait == WAIT_OBJECT_0, "Descendant survived cancellation");
    }
    std::cout << "PASS: literal argv, NUL/missing EXE, bounded capture, process-tree cancellation\n";
}

void CheckConversion(const std::wstring& engine, const std::wstring& self, const fs::path& root) {
    const auto input = root / L"input & %LITERAL% \x6e2c\x8a66.bin";
    std::string payload;
    for (int i = 0; i < 32; ++i) payload.push_back(char(i));
    Write(input, payload);
    SRecordOptions options;
    options.srec_exe = engine;
    options.input_path = input.wstring();
    options.output_path = (root / L"patched.bin").wstring();
    options.hex_output_path = (root / L"patched.hex").wstring();
    options.checksum_enabled = true;
    options.offset_enabled = true;
    options.offset = L"0x20";
    options.image_size = L"0x100";
    auto result = Execute(options);
    Require(result.outcome == ConversionOutcome::Succeeded && result.committed_paths.size() == 2, "Checksum+HEX job failed");
    const auto patched = Read(options.output_path);
    Require(patched.size() == 224 && patched.substr(0, 32) == payload, "Offset image size/payload changed");
    // Independent CRC32/IEEE reference; no conversion algorithm is added to the app.
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < 220; ++i) {
        crc ^= static_cast<unsigned char>(patched[i]);
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
    }
    crc ^= 0xFFFFFFFFu;
    for (int i = 0; i < 4; ++i)
        Require(static_cast<unsigned char>(patched[220 + i]) == ((crc >> (8 * i)) & 255), "CRC32 bytes changed");
    options.input_path = options.hex_output_path;
    options.output_path = (root / L"decoded.bin").wstring();
    options.operation = ConvertOperation::HexToBin;
    options.checksum_enabled = false;
    options.pad_enabled = false;
    options.offset_enabled = false;
    Require(Execute(options).outcome == ConversionOutcome::Succeeded, "HEX decode failed");
    const auto decoded = Read(options.output_path);
    Require(decoded.size() == 256 && decoded.substr(32) == patched, "HEX offset/bytes changed");

    options.input_path = input.wstring();
    options.operation = ConvertOperation::BinToHex;
    options.output_path = (root / L"plain.hex").wstring();
    Require(Execute(options).outcome == ConversionOutcome::Succeeded, "Plain BIN to HEX failed");
    options.input_path = options.output_path;
    options.output_path = (root / L"plain.bin").wstring();
    options.operation = ConvertOperation::HexToBin;
    Require(Execute(options).outcome == ConversionOutcome::Succeeded && Read(options.output_path) == payload,
        "Plain roundtrip changed bytes");

    ConversionPlan ignored;
    std::wstring error;
    options.operation = ConvertOperation::BinToBin;
    options.hex_copy_enabled = false;
    options.input_path = input.wstring();
    options.output_path = input.wstring();
    Require(!BuildConversionPlan(options, &ignored, &error), "Same path accepted");
    const auto alias = root / L"hardlink.bin";
    Require(CreateHardLinkW(alias.c_str(), input.c_str(), nullptr) != FALSE, "Hardlink fixture failed");
    options.output_path = alias.wstring();
    Require(!BuildConversionPlan(options, &ignored, &error), "Source hardlink accepted");
    options.output_path = (root / L"protected.bin").wstring();
    Write(options.output_path, "last-good");
    options.hex_copy_enabled = true;
    options.hex_output_path = options.output_path;
    Require(!BuildConversionPlan(options, &ignored, &error), "BIN/HEX collision accepted");
    options.output_path = (root / L"\x0410.bin").wstring();
    options.hex_output_path = (root / L"\x0430.bin").wstring();
    Require(!BuildConversionPlan(options, &ignored, &error), "Unicode case-insensitive output collision accepted");
    options.output_path = (root / L"protected.bin").wstring();
    options.hex_copy_enabled = false;
    auto plan = Plan(options);
    Write(options.output_path, "external-newer");
    std::atomic_bool cancel{false};
    Require(ExecuteConversion(plan, cancel).outcome == ConversionOutcome::Failed &&
        Read(options.output_path) == "external-newer", "Changed destination overwritten");
    plan = Plan(options);
    Write(input, payload + "changed");
    Require(ExecuteConversion(plan, cancel).outcome == ConversionOutcome::Failed, "Changed input accepted");
    Write(input, payload);
    plan = Plan(options);
    cancel.store(true);
    Require(ExecuteConversion(plan, cancel).outcome == ConversionOutcome::Cancelled, "Pre-start cancellation failed");
    cancel.store(false);

    options.operation = ConvertOperation::HexToBin;
    const auto malformed = root / L"malformed.hex";
    Write(malformed, ":not-an-intel-record\n");
    options.input_path = malformed.wstring();
    Require(Execute(options).outcome == ConversionOutcome::Failed && Read(options.output_path) == "external-newer",
        "Engine failure damaged last-good output");

    options.operation = ConvertOperation::BinToBin;
    options.input_path = input.wstring();
    options.srec_exe = self;
    options.hex_copy_enabled = true;
    options.hex_output_path = (root / L"protected.hex").wstring();
    Write(options.hex_output_path, "hex-last-good");
    Require(Execute(options).outcome == ConversionOutcome::Failed && Read(options.output_path) == "external-newer" &&
        Read(options.hex_output_path) == "hex-last-good", "Second stage failure committed outputs");
    options.input_path = (root / L"empty.bin").wstring();
    Write(options.input_path, payload);
    options.hex_copy_enabled = false;
    Require(Execute(options).outcome == ConversionOutcome::Failed && Read(options.output_path) == "external-newer",
        "Empty staging accepted");

    options.input_path = input.wstring();
    options.srec_exe = engine;
    options.hex_copy_enabled = true;
    options.output_path = (root / L"partial.bin").wstring();
    options.hex_output_path = (root / L"partial.hex").wstring();
    Write(options.output_path, "old-bin");
    Write(options.hex_output_path, "old-hex");
    Require(SetFileAttributesW(options.hex_output_path.c_str(), FILE_ATTRIBUTE_READONLY) != FALSE, "Read-only fixture failed");
    result = Execute(options);
    SetFileAttributesW(options.hex_output_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    Require(result.outcome == ConversionOutcome::PartiallyCommitted && result.committed_paths.size() == 1 &&
        Read(options.output_path) == payload && Read(options.hex_output_path) == "old-hex", "Partial commit outcome inaccurate");
    options.srec_exe = self;
    options.output_path = (root / L"protected.bin").wstring();
    options.hex_copy_enabled = false;
    options.input_path = (root / L"wait.bin").wstring();
    Write(options.input_path, payload);
    {
        ConversionJob job;
        Require(job.Start(Plan(options), &error), "Job start failed");
        const bool ready = WaitForFile(options.input_path + L".ready");
        Require(!job.Start(Plan(options), &error), "Double-trigger accepted");
        job.Cancel();
        for (int i = 0; i < 200 && !job.Done(); ++i) Sleep(20);
        Require(job.Done(), "Job cancellation stalled");
        result = job.TakeResult();
        Require(ready && result.outcome == ConversionOutcome::Cancelled && Read(options.output_path) == "external-newer",
            "Job cancellation changed output");
    }
    options.input_path = input.wstring();
    options.srec_exe = engine;
    options.output_path = (root / L"late-cancel.bin").wstring();
    {
        ConversionJob job;
        Require(job.Start(Plan(options), &error), "Late-cancel job start failed");
        for (int i = 0; i < 200 && !job.Done(); ++i) Sleep(20);
        Require(job.Done(), "Late-cancel completion stalled");
        job.Cancel();
        Require(job.TakeResult().outcome == ConversionOutcome::Succeeded, "Late cancellation masked committed success");
    }
    NoStages(root);
    Require(Read(input) == payload, "Source modified");
    std::cout << "PASS: CRC32/offset, BIN/HEX, aliases, stale plans, failed HEX, single-flight/cancel, staging cleanup\n";
}

void CheckCommit(const fs::path& root) {
    std::wstring error;
    auto target = (root / L"commit.bin").wstring();
    Write(target, "old");
    FileStamp expected;
    Require(InspectFile(target, &expected, &error), "Inspect target failed");
    OutputTransaction output;
    Require(output.Prepare({target}, &error), "Staging prepare failed");
    Write(output.Stage(0), "new");
    HANDLE lock = CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    Require(lock != INVALID_HANDLE_VALUE, "Lock fixture failed");
    const bool committed = output.Commit(0, expected, &error);
    CloseHandle(lock);
    Require(!committed && Read(target) == "old", "Commit failure damaged prior output");
    Require(output.Commit(0, expected, &error) && Read(target) == "new", "Atomic replacement failed");
    target = (root / L"appeared.bin").wstring();
    OutputTransaction absent;
    Require(absent.Prepare({target}, &error), "Absent target staging failed");
    Write(absent.Stage(0), "new");
    Write(target, "arrived");
    Require(!absent.Commit(0, {}, &error) && Read(target) == "arrived", "New destination race overwritten");
    std::vector<std::wstring> warnings;
    absent.Cleanup(&warnings);
    Require(warnings.empty(), "Staging cleanup failed");
    NoStages(root);
    std::cout << "PASS: locked commit preservation, replacement, absent-target race\n";
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc >= 2 && std::wstring(argv[1]) != L"--run") return Child(argc, argv);
        Require(argc == 4, "Usage: ExecutionRegression --run <engine> <fresh fixture directory>");
        const fs::path root = argv[3];
        Require(fs::create_directory(root), "Fixture directory must be fresh");
        CheckProcess(argv[0], root);
        CheckConversion(argv[2], argv[0], root);
        CheckCommit(root);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
