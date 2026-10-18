#ifndef BITFORGE_BUS_SYSTEM_BUS_HPP
#define BITFORGE_BUS_SYSTEM_BUS_HPP

#include "bitforge/bus/bus_interface.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/core/constants.hpp"

#include <optional>
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <cstddef>

namespace bitforge {

// Memory write journal event
struct MemoryWriteEvent {
    word_t address{0};
    byte_t old_value{0};
    byte_t new_value{0};
};

// I/O event direction
enum class IoDirection {
    Read,
    Write
};

// Virtual device I/O event
struct IoEvent {
    IoDirection direction{IoDirection::Read};
    port_t port{0};
    byte_t value{0};
};

// Concrete system bus connecting processor to memory and virtual I/O devices
class SystemBus : public IBus {
public:
    explicit SystemBus(Memory& memory) noexcept;
    ~SystemBus() override = default;

    // Memory access operations delegated to Memory instance
    byte_t read_byte(word_t address) const override;
    void write_byte(word_t address, byte_t value) override;
    word_t read_word(word_t address) const override;
    void write_word(word_t address, word_t value) override;

    // I/O operations returning explicit status:
    // read_io:
    //   - Returns byte value (0x00 when FIFO empty, logging underflow) for readable ports (Port 0x00).
    //   - Returns std::nullopt for unmapped or output-only ports.
    // write_io:
    //   - Dispatches byte to concrete device sink for defined output ports (0x01, 0x02, 0x10).
    //   - Writes to unmapped or input-only ports are rejected (returning false).
    std::optional<byte_t> read_io(port_t port) override;
    bool write_io(port_t port, byte_t value) override;

    // Side-effect-free permission queries (strictly const noexcept)
    bool can_read_io(port_t port) const noexcept override;
    bool can_write_io(port_t port) const noexcept override;

    // Direct access to underlying memory for diagnostics
    Memory& memory() noexcept { return memory_; }
    const Memory& memory() const noexcept { return memory_; }

    // -------------------------------------------------------------
    // Virtual Device Inspection & Simulation Control
    // -------------------------------------------------------------

    // Port 0x00: Input FIFO queue
    void push_input_fifo(byte_t byte);
    void push_input_fifo(const std::vector<byte_t>& bytes);
    void push_input_fifo(std::string_view str);
    [[nodiscard]] std::size_t input_fifo_size() const noexcept;
    [[nodiscard]] std::optional<byte_t> peek_input_fifo() const noexcept;
    [[nodiscard]] std::vector<byte_t> input_fifo_contents() const;
    [[nodiscard]] bool has_input_underflow() const noexcept;
    [[nodiscard]] std::size_t input_underflow_count() const noexcept;
    void clear_input_fifo() noexcept;

    // Port 0x01: Text console output
    [[nodiscard]] const std::string& text_output() const noexcept;
    [[nodiscard]] const std::vector<byte_t>& text_raw_bytes() const noexcept;
    void clear_text_output() noexcept;

    // Port 0x02: Numeric console output
    [[nodiscard]] const std::vector<byte_t>& numeric_values() const noexcept;
    [[nodiscard]] const std::string& numeric_output() const noexcept;
    void clear_numeric_output() noexcept;

    // Port 0x10: 8-bit virtual LED register
    [[nodiscard]] byte_t led_state() const noexcept;
    void set_led_state(byte_t val) noexcept;

    // Bus event journaling (for instruction execution deltas)
    [[nodiscard]] const std::vector<MemoryWriteEvent>& memory_write_events() const noexcept { return memory_write_events_; }
    [[nodiscard]] const std::vector<IoEvent>& io_events() const noexcept { return io_events_; }
    void clear_write_events() noexcept;
    std::vector<MemoryWriteEvent> take_memory_write_events() noexcept;
    std::vector<IoEvent> take_io_events() noexcept;

    // Reset all device buffers and states
    void reset_devices() noexcept;

private:
    Memory& memory_;

    // Device state
    std::deque<byte_t> input_fifo_{};
    std::size_t input_underflow_count_{0};

    std::string text_output_{};
    std::vector<byte_t> text_raw_bytes_{};

    std::vector<byte_t> numeric_values_{};
    std::string numeric_output_{};

    byte_t led_state_{0x00};

    // Instruction write events
    std::vector<MemoryWriteEvent> memory_write_events_{};
    std::vector<IoEvent> io_events_{};
};

} // namespace bitforge

#endif // BITFORGE_BUS_SYSTEM_BUS_HPP
