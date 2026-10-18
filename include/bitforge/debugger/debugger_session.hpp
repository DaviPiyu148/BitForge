#ifndef BITFORGE_DEBUGGER_DEBUGGER_SESSION_HPP
#define BITFORGE_DEBUGGER_DEBUGGER_SESSION_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/assembler/program_image.hpp"
#include "bitforge/debugger/execution_result.hpp"
#include "bitforge/debugger/breakpoint.hpp"
#include "bitforge/debugger/inspection.hpp"
#include "bitforge/debugger/trace.hpp"

#include <vector>
#include <set>
#include <deque>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace bitforge::debugger {

// Synchronous, deterministic debugger session engine
class DebuggerSession {
public:
    explicit DebuggerSession(Cpu& cpu, const assembler::ProgramImage* image = nullptr,
                            std::size_t trace_capacity = kDefaultTraceCapacity);

    // Attach or replace the active program image containing source mapping information
    void set_program_image(const assembler::ProgramImage* image) noexcept;
    [[nodiscard]] const assembler::ProgramImage* program_image() const noexcept;

    // Reset CPU registers and flags, with entry point override or default from program image
    void reset(std::optional<word_t> entry_point = std::nullopt);

    // -----------------------------------------------------------------
    // Execution Control
    // -----------------------------------------------------------------

    // Execute at most one instruction step
    // Bypasses any breakpoint at the current PC for this single step
    ExecutionResult step();

    // Run until a breakpoint, HALT, fault, or max_steps budget is reached
    ExecutionResult run(std::uint64_t max_steps = kDefaultInstructionBudget);

    // Resume execution after a breakpoint stop
    // Uses one-time bypass so the instruction at the breakpoint address executes and continues
    ExecutionResult continue_execution(std::uint64_t max_steps = kDefaultInstructionBudget);

    // Reason why the most recent execution operation paused or terminated
    [[nodiscard]] StopReason last_stop_reason() const noexcept;

    // -----------------------------------------------------------------
    // Breakpoint Management
    // -----------------------------------------------------------------

    // Add address breakpoint across valid 16-bit address space
    BreakpointResult add_breakpoint(word_t address);

    // Remove address breakpoint
    BreakpointResult remove_breakpoint(word_t address);

    // Query whether an address breakpoint is active
    [[nodiscard]] bool has_breakpoint(word_t address) const noexcept;

    // Retrieve list of all active address breakpoints (sorted ascending)
    [[nodiscard]] std::vector<word_t> breakpoints() const;

    // Remove all breakpoints
    void clear_breakpoints() noexcept;

    // Add source-line breakpoint using the program image's source map
    // Fails predictably with explanatory diagnostic if line cannot be mapped
    BreakpointResult add_line_breakpoint(std::size_t line);

    // Remove source-line breakpoint
    BreakpointResult remove_line_breakpoint(std::size_t line);

    // -----------------------------------------------------------------
    // State and Memory Inspection (Read-Only, Side-Effect Free)
    // -----------------------------------------------------------------

    [[nodiscard]] MachineState state() const noexcept;
    [[nodiscard]] word_t pc() const noexcept;
    [[nodiscard]] word_t sp() const noexcept;
    [[nodiscard]] word_t ix() const noexcept;
    [[nodiscard]] byte_t r(reg_id_t reg) const;
    [[nodiscard]] const Registers& registers() const noexcept;
    [[nodiscard]] const Flags& flags() const noexcept;
    [[nodiscard]] const Fault& last_fault() const noexcept;
    [[nodiscard]] const Cpu& cpu() const noexcept;
    [[nodiscard]] Cpu& cpu() noexcept;

    // Resolve current PC (or specified address) back to assembly source line and column
    [[nodiscard]] std::optional<assembler::SourceLocation> resolve_source_location(
        std::optional<word_t> address = std::nullopt) const noexcept;

    // Read a single byte from memory with zero device side effects
    [[nodiscard]] byte_t read_memory_byte(word_t address) const noexcept;

    // Read a bounded range of memory with overflow-safe 16-bit wrapping
    [[nodiscard]] MemoryRangeResult read_memory_range(word_t start_address, std::size_t count) const;

    // Inspect stack contents conforming to ADR-001 (0xF000..0xFFFE, SP=0xFFFF empty)
    [[nodiscard]] StackInspectionResult inspect_stack() const;

    // -----------------------------------------------------------------
    // Execution Trace
    // -----------------------------------------------------------------

    // Chronological execution trace (oldest to newest)
    [[nodiscard]] const std::vector<TraceEntry>& trace() const noexcept;
    [[nodiscard]] std::size_t trace_capacity() const noexcept;
    void set_trace_capacity(std::size_t capacity);
    void clear_trace() noexcept;

    // Total steps executed across session lifetime
    [[nodiscard]] std::uint64_t total_steps_executed() const noexcept;

    // Structured delta for the most recently executed instruction
    [[nodiscard]] const std::optional<ExecutionDelta>& last_delta() const noexcept { return last_delta_; }

private:
    Cpu& cpu_;
    const assembler::ProgramImage* image_{nullptr};

    std::set<word_t> breakpoints_{};
    std::optional<word_t> last_breakpoint_hit_{std::nullopt};
    StopReason last_stop_reason_{StopReason::None};

    std::size_t trace_capacity_{kDefaultTraceCapacity};
    std::deque<TraceEntry> trace_buffer_{};
    mutable std::vector<TraceEntry> trace_cache_{};
    mutable bool trace_cache_dirty_{false};

    std::uint64_t total_steps_executed_{0};
    std::optional<ExecutionDelta> last_delta_{std::nullopt};

    // Internal helper to step CPU once, update trace, and return StepResult
    StepResult execute_single_step();

    // Internal shared execution runner
    ExecutionResult run_internal(std::uint64_t max_steps, bool continue_mode);
};

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_DEBUGGER_SESSION_HPP
