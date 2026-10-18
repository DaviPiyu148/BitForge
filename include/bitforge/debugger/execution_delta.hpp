#ifndef BITFORGE_DEBUGGER_EXECUTION_DELTA_HPP
#define BITFORGE_DEBUGGER_EXECUTION_DELTA_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/registers.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/bus/system_bus.hpp"

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace bitforge::debugger {

// Record of a register mutation
struct RegisterChange {
    std::string name;
    word_t old_value{0};
    word_t new_value{0};
    bool is_16bit{false};
};

// Record of a condition flag mutation
struct FlagChange {
    std::string name;
    bool old_value{false};
    bool new_value{false};
};

// Structured record of state mutations caused by a single instruction execution
struct ExecutionDelta {
    std::vector<RegisterChange> register_changes{};
    std::vector<FlagChange> flag_changes{};
    std::vector<MemoryWriteEvent> memory_writes{};
    std::vector<IoEvent> io_events{};

    [[nodiscard]] bool has_changes() const noexcept {
        return !register_changes.empty() || !flag_changes.empty() ||
               !memory_writes.empty() || !io_events.empty();
    }

    [[nodiscard]] std::string to_string() const {
        if (!has_changes()) {
            return "No state changes";
        }
        std::vector<std::string> parts;

        for (const auto& rc : register_changes) {
            std::ostringstream oss;
            if (rc.is_16bit) {
                oss << rc.name << ": 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                    << rc.old_value << " -> 0x" << std::setw(4) << std::setfill('0') << rc.new_value;
            } else {
                oss << rc.name << ": 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                    << rc.old_value << " -> 0x" << std::setw(2) << std::setfill('0') << rc.new_value;
            }
            parts.push_back(oss.str());
        }

        for (const auto& fc : flag_changes) {
            std::ostringstream oss;
            oss << "Flag " << fc.name << ": " << (fc.old_value ? "1" : "0") << " -> " << (fc.new_value ? "1" : "0");
            parts.push_back(oss.str());
        }

        for (const auto& mw : memory_writes) {
            std::ostringstream oss;
            oss << "Memory[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << mw.address
                << "]: 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(mw.old_value)
                << " -> 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(mw.new_value);
            parts.push_back(oss.str());
        }

        for (const auto& io : io_events) {
            std::ostringstream oss;
            if (io.direction == IoDirection::Read) {
                oss << "Port 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(io.port)
                    << ": input 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(io.value);
            } else {
                oss << "Port 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(io.port)
                    << ": output 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(io.value);
            }
            parts.push_back(oss.str());
        }

        std::string result;
        for (std::size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) result += ", ";
            result += parts[i];
        }
        return result;
    }
};

// Computes ExecutionDelta by comparing registers and flags before/after,
// and incorporating bus events.
inline ExecutionDelta compute_execution_delta(
    const Registers& before_regs,
    const Registers& after_regs,
    const Flags& before_flags,
    const Flags& after_flags,
    std::vector<MemoryWriteEvent> mem_writes,
    std::vector<IoEvent> io_events
) {
    ExecutionDelta delta;
    delta.memory_writes = std::move(mem_writes);
    delta.io_events = std::move(io_events);

    // Check R0-R3
    for (std::size_t i = 0; i < 4; ++i) {
        if (before_regs.r[i] != after_regs.r[i]) {
            delta.register_changes.push_back(RegisterChange{
                "R" + std::to_string(i),
                before_regs.r[i],
                after_regs.r[i],
                false
            });
        }
    }
    // Check IX
    if (before_regs.ix != after_regs.ix) {
        delta.register_changes.push_back(RegisterChange{"IX", before_regs.ix, after_regs.ix, true});
    }
    // Check SP
    if (before_regs.sp != after_regs.sp) {
        delta.register_changes.push_back(RegisterChange{"SP", before_regs.sp, after_regs.sp, true});
    }
    // Check PC
    if (before_regs.pc != after_regs.pc) {
        delta.register_changes.push_back(RegisterChange{"PC", before_regs.pc, after_regs.pc, true});
    }

    // Flags
    if (before_flags.z != after_flags.z) {
        delta.flag_changes.push_back(FlagChange{"Z", before_flags.z, after_flags.z});
    }
    if (before_flags.c != after_flags.c) {
        delta.flag_changes.push_back(FlagChange{"C", before_flags.c, after_flags.c});
    }
    if (before_flags.n != after_flags.n) {
        delta.flag_changes.push_back(FlagChange{"N", before_flags.n, after_flags.n});
    }
    if (before_flags.v != after_flags.v) {
        delta.flag_changes.push_back(FlagChange{"V", before_flags.v, after_flags.v});
    }

    return delta;
}

} // namespace bitforge::debugger

#endif // BITFORGE_DEBUGGER_EXECUTION_DELTA_HPP
