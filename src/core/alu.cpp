#include "bitforge/core/alu.hpp"

namespace bitforge {

AluResult Alu::add(byte_t a, byte_t b) noexcept {
    const unsigned int sum = static_cast<unsigned int>(a) + static_cast<unsigned int>(b);
    const byte_t r = static_cast<byte_t>(sum & 0xFF);
    Flags f;
    f.z = (r == 0);
    f.c = (sum > 0xFF);
    f.n = (r & 0x80) != 0;
    // Overflow if operands have same sign, but result has different sign
    f.v = ((~(a ^ b) & (a ^ r) & 0x80) != 0);
    return {r, f};
}

AluResult Alu::sub(byte_t a, byte_t b) noexcept {
    const byte_t r = static_cast<byte_t>((static_cast<int>(a) - static_cast<int>(b)) & 0xFF);
    Flags f;
    f.z = (r == 0);
    f.c = (a >= b); // C = 1 when subtraction produces no unsigned borrow
    f.n = (r & 0x80) != 0;
    // Overflow if operands have different signs, and result has different sign from minuend
    f.v = (((a ^ b) & (a ^ r) & 0x80) != 0);
    return {r, f};
}

Flags Alu::cmp(byte_t a, byte_t b) noexcept {
    return sub(a, b).flags;
}

AluResult Alu::inc(byte_t a, const Flags& current_flags) noexcept {
    const byte_t r = static_cast<byte_t>((static_cast<unsigned int>(a) + 1u) & 0xFF);
    Flags f = current_flags; // Carry flag is explicitly preserved
    f.z = (r == 0);
    f.n = (r & 0x80) != 0;
    f.v = (a == 0x7F); // Overflow when 0x7F (+127) increments to 0x80 (-128)
    return {r, f};
}

AluResult Alu::dec(byte_t a, const Flags& current_flags) noexcept {
    const byte_t r = static_cast<byte_t>((static_cast<unsigned int>(a) + 0xFFu) & 0xFF);
    Flags f = current_flags; // Carry flag is explicitly preserved
    f.z = (r == 0);
    f.n = (r & 0x80) != 0;
    f.v = (a == 0x80); // Overflow when 0x80 (-128) decrements to 0x7F (+127)
    return {r, f};
}

AluResult Alu::and_op(byte_t a, byte_t b) noexcept {
    const byte_t r = static_cast<byte_t>(a & b);
    Flags f;
    f.z = (r == 0);
    f.c = false;
    f.n = (r & 0x80) != 0;
    f.v = false;
    return {r, f};
}

AluResult Alu::or_op(byte_t a, byte_t b) noexcept {
    const byte_t r = static_cast<byte_t>(a | b);
    Flags f;
    f.z = (r == 0);
    f.c = false;
    f.n = (r & 0x80) != 0;
    f.v = false;
    return {r, f};
}

AluResult Alu::xor_op(byte_t a, byte_t b) noexcept {
    const byte_t r = static_cast<byte_t>(a ^ b);
    Flags f;
    f.z = (r == 0);
    f.c = false;
    f.n = (r & 0x80) != 0;
    f.v = false;
    return {r, f};
}

AluResult Alu::not_op(byte_t a) noexcept {
    const byte_t r = static_cast<byte_t>((~a) & 0xFF);
    Flags f;
    f.z = (r == 0);
    f.c = false;
    f.n = (r & 0x80) != 0;
    f.v = false;
    return {r, f};
}

} // namespace bitforge
