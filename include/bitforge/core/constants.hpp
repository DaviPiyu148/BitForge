#ifndef BITFORGE_CORE_CONSTANTS_HPP
#define BITFORGE_CORE_CONSTANTS_HPP

#include "bitforge/core/types.hpp"

namespace bitforge {

// Memory dimensions and boundaries
constexpr std::size_t kMemorySize = 65536;
constexpr word_t kMinAddress = 0x0000;
constexpr word_t kMaxAddress = 0xFFFF;

// Memory conventions & stack bounds
constexpr word_t kCodeRegionEnd = 0x7FFF;
constexpr word_t kDataRegionStart = 0x8000;
constexpr word_t kDataRegionEnd = 0xEFFF;
constexpr word_t kStackReservedStart = 0xF000;
constexpr word_t kStackReservedEnd = 0xFFFE;
constexpr word_t kStackResetValue = 0xFFFF;
constexpr word_t kAssemblerDataLimit = 0xF000;
constexpr word_t kDefaultEntryAddress = 0x0000;

// Register limits and IDs
constexpr std::size_t kGeneralRegisterCount = 4;
constexpr reg_id_t kRegR0 = 0;
constexpr reg_id_t kRegR1 = 1;
constexpr reg_id_t kRegR2 = 2;
constexpr reg_id_t kRegR3 = 3;
constexpr reg_id_t kMaxRegisterId = 3;

// Canonical virtual I/O port assignments
constexpr port_t kPortInputFifo      = 0x00; // Direction: Input
constexpr port_t kPortTextConsole    = 0x01; // Direction: Output
constexpr port_t kPortNumericConsole = 0x02; // Direction: Output
constexpr port_t kPortLeds           = 0x10; // Direction: Output

// Canonical instruction set totals
constexpr std::size_t kCanonicalInstructionCount = 31;
constexpr std::size_t kControlDataInstructionCount = 11;
constexpr std::size_t kArithmeticLogicInstructionCount = 9;
constexpr std::size_t kBranchStackCallInstructionCount = 9;
constexpr std::size_t kIoInstructionCount = 2;

// Standard execution budgets for automated testing and safety
constexpr std::uint64_t kDefaultInstructionBudget   = 1'000'000;
constexpr std::uint64_t kBenchmarkInstructionBudget = 100'000;
constexpr std::uint64_t kMicroInstructionBudget     = 1'000;

} // namespace bitforge

#endif // BITFORGE_CORE_CONSTANTS_HPP
