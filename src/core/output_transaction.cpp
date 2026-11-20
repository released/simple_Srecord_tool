#include "output_transaction.h"

#include <cstdint>
#include <random>
#include <utility>
#include "text_utils.h"

namespace srecord_tool::core {
namespace {

bool Fail(std::wstring* error, const wchar_t* stage) {
    if (error) *error = std::wstring(stage) + L" (Windows " + std::to_wstring(GetLastError()) + L").";
    return false;
}

FileStamp Stamp(const BY_HANDLE_FILE_INFORMATION& info) {
    return {true, info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow,
        (uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow,
        (uint64_t(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime};
}

} // namespace

bool SameFile(const FileStamp& a, const FileStamp& b) {
    return a.exists && b.exists && a.volume == b.volume &&
        a.index_high == b.index_high && a.index_low == b.index_low;
}

bool SameVersion(const FileStamp& a, const FileStamp& b) {
    return (!a.exists && !b.exists) || (SameFile(a, b) && a.size == b.size && a.modified == b.modified);
}

bool InspectFile(const std::wstring& path, FileStamp* stamp, std::wstring* error) {
    *stamp = {};
    HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return true;
        return Fail(error, L"Inspect file");
    }
    BY_HANDLE_FILE_INFORMATION info = {};
    const bool ok = GetFileInformationByHandle(file, &info) != FALSE;
    CloseHandle(file);
    if (!ok) return Fail(error, L"Read file identity");
    if (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) {
        if (error) *error = L"A regular file is required; directories and reparse points are not accepted.";
        return false;
    }
    *stamp = Stamp(info);
    return true;
}

bool ResolveFilePath(const std::wstring& path, std::wstring* resolved, std::wstring* error) {
    if (path.empty() || path.find_first_of(L"\"\r\n<>|?*") != std::wstring::npos ||
        path.find(L'\0') != std::wstring::npos || path.rfind(L"\\\\?\\", 0) == 0 ||
        path.rfind(L"\\\\.\\", 0) == 0) {
        if (error) *error = L"Invalid file path (device paths, wildcards and control characters are unsupported).";
        return false;
    }
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetFullPathNameW(path.c_str(), DWORD(buffer.size()), buffer.data(), nullptr);
    if (!length || length >= buffer.size()) return Fail(error, L"Resolve file path");
    *resolved = std::wstring(buffer.data(), length);
    if (resolved->find(L':', 2) != std::wstring::npos) {
        if (error) *error = L"Alternate data streams are not supported.";
        return false;
    }
    // Reject aliases that Win32 silently trims and all reparse ancestors.
    std::wstring part = *resolved;
    while (!part.empty()) {
        if (part.back() == L'.' || part.back() == L' ') {
            if (error) *error = L"Path components cannot end with a dot or space.";
            return false;
        }
        const DWORD attr = GetFileAttributesW(part.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) {
            if (error) *error = L"Reparse-point files and folders are not supported for conversion.";
            return false;
        }
        const auto parent = DirectoryOf(part);
        if (parent == part) break;
        part = parent;
    }
    const std::wstring parent = DirectoryOf(*resolved);
    const DWORD attr = GetFileAttributesW(parent.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        if (error) *error = L"The containing folder must already exist.";
        return false;
    }
    HANDLE directory = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (directory == INVALID_HANDLE_VALUE) return Fail(error, L"Resolve containing folder");
    const DWORD final_length = GetFinalPathNameByHandleW(directory, buffer.data(), DWORD(buffer.size()), 0);
    const DWORD code = GetLastError();
    CloseHandle(directory);
    SetLastError(code);
    if (!final_length || final_length >= buffer.size()) return Fail(error, L"Resolve folder identity");
    std::wstring canonical(buffer.data(), final_length);
    if (canonical.rfind(L"\\\\?\\UNC\\", 0) == 0) canonical = L"\\\\" + canonical.substr(8);
    else if (canonical.rfind(L"\\\\?\\", 0) == 0) canonical = canonical.substr(4);
    *resolved = CombinePath(canonical, resolved->substr(resolved->find_last_of(L"\\/") + 1));
    return true;
}

OutputTransaction::~OutputTransaction() {
    if (!cleanup_attempted_) {
        try { Cleanup(nullptr); } catch (...) { /* Preserve files if cleanup cannot be inspected. */ }
    }
    for (auto directory : directories_) {
        if (directory != INVALID_HANDLE_VALUE) CloseHandle(directory);
    }
}

bool OutputTransaction::Prepare(const std::vector<std::wstring>& targets, std::wstring* error) {
    targets_ = targets;
    stages_.reserve(targets.size());
    std::random_device random;
    for (const auto& target : targets) {
        const auto parent = DirectoryOf(target);
        std::wstring ancestor = parent;
        while (!ancestor.empty()) {
            directories_.push_back(INVALID_HANDLE_VALUE);
            HANDLE directory = CreateFileW(ancestor.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (directory == INVALID_HANDLE_VALUE) return Fail(error, L"Pin output folder");
            directories_.back() = directory;
            BY_HANDLE_FILE_INFORMATION info = {};
            if (!GetFileInformationByHandle(directory, &info)) return Fail(error, L"Inspect output folder");
            if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                if (error) *error = L"Output folder became a reparse point.";
                return false;
            }
            const auto next = DirectoryOf(ancestor);
            if (next == ancestor || next.size() < 3) break;
            ancestor = next;
        }
        StageFile stage;
        HANDLE file = INVALID_HANDLE_VALUE;
        for (int attempt = 0; attempt < 16; ++attempt) {
            stage.path = CombinePath(parent, L".srecord-" + FormatHex(random()) +
                                      FormatHex(random()) + L".tmp");
            file = CreateFileW(stage.path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_EXISTS) break;
        }
        if (file == INVALID_HANDLE_VALUE) return Fail(error, L"Create output staging file");
        stages_.push_back(std::move(stage));
        BY_HANDLE_FILE_INFORMATION info = {};
        const bool ok = GetFileInformationByHandle(file, &info) != FALSE;
        CloseHandle(file);
        if (!ok) return Fail(error, L"Read staging identity");
        stages_.back().identity = Stamp(info);
    }
    return true;
}

bool OutputTransaction::Validate(size_t index, std::wstring* error) {
    FileStamp current;
    if (!InspectFile(Stage(index), &current, error)) return false;
    if (!SameFile(current, stages_[index].identity) || !current.size) {
        if (error) *error = L"Engine output is missing, empty, or no longer the owned staging file.";
        return false;
    }
    HANDLE file = CreateFileW(Stage(index).c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return Fail(error, L"Open completed staging file");
    const bool ok = FlushFileBuffers(file) != FALSE;
    const DWORD code = GetLastError();
    CloseHandle(file);
    SetLastError(code);
    return ok || Fail(error, L"Flush output");
}

bool OutputTransaction::Commit(size_t index, const FileStamp& expected, std::wstring* error) {
    FileStamp current;
    if (!InspectFile(targets_.at(index), &current, error)) return false;
    if (!SameVersion(current, expected)) {
        if (error) *error = L"Destination changed since conversion was confirmed; no replacement was attempted.";
        return false;
    }
    if (!Validate(index, error)) return false;
    // No pre-delete and no copy fallback. Siblings stay on the same filesystem.
    // Windows has no compare-and-swap rename: external writers must remain idle
    // between the identity check and this atomic per-file replacement.
    DWORD flags = MOVEFILE_WRITE_THROUGH | (expected.exists ? MOVEFILE_REPLACE_EXISTING : 0);
    if (!MoveFileExW(Stage(index).c_str(), targets_[index].c_str(), flags))
        return Fail(error, L"Commit output");
    stages_[index].committed = true;
    return true;
}

void OutputTransaction::Cleanup(std::vector<std::wstring>* warnings) {
    cleanup_attempted_ = true;
    for (auto& stage : stages_) {
        if (stage.committed) continue;
        FileStamp current;
        std::wstring error;
        if (InspectFile(stage.path, &current, &error) && !current.exists) {
            stage.committed = true;
            continue;
        }
        if (SameFile(current, stage.identity) && DeleteFileW(stage.path.c_str())) {
            stage.committed = true;
        } else if (warnings) {
            warnings->push_back(L"Staging cleanup requires review: " + stage.path);
        }
    }
}

} // namespace srecord_tool::core
