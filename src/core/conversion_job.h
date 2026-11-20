#pragma once

#include <atomic>
#include <thread>
#include "conversion_plan.h"

namespace srecord_tool::core {

enum class ConversionOutcome { Failed, Cancelled, Succeeded, PartiallyCommitted };

struct ConversionResult {
    ConversionOutcome outcome = ConversionOutcome::Failed;
    std::wstring error;
    std::vector<std::wstring> messages;
    std::vector<std::wstring> committed_paths;
    bool cleanup_incomplete = false;
};

ConversionResult ExecuteConversion(const ConversionPlan& plan, const std::atomic_bool& cancel);

// The owner polls completion; no window messages own heap payloads or worker state.
class ConversionJob {
public:
    ~ConversionJob();
    bool Start(ConversionPlan plan, std::wstring* error);
    void Cancel() { cancel_.store(true); }
    bool Done() const { return done_.load(std::memory_order_acquire); }
    ConversionResult TakeResult();

private:
    std::atomic_bool cancel_{false};
    std::atomic_bool done_{false};
    std::thread worker_;
    ConversionResult result_;
};

} // namespace srecord_tool::core
