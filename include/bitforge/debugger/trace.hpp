#ifndef BITFORGE_DEBUGGER_TRACE_HPP
#define BITFORGE_DEBUGGER_TRACE_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/registers.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/core/cpu_state.hpp"
#include "bitforge/core/faults.hpp"
#include "bitforge/assembler/source_location.hpp"

#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace bitforge::debugger {

constexpr std::size_t kDefaultTraceCapacity = 100;

// Record of a single instruction execution step
struct TraceEntry {
    std::uint64_t step_number{0};                          // Monotonically increasing step number
    word_t instruction_address{0};                         // Address of instruction before execution
    byte_t opcode_raw{0};                                  // Raw opcode byte fetched
    std::string mnemonic{};                                // Canonical mnemonic (e.g. "LDI", "ADD", "HALT")
    std::vector<byte_t> instruction_bytes{};               // Full instruction bytes (1..4)
    std::optional<assembler::SourceLocation> source_location{std::nullopt};
    MachineState state_after{MachineState::Ready};          // Machine state after step
    bool faulted{false};                                   // True if step faulted
    std::optional<Fault> fault{std::nullopt};              // Populated if step faulted
    Registers registers_after{};                           // Register values after step
    Flags flags_after{};                                   // Flag values after step
};

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_TRACE_HPP
