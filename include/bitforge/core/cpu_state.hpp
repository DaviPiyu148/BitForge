#ifndef BITFORGE_CORE_CPU_STATE_HPP
#define BITFORGE_CORE_CPU_STATE_HPP

#include <string_view>

namespace bitforge {

// Machine state lifecycle representation
enum class MachineState {
    Ready,   // Machine initialized/reset, ready to execute
    Running, // Continuous instruction execution active
    Paused,  // Single-stepping or user-requested pause
    Halted,  // Execution stopped normally via HALT instruction
    Faulted  // CPU fault encountered, architectural execution halted
};

constexpr std::string_view to_string(MachineState state) noexcept {
    switch (state) {
        case MachineState::Ready:   return "Ready";
        case MachineState::Running: return "Running";
        case MachineState::Paused:  return "Paused";
        case MachineState::Halted:  return "Halted";
        case MachineState::Faulted: return "Faulted";
        default:                    return "Unknown";
    }
}

} // namespace bitforge

#endif // BITFORGE_CORE_CPU_STATE_HPP
