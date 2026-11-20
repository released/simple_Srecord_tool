#include "conversion_job.h"

#include <exception>
#include <utility>
#include "process_runner.h"
#include "text_utils.h"

namespace srecord_tool::core {
namespace {

struct ReadLock {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~ReadLock() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    bool Open(const std::wstring& path, std::wstring* error) {
        value = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                            OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (value != INVALID_HANDLE_VALUE) return true;
        *error = L"Cannot lock source or engine against concurrent modification (Windows " +
                  std::to_wstring(GetLastError()) + L").";
        return false;
    }
};

} // namespace

ConversionResult ExecuteConversion(const ConversionPlan& plan, const std::atomic_bool& cancel) {
    ConversionResult result;
    result.error.reserve(256);
    OutputTransaction output;
    const auto cancelled = [&]() {
        if (!cancel.load()) return false;
        result.outcome = result.committed_paths.empty() ? ConversionOutcome::Cancelled : ConversionOutcome::PartiallyCommitted;
        result.error = L"Conversion cancelled. Already committed files, if any, remain committed.";
        return true;
    };
    const auto run = [&](const SRecordCommand& command, const wchar_t* stage) {
        if (cancelled()) return false;
        ProcessResult process;
        if (!RunProcessCapture(command.args, DirectoryOf(plan.options.input_path), &process, &result.error, &cancel))
            return false;
        if (!process.output.empty()) result.messages.push_back(std::wstring(stage) + L":\r\n" + process.output);
        if (process.cancelled) { cancelled(); return false; }
        if (process.exit_code != 0) {
            result.error = std::wstring(stage) + L" failed with exit code " + std::to_wstring(process.exit_code) + L".";
            return false;
        }
        return true;
    };
    const auto execute = [&]() {
        if (cancelled() || !RevalidatePlan(plan, &result.error)) return;
        ReadLock source, engine;
        if (!source.Open(plan.options.input_path, &result.error) || !engine.Open(plan.options.srec_exe, &result.error) ||
            !RevalidatePlan(plan, &result.error) || !output.Prepare(plan.targets, &result.error)) return;
        // After pinning the output folders, recheck the confirmed identities.
        if (!RevalidatePlan(plan, &result.error) || cancelled()) return;
        SRecordOptions staged = plan.options;
        staged.output_path = output.Stage(0);
        if (plan.has_hex_copy) staged.hex_output_path = output.Stage(1);
        SRecordCommand convert;
        if (!BuildConvertCommand(staged, &convert, &result.error) || !run(convert, L"Convert") ||
            !output.Validate(0, &result.error)) return;
        if (staged.operation != ConvertOperation::BinToHex && (staged.pad_enabled || staged.checksum_enabled)) {
            uint64_t size = 0;
            const uint64_t expected = staged.operation == ConvertOperation::BinToBin ?
                convert.derived.app_binary_size : convert.derived.image_end;
            if (!GetFileSizeBytes(staged.output_path, &size) || size != expected) {
                result.error = L"Generated BIN size does not match the prepared image size.";
                return;
            }
        }
        if (staged.checksum_enabled) {
            SRecordCommand dump;
            if (!BuildChecksumDumpCommand(staged, convert.derived, &dump, &result.error) || !run(dump, L"Checksum")) return;
        }
        if (plan.has_hex_copy) {
            SRecordCommand hex;
            if (!BuildHexCopyCommand(staged, convert.derived, &hex, &result.error) || !run(hex, L"HEX copy") ||
                !output.Validate(1, &result.error)) return;
        }
        // Parse the complete staged outputs with the existing engine, not a UI decoder.
        for (size_t i = 0; i < plan.targets.size(); ++i) {
            SRecordCommand validate;
            validate.args = {staged.srec_exe, output.Stage(i),
                (i == 1 || staged.operation == ConvertOperation::BinToHex) ? L"-Intel" : L"-binary",
                L"-o", L"NUL", L"-Intel"};
            if (!run(validate, L"Validate output")) return;
        }
        if (!RevalidatePlan(plan, &result.error) || cancelled()) return;
        // Allocate outcome bookkeeping before the first irreversible rename.
        auto committed_names = plan.targets;
        result.committed_paths.reserve(committed_names.size());
        for (size_t i = 0; i < plan.targets.size(); ++i) {
            if (cancelled()) return;
            if (!output.Commit(i, plan.target_versions[i], &result.error)) {
                if (!result.committed_paths.empty()) result.outcome = ConversionOutcome::PartiallyCommitted;
                return;
            }
            result.committed_paths.push_back(std::move(committed_names[i]));
        }
        // A cancellation arriving after the final commit cannot undo success.
        result.outcome = ConversionOutcome::Succeeded;
    };
    try { execute(); }
    catch (const std::exception&) { result.error = L"Conversion could not complete due to a runtime/resource error."; }
    catch (...) { result.error = L"Unexpected conversion failure."; }
    if (!result.committed_paths.empty() && result.outcome != ConversionOutcome::Succeeded)
        result.outcome = ConversionOutcome::PartiallyCommitted;
    try { output.Cleanup(&result.messages); }
    catch (...) { result.cleanup_incomplete = true; }
    return result;
}

ConversionJob::~ConversionJob() {
    Cancel();
    if (worker_.joinable()) worker_.join();
}

bool ConversionJob::Start(ConversionPlan plan, std::wstring* error) {
    if (worker_.joinable()) {
        if (error) *error = L"A conversion is already active or awaiting completion.";
        return false;
    }
    cancel_.store(false);
    done_.store(false);
    try {
        worker_ = std::thread([this, plan = std::move(plan)]() {
            try { result_ = ExecuteConversion(plan, cancel_); }
            catch (...) { result_.outcome = ConversionOutcome::Failed; }
            done_.store(true, std::memory_order_release);
        });
    } catch (const std::exception&) {
        if (error) *error = L"Unable to start the conversion worker.";
        return false;
    }
    return true;
}

ConversionResult ConversionJob::TakeResult() {
    if (!Done()) return {};
    if (worker_.joinable()) worker_.join();
    return std::move(result_);
}

} // namespace srecord_tool::core
