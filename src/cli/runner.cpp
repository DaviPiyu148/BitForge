#include "bitforge/cli/runner.hpp"
#include "bitforge/assembler/assembler.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/cpu.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <bitset>
#include <cctype>

namespace bitforge::cli {

namespace {

bool parse_positive_uint64(std::string_view sv, std::uint64_t& out_val) {
    if (sv.empty()) {
        return false;
    }
    for (char c : sv) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    try {
        std::string s(sv);
        std::size_t idx = 0;
        unsigned long long val = std::stoull(s, &idx);
        if (idx != s.size() || val == 0) {
            return false;
        }
        out_val = static_cast<std::uint64_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

void print_help(std::ostream& os) {
    os << "BitForge — 8-Bit Computer Emulator, Assembler, and Execution Runner\n\n"
       << "Usage:\n"
       << "  bitforge help | -h | --help\n"
       << "  bitforge version | -v | --version\n"
       << "  bitforge assemble <source.asm>\n"
       << "  bitforge run <source.asm> [--max-steps N] [--dump-regs]\n\n"
       << "Commands:\n"
       << "  help                     Display this help text\n"
       << "  version                  Display version and architectural baseline\n"
       << "  assemble <source.asm>    Assemble a source file and display segment information\n"
       << "  run <source.asm>         Assemble and execute a source file on the emulated CPU\n\n"
       << "Options for 'run':\n"
       << "  --max-steps <N>          Maximum instruction steps before stopping (default: 1000000)\n"
       << "  --dump-regs              Dump architectural registers and CPU state after execution\n\n"
       << "Exit Codes:\n"
       << "  0  Success (program halted normally or assembly succeeded)\n"
       << "  1  Command-line usage error or invalid arguments\n"
       << "  2  Source file read error or assembly error\n"
       << "  3  CPU execution fault\n"
       << "  4  Instruction execution budget exceeded\n";
}

CliOptions parse_arguments(int argc, const char* const argv[]) {
    CliOptions options;
    if (argc < 2) {
        options.error_message = "No command specified.";
        return options;
    }

    std::string_view cmd = argv[1];
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        options.command = Subcommand::Help;
        return options;
    }

    if (cmd == "version" || cmd == "-v" || cmd == "--version") {
        options.command = Subcommand::Version;
        return options;
    }

    if (cmd == "assemble") {
        options.command = Subcommand::Assemble;
        if (argc < 3) {
            options.error_message = "'assemble' requires a source file path.";
            return options;
        }
        options.source_path = argv[2];
        if (argc > 3) {
            options.error_message = "Unexpected argument '" + std::string(argv[3]) + "' for 'assemble'.";
            return options;
        }
        return options;
    }

    if (cmd == "run") {
        options.command = Subcommand::Run;
        if (argc < 3) {
            options.error_message = "'run' requires a source file path.";
            return options;
        }
        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "--dump-regs") {
                options.dump_regs = true;
            } else if (arg == "--max-steps") {
                if (i + 1 >= argc) {
                    options.error_message = "Option '--max-steps' requires a value.";
                    return options;
                }
                ++i;
                std::string_view val_str = argv[i];
                if (!parse_positive_uint64(val_str, options.max_steps)) {
                    options.error_message = "Invalid value for --max-steps: '" + std::string(val_str) +
                                            "'. Must be a positive integer.";
                    return options;
                }
            } else if (arg.rfind("--max-steps=", 0) == 0) {
                std::string_view val_str = arg.substr(12);
                if (!parse_positive_uint64(val_str, options.max_steps)) {
                    options.error_message = "Invalid value for --max-steps: '" + std::string(val_str) +
                                            "'. Must be a positive integer.";
                    return options;
                }
            } else if (arg.rfind("-", 0) == 0) {
                options.error_message = "Unknown option '" + std::string(arg) + "'.";
                return options;
            } else {
                if (!options.source_path.empty()) {
                    options.error_message = "Multiple source files specified for 'run'.";
                    return options;
                }
                options.source_path = std::string(arg);
            }
        }
        if (options.source_path.empty()) {
            options.error_message = "'run' requires a source file path.";
            return options;
        }
        return options;
    }

    options.error_message = "Unknown command '" + std::string(cmd) + "'.";
    return options;
}

void dump_registers(std::ostream& os, const Cpu& cpu, const SystemBus& bus,
                    std::string_view stop_reason, std::uint64_t steps_executed) {
    auto old_flags = os.flags();
    auto old_fill = os.fill();

    os << "==================================================\n"
       << "BitForge Register Dump\n"
       << "==================================================\n"
       << "State:          " << to_string(cpu.state()) << "\n"
       << "Stop Reason:    " << stop_reason << "\n"
       << "Steps Executed: " << steps_executed << "\n"
       << std::hex << std::uppercase << std::setfill('0')
       << "PC: 0x" << std::setw(4) << cpu.registers().pc
       << "   SP: 0x" << std::setw(4) << cpu.registers().sp
       << "   IX: 0x" << std::setw(4) << cpu.registers().ix << "\n"
       << "R0: 0x" << std::setw(2) << static_cast<int>(cpu.registers().r[0])
       << "     R1: 0x" << std::setw(2) << static_cast<int>(cpu.registers().r[1])
       << "     R2: 0x" << std::setw(2) << static_cast<int>(cpu.registers().r[2])
       << "     R3: 0x" << std::setw(2) << static_cast<int>(cpu.registers().r[3]) << "\n"
       << "Flags: [Z=" << (cpu.flags().z ? 1 : 0)
       << ", C=" << (cpu.flags().c ? 1 : 0)
       << ", N=" << (cpu.flags().n ? 1 : 0)
       << ", V=" << (cpu.flags().v ? 1 : 0) << "]\n"
       << "LEDs:  0x" << std::setw(2) << static_cast<int>(bus.led_state())
       << " (0b" << std::bitset<8>(bus.led_state()) << ")\n"
       << "==================================================\n";

    os.flags(old_flags);
    os.fill(old_fill);
}

ExitCode execute_assemble(const CliOptions& options, std::ostream& out, std::ostream& err) {
    std::filesystem::path path(options.source_path);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        err << "Error: Unable to open source file: " << options.source_path << "\n";
        return ExitCode::AssemblyError;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    assembler::Assembler as;
    auto res = as.assemble(source);

    if (!res.success) {
        err << "Assembly failed with " << res.diagnostics.size() << " error(s):\n";
        for (const auto& diag : res.diagnostics) {
            err << "  " << diag.format() << "\n";
        }
        return ExitCode::AssemblyError;
    }

    out << "Assembly successful: " << options.source_path << "\n"
        << std::hex << std::uppercase << std::setfill('0')
        << "Entry point:  0x" << std::setw(4) << res.image.entry_point() << "\n"
        << std::dec
        << "Total bytes:  " << res.image.total_bytes() << "\n"
        << "Segments (" << res.image.segments().size() << "):\n";

    for (std::size_t i = 0; i < res.image.segments().size(); ++i) {
        const auto& seg = res.image.segments()[i];
        out << "  [" << i << "] Start: 0x"
            << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << seg.start_address
            << ", Size: " << std::dec << seg.data.size() << " bytes (0x"
            << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << seg.data.size()
            << ")\n" << std::dec;
    }

    if (!res.diagnostics.empty()) {
        out << "Diagnostics (" << res.diagnostics.size() << "):\n";
        for (const auto& diag : res.diagnostics) {
            out << "  " << diag.format() << "\n";
        }
    }

    return ExitCode::Success;
}

ExitCode execute_run(const CliOptions& options, std::ostream& out, std::ostream& err) {
    std::filesystem::path path(options.source_path);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        err << "Error: Unable to open source file: " << options.source_path << "\n";
        return ExitCode::AssemblyError;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    assembler::Assembler as;
    auto res = as.assemble(source);

    if (!res.success) {
        err << "Assembly failed with " << res.diagnostics.size() << " error(s):\n";
        for (const auto& diag : res.diagnostics) {
            err << "  " << diag.format() << "\n";
        }
        return ExitCode::AssemblyError;
    }

    Memory memory;
    if (!res.image.load_into(memory)) {
        err << "Error: Failed to load assembled program into memory.\n";
        return ExitCode::AssemblyError;
    }

    SystemBus bus(memory);
    Cpu cpu(bus);
    cpu.reset(res.image.entry_point());

    std::uint64_t steps_executed = 0;
    std::string stop_reason;

    while ((cpu.state() == MachineState::Ready || cpu.state() == MachineState::Running) &&
           steps_executed < options.max_steps) {
        StepResult step_res = cpu.step();
        ++steps_executed;
        bus.clear_write_events();
        if (step_res.faulted || cpu.state() == MachineState::Faulted) {
            stop_reason = "Faulted";
            break;
        }
        if (step_res.halted || cpu.state() == MachineState::Halted) {
            stop_reason = "Halted";
            break;
        }
    }

    if (steps_executed >= options.max_steps &&
        cpu.state() != MachineState::Halted &&
        cpu.state() != MachineState::Faulted) {
        stop_reason = "InstructionBudgetExceeded";
    }

    // Virtual I/O presentation
    if (!bus.text_output().empty()) {
        out << "--- Text Output (Port 0x01) ---\n" << bus.text_output();
        if (bus.text_output().back() != '\n') {
            out << "\n";
        }
    }
    if (!bus.numeric_output().empty()) {
        out << "--- Numeric Output (Port 0x02) ---\n" << bus.numeric_output() << "\n";
    }
    if (bus.led_state() != 0) {
        out << "LED State (Port 0x10): 0x"
            << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
            << static_cast<int>(bus.led_state())
            << " (0b" << std::bitset<8>(bus.led_state()) << ")\n"
            << std::dec;
    }

    if (options.dump_regs) {
        dump_registers(out, cpu, bus, stop_reason, steps_executed);
    }

    if (cpu.state() == MachineState::Halted) {
        return ExitCode::Success;
    }

    if (cpu.state() == MachineState::Faulted) {
        const auto& fault = cpu.last_fault();
        err << "CPU Fault: " << to_string(fault.type)
            << " at PC 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4)
            << fault.instruction_address
            << " - " << fault.message << "\n" << std::dec;
        return ExitCode::CpuFault;
    }

    if (stop_reason == "InstructionBudgetExceeded") {
        err << "Error: Instruction budget exceeded (" << options.max_steps << " steps)\n";
        return ExitCode::BudgetExceeded;
    }

    err << "Error: CPU stopped unexpectedly in state: " << to_string(cpu.state()) << "\n";
    return ExitCode::CpuFault;
}

ExitCode run_cli(int argc, const char* const argv[], std::ostream& out, std::ostream& err) {
    auto options = parse_arguments(argc, argv);
    if (!options.error_message.empty()) {
        err << "Error: " << options.error_message << "\n\n";
        print_help(err);
        return ExitCode::UsageError;
    }

    switch (options.command) {
        case Subcommand::Help:
            print_help(out);
            return ExitCode::Success;
        case Subcommand::Version:
            out << "BitForge 0.1.0 (Architecture Specification v1.7)\n";
            return ExitCode::Success;
        case Subcommand::Assemble:
            return execute_assemble(options, out, err);
        case Subcommand::Run:
            return execute_run(options, out, err);
        case Subcommand::None:
        default:
            print_help(err);
            return ExitCode::UsageError;
    }
}

} // namespace bitforge::cli
