#include "process_runner.h"

#include <windows.h>

#include <array>
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
                       std::wstring* error) {
    if (result == nullptr) {
        if (error != nullptr) {
            *error = L"Internal process error: result pointer is null.";
        }
        return false;
    }
    result->exit_code = 0;
    result->output.clear();

    if (args.empty()) {
        if (error != nullptr) {
            *error = L"Command is empty.";
        }
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

    std::wstring command = JoinCommandLine(args);
    std::vector<wchar_t> mutable_cmd(command.begin(), command.end());
    mutable_cmd.push_back(L'\0');

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = write_guard.value;
    si.hStdError = write_guard.value;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {};
    const wchar_t* cwd = working_dir.empty() ? nullptr : working_dir.c_str();
    BOOL ok = CreateProcessW(nullptr,
                             mutable_cmd.data(),
                             nullptr,
                             nullptr,
                             TRUE,
                             CREATE_NO_WINDOW,
                             nullptr,
                             cwd,
                             &si,
                             &pi);
    if (!ok) {
        if (error != nullptr) {
            *error = L"CreateProcess failed: " + std::to_wstring(GetLastError()) + L"\r\n" + command;
        }
        return false;
    }

    HandleGuard process_guard(pi.hProcess);
    HandleGuard thread_guard(pi.hThread);
    write_guard.Reset();

    std::string bytes;
    std::array<char, 4096> buffer = {};
    for (;;) {
        DWORD read = 0;
        BOOL read_ok = ReadFile(read_guard.value, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr);
        if (!read_ok || read == 0) {
            break;
        }
        bytes.append(buffer.data(), buffer.data() + read);
    }

    WaitForSingleObject(process_guard.value, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process_guard.value, &exit_code);

    result->exit_code = exit_code;
    result->output = Utf8ToWide(bytes);
    return true;
}

} // namespace srecord_tool::core
