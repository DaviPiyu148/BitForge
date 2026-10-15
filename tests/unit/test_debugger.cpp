#include "test_harness.hpp"
#include "bitforge/debugger/debugger_session.hpp"
#include "bitforge/assembler/assembler.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/core/cpu.hpp"

#include <memory>

using namespace bitforge;
using namespace bitforge::debugger;
using namespace bitforge::assembler;

namespace {

struct DebuggerTestContext {
    Memory memory;
    SystemBus bus;
    Cpu cpu;
    std::optional<ProgramImage> image;
    std::unique_ptr<DebuggerSession> session;

    DebuggerTestContext()
        : bus(memory), cpu(bus) {}

    void load_source(std::string_view source) {
        Assembler as;
        auto res = as.assemble(source);
        ASSERT_TRUE(res.success);
        image = std::move(res.image);
        ASSERT_TRUE(image->load_into(memory));
        session = std::make_unique<DebuggerSession>(cpu, &(*image));
        session->reset(image->entry_point());
    }
};

} // namespace

// 1. Single-step from Ready
TEST(Debugger, SingleStepFromReady) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    HALT\n"
    );

    EXPECT_EQ(ctx.session->state(), MachineState::Ready);
    EXPECT_EQ(ctx.session->pc(), 0x0200);

    auto result = ctx.session->step();
    EXPECT_EQ(result.stop_reason, StopReason::StepCompleted);
    EXPECT_EQ(result.steps_executed, 1);
    EXPECT_EQ(ctx.session->state(), MachineState::Running);
    EXPECT_EQ(ctx.session->pc(), 0x0203);
    EXPECT_EQ(ctx.session->r(0), 42);
}

// 2. Sequential single-step and correct PC advancement
TEST(Debugger, SequentialSingleStepAndPcAdvancement) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"      // 3 bytes -> PC: 0x0203
        "    LDI R1, 20\n"      // 3 bytes -> PC: 0x0206
        "    ADD R0, R1\n"      // 3 bytes -> PC: 0x0209
        "    HALT\n"            // 1 byte  -> PC: 0x020A
    );

    auto r1 = ctx.session->step();
    EXPECT_EQ(r1.stop_reason, StopReason::StepCompleted);
    EXPECT_EQ(ctx.session->pc(), 0x0203);
    EXPECT_EQ(ctx.session->r(0), 10);

    auto r2 = ctx.session->step();
    EXPECT_EQ(r2.stop_reason, StopReason::StepCompleted);
    EXPECT_EQ(ctx.session->pc(), 0x0206);
    EXPECT_EQ(ctx.session->r(1), 20);

    auto r3 = ctx.session->step();
    EXPECT_EQ(r3.stop_reason, StopReason::StepCompleted);
    EXPECT_EQ(ctx.session->pc(), 0x0209);
    EXPECT_EQ(ctx.session->r(0), 30);
}

// 3. Single-step of HALT
TEST(Debugger, SingleStepOfHalt) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    auto res = ctx.session->step();
    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    EXPECT_EQ(ctx.session->pc(), 0x0201);
}

// 4. Single-step behavior after Halted and Faulted states
TEST(Debugger, SingleStepAfterHaltedAndFaulted) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    ctx.session->step(); // Halts
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    const word_t halt_pc = ctx.session->pc();

    // Stepping when already halted
    auto step_again = ctx.session->step();
    EXPECT_EQ(step_again.stop_reason, StopReason::AlreadyHalted);
    EXPECT_EQ(step_again.steps_executed, 0);
    EXPECT_EQ(ctx.session->pc(), halt_pc);

    // Now test stepping after a fault
    DebuggerTestContext ctx_fault;
    ctx_fault.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    .BYTE 0xFF\n" // Invalid opcode
    );

    auto fault_res = ctx_fault.session->step();
    EXPECT_EQ(fault_res.stop_reason, StopReason::Faulted);
    EXPECT_EQ(ctx_fault.session->state(), MachineState::Faulted);
    const word_t fault_pc = ctx_fault.session->pc();

    // Stepping when already faulted
    auto step_fault_again = ctx_fault.session->step();
    EXPECT_EQ(step_fault_again.stop_reason, StopReason::AlreadyFaulted);
    EXPECT_EQ(step_fault_again.steps_executed, 0);
    EXPECT_EQ(ctx_fault.session->pc(), fault_pc);
}

// 5. Address breakpoint stops before execution
TEST(Debugger, AddressBreakpointStopsBeforeExecution) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"  // 0x0200..0x0202
        "    LDI R1, 20\n"  // 0x0203..0x0205
        "    HALT\n"
    );

    // Place breakpoint at 0x0203
    auto bp_res = ctx.session->add_breakpoint(0x0203);
    EXPECT_TRUE(bp_res.success);
    EXPECT_FALSE(bp_res.duplicate);

    auto run_res = ctx.session->run();
    EXPECT_EQ(run_res.stop_reason, StopReason::BreakpointHit);
    EXPECT_EQ(run_res.steps_executed, 1);
    EXPECT_EQ(run_res.pc, 0x0203);
    EXPECT_EQ(ctx.session->pc(), 0x0203);

    // First instruction executed
    EXPECT_EQ(ctx.session->r(0), 10);
    // Breakpoint instruction NOT executed yet
    EXPECT_EQ(ctx.session->r(1), 0);
}

// 6. Continue from a breakpoint without getting trapped indefinitely
TEST(Debugger, ContinueFromBreakpointWithoutTrapping) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    HALT\n"
    );

    ctx.session->add_breakpoint(0x0203);
    auto r1 = ctx.session->run();
    EXPECT_EQ(r1.stop_reason, StopReason::BreakpointHit);

    // Continue execution from breakpoint
    auto r2 = ctx.session->continue_execution();
    EXPECT_EQ(r2.stop_reason, StopReason::Halted);
    EXPECT_EQ(r2.steps_executed, 2); // Executed LDI R1, 20 and HALT
    EXPECT_EQ(ctx.session->r(1), 20);
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
}

// 7. Single-step at an address that has a breakpoint
TEST(Debugger, SingleStepAtAddressWithBreakpoint) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    HALT\n"
    );

    // Breakpoint placed directly at entry point 0x0200
    ctx.session->add_breakpoint(0x0200);

    // Single step must execute despite the breakpoint
    auto res = ctx.session->step();
    EXPECT_EQ(res.stop_reason, StopReason::StepCompleted);
    EXPECT_EQ(res.steps_executed, 1);
    EXPECT_EQ(ctx.session->pc(), 0x0203);
    EXPECT_EQ(ctx.session->r(0), 10);
}

// 8. Breakpoints after branches and jumps
TEST(Debugger, BreakpointsAfterBranchesAndJumps) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    JMP target\n"
        "    LDI R0, 99\n"
        "target:\n"
        "    LDI R0, 42\n"
        "    HALT\n"
    );

    // Target is at 0x0206
    ctx.session->add_breakpoint(0x0206);

    auto res = ctx.session->run();
    EXPECT_EQ(res.stop_reason, StopReason::BreakpointHit);
    EXPECT_EQ(ctx.session->pc(), 0x0206);
    EXPECT_EQ(ctx.session->r(0), 0); // Not executed yet

    auto cont_res = ctx.session->continue_execution();
    EXPECT_EQ(cont_res.stop_reason, StopReason::Halted);
    EXPECT_EQ(ctx.session->r(0), 42); // 99 was bypassed
}

// 9. Run-to-HALT
TEST(Debugger, RunToHalt) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 5\n"
        "    LDI R1, 10\n"
        "    ADD R0, R1\n"
        "    HALT\n"
    );

    auto res = ctx.session->run();
    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(res.steps_executed, 4);
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    EXPECT_EQ(ctx.session->r(0), 15);
}

// 10. Budget exhaustion in an infinite loop
TEST(Debugger, BudgetExhaustionInfiniteLoop) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY loop\n"
        "loop:\n"
        "    JMP loop\n"
    );

    auto res = ctx.session->run(25);
    EXPECT_EQ(res.stop_reason, StopReason::BudgetExhausted);
    EXPECT_EQ(res.steps_executed, 25);
    EXPECT_EQ(ctx.session->pc(), 0x0200);
    EXPECT_EQ(ctx.session->state(), MachineState::Running);
}

// 11. HALT on the final permitted step
TEST(Debugger, HaltOnFinalPermittedStep) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 1\n"
        "    LDI R1, 2\n"
        "    HALT\n"
    );

    // Exactly 3 instructions to reach HALT
    auto res = ctx.session->run(3);
    EXPECT_EQ(res.stop_reason, StopReason::Halted);
    EXPECT_EQ(res.steps_executed, 3);
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
}

// 12. CPU fault reporting and faulting instruction address
TEST(Debugger, CpuFaultReportingAndFaultingAddress) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 1\n"
        "    .BYTE 0xFF\n" // Invalid opcode at 0x0203
    );

    auto res = ctx.session->run();
    EXPECT_EQ(res.stop_reason, StopReason::Faulted);
    EXPECT_TRUE(res.fault.has_value());
    EXPECT_EQ(res.fault->type, FaultType::InvalidOpcode);
    EXPECT_EQ(res.fault->instruction_address, 0x0203);
    // Fault atomicity: PC remains at faulting instruction
    EXPECT_EQ(ctx.session->pc(), 0x0203);
    EXPECT_EQ(ctx.session->state(), MachineState::Faulted);
}

// 13. Source-line breakpoint resolution
TEST(Debugger, SourceLineBreakpointResolution) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"     // Line 1
        ".ENTRY start\n"    // Line 2
        "start:\n"          // Line 3
        "    LDI R0, 10\n"  // Line 4 (0x0200)
        "    LDI R1, 20\n"  // Line 5 (0x0203)
        "    HALT\n"        // Line 6 (0x0206)
    );

    auto bp_res = ctx.session->add_line_breakpoint(5);
    EXPECT_TRUE(bp_res.success);
    EXPECT_EQ(bp_res.address, 0x0203);

    auto run_res = ctx.session->run();
    EXPECT_EQ(run_res.stop_reason, StopReason::BreakpointHit);
    EXPECT_EQ(ctx.session->pc(), 0x0203);

    auto loc = ctx.session->resolve_source_location();
    EXPECT_TRUE(loc.has_value());
    EXPECT_EQ(loc->line, 5);
}

// 14. Source mappings across multiple .ORG segments
TEST(Debugger, SourceMappingsAcrossMultipleOrgSegments) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"         // Line 1
        ".ENTRY start\n"        // Line 2
        "start:\n"              // Line 3
        "    JMP second\n"      // Line 4 (0x0200)
        "\n"                    // Line 5
        ".ORG 0x0600\n"         // Line 6
        "second:\n"             // Line 7
        "    LDI R0, 99\n"      // Line 8 (0x0600)
        "    HALT\n"            // Line 9 (0x0603)
    );

    // Resolve line in second segment
    auto bp_res = ctx.session->add_line_breakpoint(8);
    EXPECT_TRUE(bp_res.success);
    EXPECT_EQ(bp_res.address, 0x0600);

    auto run_res = ctx.session->run();
    EXPECT_EQ(run_res.stop_reason, StopReason::BreakpointHit);
    EXPECT_EQ(ctx.session->pc(), 0x0600);

    auto loc8 = ctx.session->resolve_source_location(0x0600);
    EXPECT_TRUE(loc8.has_value());
    EXPECT_EQ(loc8->line, 8);

    auto loc4 = ctx.session->resolve_source_location(0x0200);
    EXPECT_TRUE(loc4.has_value());
    EXPECT_EQ(loc4->line, 4);
}

// 15. Unmappable blank, label-only, directive-only, and data-only source lines
TEST(Debugger, UnmappableSourceLines) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"         // Line 1 (DirectiveOnly)
        ".ENTRY start\n"        // Line 2 (DirectiveOnly)
        "; just a comment\n"    // Line 3 (EmptyOrComment)
        "start:\n"              // Line 4 (LabelOnly)
        "    LDI R0, 10\n"      // Line 5 (Instruction)
        "\n"                    // Line 6 (EmptyOrComment)
        "data_table:\n"         // Line 7 (LabelOnly)
        "    .BYTE 1, 2, 3\n"   // Line 8 (DataOnly)
        "    HALT\n"            // Line 9 (Instruction)
    );

    // Line 1: DirectiveOnly (.ORG)
    auto bp1 = ctx.session->add_line_breakpoint(1);
    EXPECT_FALSE(bp1.success);
    EXPECT_NE(bp1.message.find("contains an assembler directive"), std::string::npos);

    // Line 3: Comment
    auto bp3 = ctx.session->add_line_breakpoint(3);
    EXPECT_FALSE(bp3.success);
    EXPECT_NE(bp3.message.find("blank or contains only comments"), std::string::npos);

    // Line 4: LabelOnly (start:)
    auto bp4 = ctx.session->add_line_breakpoint(4);
    EXPECT_FALSE(bp4.success);
    EXPECT_NE(bp4.message.find("contains only a label definition"), std::string::npos);

    // Line 6: Blank line
    auto bp6 = ctx.session->add_line_breakpoint(6);
    EXPECT_FALSE(bp6.success);
    EXPECT_NE(bp6.message.find("blank or contains only comments"), std::string::npos);

    // Line 8: DataOnly (.BYTE)
    auto bp8 = ctx.session->add_line_breakpoint(8);
    EXPECT_FALSE(bp8.success);
    EXPECT_NE(bp8.message.find("contains data declarations"), std::string::npos);

    // Line 100: Out of range
    auto bp100 = ctx.session->add_line_breakpoint(100);
    EXPECT_FALSE(bp100.success);
    EXPECT_NE(bp100.message.find("out of range"), std::string::npos);

    // Line 5: Valid instruction
    auto bp5 = ctx.session->add_line_breakpoint(5);
    EXPECT_TRUE(bp5.success);
    EXPECT_EQ(bp5.address, 0x0200);
}

// 16. Register and flag inspection
TEST(Debugger, RegisterAndFlagInspection) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0x7F\n"
        "    LDI R1, 0x01\n"
        "    ADD R0, R1\n"    // 0x7F + 0x01 = 0x80 -> Sets N and V
        "    HALT\n"
    );

    ctx.session->run();
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    EXPECT_EQ(ctx.session->registers().r[0], 0x80);
    EXPECT_EQ(ctx.session->registers().r[1], 0x01);
    EXPECT_EQ(ctx.session->flags().n, true);
    EXPECT_EQ(ctx.session->flags().v, true);
    EXPECT_EQ(ctx.session->flags().z, false);
    EXPECT_EQ(ctx.session->flags().c, false);
}

// 17. Read-only memory inspection
TEST(Debugger, ReadOnlyMemoryInspection) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0xAA\n"
        "    ST [0x8000], R0\n"
        "    HALT\n"
    );

    ctx.session->run();

    // Inspect single byte
    EXPECT_EQ(ctx.session->read_memory_byte(0x8000), 0xAA);

    // Inspect range
    auto range = ctx.session->read_memory_range(0x8000, 3);
    EXPECT_TRUE(range.success);
    EXPECT_EQ(range.data.size(), 3);
    EXPECT_EQ(range.data[0], 0xAA);
    EXPECT_EQ(range.data[1], 0x00);
    EXPECT_EQ(range.data[2], 0x00);

    // Side-effect-free: virtual devices untouched
    EXPECT_EQ(ctx.bus.input_fifo_size(), 0);
    EXPECT_TRUE(ctx.bus.text_output().empty());
    EXPECT_EQ(ctx.bus.led_state(), 0);
}

// 18. Stack inspection, including empty stack and boundaries
TEST(Debugger, StackInspection) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0x11\n"
        "    PUSH R0\n"
        "    LDI R1, 0x22\n"
        "    PUSH R1\n"
        "    HALT\n"
    );

    // Initial state: empty stack
    auto initial_stack = ctx.session->inspect_stack();
    EXPECT_TRUE(initial_stack.is_empty);
    EXPECT_EQ(initial_stack.depth, 0);
    EXPECT_EQ(initial_stack.sp, 0xFFFF);
    EXPECT_TRUE(initial_stack.entries.empty());

    // Step LDI R0, 11
    ctx.session->step();
    // Step PUSH R0 -> SP becomes 0xFFFE
    ctx.session->step();

    auto stack_1 = ctx.session->inspect_stack();
    EXPECT_FALSE(stack_1.is_empty);
    EXPECT_EQ(stack_1.depth, 1);
    EXPECT_EQ(stack_1.sp, 0xFFFE);
    EXPECT_EQ(stack_1.entries.size(), 1);
    EXPECT_EQ(stack_1.entries[0].address, 0xFFFE);
    EXPECT_EQ(stack_1.entries[0].value, 0x11);

    // Step LDI R1, 22 and PUSH R1 -> SP becomes 0xFFFD
    ctx.session->step();
    ctx.session->step();

    auto stack_2 = ctx.session->inspect_stack();
    EXPECT_FALSE(stack_2.is_empty);
    EXPECT_EQ(stack_2.depth, 2);
    EXPECT_EQ(stack_2.sp, 0xFFFD);
    EXPECT_EQ(stack_2.entries.size(), 2);
    EXPECT_EQ(stack_2.entries[0].address, 0xFFFD); // Top-of-stack
    EXPECT_EQ(stack_2.entries[0].value, 0x22);
    EXPECT_EQ(stack_2.entries[1].address, 0xFFFE);
    EXPECT_EQ(stack_2.entries[1].value, 0x11);
}

// 19. Bounded trace retention and trace ordering
TEST(Debugger, BoundedTraceRetentionAndOrdering) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 1\n"  // Step 1
        "    LDI R1, 2\n"  // Step 2
        "    LDI R2, 3\n"  // Step 3
        "    LDI R3, 4\n"  // Step 4
        "    HALT\n"       // Step 5
    );

    ctx.session->set_trace_capacity(3);
    EXPECT_EQ(ctx.session->trace_capacity(), 3);

    ctx.session->run();
    EXPECT_EQ(ctx.session->state(), MachineState::Halted);
    EXPECT_EQ(ctx.session->total_steps_executed(), 5);

    const auto& trace = ctx.session->trace();
    EXPECT_EQ(trace.size(), 3);

    // Oldest retained is Step 3 (LDI R2, 3)
    EXPECT_EQ(trace[0].step_number, 3);
    EXPECT_EQ(trace[0].mnemonic, "LDI");

    // Middle is Step 4 (LDI R3, 4)
    EXPECT_EQ(trace[1].step_number, 4);
    EXPECT_EQ(trace[1].mnemonic, "LDI");

    // Newest is Step 5 (HALT)
    EXPECT_EQ(trace[2].step_number, 5);
    EXPECT_EQ(trace[2].mnemonic, "HALT");
    EXPECT_EQ(trace[2].state_after, MachineState::Halted);
}

// 20. Correct behavior for memory-range requests near 0xFFFF
TEST(Debugger, MemoryRangeNear64KBoundary) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    // Write sentinel bytes across the 0xFFFF/0x0000 boundary
    ctx.memory.write_byte(0xFFFE, 0xAA);
    ctx.memory.write_byte(0xFFFF, 0xBB);
    ctx.memory.write_byte(0x0000, 0xCC);
    ctx.memory.write_byte(0x0001, 0xDD);

    auto range = ctx.session->read_memory_range(0xFFFE, 4);
    EXPECT_TRUE(range.success);
    EXPECT_EQ(range.data.size(), 4);
    EXPECT_EQ(range.data[0], 0xAA); // 0xFFFE
    EXPECT_EQ(range.data[1], 0xBB); // 0xFFFF
    EXPECT_EQ(range.data[2], 0xCC); // 0x0000 (wrapped)
    EXPECT_EQ(range.data[3], 0xDD); // 0x0001 (wrapped)
}

// Additional edge cases: duplicate breakpoints and removal
TEST(Debugger, DuplicateBreakpointsAndRemoval) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    auto r1 = ctx.session->add_breakpoint(0x0200);
    EXPECT_TRUE(r1.success);
    EXPECT_FALSE(r1.duplicate);

    // Duplicate add
    auto r2 = ctx.session->add_breakpoint(0x0200);
    EXPECT_TRUE(r2.success);
    EXPECT_TRUE(r2.duplicate);

    EXPECT_TRUE(ctx.session->has_breakpoint(0x0200));
    EXPECT_EQ(ctx.session->breakpoints().size(), 1);

    // Removal
    auto rm1 = ctx.session->remove_breakpoint(0x0200);
    EXPECT_TRUE(rm1.success);
    EXPECT_FALSE(rm1.not_found);
    EXPECT_FALSE(ctx.session->has_breakpoint(0x0200));

    // Remove non-existent
    auto rm2 = ctx.session->remove_breakpoint(0x0200);
    EXPECT_FALSE(rm2.success);
    EXPECT_TRUE(rm2.not_found);
}

// Additional edge case: zero-budget run
TEST(Debugger, ZeroBudgetRun) {
    DebuggerTestContext ctx;
    ctx.load_source(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    auto res = ctx.session->run(0);
    EXPECT_EQ(res.stop_reason, StopReason::BudgetExhausted);
    EXPECT_EQ(res.steps_executed, 0);
    EXPECT_EQ(ctx.session->state(), MachineState::Ready);
}
