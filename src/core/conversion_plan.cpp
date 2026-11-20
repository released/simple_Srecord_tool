#include "conversion_plan.h"

#include "text_utils.h"

namespace srecord_tool::core {
namespace {
bool SamePath(const std::wstring& a, const std::wstring& b) {
    return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
}
} // namespace

bool BuildConversionPlan(const SRecordOptions& options, ConversionPlan* plan, std::wstring* error) {
    *plan = {};
    auto& normalized = plan->options;
    normalized = options;
    if (!ResolveFilePath(options.input_path, &normalized.input_path, error) ||
        !ResolveFilePath(options.output_path, &normalized.output_path, error)) return false;
    std::vector<wchar_t> executable(32768);
    if (options.srec_exe.find(L'\0') != std::wstring::npos) {
        if (error) *error = L"Invalid engine path.";
        return false;
    }
    const DWORD length = SearchPathW(nullptr, options.srec_exe.c_str(), L".exe",
                                    DWORD(executable.size()), executable.data(), nullptr);
    if (!length || length >= executable.size()) {
        if (error) *error = L"SRecord executable was not found. Select srec_cat.exe.";
        return false;
    }
    if (!ResolveFilePath(executable.data(), &normalized.srec_exe, error)) return false;
    if (ExtensionOf(normalized.srec_exe) != L".exe") {
        if (error) *error = L"SRecord must be a native .exe.";
        return false;
    }
    plan->has_hex_copy = options.operation == ConvertOperation::BinToBin && options.hex_copy_enabled;
    if (plan->has_hex_copy && !ResolveFilePath(options.hex_output_path, &normalized.hex_output_path, error))
        return false;
    if (!BuildConvertCommand(normalized, &plan->convert, error)) return false;
    if (plan->has_hex_copy && !BuildHexCopyCommand(normalized, plan->convert.derived, &plan->hex_copy, error))
        return false;
    if (!InspectFile(normalized.input_path, &plan->input_version, error) ||
        !InspectFile(normalized.srec_exe, &plan->engine_version, error) || !plan->engine_version.exists)
        return false;
    plan->targets.push_back(normalized.output_path);
    if (plan->has_hex_copy) plan->targets.push_back(normalized.hex_output_path);
    for (const auto& target : plan->targets) {
        FileStamp version;
        if (!InspectFile(target, &version, error)) return false;
        if (SamePath(target, normalized.input_path) || SamePath(target, normalized.srec_exe) ||
            SameFile(version, plan->input_version) || SameFile(version, plan->engine_version)) {
            if (error) *error = L"An output conflicts with the source file or engine (including file aliases).";
            return false;
        }
        for (size_t i = 0; i < plan->target_versions.size(); ++i) {
            if (SamePath(target, plan->targets[i]) ||
                SameFile(version, plan->target_versions[i])) {
                if (error) *error = L"BIN and HEX outputs must be distinct files.";
                return false;
            }
        }
        plan->target_versions.push_back(version);
    }
    return true;
}

bool RevalidatePlan(const ConversionPlan& plan, std::wstring* error) {
    const auto check = [error](const std::wstring& path, const FileStamp& expected) {
        FileStamp current;
        std::wstring resolved;
        if (!ResolveFilePath(path, &resolved, error) || !SamePath(resolved, path) ||
            !InspectFile(path, &current, error)) return false;
        if (!SameVersion(current, expected)) {
            if (error) *error = L"An input, engine or destination changed after the plan was prepared. Prepare the conversion again.";
            return false;
        }
        return true;
    };
    if (!check(plan.options.input_path, plan.input_version) ||
        !check(plan.options.srec_exe, plan.engine_version)) return false;
    for (size_t i = 0; i < plan.targets.size(); ++i) {
        if (!check(plan.targets[i], plan.target_versions[i])) return false;
    }
    return true;
}

std::wstring DescribePlanCommands(const ConversionPlan& plan) {
    return plan.convert.command_line + (plan.has_hex_copy ? L"\r\n" + plan.hex_copy.command_line : L"");
}

} // namespace srecord_tool::core
