#include "bitforge/bus/system_bus.hpp"

#include <sstream>
#include <iomanip>

namespace bitforge {

SystemBus::SystemBus(Memory& memory) noexcept
    : memory_(memory) {}

byte_t SystemBus::read_byte(word_t address) const {
    return memory_.read_byte(address);
}

void SystemBus::write_byte(word_t address, byte_t value) {
    memory_.write_byte(address, value);
}

word_t SystemBus::read_word(word_t address) const {
    return memory_.read_word(address);
}

void SystemBus::write_word(word_t address, word_t value) {
    memory_.write_word(address, value);
}

std::optional<byte_t> SystemBus::read_io(port_t port) {
    if (!can_read_io(port)) {
        return std::nullopt;
    }

    if (port == kPortInputFifo) {
        if (input_fifo_.empty()) {
            ++input_underflow_count_;
            return static_cast<byte_t>(0x00);
        }
        const byte_t val = input_fifo_.front();
        input_fifo_.pop_front();
        return val;
    }

    return std::nullopt;
}

bool SystemBus::write_io(port_t port, byte_t value) {
    if (!can_write_io(port)) {
        return false;
    }

    if (port == kPortTextConsole) {
        text_raw_bytes_.push_back(value);
        if (value >= 0x20 && value <= 0x7E) {
            text_output_ += static_cast<char>(value);
        } else {
            std::ostringstream oss;
            oss << "\\x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(value);
            text_output_ += oss.str();
        }
        return true;
    }

    if (port == kPortNumericConsole) {
        numeric_values_.push_back(value);
        if (!numeric_output_.empty()) {
            numeric_output_ += " ";
        }
        numeric_output_ += std::to_string(static_cast<unsigned int>(value));
        return true;
    }

    if (port == kPortLeds) {
        led_state_ = value;
        return true;
    }

    return false;
}

bool SystemBus::can_read_io(port_t port) const noexcept {
    return port == kPortInputFifo;
}

bool SystemBus::can_write_io(port_t port) const noexcept {
    return (port == kPortTextConsole) ||
           (port == kPortNumericConsole) ||
           (port == kPortLeds);
}

void SystemBus::push_input_fifo(byte_t byte) {
    input_fifo_.push_back(byte);
}

void SystemBus::push_input_fifo(const std::vector<byte_t>& bytes) {
    for (const auto b : bytes) {
        input_fifo_.push_back(b);
    }
}

void SystemBus::push_input_fifo(std::string_view str) {
    for (const char c : str) {
        input_fifo_.push_back(static_cast<byte_t>(c));
    }
}

std::size_t SystemBus::input_fifo_size() const noexcept {
    return input_fifo_.size();
}

bool SystemBus::has_input_underflow() const noexcept {
    return input_underflow_count_ > 0;
}

std::size_t SystemBus::input_underflow_count() const noexcept {
    return input_underflow_count_;
}

void SystemBus::clear_input_fifo() noexcept {
    input_fifo_.clear();
}

const std::string& SystemBus::text_output() const noexcept {
    return text_output_;
}

const std::vector<byte_t>& SystemBus::text_raw_bytes() const noexcept {
    return text_raw_bytes_;
}

void SystemBus::clear_text_output() noexcept {
    text_output_.clear();
    text_raw_bytes_.clear();
}

const std::vector<byte_t>& SystemBus::numeric_values() const noexcept {
    return numeric_values_;
}

const std::string& SystemBus::numeric_output() const noexcept {
    return numeric_output_;
}

void SystemBus::clear_numeric_output() noexcept {
    numeric_values_.clear();
    numeric_output_.clear();
}

byte_t SystemBus::led_state() const noexcept {
    return led_state_;
}

void SystemBus::set_led_state(byte_t val) noexcept {
    led_state_ = val;
}

void SystemBus::reset_devices() noexcept {
    input_fifo_.clear();
    input_underflow_count_ = 0;
    text_output_.clear();
    text_raw_bytes_.clear();
    numeric_values_.clear();
    numeric_output_.clear();
    led_state_ = 0x00;
}

} // namespace bitforge
