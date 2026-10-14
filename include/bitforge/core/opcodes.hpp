#ifndef BITFORGE_CORE_OPCODES_HPP
#define BITFORGE_CORE_OPCODES_HPP

#include "bitforge/core/types.hpp"
#include <string_view>
#include <cstddef>

namespace bitforge {

// Canonical Opcode Enumeration (31 defined instruction mnemonics)
enum class Opcode : byte_t {
    // Basic Control & Data Movement (11 mnemonics)
    NOP   = 0x00,
    HALT  = 0x01,
    LDI   = 0x10,
    MOV   = 0x11,
    LD    = 0x12,
    ST    = 0x13,
    LDI16 = 0x14,
    LDX   = 0x15,
    STX   = 0x16,
    INX   = 0x17,
    DEX   = 0x18,

    // Arithmetic & Logic (9 mnemonics)
    ADD   = 0x20,
    SUB   = 0x21,
    INC   = 0x22,
    DEC   = 0x23,
    AND   = 0x30,
    OR    = 0x31,
    XOR   = 0x32,
    NOT   = 0x33,
    CMP   = 0x40,

    // Branch, Stack & Call (9 mnemonics)
    JMP   = 0x50,
    JZ    = 0x51,
    JNZ   = 0x52,
    JC    = 0x53,
    JNC   = 0x54,
    CALL  = 0x60,
    RET   = 0x61,
    PUSH  = 0x62,
    POP   = 0x63,

    // Input/Output (2 mnemonics)
    IN    = 0x70,
    OUT   = 0x71
};

// Returns true if the given raw byte corresponds to a defined canonical opcode
constexpr bool is_valid_opcode(byte_t op) noexcept {
    switch (op) {
        // Control & Data
        case 0x00: case 0x01: case 0x10: case 0x11: case 0x12:
        case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18:
        // Arithmetic & Logic
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x40:
        // Branch, Stack & Call
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54:
        case 0x60: case 0x61: case 0x62: case 0x63:
        // Input/Output
        case 0x70: case 0x71:
            return true;
        default:
            return false;
    }
}

// Returns the instruction byte length (1, 2, 3, or 4), or 0 if opcode is invalid
constexpr std::size_t instruction_length(byte_t op) noexcept {
    switch (op) {
        // 1-byte instructions
        case 0x00: // NOP
        case 0x01: // HALT
        case 0x17: // INX
        case 0x18: // DEX
        case 0x61: // RET
            return 1;

        // 2-byte instructions
        case 0x15: // LDX Rd, [IX]
        case 0x16: // STX [IX], Rs
        case 0x22: // INC Rd
        case 0x23: // DEC Rd
        case 0x33: // NOT Rd
        case 0x62: // PUSH Rs
        case 0x63: // POP Rd
            return 2;

        // 3-byte instructions
        case 0x10: // LDI Rd, imm8
        case 0x11: // MOV Rd, Rs
        case 0x14: // LDI16 IX, addr16
        case 0x20: // ADD Rd, Rs
        case 0x21: // SUB Rd, Rs
        case 0x30: // AND Rd, Rs
        case 0x31: // OR Rd, Rs
        case 0x32: // XOR Rd, Rs
        case 0x40: // CMP Ra, Rb
        case 0x50: // JMP addr16
        case 0x51: // JZ addr16
        case 0x52: // JNZ addr16
        case 0x53: // JC addr16
        case 0x54: // JNC addr16
        case 0x60: // CALL addr16
        case 0x70: // IN Rd, port8
        case 0x71: // OUT port8, Rs
            return 3;

        // 4-byte instructions
        case 0x12: // LD Rd, [addr16]
        case 0x13: // ST [addr16], Rs
            return 4;

        default:
            return 0;
    }
}

// Returns canonical mnemonic string for an opcode, or "UNKNOWN"
constexpr std::string_view opcode_to_mnemonic(byte_t op) noexcept {
    switch (op) {
        case 0x00: return "NOP";
        case 0x01: return "HALT";
        case 0x10: return "LDI";
        case 0x11: return "MOV";
        case 0x12: return "LD";
        case 0x13: return "ST";
        case 0x14: return "LDI16";
        case 0x15: return "LDX";
        case 0x16: return "STX";
        case 0x17: return "INX";
        case 0x18: return "DEX";
        case 0x20: return "ADD";
        case 0x21: return "SUB";
        case 0x22: return "INC";
        case 0x23: return "DEC";
        case 0x30: return "AND";
        case 0x31: return "OR";
        case 0x32: return "XOR";
        case 0x33: return "NOT";
        case 0x40: return "CMP";
        case 0x50: return "JMP";
        case 0x51: return "JZ";
        case 0x52: return "JNZ";
        case 0x53: return "JC";
        case 0x54: return "JNC";
        case 0x60: return "CALL";
        case 0x61: return "RET";
        case 0x62: return "PUSH";
        case 0x63: return "POP";
        case 0x70: return "IN";
        case 0x71: return "OUT";
        default:   return "UNKNOWN";
    }
}

} // namespace bitforge

#endif // BITFORGE_CORE_OPCODES_HPP
