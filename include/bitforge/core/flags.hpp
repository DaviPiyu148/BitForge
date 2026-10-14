#ifndef BITFORGE_CORE_FLAGS_HPP
#define BITFORGE_CORE_FLAGS_HPP

#include "bitforge/core/types.hpp"
#include <ostream>

namespace bitforge {

// Status flags structure (Z, C, N, V)
struct Flags {
    bool z{false}; // Zero flag: set when operation result is 0
    bool c{false}; // Carry/no-borrow flag: carry out of bit 7 for add; (a >= b) for sub/cmp
    bool n{false}; // Negative flag: copies bit 7 of 8-bit result
    bool v{false}; // Signed overflow flag: two's complement overflow (-128..127)

    constexpr void reset() noexcept {
        z = false;
        c = false;
        n = false;
        v = false;
    }

    constexpr bool operator==(const Flags& other) const noexcept {
        return z == other.z && c == other.c && n == other.n && v == other.v;
    }

    constexpr bool operator!=(const Flags& other) const noexcept {
        return !(*this == other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Flags& f) {
    return os << "[Z=" << f.z << ", C=" << f.c << ", N=" << f.n << ", V=" << f.v << "]";
}

} // namespace bitforge

#endif // BITFORGE_CORE_FLAGS_HPP
