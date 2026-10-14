#ifndef BITFORGE_CORE_CPU_HPP
#define BITFORGE_CORE_CPU_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/opcodes.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/core/registers.hpp"
#include "bitforge/core/cpu_state.hpp"
#include "bitforge/core/faults.hpp"
#include "bitforge/bus/bus_interface.hpp"

namespace bitforge {

// Result of executing at most one CPU instruction step
struct StepResult {
    bool executed{false};       // True if a supported instruction committed successfully
    bool halted{false};         // True if execution stopped due to HALT
    bool faulted{false};        // True if execution resulted in a CPU fault
    bool unhandled{false};      // True if opcode is a valid Phase 2 instruction not yet implemented
    byte_t opcode_raw{0};       // Raw byte of the fetched opcode
    Opcode opcode{Opcode::NOP}; // Decoded opcode if valid, or NOP
    word_t instruction_address{0}; // Starting address (PC) of the instruction
};

// BitForge 8-Bit Central Processing Unit
class Cpu {
public:
    explicit Cpu(IBus& bus) noexcept;

    // Reset CPU registers, flags, fault info, and transition state to Ready
    void reset(word_t entry_point = kDefaultEntryAddress) noexcept;

    // Step at most one instruction from Ready, Running, or Paused
    // Halted and Faulted states are explicit no-ops returning executed = false
    StepResult step();

    // Architectural state read-only inspection accessors
    [[nodiscard]] const Registers& registers() const noexcept { return registers_; }
    [[nodiscard]] const Flags& flags() const noexcept { return flags_; }
    [[nodiscard]] MachineState state() const noexcept { return state_; }
    [[nodiscard]] const Fault& last_fault() const noexcept { return last_fault_; }

    [[nodiscard]] const IBus& bus() const noexcept { return bus_; }
    [[nodiscard]] IBus& bus() noexcept { return bus_; }

private:
    IBus& bus_;
    Registers registers_{};
    Flags flags_{};
    MachineState state_{MachineState::Ready};
    Fault last_fault_{};

    // Internal helper to record an architectural fault and halt execution
    StepResult record_fault(FaultType type, word_t pc, std::string msg);
};

} // namespace bitforge

#endif // BITFORGE_CORE_CPU_HPP
