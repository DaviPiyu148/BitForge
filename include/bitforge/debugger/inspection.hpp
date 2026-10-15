#ifndef BITFORGE_DEBUGGER_INSPECTION_HPP
#define BITFORGE_DEBUGGER_INSPECTION_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"

#include <vector>
#include <string>
#include <cstddef>

namespace bitforge::debugger {

// Single occupied memory location within the stack region
struct StackFrameEntry {
    word_t address{0}; // Memory address in stack range [0xF000, 0xFFFE]
    byte_t value{0};   // Value stored at this stack location
};

// Result of architectural stack inspection
struct StackInspectionResult {
    bool is_empty{true};
    word_t sp{kStackResetValue};
    std::size_t depth{0};                  // Total occupied bytes on stack
    std::vector<StackFrameEntry> entries;  // Ordered from top-of-stack (SP) down to bottom (0xFFFE)
    bool overflow_or_corrupt{false};       // True if SP is out of valid bounds (< 0xF000)
    std::string message{};
};

// Result of safe, side-effect-free memory range inspection
struct MemoryRangeResult {
    bool success{true};
    word_t start_address{0};
    std::vector<byte_t> data;
    std::string error_message{};
};

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_INSPECTION_HPP
