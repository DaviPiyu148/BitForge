#ifndef BITFORGE_ASSEMBLER_SOURCE_MAP_HPP
#define BITFORGE_ASSEMBLER_SOURCE_MAP_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/assembler/source_location.hpp"

#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <cstddef>

namespace bitforge::assembler {

// Classification of source lines for debugger breakpoint resolution
enum class SourceLineKind {
    EmptyOrComment,
    LabelOnly,
    DirectiveOnly,
    DataOnly,
    Instruction
};

constexpr std::string_view to_string(SourceLineKind kind) noexcept {
    switch (kind) {
        case SourceLineKind::EmptyOrComment: return "EmptyOrComment";
        case SourceLineKind::LabelOnly:      return "LabelOnly";
        case SourceLineKind::DirectiveOnly:  return "DirectiveOnly";
        case SourceLineKind::DataOnly:       return "DataOnly";
        case SourceLineKind::Instruction:    return "Instruction";
        default:                             return "Unknown";
    }
}

// Source location and metadata for an emitted executable instruction
struct InstructionSourceMapping {
    word_t address{0};           // Start address of the instruction in 64 KiB memory
    SourceLocation location{};   // Line and column in original assembly source
    std::size_t size{0};         // Length in bytes of the instruction (1..4)
    std::string mnemonic{};      // Canonical mnemonic string (e.g. "LDI", "ADD", "HALT")
};

// Line-level source analysis entry
struct SourceLineInfo {
    std::size_t line{0};
    SourceLineKind kind{SourceLineKind::EmptyOrComment};
    std::optional<word_t> instruction_address{std::nullopt};
    std::string description{};
};

// Result of resolving a source line for a debugger breakpoint
struct LineResolutionResult {
    bool success{false};
    word_t address{0};
    std::string error_message{};
};

// Bidirectional source-to-address mapping database
class SourceMap {
public:
    SourceMap() = default;

    void add_instruction(InstructionSourceMapping mapping) {
        instructions_.push_back(std::move(mapping));
    }

    void set_line_count(std::size_t count) {
        if (lines_.size() < count) {
            const std::size_t old_size = lines_.size();
            lines_.resize(count);
            for (std::size_t i = old_size; i < count; ++i) {
                lines_[i].line = i + 1;
                lines_[i].kind = SourceLineKind::EmptyOrComment;
            }
        }
    }

    void set_line_info(std::size_t line, SourceLineKind kind,
                       std::optional<word_t> instruction_addr = std::nullopt,
                       std::string desc = "") {
        if (line == 0) return;
        if (lines_.size() < line) {
            set_line_count(line);
        }
        lines_[line - 1].line = line;
        lines_[line - 1].kind = kind;
        lines_[line - 1].instruction_address = instruction_addr;
        lines_[line - 1].description = std::move(desc);
    }

    [[nodiscard]] const std::vector<InstructionSourceMapping>& instructions() const noexcept {
        return instructions_;
    }

    [[nodiscard]] const std::vector<SourceLineInfo>& lines() const noexcept {
        return lines_;
    }

    // Lookup instruction starting at exact address (null if address is an operand byte or unmapped)
    [[nodiscard]] const InstructionSourceMapping* find_instruction_by_address(word_t address) const noexcept {
        for (const auto& inst : instructions_) {
            if (inst.address == address) {
                return &inst;
            }
        }
        return nullptr;
    }

    // Lookup instruction emitted on a given source line (null if line has no instruction)
    [[nodiscard]] const InstructionSourceMapping* find_instruction_by_line(std::size_t line) const noexcept {
        for (const auto& inst : instructions_) {
            if (inst.location.line == line) {
                return &inst;
            }
        }
        return nullptr;
    }

    // Get line classification and info for a line (null if out of bounds)
    [[nodiscard]] const SourceLineInfo* get_line_info(std::size_t line) const noexcept {
        if (line == 0 || line > lines_.size()) {
            return nullptr;
        }
        return &lines_[line - 1];
    }

    // Resolve a source line to an executable instruction address for breakpoint binding
    [[nodiscard]] LineResolutionResult resolve_line_breakpoint(std::size_t line) const {
        if (line == 0 || line > lines_.size()) {
            return {false, 0, "Line " + std::to_string(line) + " is out of range."};
        }
        const auto& info = lines_[line - 1];
        switch (info.kind) {
            case SourceLineKind::Instruction:
                if (info.instruction_address.has_value()) {
                    return {true, *info.instruction_address, ""};
                }
                return {false, 0, "Line " + std::to_string(line) + " has no bound instruction address."};
            case SourceLineKind::EmptyOrComment:
                return {false, 0, "Line " + std::to_string(line) + " is blank or contains only comments."};
            case SourceLineKind::LabelOnly:
                return {false, 0, "Line " + std::to_string(line) + " contains only a label definition."};
            case SourceLineKind::DirectiveOnly:
                return {false, 0, "Line " + std::to_string(line) + " contains an assembler directive without instructions."};
            case SourceLineKind::DataOnly:
                return {false, 0, "Line " + std::to_string(line) + " contains data declarations (.BYTE, .WORD, .ASCII) but no executable instruction."};
        }
        return {false, 0, "Line " + std::to_string(line) + " cannot be mapped to an executable instruction."};
    }

private:
    std::vector<InstructionSourceMapping> instructions_{};
    std::vector<SourceLineInfo> lines_{};
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_SOURCE_MAP_HPP
