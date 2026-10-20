#include "test_harness.hpp"
#include "bitforge/cli/runner.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/constants.hpp"

#include <sstream>
#include <fstream>
#include <filesystem>

using namespace bitforge::cli;

namespace {

struct ScopedTempFile {
    std::filesystem::path path;
    explicit ScopedTempFile(const std::string& name, const std::string& content)
        : path(std::filesystem::temp_directory_path() / name) {
        std::ofstream ofs(path, std::ios::binary);
        ofs << content;
    }
    ~ScopedTempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

} // namespace

TEST(Cli, HelpFlag) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "--help"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("Usage:"), std::string::npos);
    EXPECT_NE(out.str().find("bitforge assemble"), std::string::npos);
    EXPECT_NE(out.str().find("bitforge run"), std::string::npos);
}

TEST(Cli, HelpShortFlag) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "-h"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("BitForge"), std::string::npos);
}

TEST(Cli, HelpSubcommand) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "help"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("Exit Codes:"), std::string::npos);
}

TEST(Cli, VersionFlag) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "--version"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("BitForge 0.1.0"), std::string::npos);
    EXPECT_NE(out.str().find("Architecture Specification v1.7"), std::string::npos);
}

TEST(Cli, VersionShortFlag) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "-v"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("BitForge 0.1.0"), std::string::npos);
}

TEST(Cli, VersionSubcommand) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "version"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("BitForge 0.1.0"), std::string::npos);
}

TEST(Cli, UsageErrorsNoArguments) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge"};
    auto code = run_cli(1, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Error: No command specified"), std::string::npos);
}

TEST(Cli, UsageErrorsUnknownCommand) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "disassemble"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Error: Unknown command 'disassemble'"), std::string::npos);
}

TEST(Cli, UsageErrorsAssembleMissingSource) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "assemble"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("requires a source file path"), std::string::npos);
}

TEST(Cli, UsageErrorsAssembleUnexpectedArgument) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "assemble", "prog.asm", "extra"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Unexpected argument 'extra'"), std::string::npos);
}

TEST(Cli, UsageErrorsRunMissingSource) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run"};
    auto code = run_cli(2, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("requires a source file path"), std::string::npos);
}

TEST(Cli, UsageErrorsRunMissingMaxStepsValue) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", "prog.asm", "--max-steps"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Option '--max-steps' requires a value"), std::string::npos);
}

TEST(Cli, UsageErrorsRunInvalidMaxStepsNonNumeric) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", "prog.asm", "--max-steps", "100abc"};
    auto code = run_cli(5, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Must be a positive integer"), std::string::npos);
}

TEST(Cli, UsageErrorsRunInvalidMaxStepsZero) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", "prog.asm", "--max-steps", "0"};
    auto code = run_cli(5, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Must be a positive integer"), std::string::npos);
}

TEST(Cli, UsageErrorsRunUnknownOption) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", "prog.asm", "--trace-all"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Unknown option '--trace-all'"), std::string::npos);
}

TEST(Cli, UsageErrorsRunMultipleFiles) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", "prog1.asm", "prog2.asm"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::UsageError);
    EXPECT_NE(err.str().find("Multiple source files specified"), std::string::npos);
}

TEST(Cli, AssembleNonExistentFile) {
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "assemble", "does_not_exist_98765.asm"};
    auto code = run_cli(3, argv, out, err);

    EXPECT_EQ(code, ExitCode::AssemblyError);
    EXPECT_NE(err.str().find("Unable to open source file"), std::string::npos);
}

TEST(Cli, AssembleSyntaxError) {
    ScopedTempFile file("invalid_syntax.asm", ".ORG 0x0200\nINVALID_OPCODE R0\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "assemble", path_str.c_str()};
    auto code = run_cli(3, argv, out, err);

    EXPECT_EQ(code, ExitCode::AssemblyError);
    EXPECT_NE(err.str().find("Assembly failed with"), std::string::npos);
    EXPECT_NE(err.str().find("Unknown instruction mnemonic"), std::string::npos);
}

TEST(Cli, AssembleSuccess) {
    ScopedTempFile file("valid_program.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "assemble", path_str.c_str()};
    auto code = run_cli(3, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("Assembly successful:"), std::string::npos);
    EXPECT_NE(out.str().find("Entry point:  0x0200"), std::string::npos);
    EXPECT_NE(out.str().find("Segments (1):"), std::string::npos);
    EXPECT_NE(out.str().find("Start: 0x0200"), std::string::npos);
}

TEST(Cli, RunSuccessBaseline) {
    ScopedTempFile file("arithmetic.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    ADD R0, R1\n"
        "    ST [0x8000], R0\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str()};
    auto code = run_cli(3, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
}

TEST(Cli, RunSuccessWithDumpRegs) {
    ScopedTempFile file("arithmetic_dump.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    ADD R0, R1\n"
        "    ST [0x8000], R0\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--dump-regs"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());

    const std::string out_str = out.str();
    EXPECT_NE(out_str.find("BitForge Register Dump"), std::string::npos);
    EXPECT_NE(out_str.find("State:          Halted"), std::string::npos);
    EXPECT_NE(out_str.find("Stop Reason:    Halted"), std::string::npos);
    EXPECT_NE(out_str.find("R0: 0x1E"), std::string::npos);
    EXPECT_NE(out_str.find("R1: 0x14"), std::string::npos);
    EXPECT_NE(out_str.find("PC: 0x020E"), std::string::npos);
    EXPECT_NE(out_str.find("SP: 0xFFFF"), std::string::npos);
    EXPECT_NE(out_str.find("Flags: [Z=0, C=0, N=0, V=0]"), std::string::npos);
}

TEST(Cli, RunVirtualIo) {
    ScopedTempFile file("io_test.asm",
        ".ORG 0x0200\n"
        ".ENTRY main\n"
        "main:\n"
        "    LDI R0, 'H'\n"
        "    OUT 0x01, R0\n"
        "    LDI R0, 'I'\n"
        "    OUT 0x01, R0\n"
        "    LDI R0, '!'\n"
        "    OUT 0x01, R0\n"
        "    LDI R0, 42\n"
        "    OUT 0x02, R0\n"
        "    LDI R0, 0x55\n"
        "    OUT 0x10, R0\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--dump-regs"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());

    const std::string out_str = out.str();
    EXPECT_NE(out_str.find("--- Text Output (Port 0x01) ---"), std::string::npos);
    EXPECT_NE(out_str.find("HI!"), std::string::npos);
    EXPECT_NE(out_str.find("--- Numeric Output (Port 0x02) ---"), std::string::npos);
    EXPECT_NE(out_str.find("42"), std::string::npos);
    EXPECT_NE(out_str.find("LED State (Port 0x10): 0x55 (0b01010101)"), std::string::npos);
    EXPECT_NE(out_str.find("LEDs:  0x55 (0b01010101)"), std::string::npos);
}

TEST(Cli, RunCpuFault) {
    ScopedTempFile file("fault_test.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    .BYTE 0xFF\n"); // 0xFF is an invalid opcode
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str()};
    auto code = run_cli(3, argv, out, err);

    EXPECT_EQ(code, ExitCode::CpuFault);
    EXPECT_NE(err.str().find("CPU Fault: InvalidOpcode"), std::string::npos);
    EXPECT_NE(err.str().find("PC 0x0200"), std::string::npos);
}

TEST(Cli, RunBudgetExceeded) {
    ScopedTempFile file("infinite_loop.asm",
        ".ORG 0x0200\n"
        ".ENTRY loop\n"
        "loop:\n"
        "    JMP loop\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--max-steps", "10"};
    auto code = run_cli(5, argv, out, err);

    EXPECT_EQ(code, ExitCode::BudgetExceeded);
    EXPECT_NE(err.str().find("Instruction budget exceeded (10 steps)"), std::string::npos);
}

TEST(Cli, RunBudgetExceededEqualsSyntax) {
    ScopedTempFile file("infinite_loop2.asm",
        ".ORG 0x0200\n"
        ".ENTRY loop\n"
        "loop:\n"
        "    JMP loop\n");
    std::string path_str = file.path.string();

    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--max-steps=15"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::BudgetExceeded);
    EXPECT_NE(err.str().find("Instruction budget exceeded (15 steps)"), std::string::npos);
}

// =============================================================
// Phase 5: Budget Boundaries, Headless I/O & Drainage Tests
// =============================================================

TEST(Cli, InstructionBudgetBoundaryExactSteps) {
    // Exact 5-instruction program: LDI, LDI, ADD, ST, HALT
    ScopedTempFile file("finite_5step.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    ADD R0, R1\n"
        "    ST [0x8000], R0\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    // Budget 4: halts before reaching HALT -> BudgetExceeded (4)
    {
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str(), "--max-steps", "4"};
        auto code = run_cli(5, argv, out, err);
        EXPECT_EQ(code, ExitCode::BudgetExceeded);
        EXPECT_NE(err.str().find("Instruction budget exceeded (4 steps)"), std::string::npos);
    }

    // Budget 5: HALT wins on exact step boundary -> Success (0)
    {
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str(), "--max-steps", "5", "--dump-regs"};
        auto code = run_cli(6, argv, out, err);
        EXPECT_EQ(code, ExitCode::Success);
        EXPECT_TRUE(err.str().empty());
        EXPECT_NE(out.str().find("Steps Executed: 5"), std::string::npos);
        EXPECT_NE(out.str().find("State:          Halted"), std::string::npos);
    }

    // Budget 6: exceeds required steps -> Success (0)
    {
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str(), "--max-steps", "6"};
        auto code = run_cli(5, argv, out, err);
        EXPECT_EQ(code, ExitCode::Success);
        EXPECT_TRUE(err.str().empty());
    }
}

TEST(Cli, VirtualIoEmptyFifoUnderflowNoFault) {
    // Reading from empty input FIFO returns 0x00 and must not fault the CPU
    ScopedTempFile file("fifo_empty.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    IN R0, 0x00\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out, err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--dump-regs"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());
    EXPECT_NE(out.str().find("State:          Halted"), std::string::npos);
    EXPECT_NE(out.str().find("R0: 0x00"), std::string::npos);
    EXPECT_NE(out.str().find("Steps Executed: 2"), std::string::npos);
}

TEST(Cli, VirtualIoInvalidPortDirectionFaults) {
    // 1. Write to Port 0x00 (FIFO is read-only)
    {
        ScopedTempFile file("bad_fifo_write.asm", ".ORG 0x0200\nOUT 0x00, R0\n");
        std::string path_str = file.path.string();
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str()};
        auto code = run_cli(3, argv, out, err);
        EXPECT_EQ(code, ExitCode::CpuFault);
        EXPECT_NE(err.str().find("CPU Fault: InvalidPort"), std::string::npos);
    }

    // 2. Read from Port 0x01 (Text console is write-only)
    {
        ScopedTempFile file("bad_text_read.asm", ".ORG 0x0200\nIN R0, 0x01\n");
        std::string path_str = file.path.string();
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str()};
        auto code = run_cli(3, argv, out, err);
        EXPECT_EQ(code, ExitCode::CpuFault);
        EXPECT_NE(err.str().find("CPU Fault: InvalidPort"), std::string::npos);
    }

    // 3. Read from Port 0x02 (Numeric console is write-only)
    {
        ScopedTempFile file("bad_num_read.asm", ".ORG 0x0200\nIN R0, 0x02\n");
        std::string path_str = file.path.string();
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str()};
        auto code = run_cli(3, argv, out, err);
        EXPECT_EQ(code, ExitCode::CpuFault);
        EXPECT_NE(err.str().find("CPU Fault: InvalidPort"), std::string::npos);
    }

    // 4. Read from Port 0x10 (Virtual LEDs is write-only)
    {
        ScopedTempFile file("bad_led_read.asm", ".ORG 0x0200\nIN R0, 0x10\n");
        std::string path_str = file.path.string();
        std::ostringstream out, err;
        const char* argv[] = {"bitforge", "run", path_str.c_str()};
        auto code = run_cli(3, argv, out, err);
        EXPECT_EQ(code, ExitCode::CpuFault);
        EXPECT_NE(err.str().find("CPU Fault: InvalidPort"), std::string::npos);
    }
}

TEST(Cli, HeadlessExecutionJournalDrainageAndOutputPreservation) {
    // 1. Direct unit verification: clear_write_events() empties both memory and I/O event journals
    {
        bitforge::Memory bus_mem;
        bitforge::SystemBus bus(bus_mem);
        bus.write_byte(0x1000, 0x55);
        bus.write_io(bitforge::kPortLeds, 0x55);
        EXPECT_EQ(bus.memory_write_events().size(), 1);
        EXPECT_EQ(bus.io_events().size(), 1);
        bus.clear_write_events();
        EXPECT_TRUE(bus.memory_write_events().empty());
        EXPECT_TRUE(bus.io_events().empty());
    }

    // 2. Integration verification: headless run loop performing memory and I/O writes drains
    // event journals every step while strictly preserving virtual peripheral state and register dumps
    ScopedTempFile file("loop_io_mem.asm",
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 5\n"
        "    LDI R1, 0x55\n"
        "loop:\n"
        "    ST [0x1000], R1\n"
        "    OUT 0x10, R1\n"
        "    DEC R0\n"
        "    JNZ loop\n"
        "    HALT\n");
    std::string path_str = file.path.string();

    std::ostringstream out, err;
    const char* argv[] = {"bitforge", "run", path_str.c_str(), "--dump-regs"};
    auto code = run_cli(4, argv, out, err);

    EXPECT_EQ(code, ExitCode::Success);
    EXPECT_TRUE(err.str().empty());

    const std::string out_str = out.str();
    EXPECT_NE(out_str.find("LED State (Port 0x10): 0x55 (0b01010101)"), std::string::npos);
    EXPECT_NE(out_str.find("LEDs:  0x55 (0b01010101)"), std::string::npos);
    EXPECT_NE(out_str.find("State:          Halted"), std::string::npos);
    EXPECT_NE(out_str.find("Stop Reason:    Halted"), std::string::npos);
    EXPECT_NE(out_str.find("R0: 0x00"), std::string::npos);
    EXPECT_NE(out_str.find("R1: 0x55"), std::string::npos);
    // Steps: 2 setup (LDI, LDI) + 4 iters * 4 instrs (ST, OUT, DEC, JNZ) + 1 final iter (ST, OUT, DEC, JNZ taken/not-taken) + 1 HALT = 2 + 16 + 4 + 1 = 23 steps
    EXPECT_NE(out_str.find("Steps Executed: 23"), std::string::npos);
}
