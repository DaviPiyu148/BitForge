#ifndef BITFORGE_CORE_INSTRUCTION_INFO_HPP
#define BITFORGE_CORE_INSTRUCTION_INFO_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/opcodes.hpp"
#include <string_view>
#include <optional>
#include <array>
#include <cctype>

namespace bitforge {

enum class InstructionGroup : byte_t {
    Control,
    DataMovement,
    Arithmetic,
    Logic,
    Branch,
    Stack,
    Io
};

constexpr std::string_view to_string(InstructionGroup group) noexcept {
    switch (group) {
        case InstructionGroup::Control:      return "Control";
        case InstructionGroup::DataMovement: return "Data Movement";
        case InstructionGroup::Arithmetic:   return "Arithmetic";
        case InstructionGroup::Logic:        return "Logic";
        case InstructionGroup::Branch:       return "Branch";
        case InstructionGroup::Stack:        return "Stack & Call";
        case InstructionGroup::Io:           return "I/O";
        default:                             return "Unknown";
    }
}

struct InstructionDescriptor {
    byte_t opcode{0};
    std::string_view mnemonic{};
    std::size_t length{0};
    std::string_view operand_format{};
    InstructionGroup group{InstructionGroup::Control};
    std::string_view description{};
};

// Complete canonical table for all 31 BitForge instructions
inline constexpr std::array<InstructionDescriptor, 31> kInstructionDescriptors = {{
    {0x00, "NOP",   1, "",              InstructionGroup::Control,      "No operation; advances PC by 1 byte."},
    {0x01, "HALT",  1, "",              InstructionGroup::Control,      "Halts CPU execution until reset."},
    {0x10, "LDI",   3, "Rd, imm8",      InstructionGroup::DataMovement, "Load 8-bit immediate into register Rd."},
    {0x11, "MOV",   3, "Rd, Rs",        InstructionGroup::DataMovement, "Move 8-bit value from Rs to Rd."},
    {0x12, "LD",    4, "Rd, [addr16]",  InstructionGroup::DataMovement, "Load 8-bit value from memory [addr16] into Rd."},
    {0x13, "ST",    4, "[addr16], Rs",  InstructionGroup::DataMovement, "Store 8-bit value from Rs into memory [addr16]."},
    {0x14, "LDI16", 3, "IX, addr16",    InstructionGroup::DataMovement, "Load 16-bit address into index register IX."},
    {0x15, "LDX",   2, "Rd, [IX]",      InstructionGroup::DataMovement, "Load 8-bit value from memory [IX] into Rd."},
    {0x16, "STX",   2, "[IX], Rs",      InstructionGroup::DataMovement, "Store 8-bit value from Rs into memory [IX]."},
    {0x17, "INX",   1, "",              InstructionGroup::DataMovement, "Increment index register IX by 1."},
    {0x18, "DEX",   1, "",              InstructionGroup::DataMovement, "Decrement index register IX by 1."},
    {0x20, "ADD",   3, "Rd, Rs",        InstructionGroup::Arithmetic,   "Add Rs to Rd (Rd = Rd + Rs); updates Z, C, N, V."},
    {0x21, "SUB",   3, "Rd, Rs",        InstructionGroup::Arithmetic,   "Subtract Rs from Rd (Rd = Rd - Rs); updates Z, C, N, V."},
    {0x22, "INC",   2, "Rd",            InstructionGroup::Arithmetic,   "Increment Rd by 1; updates Z, N, V (C unchanged)."},
    {0x23, "DEC",   2, "Rd",            InstructionGroup::Arithmetic,   "Decrement Rd by 1; updates Z, N, V (C unchanged)."},
    {0x30, "AND",   3, "Rd, Rs",        InstructionGroup::Logic,        "Bitwise AND of Rd and Rs; updates Z, N (C=0, V=0)."},
    {0x31, "OR",    3, "Rd, Rs",        InstructionGroup::Logic,        "Bitwise OR of Rd and Rs; updates Z, N (C=0, V=0)."},
    {0x32, "XOR",   3, "Rd, Rs",        InstructionGroup::Logic,        "Bitwise XOR of Rd and Rs; updates Z, N (C=0, V=0)."},
    {0x33, "NOT",   2, "Rd",            InstructionGroup::Logic,        "Bitwise NOT of Rd; updates Z, N (C=0, V=0)."},
    {0x40, "CMP",   3, "Ra, Rb",        InstructionGroup::Arithmetic,   "Compare Ra with Rb (Ra - Rb); updates Z, C, N, V."},
    {0x50, "JMP",   3, "addr16",        InstructionGroup::Branch,       "Unconditional jump to 16-bit target address."},
    {0x51, "JZ",    3, "addr16",        InstructionGroup::Branch,       "Jump to 16-bit address if Zero flag Z=1."},
    {0x52, "JNZ",   3, "addr16",        InstructionGroup::Branch,       "Jump to 16-bit address if Zero flag Z=0."},
    {0x53, "JC",    3, "addr16",        InstructionGroup::Branch,       "Jump to 16-bit address if Carry flag C=1."},
    {0x54, "JNC",   3, "addr16",        InstructionGroup::Branch,       "Jump to 16-bit address if Carry flag C=0."},
    {0x60, "CALL",  3, "addr16",        InstructionGroup::Stack,        "Push return address and call subroutine at addr16."},
    {0x61, "RET",   1, "",              InstructionGroup::Stack,        "Pop return address from stack and resume execution."},
    {0x62, "PUSH",  2, "Rs",            InstructionGroup::Stack,        "Push 8-bit register Rs onto stack."},
    {0x63, "POP",   2, "Rd",            InstructionGroup::Stack,        "Pop 8-bit value from stack into register Rd."},
    {0x70, "IN",    3, "Rd, port8",     InstructionGroup::Io,           "Read 8-bit byte from virtual I/O port into Rd."},
    {0x71, "OUT",   3, "port8, Rs",     InstructionGroup::Io,           "Write 8-bit byte from register Rs to virtual I/O port."}
}};

// Look up instruction descriptor by opcode byte
inline std::optional<InstructionDescriptor> get_instruction_info(byte_t opcode) noexcept {
    for (const auto& desc : kInstructionDescriptors) {
        if (desc.opcode == opcode) {
            return desc;
        }
    }
    return std::nullopt;
}

// Look up instruction descriptor by mnemonic (case-insensitive)
inline std::optional<InstructionDescriptor> find_instruction_info_by_mnemonic(std::string_view mnemonic) noexcept {
    for (const auto& desc : kInstructionDescriptors) {
        if (desc.mnemonic.size() == mnemonic.size()) {
            bool match = true;
            for (std::size_t i = 0; i < mnemonic.size(); ++i) {
                char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(desc.mnemonic[i])));
                char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(mnemonic[i])));
                if (c1 != c2) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return desc;
            }
        }
    }
    return std::nullopt;
}

} // namespace bitforge

#endif // BITFORGE_CORE_INSTRUCTION_INFO_HPP
