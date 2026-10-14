#ifndef BITFORGE_CORE_TYPES_HPP
#define BITFORGE_CORE_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace bitforge {

// Fixed-width integer types mandated by BitForge Architecture Specification
using byte_t  = std::uint8_t;   // Exactly 8 bits unsigned
using word_t  = std::uint16_t;  // Exactly 16 bits unsigned
using sbyte_t = std::int8_t;    // Exactly 8 bits signed two's complement
using sword_t = std::int16_t;   // Exactly 16 bits signed two's complement
using reg_id_t = std::uint8_t;  // Register identifier (0..3)
using port_t   = std::uint8_t;  // I/O port address (0..255)

// Big-endian word composition and decomposition helpers
constexpr word_t make_word(byte_t hi, byte_t lo) noexcept {
    return static_cast<word_t>((static_cast<word_t>(hi) << 8) | static_cast<word_t>(lo));
}

constexpr byte_t high_byte(word_t val) noexcept {
    return static_cast<byte_t>((val >> 8) & 0xFF);
}

constexpr byte_t low_byte(word_t val) noexcept {
    return static_cast<byte_t>(val & 0xFF);
}

} // namespace bitforge

#endif // BITFORGE_CORE_TYPES_HPP
