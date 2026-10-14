#ifndef BITFORGE_CLI_RUNNER_HPP
#define BITFORGE_CLI_RUNNER_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/bus/system_bus.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <ostream>

namespace bitforge::cli {

// Exit codes adhering to the BitForge CLI specification
enum class ExitCode : int {
    Success = 0,         // Execution halted normally or assembly succeeded
    UsageError = 1,      // Invalid command-line arguments or usage
    AssemblyError = 2,   // Source file read failure or assembly diagnostics error
    CpuFault = 3,        // Architectural CPU fault encountered
    BudgetExceeded = 4   // Instruction execution budget exhausted
};

// Subcommands supported by the bitforge CLI executable
enum class Subcommand {
    None,
    Help,
    Assemble,
    Run
};

// Parsed command-line options
struct CliOptions {
    Subcommand command{Subcommand::None};
    std::string source_path{};
    std::uint64_t max_steps{kDefaultInstructionBudget}; // 1'000'000 steps by default
    bool dump_regs{false};
    std::string error_message{};
};

// Parse command-line arguments into CliOptions structure
CliOptions parse_arguments(int argc, const char* const argv[]);

// Print CLI usage and help manual to the specified output stream
void print_help(std::ostream& os);

// Format and output architectural register and status dump
void dump_registers(std::ostream& os, const Cpu& cpu, const SystemBus& bus,
                    std::string_view stop_reason, std::uint64_t steps_executed);

// Subcommand execution routines
ExitCode execute_assemble(const CliOptions& options, std::ostream& out, std::ostream& err);
ExitCode execute_run(const CliOptions& options, std::ostream& out, std::ostream& err);

// Main CLI dispatch entry point accepting arbitrary streams for headless testing
ExitCode run_cli(int argc, const char* const argv[], std::ostream& out, std::ostream& err);

} // namespace bitforge::cli

#endif // BITFORGE_CLI_RUNNER_HPP
