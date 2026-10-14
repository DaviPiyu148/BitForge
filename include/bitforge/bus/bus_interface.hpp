#ifndef BITFORGE_BUS_BUS_INTERFACE_HPP
#define BITFORGE_BUS_BUS_INTERFACE_HPP

#include "bitforge/core/types.hpp"
#include <optional>

namespace bitforge {

// Pure virtual interface for system memory and I/O bus transactions
class IBus {
public:
    virtual ~IBus() = default;

    // Memory access operations
    virtual byte_t read_byte(word_t address) const = 0;
    virtual void write_byte(word_t address, byte_t value) = 0;
    virtual word_t read_word(word_t address) const = 0;
    virtual void write_word(word_t address, word_t value) = 0;

    // Virtual I/O port operations:
    // read_io returns std::optional<byte_t>:
    //   - std::optional containing byte value on success (e.g. valid FIFO read)
    //   - std::nullopt if the port is unmapped or does not permit reads
    virtual std::optional<byte_t> read_io(port_t port) = 0;

    // write_io returns bool:
    //   - true on successful write to a mapped output device
    //   - false if the port is unmapped or does not permit writes
    virtual bool write_io(port_t port, byte_t value) = 0;

    // Side-effect-free permission validation (strictly const noexcept)
    virtual bool can_read_io(port_t port) const noexcept = 0;
    virtual bool can_write_io(port_t port) const noexcept = 0;
};

} // namespace bitforge

#endif // BITFORGE_BUS_BUS_INTERFACE_HPP
