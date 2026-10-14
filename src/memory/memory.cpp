#include "bitforge/memory/memory.hpp"
#include <algorithm>

namespace bitforge {

Memory::Memory() noexcept {
    reset();
}

byte_t Memory::read_byte(word_t address) const noexcept {
    // Address is 16-bit unsigned (0x0000..0xFFFF), guaranteeing index is within [0, 65535]
    return storage_[address];
}

void Memory::write_byte(word_t address, byte_t value) noexcept {
    storage_[address] = value;
}

word_t Memory::read_word(word_t address) const noexcept {
    const byte_t hi = storage_[address];
    const word_t next_addr = static_cast<word_t>((address + 1) & 0xFFFF);
    const byte_t lo = storage_[next_addr];
    return make_word(hi, lo);
}

void Memory::write_word(word_t address, word_t value) noexcept {
    const byte_t hi = high_byte(value);
    const byte_t lo = low_byte(value);
    storage_[address] = hi;
    const word_t next_addr = static_cast<word_t>((address + 1) & 0xFFFF);
    storage_[next_addr] = lo;
}

void Memory::reset() noexcept {
    storage_.fill(0x00);
}

bool Memory::load_segment(word_t start_address, const byte_t* data, std::size_t length) noexcept {
    // Valid empty segment is preserved as a successful no-op
    if (length == 0) {
        return true;
    }
    // Reject null pointer when length is non-zero
    if (data == nullptr) {
        return false;
    }
    // Overflow-safe bounds check:
    // start_address is word_t (0..65535), so remaining capacity is strictly in [1, 65536].
    // Testing length > remaining_capacity completely avoids integer overflow on start + length.
    const std::size_t start_idx = static_cast<std::size_t>(start_address);
    const std::size_t remaining_capacity = kMemorySize - start_idx;
    if (length > remaining_capacity) {
        return false;
    }
    std::copy_n(data, length, storage_.begin() + start_idx);
    return true;
}

bool Memory::load_segment(word_t start_address, const std::vector<byte_t>& data) noexcept {
    return load_segment(start_address, data.data(), data.size());
}

} // namespace bitforge
