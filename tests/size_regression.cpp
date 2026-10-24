#include "core/process_runner.h"
#include "core/srecord_command.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace srecord_tool::core;

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

std::vector<char> Read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    Require(stream.good(), "Missing test output");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

SRecordCommand Convert(const SRecordOptions& options, const std::wstring& directory) {
    SRecordCommand command;
    std::wstring error;
    Require(BuildConvertCommand(options, &command, &error), "Command generation failed");
    ProcessResult result;
    Require(RunProcessCapture(command.args, directory, &result, &error) &&
        result.exit_code == 0, "SRecord conversion failed");
    return command;
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        Require(argc == 3, "Pass srec_cat and a new test-owned directory");
        const std::filesystem::path root(argv[2]);
        Require(std::filesystem::create_directory(root), "Fixture directory must be new");
        const auto input = root / L"input fixture.bin";
        std::vector<char> payload;
        for (int value = 0; value < 32; ++value) { payload.push_back(static_cast<char>(value)); }
        { std::ofstream file(input, std::ios::binary); file.write(payload.data(), payload.size());
          Require(file.good(), "Cannot create input fixture"); }
        SRecordOptions options;
        options.srec_exe = std::filesystem::absolute(argv[1]).wstring();
        options.input_path = input.wstring();
        options.output_path = (root / L"plain.hex").wstring();
        options.operation = ConvertOperation::BinToHex;
        options.pad_enabled = false;
        options.image_size = L"0x100";
        Convert(options, root.wstring());
        options.input_path = options.output_path;
        options.output_path = (root / L"roundtrip.bin").wstring();
        options.operation = ConvertOperation::HexToBin;
        Convert(options, root.wstring());
        Require(Read(options.output_path) == payload, "BIN/HEX roundtrip changed bytes");

        const wchar_t* algorithms[] = {L"-crc32-l-e", L"-crc32-b-e",
            L"-checksum-n-l-e", L"-checksum-n-b-e", L"-checksum-p-l-e", L"-checksum-p-b-e"};
        int index = 0;
        for (const auto* algorithm : algorithms) {
            options.input_path = input.wstring();
            options.output_path = (root / (L"checksum-" + std::to_wstring(index++) + L".bin")).wstring();
            options.operation = ConvertOperation::BinToBin;
            options.pad_enabled = true;
            options.checksum_enabled = true;
            options.checksum_algorithm = algorithm;
            const auto command = Convert(options, root.wstring());
            const auto output = Read(options.output_path);
            Require(output.size() == 256 && command.derived.checksum_address == 252,
                "Checksum image/address changed");
            Require(std::equal(payload.begin(), payload.end(), output.begin()), "Payload changed");
            Require(std::all_of(output.begin() + 32, output.begin() + 252,
                [](char value) { return static_cast<unsigned char>(value) == 255; }), "Padding changed");
            SRecordCommand dump;
            ProcessResult result;
            std::wstring error;
            Require(BuildChecksumDumpCommand(options, command.derived, &dump, &error) &&
                RunProcessCapture(dump.args, root.wstring(), &result, &error) && result.exit_code == 0 &&
                !result.output.empty(), "Checksum dump failed");
        }
        SRecordCommand ignored;
        std::wstring error;
        options.checksum_algorithm = L"not-an-algorithm";
        Require(!BuildConvertCommand(options, &ignored, &error), "Invalid algorithm accepted");
        options.checksum_algorithm = DefaultChecksumAlgorithm();
        options.fill_byte = L"0x100";
        Require(!BuildConvertCommand(options, &ignored, &error), "Invalid fill accepted");
        options.fill_byte = L"0xFF";
        options.base_address = L"0xFFFFFFFFFFFFFFFE";
        Require(!BuildConvertCommand(options, &ignored, &error), "Address overflow accepted");
        Require(Read(input) == payload, "Input fixture was modified");
        std::cout << "PASS: BIN/HEX roundtrip, 6 checksum workflows/padding/dumps, validation, input preservation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
