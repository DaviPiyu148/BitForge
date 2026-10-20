#include "test_harness.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/constants.hpp"

// =============================================================
// Helper Fixture for Stack Instruction Tests
// =============================================================
struct StackFixture {
    bitforge::Memory memory;
    bitforge::SystemBus bus;
    bitforge::Cpu cpu;

    StackFixture()
        : memory(), bus(memory), cpu(bus) {
        cpu.reset(0x0200);
    }
};

// =============================================================
// 1. Basic PUSH and POP Functionality
// =============================================================

TEST(StackTest, PushAndPopSingleByte) {
    StackFixture fix;
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Empty sentinel

    // 1. LDI R0, 0x42 (0x10, 0x00, 0x42)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x42);
    // 2. PUSH R0 (0x62, 0x00)
    fix.memory.write_byte(0x0203, 0x62); fix.memory.write_byte(0x0204, 0x00);
    // 3. POP R1 (0x63, 0x01)
    fix.memory.write_byte(0x0205, 0x63); fix.memory.write_byte(0x0206, 0x01);

    fix.cpu.step(); // LDI R0
    EXPECT_EQ(fix.cpu.registers().r[0], 0x42);

    auto r_push = fix.cpu.step(); // PUSH R0
    EXPECT_TRUE(r_push.executed);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE); // Decrements before write
    EXPECT_EQ(fix.memory.read_byte(0xFFFE), 0x42); // Stored at occupied top-of-stack

    auto r_pop = fix.cpu.step(); // POP R1
    EXPECT_TRUE(r_pop.executed);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x42);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Restores empty sentinel
}

TEST(StackTest, LifoRegisterRestoration) {
    StackFixture fix;
    // Push R0=10, R1=20, R2=30; Pop into R3, then R2, then R1
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 10);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 20);
    fix.memory.write_byte(0x0206, 0x10); fix.memory.write_byte(0x0207, 0x02); fix.memory.write_byte(0x0208, 30);

    fix.memory.write_byte(0x0209, 0x62); fix.memory.write_byte(0x020A, 0x00); // PUSH R0 -> SP 0xFFFE
    fix.memory.write_byte(0x020B, 0x62); fix.memory.write_byte(0x020C, 0x01); // PUSH R1 -> SP 0xFFFD
    fix.memory.write_byte(0x020D, 0x62); fix.memory.write_byte(0x020E, 0x02); // PUSH R2 -> SP 0xFFFC

    fix.memory.write_byte(0x020F, 0x63); fix.memory.write_byte(0x0210, 0x03); // POP R3 (should get 30)
    fix.memory.write_byte(0x0211, 0x63); fix.memory.write_byte(0x0212, 0x02); // POP R2 (should get 20)
    fix.memory.write_byte(0x0213, 0x63); fix.memory.write_byte(0x0214, 0x01); // POP R1 (should get 10)

    for (int i = 0; i < 9; ++i) {
        auto step_res = fix.cpu.step();
        EXPECT_TRUE(step_res.executed);
    }

    EXPECT_EQ(fix.cpu.registers().r[3], 30);
    EXPECT_EQ(fix.cpu.registers().r[2], 20);
    EXPECT_EQ(fix.cpu.registers().r[1], 10);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
}

// =============================================================
// 2. Stack Boundary Failures (Underflow and Overflow)
// =============================================================

TEST(StackTest, EmptyStackUnderflowFault) {
    StackFixture fix;
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Empty

    // POP R0 when SP == 0xFFFF
    fix.memory.write_byte(0x0200, 0x63);
    fix.memory.write_byte(0x0201, 0x00);

    auto res = fix.cpu.step();
    EXPECT_FALSE(res.executed);
    EXPECT_TRUE(res.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackUnderflow);
    EXPECT_EQ(fix.cpu.last_fault().instruction_address, 0x0200);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200); // Universal PC preservation
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // SP unchanged
}

TEST(StackTest, PushToLastValidByteAndOverflow) {
    StackFixture fix;
    // To position SP near full stack (0xF000) without running thousands of steps:
    // PUSH from SP == 0xF001 succeeds, storing at 0xF000 and setting SP = 0xF000.
    // Next PUSH from SP == 0xF000 must fault with StackOverflow.

    // Fill stack by repeatedly pushing or calling.
    // Instead of artificial SP assignment, execute a loop of CALLs to consume stack.
    // Each CALL consumes 2 bytes. From 0xFFFF to 0xF001 is 4094 bytes = 2047 CALLs.
    // We can also verify by calling from a helper function.
    // Let's test overflow boundary condition:
    // When SP reaches 0xF000, PUSH must fault.
    // Let's set up memory with CALL instructions that recurse until stack overflows!
    // At 0x0200: CALL 0x0200 (recursive infinite call)
    fix.memory.write_byte(0x0200, 0x60);
    fix.memory.write_byte(0x0201, 0x02);
    fix.memory.write_byte(0x0202, 0x00);

    // Run until fault
    auto run_res = bitforge::test::run_with_budget(fix.cpu, 10000);
    EXPECT_FALSE(run_res.success);
    EXPECT_EQ(run_res.stop_reason, "Faulted");
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackOverflow);
    EXPECT_EQ(fix.cpu.last_fault().instruction_address, 0x0200);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
    // SP must be >= 0xF000 and < 0xF002 when CALL aborted
    EXPECT_TRUE(fix.cpu.registers().sp >= 0xF000 && fix.cpu.registers().sp < 0xF002);
}

TEST(StackTest, RetUnderflowFault) {
    StackFixture fix;
    // RET when stack is empty (SP == 0xFFFF)
    fix.memory.write_byte(0x0200, 0x61);

    auto res = fix.cpu.step();
    EXPECT_FALSE(res.executed);
    EXPECT_TRUE(res.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackUnderflow);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);

    // RET when stack has only 1 byte (SP == 0xFFFE)
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x00); fix.memory.write_byte(0x0302, 0x11);
    fix.memory.write_byte(0x0303, 0x62); fix.memory.write_byte(0x0304, 0x00); // PUSH R0 -> SP becomes 0xFFFE
    fix.memory.write_byte(0x0305, 0x61); // RET requires at least 2 bytes (SP <= 0xFFFD)

    fix.cpu.step(); // LDI
    fix.cpu.step(); // PUSH
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE);

    auto r_ret_fail = fix.cpu.step(); // RET with only 1 byte
    EXPECT_FALSE(r_ret_fail.executed);
    EXPECT_TRUE(r_ret_fail.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackUnderflow);
    EXPECT_EQ(fix.cpu.last_fault().instruction_address, 0x0305);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0305);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE); // Preserved
}

// =============================================================
// 3. Subroutine CALL and RET
// =============================================================

TEST(StackTest, CallAndRetExecution) {
    StackFixture fix;
    // Main at 0x0200:
    // 0x0200: LDI R0, 5
    // 0x0203: CALL 0x0300
    // 0x0206: HALT
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 5);
    fix.memory.write_byte(0x0203, 0x60); fix.memory.write_byte(0x0204, 0x03); fix.memory.write_byte(0x0205, 0x00);
    fix.memory.write_byte(0x0206, 0x01); // HALT

    // Subroutine at 0x0300:
    // 0x0300: LDI R1, 10
    // 0x0303: ADD R0, R1 (R0 = 15)
    // 0x0306: RET
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x01); fix.memory.write_byte(0x0302, 10);
    fix.memory.write_byte(0x0303, 0x20); fix.memory.write_byte(0x0304, 0x00); fix.memory.write_byte(0x0305, 0x01);
    fix.memory.write_byte(0x0306, 0x61);

    auto result = bitforge::test::run_with_budget(fix.cpu, 20);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stop_reason, "Halted");
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(fix.cpu.registers().r[0], 15);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Stack fully restored
    EXPECT_EQ(fix.cpu.registers().pc, 0x0207); // Advanced past HALT
}

TEST(StackTest, CallBigEndianReturnAddressLayout) {
    StackFixture fix;
    // CALL 0x0400 placed at 0x0200.
    // Return address is 0x0203 (high=0x02, low=0x03).
    fix.memory.write_byte(0x0200, 0x60);
    fix.memory.write_byte(0x0201, 0x04);
    fix.memory.write_byte(0x0202, 0x00);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0400);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFD); // 0xFFFF - 2 = 0xFFFD

    // Verify big-endian return address stored in memory:
    // memory[SP] == memory[0xFFFD] holds ret_hi (0x02)
    // memory[SP + 1] == memory[0xFFFE] holds ret_lo (0x03)
    EXPECT_EQ(fix.memory.read_byte(0xFFFD), 0x02);
    EXPECT_EQ(fix.memory.read_byte(0xFFFE), 0x03);
}

TEST(StackTest, NestedSubroutineCalls) {
    StackFixture fix;
    // Main at 0x0200:
    // 0x0200: CALL 0x0300 (Sub1)
    // 0x0203: HALT
    fix.memory.write_byte(0x0200, 0x60); fix.memory.write_byte(0x0201, 0x03); fix.memory.write_byte(0x0202, 0x00);
    fix.memory.write_byte(0x0203, 0x01);

    // Sub1 at 0x0300:
    // 0x0300: INC R0
    // 0x0302: CALL 0x0400 (Sub2)
    // 0x0305: INC R0
    // 0x0307: RET
    fix.memory.write_byte(0x0300, 0x22); fix.memory.write_byte(0x0301, 0x00);
    fix.memory.write_byte(0x0302, 0x60); fix.memory.write_byte(0x0303, 0x04); fix.memory.write_byte(0x0304, 0x00);
    fix.memory.write_byte(0x0305, 0x22); fix.memory.write_byte(0x0306, 0x00);
    fix.memory.write_byte(0x0307, 0x61);

    // Sub2 at 0x0400:
    // 0x0400: INC R0
    // 0x0402: CALL 0x0500 (Sub3)
    // 0x0405: INC R0
    // 0x0407: RET
    fix.memory.write_byte(0x0400, 0x22); fix.memory.write_byte(0x0401, 0x00);
    fix.memory.write_byte(0x0402, 0x60); fix.memory.write_byte(0x0403, 0x05); fix.memory.write_byte(0x0404, 0x00);
    fix.memory.write_byte(0x0405, 0x22); fix.memory.write_byte(0x0406, 0x00);
    fix.memory.write_byte(0x0407, 0x61);

    // Sub3 at 0x0500:
    // 0x0500: INC R0
    // 0x0502: RET
    fix.memory.write_byte(0x0500, 0x22); fix.memory.write_byte(0x0501, 0x00);
    fix.memory.write_byte(0x0502, 0x61);

    auto result = bitforge::test::run_with_budget(fix.cpu, 50);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stop_reason, "Halted");
    EXPECT_EQ(fix.cpu.registers().r[0], 5); // Incremented 5 times total
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Stack restored
    EXPECT_EQ(fix.cpu.registers().pc, 0x0204);
}

// =============================================================
// 4. Invalid Register Encodings
// =============================================================

TEST(StackTest, InvalidRegisterEncodingsInPushAndPop) {
    StackFixture fix;

    // PUSH R4 (invalid register 4)
    fix.memory.write_byte(0x0200, 0x62);
    fix.memory.write_byte(0x0201, 0x04);

    auto r_push_fault = fix.cpu.step();
    EXPECT_FALSE(r_push_fault.executed);
    EXPECT_TRUE(r_push_fault.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);

    // POP R5 (invalid register 5)
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x63);
    fix.memory.write_byte(0x0301, 0x05);

    auto r_pop_fault = fix.cpu.step();
    EXPECT_FALSE(r_pop_fault.executed);
    EXPECT_TRUE(r_pop_fault.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0300);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
}

// =============================================================
// 5. Phase 3 ADR-001 Boundary, Sentinel, and Atomicity Tests
// =============================================================

TEST(StackTest, ExactBoundaryPushAtF001AndOverflowAtF000) {
    StackFixture fix;

    // Setup push loop at 0x0200:
    // 0x0200: PUSH R0 (0x62, 0x00)
    // 0x0202: JMP 0x0200 (0x50, 0x02, 0x00)
    fix.memory.write_byte(0x01F0, 0x10); fix.memory.write_byte(0x01F1, 0x00); fix.memory.write_byte(0x01F2, 0x5A); // LDI R0, 0x5A
    fix.memory.write_byte(0x01F3, 0x50); fix.memory.write_byte(0x01F4, 0x02); fix.memory.write_byte(0x01F5, 0x00); // JMP 0x0200

    fix.memory.write_byte(0x0200, 0x62); fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x50); fix.memory.write_byte(0x0203, 0x02); fix.memory.write_byte(0x0204, 0x00);

    fix.cpu.reset(0x01F0);
    fix.cpu.step(); // LDI R0, 0x5A
    fix.cpu.step(); // JMP 0x0200
    EXPECT_EQ(fix.cpu.registers().r[0], 0x5A);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);

    // Execute 4094 iterations of (PUSH R0; JMP 0x0200) to advance SP from 0xFFFF down to 0xF001
    for (int i = 0; i < 4094; ++i) {
        auto r_push = fix.cpu.step();
        EXPECT_TRUE(r_push.executed);
        auto r_jmp = fix.cpu.step();
        EXPECT_TRUE(r_jmp.executed);
    }
    EXPECT_EQ(fix.cpu.registers().sp, 0xF001);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
    EXPECT_EQ(fix.memory.read_byte(0xF001), 0x5A);
    EXPECT_EQ(fix.memory.read_byte(0xF000), 0x00);
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00);

    // Final valid push from SP == 0xF001 under ADR-001 §4:
    // Pre-validation (SP > 0xF000) passes; SP decrements to 0xF000 and writes byte to 0xF000
    auto r_last_valid_push = fix.cpu.step();
    EXPECT_TRUE(r_last_valid_push.executed);
    EXPECT_FALSE(r_last_valid_push.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Running);
    EXPECT_EQ(fix.cpu.registers().sp, 0xF000);
    EXPECT_EQ(fix.memory.read_byte(0xF000), 0x5A);
    // Boundary neighbor isolation: RAM below 0xF000 must NOT be modified
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00);

    // Jump back to 0x0200
    auto r_jmp2 = fix.cpu.step();
    EXPECT_TRUE(r_jmp2.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
    EXPECT_EQ(fix.cpu.registers().sp, 0xF000);

    // Subsequent PUSH when SP == 0xF000 must immediately fault with StackOverflow!
    auto r_overflow = fix.cpu.step();
    EXPECT_FALSE(r_overflow.executed);
    EXPECT_TRUE(r_overflow.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackOverflow);
    EXPECT_EQ(fix.cpu.last_fault().instruction_address, 0x0200);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200); // Universal PC preservation
    EXPECT_EQ(fix.cpu.registers().sp, 0xF000); // SP unchanged
    EXPECT_EQ(fix.cpu.registers().r[0], 0x5A); // Register unchanged
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00); // Memory below 0xF000 untouched
    EXPECT_EQ(fix.memory.read_byte(0xF000), 0x5A); // Existing top byte intact
}

TEST(StackTest, ExactBoundaryCallAtF002AndOverflowAtF001AndF000) {
    StackFixture fix;

    // 1. Position SP at 0xF002 by pushing 4093 bytes (0xFFFF - 4093 = 0xF002)
    fix.memory.write_byte(0x0200, 0x62); // PUSH R0
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x50); // JMP 0x0200
    fix.memory.write_byte(0x0203, 0x02);
    fix.memory.write_byte(0x0204, 0x00);

    fix.cpu.reset(0x0200);
    for (int i = 0; i < 4093; ++i) {
        fix.cpu.step(); // PUSH
        fix.cpu.step(); // JMP
    }
    EXPECT_EQ(fix.cpu.registers().sp, 0xF002);

    // Write CALL 0x0300 at 0x0200
    fix.memory.write_byte(0x0200, 0x60);
    fix.memory.write_byte(0x0201, 0x03);
    fix.memory.write_byte(0x0202, 0x00);

    // Write CALL 0x0400 at 0x0300
    fix.memory.write_byte(0x0300, 0x60);
    fix.memory.write_byte(0x0301, 0x04);
    fix.memory.write_byte(0x0302, 0x00);

    // CALL at SP == 0xF002 succeeds and decrements SP to 0xF000
    auto r_call = fix.cpu.step();
    EXPECT_TRUE(r_call.executed);
    EXPECT_FALSE(r_call.faulted);
    EXPECT_EQ(fix.cpu.registers().sp, 0xF000);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0300);
    // Return address 0x0203 stored big-endian: hi at 0xF000, lo at 0xF001
    EXPECT_EQ(fix.memory.read_byte(0xF000), 0x02);
    EXPECT_EQ(fix.memory.read_byte(0xF001), 0x03);
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00); // Neighbor isolation

    // Next CALL at SP == 0xF000 must fault with StackOverflow
    auto r_overflow = fix.cpu.step();
    EXPECT_FALSE(r_overflow.executed);
    EXPECT_TRUE(r_overflow.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackOverflow);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0300);
    EXPECT_EQ(fix.cpu.registers().sp, 0xF000);
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00);

    // 2. Also test CALL when SP == 0xF001
    fix.memory.reset();
    fix.cpu.reset(0x0200);
    fix.memory.write_byte(0x0200, 0x62); // restore PUSH loop
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x50); // JMP 0x0200
    fix.memory.write_byte(0x0203, 0x02);
    fix.memory.write_byte(0x0204, 0x00);
    for (int i = 0; i < 4094; ++i) { // 4094 pushes -> SP = 0xF001
        fix.cpu.step();
        fix.cpu.step();
    }
    EXPECT_EQ(fix.cpu.registers().sp, 0xF001);

    // Write CALL 0x0300 at 0x0200
    fix.memory.write_byte(0x0200, 0x60);
    fix.memory.write_byte(0x0201, 0x03);
    fix.memory.write_byte(0x0202, 0x00);

    auto r_call_f001 = fix.cpu.step();
    EXPECT_FALSE(r_call_f001.executed);
    EXPECT_TRUE(r_call_f001.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackOverflow);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
    EXPECT_EQ(fix.cpu.registers().sp, 0xF001);
    EXPECT_EQ(fix.memory.read_byte(0xF000), 0x00);
    EXPECT_EQ(fix.memory.read_byte(0xEFFF), 0x00);
}

TEST(StackTest, SentinelAddress0xFFFFGeneralMemoryIndependence) {
    StackFixture fix;
    // Preload RAM at 0xFFFF with 0x7E
    fix.memory.write_byte(0xFFFF, 0x7E);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);

    // Program at 0x0200:
    // LDI R0, 0x42
    // PUSH R0 (decrements to 0xFFFE, writes 0x42 to 0xFFFE)
    // POP R1  (reads 0x42 from 0xFFFE, increments to 0xFFFF)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x42);
    fix.memory.write_byte(0x0203, 0x62); fix.memory.write_byte(0x0204, 0x00);
    fix.memory.write_byte(0x0205, 0x63); fix.memory.write_byte(0x0206, 0x01);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // PUSH R0
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE);
    EXPECT_EQ(fix.memory.read_byte(0xFFFE), 0x42);
    EXPECT_EQ(fix.memory.read_byte(0xFFFF), 0x7E); // Memory at 0xFFFF is distinct and untouched

    fix.cpu.step(); // POP R1
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x42);
    EXPECT_EQ(fix.memory.read_byte(0xFFFF), 0x7E); // Still untouched
}

TEST(StackTest, StackFaultAtomicityAndRegisterPreservation) {
    StackFixture fix;

    // 1. POP on empty stack must preserve destination register and PC
    // LDI R0, 0xAA; POP R0
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0xAA);
    fix.memory.write_byte(0x0203, 0x63); fix.memory.write_byte(0x0204, 0x00);

    fix.cpu.step(); // LDI R0
    EXPECT_EQ(fix.cpu.registers().r[0], 0xAA);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);

    auto r_pop_underflow = fix.cpu.step(); // POP R0
    EXPECT_FALSE(r_pop_underflow.executed);
    EXPECT_TRUE(r_pop_underflow.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackUnderflow);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0203);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(fix.cpu.registers().r[0], 0xAA); // Preserved

    // 2. RET on 1-byte stack (SP == 0xFFFE)
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x01); fix.memory.write_byte(0x0302, 0x55);
    fix.memory.write_byte(0x0303, 0x62); fix.memory.write_byte(0x0304, 0x01); // PUSH R1 -> SP 0xFFFE
    fix.memory.write_byte(0x0305, 0x61); // RET requires at least 2 bytes

    fix.cpu.step(); // LDI
    fix.cpu.step(); // PUSH
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE);

    auto r_ret_underflow = fix.cpu.step();
    EXPECT_FALSE(r_ret_underflow.executed);
    EXPECT_TRUE(r_ret_underflow.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::StackUnderflow);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0305);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x55);
}
