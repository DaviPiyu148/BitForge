#ifndef BITFORGE_CORE_REGISTERS_HPP
#define BITFORGE_CORE_REGISTERS_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include <array>
#include <ostream>

namespace bitforge {

// Architectural registers for the BitForge 8-bit processor
struct Registers {
    std::array<byte_t, kGeneralRegisterCount> r{0, 0, 0, 0}; // R0, R1, R2, R3
    word_t ix{0};                                            // 16-bit Index register
    word_t pc{kDefaultEntryAddress};                         // 16-bit Program Counter
    word_t sp{kStackResetValue};                             // 16-bit Stack Pointer (0xFFFF default)

    void reset(word_t entry_point = kDefaultEntryAddress) noexcept {
        r.fill(0);
        ix = 0;
        pc = entry_point;
        sp = kStackResetValue;
    }

    bool operator==(const Registers& other) const noexcept {
        return r == other.r && ix == other.ix && pc == other.pc && sp == other.sp;
    }

    bool operator!=(const Registers& other) const noexcept {
        return !(*this == other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Registers& reg) {
    return os << "[R0=" << static_cast<int>(reg.r[0])
              << ", R1=" << static_cast<int>(reg.r[1])
              << ", R2=" << static_cast<int>(reg.r[2])
              << ", R3=" << static_cast<int>(reg.r[3])
              << ", IX=" << reg.ix
              << ", PC=" << reg.pc
              << ", SP=" << reg.sp << "]";
}

} // namespace bitforge

#endif // BITFORGE_CORE_REGISTERS_HPP
