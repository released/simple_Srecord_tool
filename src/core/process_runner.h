#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace srecord_tool::core {

struct ProcessResult {
    uint32_t exit_code = 0;
    std::wstring output;
};

bool RunProcessCapture(const std::vector<std::wstring>& args,
                       const std::wstring& working_dir,
                       ProcessResult* result,
                       std::wstring* error = nullptr);

} // namespace srecord_tool::core
