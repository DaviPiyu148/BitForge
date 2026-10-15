#ifndef BITFORGE_DEBUGGER_BREAKPOINT_HPP
#define BITFORGE_DEBUGGER_BREAKPOINT_HPP

#include "bitforge/core/types.hpp"
#include <string>

namespace bitforge::debugger {

// Structured outcome of adding or removing a breakpoint
struct BreakpointResult {
    bool success{false};
    word_t address{0};
    bool duplicate{false};  // True if added address was already an active breakpoint
    bool not_found{false};  // True if removed address was not an active breakpoint
    std::string message{};  // Diagnostic explanation (e.g. failure rationale for unmappable lines)
};

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_BREAKPOINT_HPP
