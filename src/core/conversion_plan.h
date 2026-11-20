#pragma once

#include "output_transaction.h"
#include "srecord_command.h"

namespace srecord_tool::core {

struct ConversionPlan {
    SRecordOptions options;
    SRecordCommand convert;
    SRecordCommand hex_copy;
    std::vector<std::wstring> targets;
    std::vector<FileStamp> target_versions;
    FileStamp input_version;
    FileStamp engine_version;
    bool has_hex_copy = false;
};

bool BuildConversionPlan(const SRecordOptions& options, ConversionPlan* plan, std::wstring* error);
bool RevalidatePlan(const ConversionPlan& plan, std::wstring* error);
std::wstring DescribePlanCommands(const ConversionPlan& plan);

} // namespace srecord_tool::core
