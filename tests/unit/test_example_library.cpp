#include "test_harness.hpp"
#include "bitforge/assembler/assembler.hpp"
#include "bitforge/core/opcodes.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/debugger/debugger_session.hpp"

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

using namespace bitforge;
using namespace bitforge::assembler;
using namespace bitforge::debugger;

namespace {

std::string resolve_path(const std::string& rel) {
    if (std::filesystem::exists(rel)) return rel;
    if (std::filesystem::exists("../" + rel)) return "../" + rel;
    if (std::filesystem::exists("../../" + rel)) return "../../" + rel;
    return rel;
}

std::string read_file(const std::string& rel_path) {
    std::string actual = resolve_path(rel_path);
    std::ifstream ifs(actual, std::ios::binary);
    EXPECT_TRUE(ifs.is_open());
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

struct ExampleContext {
    Memory memory{};
    SystemBus bus{memory};
    Cpu cpu{bus};
    std::optional<ProgramImage> image;
    std::unique_ptr<DebuggerSession> session;

    void assemble_and_load(const std::string& rel_path) {
        std::string source = read_file(rel_path);
        Assembler as;
        auto res = as.assemble(source);
        ASSERT_TRUE(res.success);
        image = std::move(res.image);
        ASSERT_TRUE(image->load_into(memory));
        session = std::make_unique<DebuggerSession>(cpu, &(*image));
        session->reset(image->entry_point());
    }

    ExecutionResult run(std::uint64_t max_steps = 100000) {
        return session->run(max_steps);
    }
};

} // namespace

// ==============================================================================
// 1. Fundamentals
// ==============================================================================

TEST(ExampleLibrary, Fundamentals_NopHalt) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/fundamentals/01_nop_halt.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    EXPECT_EQ(ctx.session->pc(), static_cast<word_t>(0x0204));
    EXPECT_EQ(ctx.session->registers().r[0], 0);
    EXPECT_EQ(ctx.session->registers().r[1], 0);
    EXPECT_EQ(ctx.session->registers().r[2], 0);
    EXPECT_EQ(ctx.session->registers().r[3], 0);
    EXPECT_FALSE(ctx.session->flags().z);
    EXPECT_FALSE(ctx.session->flags().c);
    EXPECT_FALSE(ctx.session->flags().n);
    EXPECT_FALSE(ctx.session->flags().v);
}

TEST(ExampleLibrary, Fundamentals_RegistersTransfer) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/fundamentals/02_registers_transfer.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x99);
    EXPECT_EQ(ctx.session->registers().r[1], 0x42);
    EXPECT_EQ(ctx.session->registers().r[2], 0x99);
    EXPECT_EQ(ctx.session->registers().r[3], 0x99);
}

// ==============================================================================
// 2. Arithmetic & Logic
// ==============================================================================

TEST(ExampleLibrary, Arithmetic_AdditionSubtraction) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/01_addition_subtraction.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 60);
    EXPECT_EQ(ctx.session->registers().r[1], 25);
    EXPECT_EQ(ctx.session->registers().r[2], 15);
    EXPECT_FALSE(ctx.session->flags().z);
    EXPECT_TRUE(ctx.session->flags().c);
    EXPECT_FALSE(ctx.session->flags().n);
    EXPECT_FALSE(ctx.session->flags().v);
}

TEST(ExampleLibrary, Arithmetic_IncrementDecrement) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/02_increment_decrement.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x00);
    EXPECT_EQ(ctx.session->registers().r[1], 0xFF);
    EXPECT_TRUE(ctx.session->flags().n);
    EXPECT_FALSE(ctx.session->flags().c); // INC/DEC preserves carry
}

TEST(ExampleLibrary, Arithmetic_BitwiseLogic) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/03_bitwise_logic.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x5A);
    EXPECT_EQ(ctx.session->registers().r[1], 0x0F);
    EXPECT_EQ(ctx.session->registers().r[2], 0x50);
    EXPECT_EQ(ctx.session->registers().r[3], 0xFF);
    EXPECT_FALSE(ctx.session->flags().c);
    EXPECT_FALSE(ctx.session->flags().v);
}

TEST(ExampleLibrary, Arithmetic_ComparisonFlags) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/04_comparison_flags.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 10);
    EXPECT_EQ(ctx.session->registers().r[1], 20);
    EXPECT_FALSE(ctx.session->flags().c);    // Borrow: 10 < 20 (C=0 per v1.7)
    EXPECT_TRUE(ctx.session->flags().n);
    EXPECT_FALSE(ctx.session->flags().z);
}

TEST(ExampleLibrary, Arithmetic_CarryBorrowBoundaries) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/05_carry_borrow_boundaries.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x00);
    EXPECT_EQ(ctx.session->registers().r[2], 0xFE);
    EXPECT_FALSE(ctx.session->flags().c);    // Final op is 4 - 6: borrow produces C=0
    EXPECT_TRUE(ctx.session->flags().n);
}

TEST(ExampleLibrary, Arithmetic_SignedOverflowBoundaries) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/arithmetic/06_signed_overflow_boundaries.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x80);
    EXPECT_EQ(ctx.session->registers().r[2], 0x7F);
    EXPECT_TRUE(ctx.session->flags().v);     // V=1 from -128 - 1 = +127
    EXPECT_FALSE(ctx.session->flags().n);
}

// ==============================================================================
// 3. Control Flow
// ==============================================================================

TEST(ExampleLibrary, ControlFlow_UnconditionalJump) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/control_flow/01_unconditional_jump.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 1);
    EXPECT_EQ(ctx.session->registers().r[1], 2);
    EXPECT_EQ(ctx.session->registers().r[2], 0); // Trap was jumped over
}

TEST(ExampleLibrary, ControlFlow_BranchZero) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/control_flow/02_branch_zero.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x00);
    EXPECT_EQ(ctx.session->registers().r[1], 0x01);
    EXPECT_EQ(ctx.session->registers().r[2], 0xAA);
}

TEST(ExampleLibrary, ControlFlow_BranchCarry) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/control_flow/03_branch_carry.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x20);
    EXPECT_EQ(ctx.session->registers().r[1], 0xBB);
}

TEST(ExampleLibrary, ControlFlow_CountdownLoop) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/control_flow/04_countdown_loop.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0);
    EXPECT_EQ(ctx.session->registers().r[1], 15);
    EXPECT_TRUE(ctx.session->flags().z);
}

// ==============================================================================
// 4. Memory & Pointers
// ==============================================================================

TEST(ExampleLibrary, Memory_DirectLoadStore) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/memory/01_direct_load_store.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x00);
    EXPECT_EQ(ctx.session->registers().r[1], 0x7E);
    EXPECT_EQ(ctx.memory.read_byte(0x8050), 0x7E);
}

TEST(ExampleLibrary, Memory_IndexedAccessIX) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/memory/02_indexed_access_ix.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().ix, 0x8001);
    EXPECT_EQ(ctx.session->registers().r[1], 0x11);
    EXPECT_EQ(ctx.session->registers().r[2], 0x22);
    EXPECT_EQ(ctx.memory.read_byte(0x8000), 0x11);
    EXPECT_EQ(ctx.memory.read_byte(0x8001), 0x22);
}

TEST(ExampleLibrary, Memory_ArrayTraversal) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/memory/03_array_traversal.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 50);
    EXPECT_EQ(ctx.session->registers().r[1], 0);
    EXPECT_EQ(ctx.session->registers().ix, 0x0305);
}

TEST(ExampleLibrary, Memory_ArraySummation) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/memory/04_array_summation.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 75);
    EXPECT_EQ(ctx.session->registers().r[1], 0);
    EXPECT_EQ(ctx.memory.read_byte(0x8100), 75);
}

// ==============================================================================
// 5. Stack & Subroutines
// ==============================================================================

TEST(ExampleLibrary, Stack_PushPopBasic) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/stack_subroutines/01_push_pop_basic.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF); // Empty stack sentinel per ADR-001
    EXPECT_EQ(ctx.session->registers().r[1], 0x33);
    EXPECT_EQ(ctx.session->registers().r[2], 0x22);
    EXPECT_EQ(ctx.session->registers().r[3], 0x11);
}

TEST(ExampleLibrary, Stack_SubroutineCallRet) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/stack_subroutines/02_subroutine_call_ret.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF);
    EXPECT_EQ(ctx.session->registers().r[0], 14);
}

TEST(ExampleLibrary, Stack_RegisterPreservation) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/stack_subroutines/03_register_preservation.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF);
    EXPECT_EQ(ctx.session->registers().r[0], 30);
    EXPECT_EQ(ctx.session->registers().r[1], 0xAA); // Callee-preserved
}

TEST(ExampleLibrary, Stack_NestedCalls) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/stack_subroutines/04_nested_calls.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF);
    EXPECT_EQ(ctx.session->registers().r[0], 16);
}

// ==============================================================================
// 6. I/O Devices
// ==============================================================================

TEST(ExampleLibrary, IoDevices_TextConsoleOutput) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/io_devices/01_text_console_output.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.bus.text_output(), "BitForge!");
}

TEST(ExampleLibrary, IoDevices_NumericConsoleOutput) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/io_devices/02_numeric_console_output.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.bus.numeric_output(), "0 42 100 255");
}

TEST(ExampleLibrary, IoDevices_VirtualLedPatterns) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/io_devices/03_virtual_led_patterns.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.bus.led_state(), 0xFF);
}

TEST(ExampleLibrary, IoDevices_InputFifoPoll) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/io_devices/04_input_fifo_poll.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x00);
}

// ==============================================================================
// 7. Algorithms
// ==============================================================================

TEST(ExampleLibrary, Algorithms_Multiplication) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/algorithms/01_multiplication_repeated_addition.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 42);
    EXPECT_EQ(ctx.session->registers().r[1], 7);
    EXPECT_EQ(ctx.session->registers().r[2], 0);
}

TEST(ExampleLibrary, Algorithms_Division) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/algorithms/02_division_repeated_subtraction.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 6); // Quotient
    EXPECT_EQ(ctx.session->registers().r[1], 3); // Remainder
    EXPECT_EQ(ctx.session->registers().r[2], 7);
}

TEST(ExampleLibrary, Algorithms_SumFirstNIntegers) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/algorithms/03_sum_first_n_integers.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 55);
    EXPECT_EQ(ctx.session->registers().r[1], 0);
}

TEST(ExampleLibrary, Algorithms_FibonacciSequence) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/algorithms/04_fibonacci_sequence.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 21);
    EXPECT_EQ(ctx.bus.numeric_output(), "21");

    // Verify stored sequence in memory table at 0x8000
    const byte_t expected_fib[9] = {0, 1, 1, 2, 3, 5, 8, 13, 21};
    for (std::size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(ctx.memory.read_byte(static_cast<word_t>(0x8000 + i)), expected_fib[i]);
    }
}

TEST(ExampleLibrary, Algorithms_GreatestCommonDivisor) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/algorithms/05_greatest_common_divisor.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 6);
    EXPECT_EQ(ctx.session->registers().r[1], 6);
}

// ==============================================================================
// 8. Debugger Examples
// ==============================================================================

TEST(ExampleLibrary, Debugger_SteppingSequential) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/01_stepping_sequential.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 3);
    EXPECT_EQ(ctx.session->registers().r[1], 2);
    EXPECT_EQ(ctx.session->registers().r[2], 7);
    EXPECT_EQ(ctx.session->registers().r[3], 4);
}

TEST(ExampleLibrary, Debugger_BranchTakenNotTaken) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/02_branch_taken_not_taken.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[2], 0x22);
    EXPECT_EQ(ctx.session->registers().r[3], 0xAA);
}

TEST(ExampleLibrary, Debugger_BreakpointTarget) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/03_breakpoint_target.asm");

    // Add breakpoint at line 34 (checkpoint_1)
    auto bp_res = ctx.session->add_line_breakpoint(34);
    ASSERT_TRUE(bp_res.success);

    // Run until breakpoint
    auto res1 = ctx.run();
    EXPECT_EQ(res1.stop_reason, StopReason::BreakpointHit);
    EXPECT_EQ(ctx.session->state(), MachineState::Running);
    EXPECT_EQ(ctx.session->registers().r[0], 0x01);
    EXPECT_EQ(ctx.session->registers().r[1], 0x00); // LDI R1, 0x11 has NOT executed yet!

    // Continue past breakpoint to completion
    auto res2 = ctx.session->continue_execution(100000);
    EXPECT_EQ(res2.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[1], 0x11);
    EXPECT_EQ(ctx.session->registers().r[2], 0x22);
    EXPECT_EQ(ctx.session->registers().r[3], 0x33);
}

TEST(ExampleLibrary, Debugger_SelfLoopBudget) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/04_self_loop_budget.asm");

    // Run with 500 steps budget
    auto res = ctx.run(500);
    EXPECT_EQ(res.stop_reason, StopReason::BudgetExhausted);
    EXPECT_EQ(ctx.session->state(), MachineState::Running);
    EXPECT_EQ(ctx.session->pc(), static_cast<word_t>(0x0203)); // spin label address
    EXPECT_EQ(ctx.session->registers().r[0], 42);
}

TEST(ExampleLibrary, Debugger_DisjointOrgSegments) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/05_disjoint_org_segments.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 10);
    EXPECT_EQ(ctx.session->registers().r[1], 25);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF);
}

TEST(ExampleLibrary, Debugger_MemoryInspectionStep) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/debugger/06_memory_inspection_step.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.memory.read_byte(0x8000), 0xAA);
    EXPECT_EQ(ctx.memory.read_byte(0x8001), 0xBB);
    EXPECT_EQ(ctx.memory.read_byte(0x8002), 0xCC);
    EXPECT_EQ(ctx.memory.read_byte(0x8003), 0xDD);
}

// ==============================================================================
// 9. Runtime Fault Demonstrations
// ==============================================================================

TEST(ExampleLibrary, Faults_InvalidOpcode) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/faults/01_invalid_opcode_fault.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Faulted);
    EXPECT_EQ(ctx.session->state(), MachineState::Faulted);
    EXPECT_EQ(ctx.cpu.last_fault().type, FaultType::InvalidOpcode);
    EXPECT_EQ(ctx.cpu.last_fault().instruction_address, 0x0200);
}

TEST(ExampleLibrary, Faults_StackUnderflow) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/faults/02_stack_underflow_fault.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Faulted);
    EXPECT_EQ(ctx.session->state(), MachineState::Faulted);
    EXPECT_EQ(ctx.cpu.last_fault().type, FaultType::StackUnderflow);
    EXPECT_EQ(ctx.cpu.last_fault().instruction_address, 0x0200);
    EXPECT_EQ(ctx.session->registers().sp, 0xFFFF); // Preserved
}

TEST(ExampleLibrary, Faults_StackOverflow) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/faults/03_stack_overflow_fault.asm");
    auto res = ctx.run(10000);

    EXPECT_EQ(res.stop_reason, StopReason::Faulted);
    EXPECT_EQ(ctx.session->state(), MachineState::Faulted);
    EXPECT_EQ(ctx.cpu.last_fault().type, FaultType::StackOverflow);
    EXPECT_EQ(ctx.cpu.last_fault().instruction_address, 0x0200);
}

TEST(ExampleLibrary, Faults_InvalidPort) {
    ExampleContext ctx;
    ctx.assemble_and_load("examples/faults/04_invalid_port_fault.asm");
    auto res = ctx.run();

    EXPECT_EQ(res.stop_reason, StopReason::Faulted);
    EXPECT_EQ(ctx.session->state(), MachineState::Faulted);
    EXPECT_EQ(ctx.cpu.last_fault().type, FaultType::InvalidPort);
    EXPECT_EQ(ctx.cpu.last_fault().instruction_address, 0x0203);
}

// ==============================================================================
// 10. Invalid Assembly Fixtures (Diagnostic Engine Verification)
// ==============================================================================

static void assert_invalid_assembly(const std::string& rel_path, const std::string& expected_substring) {
    std::string source = read_file(rel_path);
    Assembler as;
    auto res = as.assemble(source);
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.diagnostics.empty());

    bool found = false;
    for (const auto& d : res.diagnostics) {
        if (d.message.find(expected_substring) != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(ExampleLibrary, Invalid_UnknownMnemonic) {
    assert_invalid_assembly("examples/invalid/01_unknown_mnemonic.asm", "FOOBAR");
}

TEST(ExampleLibrary, Invalid_InvalidRegister) {
    assert_invalid_assembly("examples/invalid/02_invalid_register.asm", "R9");
}

TEST(ExampleLibrary, Invalid_MissingOperand) {
    assert_invalid_assembly("examples/invalid/03_missing_operand.asm", "Expected ','");
}

TEST(ExampleLibrary, Invalid_ExtraOperand) {
    assert_invalid_assembly("examples/invalid/04_extra_operand.asm", "Unexpected trailing token");
}

TEST(ExampleLibrary, Invalid_DuplicateLabel) {
    assert_invalid_assembly("examples/invalid/05_duplicate_label.asm", "Duplicate label definition");
}

TEST(ExampleLibrary, Invalid_UnresolvedLabel) {
    assert_invalid_assembly("examples/invalid/06_unresolved_label.asm", "Unresolved label");
}

TEST(ExampleLibrary, Invalid_LiteralOverflow) {
    assert_invalid_assembly("examples/invalid/07_literal_overflow.asm", "out of range");
}

TEST(ExampleLibrary, Invalid_OverlappingSegments) {
    assert_invalid_assembly("examples/invalid/08_overlapping_segments.asm", "Overlapping");
}

TEST(ExampleLibrary, Invalid_ReservedStackEmission) {
    assert_invalid_assembly("examples/invalid/09_reserved_stack_emission.asm", "reserved stack");
}

TEST(ExampleLibrary, Invalid_MalformedString) {
    assert_invalid_assembly("examples/invalid/10_malformed_string.asm", "Unterminated string literal");
}

// ==============================================================================
// 11. Instruction and Device Documentation Consistency Audit
// ==============================================================================

TEST(ExampleLibrary, Audit_Canonical31InstructionEncodingsAndExamples) {
    struct MnemonicMapping {
        Opcode opcode;
        byte_t opcode_byte;
        std::string_view mnemonic;
        std::size_t length;
        std::string_view example_file;
    };

    const MnemonicMapping canonical_matrix[] = {
        // Group 1: Basic Control & Data Movement (11 mnemonics)
        {Opcode::NOP,   0x00, "NOP",   1, "examples/fundamentals/01_nop_halt.asm"},
        {Opcode::HALT,  0x01, "HALT",  1, "examples/fundamentals/01_nop_halt.asm"},
        {Opcode::LDI,   0x10, "LDI",   3, "examples/fundamentals/02_registers_transfer.asm"},
        {Opcode::MOV,   0x11, "MOV",   3, "examples/fundamentals/02_registers_transfer.asm"},
        {Opcode::LD,    0x12, "LD",    4, "examples/memory/01_direct_load_store.asm"},
        {Opcode::ST,    0x13, "ST",    4, "examples/memory/01_direct_load_store.asm"},
        {Opcode::LDI16, 0x14, "LDI16", 3, "examples/memory/02_indexed_access_ix.asm"},
        {Opcode::LDX,   0x15, "LDX",   2, "examples/memory/02_indexed_access_ix.asm"},
        {Opcode::STX,   0x16, "STX",   2, "examples/memory/02_indexed_access_ix.asm"},
        {Opcode::INX,   0x17, "INX",   1, "examples/memory/02_indexed_access_ix.asm"},
        {Opcode::DEX,   0x18, "DEX",   1, "examples/memory/02_indexed_access_ix.asm"},

        // Group 2: Arithmetic & Logic (9 mnemonics)
        {Opcode::ADD,   0x20, "ADD",   3, "examples/arithmetic/01_addition_subtraction.asm"},
        {Opcode::SUB,   0x21, "SUB",   3, "examples/arithmetic/01_addition_subtraction.asm"},
        {Opcode::INC,   0x22, "INC",   2, "examples/arithmetic/02_increment_decrement.asm"},
        {Opcode::DEC,   0x23, "DEC",   2, "examples/arithmetic/02_increment_decrement.asm"},
        {Opcode::AND,   0x30, "AND",   3, "examples/arithmetic/03_bitwise_logic.asm"},
        {Opcode::OR,    0x31, "OR",    3, "examples/arithmetic/03_bitwise_logic.asm"},
        {Opcode::XOR,   0x32, "XOR",   3, "examples/arithmetic/03_bitwise_logic.asm"},
        {Opcode::NOT,   0x33, "NOT",   2, "examples/arithmetic/03_bitwise_logic.asm"},
        {Opcode::CMP,   0x40, "CMP",   3, "examples/arithmetic/04_comparison_flags.asm"},

        // Group 3: Branch, Stack & Call (9 mnemonics)
        {Opcode::JMP,   0x50, "JMP",   3, "examples/control_flow/01_unconditional_jump.asm"},
        {Opcode::JZ,    0x51, "JZ",    3, "examples/control_flow/02_branch_zero.asm"},
        {Opcode::JNZ,   0x52, "JNZ",   3, "examples/control_flow/02_branch_zero.asm"},
        {Opcode::JC,    0x53, "JC",    3, "examples/control_flow/03_branch_carry.asm"},
        {Opcode::JNC,   0x54, "JNC",   3, "examples/control_flow/03_branch_carry.asm"},
        {Opcode::CALL,  0x60, "CALL",  3, "examples/stack_subroutines/02_subroutine_call_ret.asm"},
        {Opcode::RET,   0x61, "RET",   1, "examples/stack_subroutines/02_subroutine_call_ret.asm"},
        {Opcode::PUSH,  0x62, "PUSH",  2, "examples/stack_subroutines/01_push_pop_basic.asm"},
        {Opcode::POP,   0x63, "POP",   2, "examples/stack_subroutines/01_push_pop_basic.asm"},

        // Group 4: Input/Output (2 mnemonics)
        {Opcode::IN,    0x70, "IN",    3, "examples/io_devices/04_input_fifo_poll.asm"},
        {Opcode::OUT,   0x71, "OUT",   3, "examples/io_devices/01_text_console_output.asm"}
    };

    EXPECT_EQ(sizeof(canonical_matrix) / sizeof(canonical_matrix[0]), 31);

    for (const auto& entry : canonical_matrix) {
        // 1. Verify architectural metadata matches canonical opcodes header
        EXPECT_EQ(static_cast<byte_t>(entry.opcode), entry.opcode_byte);
        EXPECT_TRUE(is_valid_opcode(entry.opcode_byte));
        EXPECT_EQ(instruction_length(entry.opcode_byte), entry.length);
        EXPECT_EQ(opcode_to_mnemonic(entry.opcode_byte), entry.mnemonic);

        // 2. Assemble corresponding example file and verify presence of the opcode byte
        std::string src = read_file(std::string(entry.example_file));
        Assembler as;
        auto res = as.assemble(src);
        EXPECT_TRUE(res.success);

        bool opcode_found = false;
        for (const auto& seg : res.image.segments()) {
            for (byte_t b : seg.data) {
                if (b == entry.opcode_byte) {
                    opcode_found = true;
                    break;
                }
            }
            if (opcode_found) break;
        }
        EXPECT_TRUE(opcode_found);
    }
}

TEST(ExampleLibrary, Audit_VirtualPortAssignmentsAndLedDevice) {
    // Port 0x10 is the strictly specified LED output port
    EXPECT_EQ(kPortLeds, 0x10);
    EXPECT_EQ(kPortInputFifo, 0x00);
    EXPECT_EQ(kPortTextConsole, 0x01);
    EXPECT_EQ(kPortNumericConsole, 0x02);

    // Port 0x03 is UNMAPPED and must reject both reads and writes
    Memory mem{};
    SystemBus bus{mem};
    EXPECT_FALSE(bus.can_read_io(0x03));
    EXPECT_FALSE(bus.can_write_io(0x03));
    EXPECT_FALSE(bus.read_io(0x03).has_value());
    EXPECT_FALSE(bus.write_io(0x03, 0xFF));

    // Port 0x10 is write-only for LEDs (reads are rejected)
    EXPECT_FALSE(bus.can_read_io(kPortLeds));
    EXPECT_TRUE(bus.can_write_io(kPortLeds));
    EXPECT_FALSE(bus.read_io(kPortLeds).has_value());

    // Writing to Port 0x10 updates LED state correctly
    EXPECT_EQ(bus.led_state(), 0x00);
    EXPECT_TRUE(bus.write_io(kPortLeds, 0xAA));
    EXPECT_EQ(bus.led_state(), 0xAA);
    EXPECT_TRUE(bus.write_io(kPortLeds, 0xFF));
    EXPECT_EQ(bus.led_state(), 0xFF);
}

