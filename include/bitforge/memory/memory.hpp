#ifndef BITFORGE_MEMORY_MEMORY_HPP
#define BITFORGE_MEMORY_MEMORY_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include <array>
#include <vector>
#include <cstddef>

namespace bitforge {

// Flat 64 KiB byte-addressable system memory (65,536 bytes)
class Memory {
public:
    Memory() noexcept;

    // Direct byte access with modulo 65,536 address safety
    byte_t read_byte(word_t address) const noexcept;
    void write_byte(word_t address, byte_t value) noexcept;

    // Big-endian 16-bit word access with safe boundary wrapping across 0xFFFF/0x0000
    word_t read_word(word_t address) const noexcept;
    void write_word(word_t address, word_t value) noexcept;

    // Clear all 65,536 bytes to 0x00
    void reset() noexcept;

    // Load contiguous byte segment into memory.
    // Returns false if segment would exceed the 64 KiB physical address space.
    bool load_segment(word_t start_address, const byte_t* data, std::size_t length) noexcept;
    bool load_segment(word_t start_address, const std::vector<byte_t>& data) noexcept;

    // Inspection accessors for debugger/UI
    const byte_t* data() const noexcept { return storage_.data(); }
    std::size_t size() const noexcept { return storage_.size(); }
    const std::array<byte_t, kMemorySize>& raw_storage() const noexcept { return storage_; }

private:
    std::array<byte_t, kMemorySize> storage_{};
};

} // namespace bitforge

#endif // BITFORGE_MEMORY_MEMORY_HPP
