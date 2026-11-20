#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <vector>

namespace srecord_tool::core {

struct ProcessResult {
    uint32_t exit_code = 0;
    std::wstring output;
    bool cancelled = false;
    bool output_truncated = false;
};

bool RunProcessCapture(const std::vector<std::wstring>& args,
                       const std::wstring& working_dir,
                       ProcessResult* result,
                       std::wstring* error = nullptr,
                       const std::atomic_bool* cancel = nullptr);

} // namespace srecord_tool::core
