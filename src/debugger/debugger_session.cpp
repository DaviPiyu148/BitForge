#include "bitforge/debugger/debugger_session.hpp"
#include "bitforge/core/opcodes.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace bitforge::debugger {

namespace {

std::string to_hex_word(word_t w) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << w;
    return oss.str();
}

} // namespace

DebuggerSession::DebuggerSession(Cpu& cpu, const assembler::ProgramImage* image,
                                 std::size_t trace_capacity)
    : cpu_(cpu), image_(image), trace_capacity_(trace_capacity) {}

void DebuggerSession::set_program_image(const assembler::ProgramImage* image) noexcept {
    image_ = image;
}

const assembler::ProgramImage* DebuggerSession::program_image() const noexcept {
    return image_;
}

void DebuggerSession::reset(std::optional<word_t> entry_point) {
    word_t ep = kDefaultEntryAddress;
    if (entry_point.has_value()) {
        ep = *entry_point;
    } else if (image_ != nullptr) {
        ep = image_->entry_point();
    }
    cpu_.reset(ep);
    last_breakpoint_hit_.reset();
    last_stop_reason_ = StopReason::None;
}

// -----------------------------------------------------------------
// Execution Control
// -----------------------------------------------------------------

StepResult DebuggerSession::execute_single_step() {
    const word_t start_pc = cpu_.registers().pc;
    const byte_t opcode_raw = read_memory_byte(start_pc);

    // Determine mnemonic and instruction size before step
    std::string mnemonic;
    std::size_t inst_size = 1;
    std::optional<assembler::SourceLocation> src_loc = resolve_source_location(start_pc);

    if (image_ != nullptr) {
        const auto* mapping = image_->source_map().find_instruction_by_address(start_pc);
        if (mapping != nullptr) {
            mnemonic = mapping->mnemonic;
            inst_size = mapping->size;
        }
    }
    if (mnemonic.empty()) {
        if (is_valid_opcode(opcode_raw)) {
            mnemonic = std::string(opcode_to_mnemonic(opcode_raw));
            inst_size = instruction_length(opcode_raw);
        } else {
            mnemonic = "UNKNOWN";
            inst_size = 1;
        }
    }

    // Read raw instruction bytes
    std::vector<byte_t> raw_bytes;
    raw_bytes.reserve(inst_size);
    for (std::size_t i = 0; i < inst_size; ++i) {
        raw_bytes.push_back(read_memory_byte(static_cast<word_t>((start_pc + i) & 0xFFFF)));
    }

    // Perform instruction step on CPU
    StepResult res = cpu_.step();
    ++total_steps_executed_;

    // Construct trace entry
    TraceEntry entry;
    entry.step_number = total_steps_executed_;
    entry.instruction_address = start_pc;
    entry.opcode_raw = opcode_raw;
    entry.mnemonic = std::move(mnemonic);
    entry.instruction_bytes = std::move(raw_bytes);
    entry.source_location = src_loc;
    entry.state_after = cpu_.state();
    entry.faulted = res.faulted || cpu_.state() == MachineState::Faulted;
    if (entry.faulted && cpu_.last_fault().has_fault()) {
        entry.fault = cpu_.last_fault();
    }
    entry.registers_after = cpu_.registers();
    entry.flags_after = cpu_.flags();

    // Store into bounded trace ring buffer
    if (trace_capacity_ > 0) {
        if (trace_buffer_.size() >= trace_capacity_) {
            trace_buffer_.pop_front();
        }
        trace_buffer_.push_back(std::move(entry));
        trace_cache_dirty_ = true;
    }

    return res;
}

ExecutionResult DebuggerSession::step() {
    if (cpu_.state() == MachineState::Halted) {
        last_stop_reason_ = StopReason::AlreadyHalted;
        return ExecutionResult{
            StopReason::AlreadyHalted,
            cpu_.state(),
            0,
            cpu_.registers().pc,
            std::nullopt,
            std::nullopt
        };
    }
    if (cpu_.state() == MachineState::Faulted) {
        last_stop_reason_ = StopReason::AlreadyFaulted;
        return ExecutionResult{
            StopReason::AlreadyFaulted,
            cpu_.state(),
            0,
            cpu_.registers().pc,
            std::nullopt,
            cpu_.last_fault()
        };
    }

    // Single-step always executes the instruction at current PC, even if that address has a breakpoint
    last_breakpoint_hit_.reset();

    StepResult step_res = execute_single_step();

    if (step_res.faulted || cpu_.state() == MachineState::Faulted) {
        last_stop_reason_ = StopReason::Faulted;
        return ExecutionResult{
            StopReason::Faulted,
            cpu_.state(),
            1,
            cpu_.registers().pc,
            std::nullopt,
            cpu_.last_fault()
        };
    }

    if (step_res.halted || cpu_.state() == MachineState::Halted) {
        last_stop_reason_ = StopReason::Halted;
        return ExecutionResult{
            StopReason::Halted,
            cpu_.state(),
            1,
            cpu_.registers().pc,
            std::nullopt,
            std::nullopt
        };
    }

    last_stop_reason_ = StopReason::StepCompleted;
    return ExecutionResult{
        StopReason::StepCompleted,
        cpu_.state(),
        1,
        cpu_.registers().pc,
        std::nullopt,
        std::nullopt
    };
}

ExecutionResult DebuggerSession::run_internal(std::uint64_t max_steps, bool continue_mode) {
    if (cpu_.state() == MachineState::Halted) {
        last_stop_reason_ = StopReason::AlreadyHalted;
        return ExecutionResult{
            StopReason::AlreadyHalted,
            cpu_.state(),
            0,
            cpu_.registers().pc,
            std::nullopt,
            std::nullopt
        };
    }
    if (cpu_.state() == MachineState::Faulted) {
        last_stop_reason_ = StopReason::AlreadyFaulted;
        return ExecutionResult{
            StopReason::AlreadyFaulted,
            cpu_.state(),
            0,
            cpu_.registers().pc,
            std::nullopt,
            cpu_.last_fault()
        };
    }

    if (max_steps == 0) {
        last_stop_reason_ = StopReason::BudgetExhausted;
        return ExecutionResult{
            StopReason::BudgetExhausted,
            cpu_.state(),
            0,
            cpu_.registers().pc,
            std::nullopt,
            std::nullopt
        };
    }

    // Determine one-time bypass if continuing from a breakpoint stop
    bool bypass_initial_breakpoint = false;
    if (continue_mode && last_breakpoint_hit_.has_value() &&
        *last_breakpoint_hit_ == cpu_.registers().pc) {
        bypass_initial_breakpoint = true;
    }
    last_breakpoint_hit_.reset();

    std::uint64_t steps_taken = 0;

    while (steps_taken < max_steps) {
        const word_t current_pc = cpu_.registers().pc;

        // Check for breakpoint prior to executing instruction at current_pc
        if (has_breakpoint(current_pc)) {
            if (bypass_initial_breakpoint) {
                bypass_initial_breakpoint = false; // Bypassed for this one initial step
            } else {
                last_breakpoint_hit_ = current_pc;
                last_stop_reason_ = StopReason::BreakpointHit;
                return ExecutionResult{
                    StopReason::BreakpointHit,
                    cpu_.state(),
                    steps_taken,
                    current_pc,
                    current_pc,
                    std::nullopt
                };
            }
        }

        StepResult step_res = execute_single_step();
        ++steps_taken;

        if (step_res.faulted || cpu_.state() == MachineState::Faulted) {
            last_stop_reason_ = StopReason::Faulted;
            return ExecutionResult{
                StopReason::Faulted,
                cpu_.state(),
                steps_taken,
                cpu_.registers().pc,
                std::nullopt,
                cpu_.last_fault()
            };
        }

        if (step_res.halted || cpu_.state() == MachineState::Halted) {
            last_stop_reason_ = StopReason::Halted;
            return ExecutionResult{
                StopReason::Halted,
                cpu_.state(),
                steps_taken,
                cpu_.registers().pc,
                std::nullopt,
                std::nullopt
            };
        }
    }

    last_stop_reason_ = StopReason::BudgetExhausted;
    return ExecutionResult{
        StopReason::BudgetExhausted,
        cpu_.state(),
        steps_taken,
        cpu_.registers().pc,
        std::nullopt,
        std::nullopt
    };
}

ExecutionResult DebuggerSession::run(std::uint64_t max_steps) {
    return run_internal(max_steps, false);
}

ExecutionResult DebuggerSession::continue_execution(std::uint64_t max_steps) {
    return run_internal(max_steps, true);
}

StopReason DebuggerSession::last_stop_reason() const noexcept {
    return last_stop_reason_;
}

// -----------------------------------------------------------------
// Breakpoint Management
// -----------------------------------------------------------------

BreakpointResult DebuggerSession::add_breakpoint(word_t address) {
    auto [it, inserted] = breakpoints_.insert(address);
    if (!inserted) {
        return BreakpointResult{
            true,
            address,
            true,
            false,
            "Breakpoint already active at address " + to_hex_word(address)
        };
    }
    return BreakpointResult{
        true,
        address,
        false,
        false,
        "Breakpoint added at address " + to_hex_word(address)
    };
}

BreakpointResult DebuggerSession::remove_breakpoint(word_t address) {
    auto it = breakpoints_.find(address);
    if (it == breakpoints_.end()) {
        return BreakpointResult{
            false,
            address,
            false,
            true,
            "No breakpoint exists at address " + to_hex_word(address)
        };
    }
    breakpoints_.erase(it);
    return BreakpointResult{
        true,
        address,
        false,
        false,
        "Breakpoint removed at address " + to_hex_word(address)
    };
}

bool DebuggerSession::has_breakpoint(word_t address) const noexcept {
    return breakpoints_.find(address) != breakpoints_.end();
}

std::vector<word_t> DebuggerSession::breakpoints() const {
    return std::vector<word_t>(breakpoints_.begin(), breakpoints_.end());
}

void DebuggerSession::clear_breakpoints() noexcept {
    breakpoints_.clear();
    last_breakpoint_hit_.reset();
}

BreakpointResult DebuggerSession::add_line_breakpoint(std::size_t line) {
    if (image_ == nullptr) {
        return BreakpointResult{
            false,
            0,
            false,
            false,
            "No program image attached to debugger session."
        };
    }
    auto resolution = image_->source_map().resolve_line_breakpoint(line);
    if (!resolution.success) {
        return BreakpointResult{
            false,
            0,
            false,
            false,
            resolution.error_message
        };
    }
    return add_breakpoint(resolution.address);
}

BreakpointResult DebuggerSession::remove_line_breakpoint(std::size_t line) {
    if (image_ == nullptr) {
        return BreakpointResult{
            false,
            0,
            false,
            false,
            "No program image attached to debugger session."
        };
    }
    auto resolution = image_->source_map().resolve_line_breakpoint(line);
    if (!resolution.success) {
        return BreakpointResult{
            false,
            0,
            false,
            false,
            resolution.error_message
        };
    }
    return remove_breakpoint(resolution.address);
}

// -----------------------------------------------------------------
// State and Memory Inspection (Read-Only, Side-Effect Free)
// -----------------------------------------------------------------

MachineState DebuggerSession::state() const noexcept {
    return cpu_.state();
}

word_t DebuggerSession::pc() const noexcept {
    return cpu_.registers().pc;
}

word_t DebuggerSession::sp() const noexcept {
    return cpu_.registers().sp;
}

word_t DebuggerSession::ix() const noexcept {
    return cpu_.registers().ix;
}

byte_t DebuggerSession::r(reg_id_t reg) const {
    if (reg > kMaxRegisterId) return 0;
    return cpu_.registers().r[reg];
}

const Registers& DebuggerSession::registers() const noexcept {
    return cpu_.registers();
}

const Flags& DebuggerSession::flags() const noexcept {
    return cpu_.flags();
}

const Fault& DebuggerSession::last_fault() const noexcept {
    return cpu_.last_fault();
}

const Cpu& DebuggerSession::cpu() const noexcept {
    return cpu_;
}

Cpu& DebuggerSession::cpu() noexcept {
    return cpu_;
}

std::optional<assembler::SourceLocation> DebuggerSession::resolve_source_location(
    std::optional<word_t> address) const noexcept {
    if (image_ == nullptr) {
        return std::nullopt;
    }
    const word_t addr = address.has_value() ? *address : cpu_.registers().pc;
    const auto* mapping = image_->source_map().find_instruction_by_address(addr);
    if (mapping != nullptr) {
        return mapping->location;
    }
    return std::nullopt;
}

byte_t DebuggerSession::read_memory_byte(word_t address) const noexcept {
    // Pure memory read via bus with zero device side-effects
    return cpu_.bus().read_byte(address);
}

MemoryRangeResult DebuggerSession::read_memory_range(word_t start_address, std::size_t count) const {
    MemoryRangeResult result;
    result.start_address = start_address;

    if (count == 0) {
        return result;
    }

    // Clamp count to kMemorySize (65536) to prevent excessive memory allocation
    const std::size_t bounded_count = std::min(count, kMemorySize);
    result.data.reserve(bounded_count);

    for (std::size_t i = 0; i < bounded_count; ++i) {
        const word_t addr = static_cast<word_t>((start_address + i) & 0xFFFF);
        result.data.push_back(read_memory_byte(addr));
    }

    return result;
}

StackInspectionResult DebuggerSession::inspect_stack() const {
    StackInspectionResult result;
    result.sp = cpu_.registers().sp;

    if (result.sp == kStackResetValue) { // 0xFFFF
        result.is_empty = true;
        result.depth = 0;
        result.message = "Stack is empty (SP = 0xFFFF).";
        return result;
    }

    result.is_empty = false;

    // Check bounds: valid occupied stack SP is 0xF000 .. 0xFFFE
    if (result.sp < kStackReservedStart || result.sp > kStackReservedEnd) {
        result.overflow_or_corrupt = true;
        result.message = "Stack pointer is out of valid stack range [0xF000, 0xFFFE]: " +
                         to_hex_word(result.sp);
        return result;
    }

    // Total pushed bytes is 0xFFFF - SP
    result.depth = static_cast<std::size_t>(kStackResetValue - result.sp);
    result.entries.reserve(result.depth);

    // Read stack from top-of-stack (SP) down to bottom (0xFFFE)
    for (std::uint32_t addr = result.sp; addr <= kStackReservedEnd; ++addr) {
        const word_t waddr = static_cast<word_t>(addr);
        result.entries.push_back(StackFrameEntry{
            waddr,
            read_memory_byte(waddr)
        });
    }

    result.message = "Stack contains " + std::to_string(result.depth) + " byte(s).";
    return result;
}

// -----------------------------------------------------------------
// Execution Trace
// -----------------------------------------------------------------

const std::vector<TraceEntry>& DebuggerSession::trace() const noexcept {
    if (trace_cache_dirty_) {
        trace_cache_.assign(trace_buffer_.begin(), trace_buffer_.end());
        trace_cache_dirty_ = false;
    }
    return trace_cache_;
}

std::size_t DebuggerSession::trace_capacity() const noexcept {
    return trace_capacity_;
}

void DebuggerSession::set_trace_capacity(std::size_t capacity) {
    trace_capacity_ = capacity;
    while (trace_buffer_.size() > trace_capacity_) {
        trace_buffer_.pop_front();
    }
    trace_cache_dirty_ = true;
}

void DebuggerSession::clear_trace() noexcept {
    trace_buffer_.clear();
    trace_cache_.clear();
    trace_cache_dirty_ = false;
}

std::uint64_t DebuggerSession::total_steps_executed() const noexcept {
    return total_steps_executed_;
}

} // namespace bitforge::debugger
