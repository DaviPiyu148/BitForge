#ifndef BITFORGE_CORE_ALU_HPP
#define BITFORGE_CORE_ALU_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/flags.hpp"

namespace bitforge {

// ALU operation result containing 8-bit output and updated status flags
struct AluResult {
    byte_t result{0};
    Flags flags{};
};

// Pure functional Arithmetic and Logic Unit (ALU)
// Stateless computations with zero side effects on processor state
class Alu {
public:
    // Arithmetic operations
    static AluResult add(byte_t a, byte_t b) noexcept;
    static AluResult sub(byte_t a, byte_t b) noexcept;
    static Flags     cmp(byte_t a, byte_t b) noexcept;

    // Increment and decrement preserve carry flag C
    static AluResult inc(byte_t a, const Flags& current_flags) noexcept;
    static AluResult dec(byte_t a, const Flags& current_flags) noexcept;

    // Bitwise logical operations (C and V are cleared to 0)
    static AluResult and_op(byte_t a, byte_t b) noexcept;
    static AluResult or_op(byte_t a, byte_t b) noexcept;
    static AluResult xor_op(byte_t a, byte_t b) noexcept;
    static AluResult not_op(byte_t a) noexcept;
};

} // namespace bitforge

#endif // BITFORGE_CORE_ALU_HPP
