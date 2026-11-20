#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace srecord_tool::core {

struct FileStamp {
    bool exists = false;
    DWORD volume = 0;
    DWORD index_high = 0;
    DWORD index_low = 0;
    uint64_t size = 0;
    uint64_t modified = 0;
};

bool SameFile(const FileStamp& a, const FileStamp& b);
bool SameVersion(const FileStamp& a, const FileStamp& b);
bool InspectFile(const std::wstring& path, FileStamp* stamp, std::wstring* error);
bool ResolveFilePath(const std::wstring& path, std::wstring* resolved, std::wstring* error);

class OutputTransaction {
public:
    OutputTransaction() = default;
    OutputTransaction(const OutputTransaction&) = delete;
    OutputTransaction& operator=(const OutputTransaction&) = delete;
    ~OutputTransaction();
    bool Prepare(const std::vector<std::wstring>& targets, std::wstring* error);
    const std::wstring& Stage(size_t index) const { return stages_.at(index).path; }
    bool Validate(size_t index, std::wstring* error);
    bool Commit(size_t index, const FileStamp& expected, std::wstring* error);
    void Cleanup(std::vector<std::wstring>* warnings);

private:
    struct StageFile { std::wstring path; FileStamp identity; bool committed = false; };
    std::vector<std::wstring> targets_;
    std::vector<StageFile> stages_;
    std::vector<HANDLE> directories_;
    bool cleanup_attempted_ = false;
};

} // namespace srecord_tool::core
