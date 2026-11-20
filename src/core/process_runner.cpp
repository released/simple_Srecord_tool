#include "process_runner.h"

#include <windows.h>

#include <array>
#include <algorithm>
#include <vector>

#include "text_utils.h"

namespace srecord_tool::core {
namespace {

struct HandleGuard {
    HANDLE value = nullptr;

    HandleGuard() = default;
    explicit HandleGuard(HANDLE h) : value(h) {}
    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    ~HandleGuard() {
        Reset();
    }

    void Reset(HANDLE h = nullptr) {
        if (value != nullptr && value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
        value = h;
    }

    HANDLE Release() {
        HANDLE h = value;
        value = nullptr;
        return h;
    }
};

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (needed <= 0) {
        code_page = CP_ACP;
        flags = 0;
        needed = MultiByteToWideChar(code_page, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
    }
    if (needed <= 0) {
        return L"";
    }

    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(code_page, flags, text.data(), static_cast<int>(text.size()), out.data(), needed);
    return out;
}

} // namespace

bool RunProcessCapture(const std::vector<std::wstring>& args,
                       const std::wstring& working_dir,
                       ProcessResult* result,
                       std::wstring* error,
                       const std::atomic_bool* cancel) {
    if (result == nullptr) {
        if (error != nullptr) {
            *error = L"Internal process error: result pointer is null.";
        }
        return false;
    }
    *result = {};
    const auto fail = [error](const wchar_t* stage) {
        if (error) *error = std::wstring(stage) + L": " + std::to_wstring(GetLastError());
        return false;
    };

    if (args.empty()) {
        if (error != nullptr) {
            *error = L"Command is empty.";
        }
        return false;
    }
    for (const auto& arg : args) {
        if (arg.find(L'\0') != std::wstring::npos) {
            if (error) *error = L"An argument contains an embedded NUL.";
            return false;
        }
    }
    if (cancel && cancel->load()) {
        result->cancelled = true;
        return true;
    }

    // Resolve argv[0] separately: never let CreateProcess guess an executable
    // from a spaced command line, and never use a command shell.
    std::vector<wchar_t> executable(32768);
    DWORD length = SearchPathW(nullptr, args.front().c_str(), L".exe",
                              static_cast<DWORD>(executable.size()), executable.data(), nullptr);
    if (!length || length >= executable.size()) return fail(L"Resolve executable");
    const std::wstring exe(executable.data(), length);
    if (ToLower(ExtensionOf(exe)) != L".exe") {
        if (error) *error = L"The engine must be a native .exe, not a batch or shell command.";
        return false;
    }

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        if (error != nullptr) {
            *error = L"CreatePipe failed: " + std::to_wstring(GetLastError());
        }
        return false;
    }

    HandleGuard read_guard(read_pipe);
    HandleGuard write_guard(write_pipe);
    if (!SetHandleInformation(read_guard.value, HANDLE_FLAG_INHERIT, 0)) {
        if (error != nullptr) {
            *error = L"SetHandleInformation failed: " + std::to_wstring(GetLastError());
        }
        return false;
    }

    auto literal_args = args;
    literal_args.front() = exe;
    std::wstring command = JoinCommandLine(literal_args);
    if (command.size() >= 32767) {
        if (error) *error = L"Command exceeds the Windows command-line limit.";
        return false;
    }
    std::vector<wchar_t> mutable_cmd(command.begin(), command.end());
    mutable_cmd.push_back(L'\0');

    HandleGuard input_guard(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (input_guard.value == INVALID_HANDLE_VALUE) return fail(L"Open stdin");
    HandleGuard job(CreateJobObjectW(nullptr, nullptr));
    if (!job.value) return fail(L"Create process job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        return fail(L"Configure process job");

    SIZE_T attribute_bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_bytes);
    std::vector<unsigned char> attributes(attribute_bytes);
    auto* attribute_list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if (!InitializeProcThreadAttributeList(attribute_list, 1, 0, &attribute_bytes))
        return fail(L"Initialize handle list");
    struct AttributeGuard {
        LPPROC_THREAD_ATTRIBUTE_LIST value;
        ~AttributeGuard() { DeleteProcThreadAttributeList(value); }
    } attribute_guard{attribute_list};
    HANDLE inherited[] = {write_guard.value, input_guard.value};
    if (!UpdateProcThreadAttribute(attribute_list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   inherited, sizeof(inherited), nullptr, nullptr))
        return fail(L"Set inherited handles");
    STARTUPINFOEXW si = {};
    si.StartupInfo.cb = sizeof(si);
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    si.StartupInfo.hStdOutput = write_guard.value;
    si.StartupInfo.hStdError = write_guard.value;
    si.StartupInfo.hStdInput = input_guard.value;
    si.lpAttributeList = attribute_list;

    PROCESS_INFORMATION pi = {};
    const wchar_t* cwd = working_dir.empty() ? nullptr : working_dir.c_str();
    BOOL ok = CreateProcessW(exe.c_str(),
                             mutable_cmd.data(),
                             nullptr,
                             nullptr,
                             TRUE,
                             CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
                             nullptr,
                             cwd,
                             &si.StartupInfo,
                             &pi);
    if (!ok) {
        if (error != nullptr) {
            *error = L"CreateProcess failed: " + std::to_wstring(GetLastError());
        }
        return false;
    }

    HandleGuard process_guard(pi.hProcess);
    HandleGuard thread_guard(pi.hThread);
    if (!AssignProcessToJobObject(job.value, pi.hProcess) || ResumeThread(pi.hThread) == DWORD(-1)) {
        const DWORD code = GetLastError();
        TerminateProcess(pi.hProcess, ERROR_PROCESS_ABORTED);
        WaitForSingleObject(pi.hProcess, INFINITE);
        SetLastError(code);
        return fail(L"Start process job");
    }
    write_guard.Reset();

    std::string bytes;
    constexpr size_t kCaptureLimit = 256 * 1024;
    std::array<char, 4096> buffer = {};
    bool io_ok = true;
    bool pipe_open = true;
    bool descendants_retired = false;
    DWORD io_error = ERROR_SUCCESS;
    for (;;) {
        const DWORD wait = WaitForSingleObject(process_guard.value, 0);
        if (wait == WAIT_FAILED) { io_ok = false; io_error = GetLastError(); break; }
        const bool exited = wait == WAIT_OBJECT_0;
        if (exited && !descendants_retired) {
            TerminateJobObject(job.value, ERROR_PROCESS_ABORTED);
            descendants_retired = true;
        }
        if (cancel && cancel->load()) {
            // Observe an already completed process as completion, not cancellation.
            if (!exited) {
                result->cancelled = true;
                TerminateJobObject(job.value, ERROR_CANCELLED);
                break;
            }
        }
        DWORD available = 0;
        if (pipe_open && !PeekNamedPipe(read_guard.value, nullptr, 0, nullptr, &available, nullptr)) {
            io_error = GetLastError();
            if (io_error != ERROR_BROKEN_PIPE) { io_ok = false; break; }
            pipe_open = false;
        }
        if (available) {
            DWORD read = 0;
            if (!ReadFile(read_guard.value, buffer.data(), (std::min)(available, DWORD(buffer.size())), &read, nullptr)) {
                io_error = GetLastError();
                io_ok = false;
                break;
            }
            const size_t take = (std::min)(size_t(read), kCaptureLimit - bytes.size());
            bytes.append(buffer.data(), take);
            result->output_truncated |= take != read;
        } else {
            if (exited) break;
            WaitForSingleObject(process_guard.value, 30);
        }
    }

    // Also retire descendants that kept stdout open after the engine exited.
    TerminateJobObject(job.value, ERROR_PROCESS_ABORTED);
    WaitForSingleObject(process_guard.value, INFINITE);
    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process_guard.value, &exit_code)) return fail(L"Read process status");

    result->exit_code = exit_code;
    result->output = Utf8ToWide(bytes);
    if (result->output_truncated) result->output += L"\r\n[Process output truncated at 256 KiB]";
    if (!io_ok) {
        SetLastError(io_error);
        return fail(L"Capture process output");
    }
    return true;
}

} // namespace srecord_tool::core
