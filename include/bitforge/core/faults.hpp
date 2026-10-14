#ifndef BITFORGE_CORE_FAULTS_HPP
#define BITFORGE_CORE_FAULTS_HPP

#include "bitforge/core/types.hpp"
#include <string>
#include <string_view>

namespace bitforge {

// Defined architectural CPU fault types
enum class FaultType {
    None,
    InvalidOpcode,
    InvalidRegisterEncoding,
    InvalidPort,
    StackOverflow,
    StackUnderflow
};

constexpr std::string_view to_string(FaultType type) noexcept {
    switch (type) {
        case FaultType::None:                    return "None";
        case FaultType::InvalidOpcode:           return "InvalidOpcode";
        case FaultType::InvalidRegisterEncoding: return "InvalidRegisterEncoding";
        case FaultType::InvalidPort:             return "InvalidPort";
        case FaultType::StackOverflow:           return "StackOverflow";
        case FaultType::StackUnderflow:          return "StackUnderflow";
        default:                                 return "UnknownFault";
    }
}

// Architectural fault diagnostic record
struct Fault {
    FaultType type{FaultType::None};
    word_t instruction_address{0};
    std::string message;

    void clear() noexcept {
        type = FaultType::None;
        instruction_address = 0;
        message.clear();
    }

    bool has_fault() const noexcept {
        return type != FaultType::None;
    }
};

} // namespace bitforge

#endif // BITFORGE_CORE_FAULTS_HPP
