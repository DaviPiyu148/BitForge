#ifndef BITFORGE_DEBUGGER_EXECUTION_RESULT_HPP
#define BITFORGE_DEBUGGER_EXECUTION_RESULT_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/cpu_state.hpp"
#include "bitforge/core/faults.hpp"

#include <string_view>
#include <optional>
#include <cstdint>

namespace bitforge::debugger {

// Reasons explaining why a debugger step or run operation concluded
enum class StopReason {
    None,            // Execution not yet initiated
    StepCompleted,   // Single instruction step committed successfully
    BreakpointHit,   // Encountered breakpoint before executing instruction at that address
    Halted,          // CPU executed HALT instruction normally
    Faulted,         // CPU encountered an architectural fault
    BudgetExhausted, // Instruction budget limit reached without halting or hitting breakpoint
    AlreadyHalted,   // Stepping or running rejected because CPU is already halted
    AlreadyFaulted   // Stepping or running rejected because CPU is already faulted
};

constexpr std::string_view to_string(StopReason reason) noexcept {
    switch (reason) {
        case StopReason::None:            return "None";
        case StopReason::StepCompleted:   return "StepCompleted";
        case StopReason::BreakpointHit:   return "BreakpointHit";
        case StopReason::Halted:          return "Halted";
        case StopReason::Faulted:         return "Faulted";
        case StopReason::BudgetExhausted: return "BudgetExhausted";
        case StopReason::AlreadyHalted:   return "AlreadyHalted";
        case StopReason::AlreadyFaulted:  return "AlreadyFaulted";
        default:                          return "Unknown";
    }
}

// Structured outcome of an execution request (step, run, or continue)
struct ExecutionResult {
    StopReason stop_reason{StopReason::None};
    MachineState state{MachineState::Ready};
    std::uint64_t steps_executed{0};                        // Steps executed during this operation
    word_t pc{0};                                           // Architectural PC upon completion
    std::optional<word_t> breakpoint_address{std::nullopt}; // Populated if stop_reason is BreakpointHit
    std::optional<Fault> fault{std::nullopt};               // Populated if stop_reason is Faulted

    [[nodiscard]] bool is_breakpoint_hit() const noexcept {
        return stop_reason == StopReason::BreakpointHit;
    }

    [[nodiscard]] bool is_halted() const noexcept {
        return stop_reason == StopReason::Halted || stop_reason == StopReason::AlreadyHalted;
    }

    [[nodiscard]] bool is_faulted() const noexcept {
        return stop_reason == StopReason::Faulted || stop_reason == StopReason::AlreadyFaulted;
    }

    [[nodiscard]] bool is_budget_exhausted() const noexcept {
        return stop_reason == StopReason::BudgetExhausted;
    }
};

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_EXECUTION_RESULT_HPP
