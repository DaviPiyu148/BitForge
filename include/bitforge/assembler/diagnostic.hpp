#ifndef BITFORGE_ASSEMBLER_DIAGNOSTIC_HPP
#define BITFORGE_ASSEMBLER_DIAGNOSTIC_HPP

#include "bitforge/assembler/source_location.hpp"
#include <string>
#include <string_view>

namespace bitforge::assembler {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Info
};

inline std::string_view to_string(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::Error:   return "error";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Info:    return "info";
        default:                          return "diagnostic";
    }
}

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    SourceLocation location{};
    std::string message;

    std::string format() const {
        return location.to_string() + ": " + std::string(to_string(severity)) + ": " + message;
    }
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_DIAGNOSTIC_HPP
