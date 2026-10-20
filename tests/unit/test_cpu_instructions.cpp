#include "test_harness.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/core/alu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/constants.hpp"

#include <vector>

// =============================================================
// Helper Fixture for CPU Instruction Tests
// =============================================================
struct CpuFixture {
    bitforge::Memory memory;
    bitforge::SystemBus bus;
    bitforge::Cpu cpu;

    CpuFixture()
        : memory(), bus(memory), cpu(bus) {
        cpu.reset(0x0200);
    }
};

// =============================================================
// 1. Control & Data Movement Instructions (11 instructions)
// =============================================================

TEST(CpuInstructions, NopExecution) {
    CpuFixture fix;
    fix.memory.write_byte(0x0200, 0x00); // NOP

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.halted);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0201);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Running);
}

TEST(CpuInstructions, HaltExecution) {
    CpuFixture fix;
    fix.memory.write_byte(0x0200, 0x01); // HALT

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_TRUE(res.halted);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0201);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);

    // Subsequent step while halted must be an explicit no-op
    auto res_after = fix.cpu.step();
    EXPECT_FALSE(res_after.executed);
    EXPECT_TRUE(res_after.halted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0201);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
}

TEST(CpuInstructions, LdiExecution) {
    CpuFixture fix;
    // LDI R0, 0x42 (0x10, 0x00, 0x42); LDI R3, 0xFE (0x10, 0x03, 0xFE)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x42);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x03); fix.memory.write_byte(0x0205, 0xFE);

    auto r1 = fix.cpu.step();
    EXPECT_TRUE(r1.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x42);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0203);

    auto r2 = fix.cpu.step();
    EXPECT_TRUE(r2.executed);
    EXPECT_EQ(fix.cpu.registers().r[3], 0xFE);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0206);
}

TEST(CpuInstructions, MovExecution) {
    CpuFixture fix;
    // LDI R1, 0x55 (0x10, 0x01, 0x55)
    fix.memory.write_byte(0x0200, 0x10);
    fix.memory.write_byte(0x0201, 0x01);
    fix.memory.write_byte(0x0202, 0x55);

    // MOV R2, R1 (0x11, 0x02, 0x01)
    fix.memory.write_byte(0x0203, 0x11);
    fix.memory.write_byte(0x0204, 0x02);
    fix.memory.write_byte(0x0205, 0x01);

    auto r_ldi = fix.cpu.step();
    EXPECT_TRUE(r_ldi.executed);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x55);

    auto r_mov = fix.cpu.step();
    EXPECT_TRUE(r_mov.executed);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x55);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x55);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0206);
}

TEST(CpuInstructions, LdAndStExecution) {
    CpuFixture fix;
    // LDI R0, 0xAA (0x10, 0x00, 0xAA)
    fix.memory.write_byte(0x0200, 0x10);
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0xAA);

    // ST [0x8050], R0 (0x13, 0x80, 0x50, 0x00)
    fix.memory.write_byte(0x0203, 0x13);
    fix.memory.write_byte(0x0204, 0x80);
    fix.memory.write_byte(0x0205, 0x50);
    fix.memory.write_byte(0x0206, 0x00);

    // LD R1, [0x8050] (0x12, 0x01, 0x80, 0x50)
    fix.memory.write_byte(0x0207, 0x12);
    fix.memory.write_byte(0x0208, 0x01);
    fix.memory.write_byte(0x0209, 0x80);
    fix.memory.write_byte(0x020A, 0x50);

    auto r_ldi = fix.cpu.step();
    EXPECT_TRUE(r_ldi.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0xAA);

    auto r_st = fix.cpu.step();
    EXPECT_TRUE(r_st.executed);
    EXPECT_EQ(fix.memory.read_byte(0x8050), 0xAA);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0207);

    auto r_ld = fix.cpu.step();
    EXPECT_TRUE(r_ld.executed);
    EXPECT_EQ(fix.cpu.registers().r[1], 0xAA);
    EXPECT_EQ(fix.cpu.registers().pc, 0x020B);
}

TEST(CpuInstructions, Ldi16Execution) {
    CpuFixture fix;
    // LDI16 IX, 0x8000 (0x14, 0x80, 0x00)
    fix.memory.write_byte(0x0200, 0x14);
    fix.memory.write_byte(0x0201, 0x80);
    fix.memory.write_byte(0x0202, 0x00);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().ix, 0x8000);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0203);
}

TEST(CpuInstructions, LdxAndStxExecution) {
    CpuFixture fix;
    // LDI16 IX, 0x8100 (0x14, 0x81, 0x00)
    fix.memory.write_byte(0x0200, 0x14);
    fix.memory.write_byte(0x0201, 0x81);
    fix.memory.write_byte(0x0202, 0x00);

    // LDI R2, 0x77 (0x10, 0x02, 0x77)
    fix.memory.write_byte(0x0203, 0x10);
    fix.memory.write_byte(0x0204, 0x02);
    fix.memory.write_byte(0x0205, 0x77);

    // STX [IX], R2 (0x16, 0x02)
    fix.memory.write_byte(0x0206, 0x16);
    fix.memory.write_byte(0x0207, 0x02);

    // LDX R3, [IX] (0x15, 0x03)
    fix.memory.write_byte(0x0208, 0x15);
    fix.memory.write_byte(0x0209, 0x03);

    fix.cpu.step(); // LDI16 IX
    fix.cpu.step(); // LDI R2

    auto r_stx = fix.cpu.step();
    EXPECT_TRUE(r_stx.executed);
    EXPECT_EQ(fix.memory.read_byte(0x8100), 0x77);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0208);

    auto r_ldx = fix.cpu.step();
    EXPECT_TRUE(r_ldx.executed);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x77);
    EXPECT_EQ(fix.cpu.registers().pc, 0x020A);
}

TEST(CpuInstructions, InxAndDexExecution) {
    CpuFixture fix;
    // LDI16 IX, 0xFFFF (0x14, 0xFF, 0xFF)
    fix.memory.write_byte(0x0200, 0x14);
    fix.memory.write_byte(0x0201, 0xFF);
    fix.memory.write_byte(0x0202, 0xFF);

    // INX (0x17) (wraps 0xFFFF -> 0x0000)
    fix.memory.write_byte(0x0203, 0x17);

    // DEX (0x18) (wraps 0x0000 -> 0xFFFF)
    fix.memory.write_byte(0x0204, 0x18);

    fix.cpu.step(); // LDI16 IX

    auto r_inx = fix.cpu.step();
    EXPECT_TRUE(r_inx.executed);
    EXPECT_EQ(fix.cpu.registers().ix, 0x0000);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0204);

    auto r_dex = fix.cpu.step();
    EXPECT_TRUE(r_dex.executed);
    EXPECT_EQ(fix.cpu.registers().ix, 0xFFFF);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0205);
}

// =============================================================
// 2. Arithmetic & Logic Instructions (9 instructions)
// =============================================================

TEST(CpuInstructions, AddExecutionAndFlags) {
    CpuFixture fix;
    // LDI R0, 0x7F
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x7F);
    // LDI R1, 0x01
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x01);
    // ADD R0, R1 (0x7F + 0x01 = 0x80 -> V=1, N=1, Z=0, C=0)
    fix.memory.write_byte(0x0206, 0x20); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // LDI R1

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x80);
    EXPECT_TRUE(fix.cpu.flags().v);
    EXPECT_TRUE(fix.cpu.flags().n);
    EXPECT_FALSE(fix.cpu.flags().z);
    EXPECT_FALSE(fix.cpu.flags().c);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0209);
}

TEST(CpuInstructions, SubExecutionAndFlags) {
    CpuFixture fix;
    // LDI R0, 0x10
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x10);
    // LDI R1, 0x10
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x10);
    // SUB R0, R1 (0x10 - 0x10 = 0x00 -> Z=1, C=1, N=0, V=0)
    fix.memory.write_byte(0x0206, 0x21); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // LDI R1

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x00);
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().c); // No borrow convention (a >= b)
    EXPECT_FALSE(fix.cpu.flags().n);
    EXPECT_FALSE(fix.cpu.flags().v);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0209);
}

TEST(CpuInstructions, IncAndDecPreserveCarry) {
    CpuFixture fix;
    // Establish Carry=1 via ADD with overflow: 0xFF + 0x01 = 0x100 (C=1)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0xFF);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x01);
    fix.memory.write_byte(0x0206, 0x20); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);

    // LDI R0, 0xFF (LDI preserves existing flags: C remains 1)
    fix.memory.write_byte(0x0209, 0x10); fix.memory.write_byte(0x020A, 0x00); fix.memory.write_byte(0x020B, 0xFF);

    // INC R0 (0xFF -> 0x00, Z=1, C preserved as 1)
    fix.memory.write_byte(0x020C, 0x22); fix.memory.write_byte(0x020D, 0x00);

    // DEC R0 (0x00 -> 0xFF, Z=0, N=1, C preserved as 1)
    fix.memory.write_byte(0x020E, 0x23); fix.memory.write_byte(0x020F, 0x00);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // LDI R1
    fix.cpu.step(); // ADD (sets C=1)
    EXPECT_TRUE(fix.cpu.flags().c);

    fix.cpu.step(); // LDI R0
    EXPECT_TRUE(fix.cpu.flags().c);

    auto r_inc = fix.cpu.step();
    EXPECT_TRUE(r_inc.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x00);
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().c); // Carry preserved

    auto r_dec = fix.cpu.step();
    EXPECT_TRUE(r_dec.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0xFF);
    EXPECT_FALSE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().n);
    EXPECT_TRUE(fix.cpu.flags().c); // Carry preserved
}

TEST(CpuInstructions, BitwiseInstructionsClearCarryAndOverflow) {
    CpuFixture fix;
    // Seed C=1 and V=1 via ADD: 0x80 + 0x80 = 0x100 (C=1, V=1)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x80);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x80);
    fix.memory.write_byte(0x0206, 0x20); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);

    // Set up operands for AND: R0=0xAA, R1=0x55 (LDI preserves C and V)
    fix.memory.write_byte(0x0209, 0x10); fix.memory.write_byte(0x020A, 0x00); fix.memory.write_byte(0x020B, 0xAA);
    fix.memory.write_byte(0x020C, 0x10); fix.memory.write_byte(0x020D, 0x01); fix.memory.write_byte(0x020E, 0x55);

    // AND R0, R1 (0xAA & 0x55 = 0x00, Z=1, C cleared to 0, V cleared to 0)
    fix.memory.write_byte(0x020F, 0x30); fix.memory.write_byte(0x0210, 0x00); fix.memory.write_byte(0x0211, 0x01);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // LDI R1
    fix.cpu.step(); // ADD (sets C=1, V=1)
    EXPECT_TRUE(fix.cpu.flags().c);
    EXPECT_TRUE(fix.cpu.flags().v);

    fix.cpu.step(); // LDI R0, 0xAA
    fix.cpu.step(); // LDI R1, 0x55
    EXPECT_TRUE(fix.cpu.flags().c);
    EXPECT_TRUE(fix.cpu.flags().v);

    auto r_and = fix.cpu.step();
    EXPECT_TRUE(r_and.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x00);
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_FALSE(fix.cpu.flags().c); // Cleared
    EXPECT_FALSE(fix.cpu.flags().v); // Cleared

    // Test OR clears C and V: reseed C and V first
    // ADD R0, R1 where R0=0x80, R1=0x80
    fix.memory.write_byte(0x0212, 0x10); fix.memory.write_byte(0x0213, 0x00); fix.memory.write_byte(0x0214, 0x80);
    fix.memory.write_byte(0x0215, 0x10); fix.memory.write_byte(0x0216, 0x01); fix.memory.write_byte(0x0217, 0x80);
    fix.memory.write_byte(0x0218, 0x20); fix.memory.write_byte(0x0219, 0x00); fix.memory.write_byte(0x021A, 0x01);
    // OR R0, R1 (0x00 | 0x80 = 0x80)
    fix.memory.write_byte(0x021B, 0x31); fix.memory.write_byte(0x021C, 0x00); fix.memory.write_byte(0x021D, 0x01);

    fix.cpu.step(); // LDI
    fix.cpu.step(); // LDI
    fix.cpu.step(); // ADD (sets C=1, V=1)
    EXPECT_TRUE(fix.cpu.flags().c);

    auto r_or = fix.cpu.step();
    EXPECT_TRUE(r_or.executed);
    EXPECT_FALSE(fix.cpu.flags().c); // Cleared
    EXPECT_FALSE(fix.cpu.flags().v); // Cleared

    // Test XOR clears C and V: reseed C and V first
    fix.memory.write_byte(0x021E, 0x10); fix.memory.write_byte(0x021F, 0x00); fix.memory.write_byte(0x0220, 0x80);
    fix.memory.write_byte(0x0221, 0x10); fix.memory.write_byte(0x0222, 0x01); fix.memory.write_byte(0x0223, 0x80);
    fix.memory.write_byte(0x0224, 0x20); fix.memory.write_byte(0x0225, 0x00); fix.memory.write_byte(0x0226, 0x01);
    // XOR R0, R1 (0x00 ^ 0x80 = 0x80)
    fix.memory.write_byte(0x0227, 0x32); fix.memory.write_byte(0x0228, 0x00); fix.memory.write_byte(0x0229, 0x01);

    fix.cpu.step();
    fix.cpu.step();
    fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().v);

    auto r_xor = fix.cpu.step();
    EXPECT_TRUE(r_xor.executed);
    EXPECT_FALSE(fix.cpu.flags().c);
    EXPECT_FALSE(fix.cpu.flags().v);

    // Test NOT clears C and V: reseed C and V first
    fix.memory.write_byte(0x022A, 0x10); fix.memory.write_byte(0x022B, 0x00); fix.memory.write_byte(0x022C, 0x80);
    fix.memory.write_byte(0x022D, 0x10); fix.memory.write_byte(0x022E, 0x01); fix.memory.write_byte(0x022F, 0x80);
    fix.memory.write_byte(0x0230, 0x20); fix.memory.write_byte(0x0231, 0x00); fix.memory.write_byte(0x0232, 0x01);
    // NOT R0 (~0x00 = 0xFF)
    fix.memory.write_byte(0x0233, 0x33); fix.memory.write_byte(0x0234, 0x00);

    fix.cpu.step();
    fix.cpu.step();
    fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().c);

    auto r_not = fix.cpu.step();
    EXPECT_TRUE(r_not.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0xFF);
    EXPECT_TRUE(fix.cpu.flags().n);
    EXPECT_FALSE(fix.cpu.flags().c);
    EXPECT_FALSE(fix.cpu.flags().v);
}

TEST(CpuInstructions, CmpExecutionDoesNotModifyRegisters) {
    CpuFixture fix;
    // LDI R1, 0x40
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x01); fix.memory.write_byte(0x0202, 0x40);
    // LDI R2, 0x50
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x02); fix.memory.write_byte(0x0205, 0x50);
    // CMP R1, R2 (0x40 < 0x50 -> C=0, N=1, Z=0, registers unchanged)
    fix.memory.write_byte(0x0206, 0x40); fix.memory.write_byte(0x0207, 0x01); fix.memory.write_byte(0x0208, 0x02);

    fix.cpu.step(); // LDI R1
    fix.cpu.step(); // LDI R2

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x40); // Preserved
    EXPECT_EQ(fix.cpu.registers().r[2], 0x50); // Preserved
    EXPECT_FALSE(fix.cpu.flags().c);           // Borrow occurred (0x40 < 0x50)
    EXPECT_TRUE(fix.cpu.flags().n);
    EXPECT_FALSE(fix.cpu.flags().z);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0209);
}

// =============================================================
// 3. Branch Instructions (5 instructions)
// =============================================================

TEST(CpuInstructions, JmpUnconditional) {
    CpuFixture fix;
    // JMP 0x0500 (0x50, 0x05, 0x00)
    fix.memory.write_byte(0x0200, 0x50);
    fix.memory.write_byte(0x0201, 0x05);
    fix.memory.write_byte(0x0202, 0x00);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0500);
}

TEST(CpuInstructions, JzTakenAndNotTaken) {
    CpuFixture fix;
    // 1. Not taken when Z = 0: establish Z=0 via CMP with different values
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x01);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x02);
    fix.memory.write_byte(0x0206, 0x40); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);
    // JZ 0x0600 (not taken)
    fix.memory.write_byte(0x0209, 0x51); fix.memory.write_byte(0x020A, 0x06); fix.memory.write_byte(0x020B, 0x00);

    fix.cpu.step(); fix.cpu.step(); fix.cpu.step();
    EXPECT_FALSE(fix.cpu.flags().z);

    auto r_not_taken = fix.cpu.step();
    EXPECT_TRUE(r_not_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x020C);

    // 2. Taken when Z = 1: establish Z=1 via CMP with equal values
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x00); fix.memory.write_byte(0x0302, 0x05);
    fix.memory.write_byte(0x0303, 0x40); fix.memory.write_byte(0x0304, 0x00); fix.memory.write_byte(0x0305, 0x00);
    // JZ 0x0600 (taken)
    fix.memory.write_byte(0x0306, 0x51); fix.memory.write_byte(0x0307, 0x06); fix.memory.write_byte(0x0308, 0x00);

    fix.cpu.step(); fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().z);

    auto r_taken = fix.cpu.step();
    EXPECT_TRUE(r_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0600);
}

TEST(CpuInstructions, JnzTakenAndNotTaken) {
    CpuFixture fix;
    // 1. Not taken when Z = 1: establish Z=1
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x05);
    fix.memory.write_byte(0x0203, 0x40); fix.memory.write_byte(0x0204, 0x00); fix.memory.write_byte(0x0205, 0x00);
    // JNZ 0x0700 (not taken)
    fix.memory.write_byte(0x0206, 0x52); fix.memory.write_byte(0x0207, 0x07); fix.memory.write_byte(0x0208, 0x00);

    fix.cpu.step(); fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().z);

    auto r_not_taken = fix.cpu.step();
    EXPECT_TRUE(r_not_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0209);

    // 2. Taken when Z = 0: establish Z=0
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x00); fix.memory.write_byte(0x0302, 0x01);
    fix.memory.write_byte(0x0303, 0x10); fix.memory.write_byte(0x0304, 0x01); fix.memory.write_byte(0x0305, 0x02);
    fix.memory.write_byte(0x0306, 0x40); fix.memory.write_byte(0x0307, 0x00); fix.memory.write_byte(0x0308, 0x01);
    // JNZ 0x0700 (taken)
    fix.memory.write_byte(0x0309, 0x52); fix.memory.write_byte(0x030A, 0x07); fix.memory.write_byte(0x030B, 0x00);

    fix.cpu.step(); fix.cpu.step(); fix.cpu.step();
    EXPECT_FALSE(fix.cpu.flags().z);

    auto r_taken = fix.cpu.step();
    EXPECT_TRUE(r_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0700);
}

TEST(CpuInstructions, JcTakenAndNotTaken) {
    CpuFixture fix;
    // 1. Not taken when C = 0: clear C via AND
    fix.memory.write_byte(0x0200, 0x30); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x00);
    // JC 0x0800 (not taken)
    fix.memory.write_byte(0x0203, 0x53); fix.memory.write_byte(0x0204, 0x08); fix.memory.write_byte(0x0205, 0x00);

    fix.cpu.step();
    EXPECT_FALSE(fix.cpu.flags().c);

    auto r_not_taken = fix.cpu.step();
    EXPECT_TRUE(r_not_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0206);

    // 2. Taken when C = 1: set C via ADD 0xFF + 0x01
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x10); fix.memory.write_byte(0x0301, 0x00); fix.memory.write_byte(0x0302, 0xFF);
    fix.memory.write_byte(0x0303, 0x10); fix.memory.write_byte(0x0304, 0x01); fix.memory.write_byte(0x0305, 0x01);
    fix.memory.write_byte(0x0306, 0x20); fix.memory.write_byte(0x0307, 0x00); fix.memory.write_byte(0x0308, 0x01);
    // JC 0x0800 (taken)
    fix.memory.write_byte(0x0309, 0x53); fix.memory.write_byte(0x030A, 0x08); fix.memory.write_byte(0x030B, 0x00);

    fix.cpu.step(); fix.cpu.step(); fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().c);

    auto r_taken = fix.cpu.step();
    EXPECT_TRUE(r_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0800);
}

TEST(CpuInstructions, JncTakenAndNotTaken) {
    CpuFixture fix;
    // 1. Not taken when C = 1: set C via ADD 0xFF + 0x01
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0xFF);
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x01);
    fix.memory.write_byte(0x0206, 0x20); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);
    // JNC 0x0900 (not taken)
    fix.memory.write_byte(0x0209, 0x54); fix.memory.write_byte(0x020A, 0x09); fix.memory.write_byte(0x020B, 0x00);

    fix.cpu.step(); fix.cpu.step(); fix.cpu.step();
    EXPECT_TRUE(fix.cpu.flags().c);

    auto r_not_taken = fix.cpu.step();
    EXPECT_TRUE(r_not_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x020C);

    // 2. Taken when C = 0: clear C via AND
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x30); fix.memory.write_byte(0x0301, 0x00); fix.memory.write_byte(0x0302, 0x00);
    // JNC 0x0900 (taken)
    fix.memory.write_byte(0x0303, 0x54); fix.memory.write_byte(0x0304, 0x09); fix.memory.write_byte(0x0305, 0x00);

    fix.cpu.step();
    EXPECT_FALSE(fix.cpu.flags().c);

    auto r_taken = fix.cpu.step();
    EXPECT_TRUE(r_taken.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0900);
}

// =============================================================
// 4. Register Validation in All Applicable Instructions
// =============================================================

TEST(CpuInstructions, OutOfRangeRegisterEncodingValidation) {
    CpuFixture fix;

    // Table of opcode instructions that require register validation
    struct RegTestEntry {
        bitforge::byte_t opcode;
        bitforge::byte_t b1;
        bitforge::byte_t b2;
        bitforge::byte_t b3;
    };

    const RegTestEntry entries[] = {
        {0x10, 0x04, 0x00, 0x00}, // LDI R4, 0 (invalid dst)
        {0x11, 0x04, 0x00, 0x00}, // MOV R4, R0 (invalid dst)
        {0x11, 0x00, 0x04, 0x00}, // MOV R0, R4 (invalid src)
        {0x12, 0x04, 0x10, 0x00}, // LD R4, [0x1000] (invalid dst)
        {0x13, 0x10, 0x00, 0x04}, // ST [0x1000], R4 (invalid src)
        {0x15, 0x04, 0x00, 0x00}, // LDX R4, [IX] (invalid dst)
        {0x16, 0x04, 0x00, 0x00}, // STX [IX], R4 (invalid src)
        {0x20, 0x04, 0x00, 0x00}, // ADD R4, R0
        {0x20, 0x00, 0x04, 0x00}, // ADD R0, R4
        {0x21, 0x04, 0x00, 0x00}, // SUB R4, R0
        {0x21, 0x00, 0x04, 0x00}, // SUB R0, R4
        {0x22, 0x04, 0x00, 0x00}, // INC R4
        {0x23, 0x04, 0x00, 0x00}, // DEC R4
        {0x30, 0x04, 0x00, 0x00}, // AND R4, R0
        {0x30, 0x00, 0x04, 0x00}, // AND R0, R4
        {0x31, 0x04, 0x00, 0x00}, // OR R4, R0
        {0x31, 0x00, 0x04, 0x00}, // OR R0, R4
        {0x32, 0x04, 0x00, 0x00}, // XOR R4, R0
        {0x32, 0x00, 0x04, 0x00}, // XOR R0, R4
        {0x33, 0x04, 0x00, 0x00}, // NOT R4
        {0x40, 0x04, 0x00, 0x00}, // CMP R4, R0
        {0x40, 0x00, 0x04, 0x00}  // CMP R0, R4
    };

    for (const auto& entry : entries) {
        fix.cpu.reset(0x0200);
        fix.memory.write_byte(0x0200, entry.opcode);
        fix.memory.write_byte(0x0201, entry.b1);
        fix.memory.write_byte(0x0202, entry.b2);
        fix.memory.write_byte(0x0203, entry.b3);

        auto res = fix.cpu.step();
        EXPECT_FALSE(res.executed);
        EXPECT_TRUE(res.faulted);
        EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
        EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
        EXPECT_EQ(fix.cpu.registers().pc, 0x0200); // PC must be preserved
    }
}

// =============================================================
// 5. Invalid Opcodes (Exhaustive 225 undefined opcodes)
// =============================================================

TEST(CpuInstructions, ExhaustiveInvalidOpcodesFault) {
    CpuFixture fix;

    std::size_t invalid_count = 0;
    for (int b = 0; b <= 0xFF; ++b) {
        const auto op = static_cast<bitforge::byte_t>(b);
        if (!bitforge::is_valid_opcode(op)) {
            invalid_count++;
            fix.cpu.reset(0x0300);
            fix.memory.write_byte(0x0300, op);

            auto res = fix.cpu.step();
            EXPECT_FALSE(res.executed);
            EXPECT_TRUE(res.faulted);
            EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
            EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidOpcode);
            EXPECT_EQ(fix.cpu.registers().pc, 0x0300); // Preserved PC
        }
    }
    EXPECT_EQ(invalid_count, 225);
}

TEST(CpuInstructions, RecognizedPhase2OpcodesDoNotFaultAsInvalid) {
    CpuFixture fix;
    const bitforge::byte_t phase2_ops[] = {
        0x60, // CALL
        0x61, // RET
        0x62, // PUSH
        0x63, // POP
        0x70, // IN
        0x71  // OUT
    };

    for (const auto op : phase2_ops) {
        EXPECT_TRUE(bitforge::is_valid_opcode(op));
    }

    // 1. PUSH R0 (0x62, 0x00)
    fix.cpu.reset(0x0400);
    fix.memory.write_byte(0x0400, 0x62); fix.memory.write_byte(0x0401, 0x00);
    auto r_push = fix.cpu.step();
    EXPECT_TRUE(r_push.executed);
    EXPECT_FALSE(r_push.faulted);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFE);

    // 2. POP R1 (0x63, 0x01)
    fix.memory.write_byte(0x0402, 0x63); fix.memory.write_byte(0x0403, 0x01);
    auto r_pop = fix.cpu.step();
    EXPECT_TRUE(r_pop.executed);
    EXPECT_FALSE(r_pop.faulted);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);

    // 3. CALL 0x0500 (0x60, 0x05, 0x00)
    fix.memory.write_byte(0x0404, 0x60); fix.memory.write_byte(0x0405, 0x05); fix.memory.write_byte(0x0406, 0x00);
    auto r_call = fix.cpu.step();
    EXPECT_TRUE(r_call.executed);
    EXPECT_FALSE(r_call.faulted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0500);

    // 4. RET (0x61)
    fix.memory.write_byte(0x0500, 0x61);
    auto r_ret = fix.cpu.step();
    EXPECT_TRUE(r_ret.executed);
    EXPECT_FALSE(r_ret.faulted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0407);

    // 5. IN R2, 0x00 (0x70, 0x02, 0x00)
    fix.memory.write_byte(0x0407, 0x70); fix.memory.write_byte(0x0408, 0x02); fix.memory.write_byte(0x0409, 0x00);
    auto r_in = fix.cpu.step();
    EXPECT_TRUE(r_in.executed);
    EXPECT_FALSE(r_in.faulted);

    // 6. OUT 0x10, R2 (0x71, 0x10, 0x02)
    fix.memory.write_byte(0x040A, 0x71); fix.memory.write_byte(0x040B, 0x10); fix.memory.write_byte(0x040C, 0x02);
    auto r_out = fix.cpu.step();
    EXPECT_TRUE(r_out.executed);
    EXPECT_FALSE(r_out.faulted);
}

// =============================================================
// 6. Fault Atomicity
// =============================================================

TEST(CpuInstructions, FaultAtomicityPreservesArchitecturalState) {
    CpuFixture fix;

    // Seed registers and flags using real instructions
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x11); // LDI R0, 0x11
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x22); // LDI R1, 0x22
    fix.memory.write_byte(0x0206, 0x10); fix.memory.write_byte(0x0207, 0x02); fix.memory.write_byte(0x0208, 0x33); // LDI R2, 0x33
    fix.memory.write_byte(0x0209, 0x10); fix.memory.write_byte(0x020A, 0x03); fix.memory.write_byte(0x020B, 0x44); // LDI R3, 0x44
    fix.memory.write_byte(0x020C, 0x14); fix.memory.write_byte(0x020D, 0x55); fix.memory.write_byte(0x020E, 0x66); // LDI16 IX, 0x5566

    // Set flags Z=1, C=1, V=1 via ADD 0x80 + 0x80
    fix.memory.write_byte(0x020F, 0x10); fix.memory.write_byte(0x0210, 0x00); fix.memory.write_byte(0x0211, 0x80);
    fix.memory.write_byte(0x0212, 0x10); fix.memory.write_byte(0x0213, 0x01); fix.memory.write_byte(0x0214, 0x80);
    fix.memory.write_byte(0x0215, 0x20); fix.memory.write_byte(0x0216, 0x00); fix.memory.write_byte(0x0217, 0x01);
    // Restore R0 and R1 (LDI preserves flags)
    fix.memory.write_byte(0x0218, 0x10); fix.memory.write_byte(0x0219, 0x00); fix.memory.write_byte(0x021A, 0x11);
    fix.memory.write_byte(0x021B, 0x10); fix.memory.write_byte(0x021C, 0x01); fix.memory.write_byte(0x021D, 0x22);

    // Target memory address seeded
    fix.memory.write_byte(0x8000, 0x99);

    // Provoke a fault with an invalid register operand in ST at 0x021E
    fix.memory.write_byte(0x021E, 0x13); // ST [0x8000], R9
    fix.memory.write_byte(0x021F, 0x80);
    fix.memory.write_byte(0x0220, 0x00);
    fix.memory.write_byte(0x0221, 0x09); // Invalid register 9

    // Step through the setup instructions
    for (int i = 0; i < 10; ++i) {
        auto step_ok = fix.cpu.step();
        EXPECT_TRUE(step_ok.executed);
    }

    // Verify state before fault
    EXPECT_EQ(fix.cpu.registers().pc, 0x021E);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x11);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x22);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x33);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x44);
    EXPECT_EQ(fix.cpu.registers().ix, 0x5566);
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().c);
    EXPECT_TRUE(fix.cpu.flags().v);

    // Step the faulting instruction
    auto res = fix.cpu.step();
    EXPECT_TRUE(res.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
    EXPECT_EQ(fix.cpu.last_fault().instruction_address, 0x021E);
    EXPECT_EQ(fix.cpu.registers().pc, 0x021E); // PC must remain at instruction start

    // Verify all registers are completely preserved
    EXPECT_EQ(fix.cpu.registers().r[0], 0x11);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x22);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x33);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x44);
    EXPECT_EQ(fix.cpu.registers().ix, 0x5566);

    // Verify all flags are completely preserved
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().c);
    EXPECT_TRUE(fix.cpu.flags().v);

    // Verify memory write was never committed
    EXPECT_EQ(fix.memory.read_byte(0x8000), 0x99);

    // Verify subsequent step while faulted is a strict no-op
    auto step_again = fix.cpu.step();
    EXPECT_FALSE(step_again.executed);
    EXPECT_TRUE(step_again.faulted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x021E);
}

// =============================================================
// 7. 16-Bit Address Wraparound
// =============================================================

TEST(CpuInstructions, SequentialExecutionWrapsAcross64K) {
    CpuFixture fix;
    fix.cpu.reset(0xFFFF);
    fix.memory.write_byte(0xFFFF, 0x00); // NOP at 0xFFFF

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0000); // Wraps from 0xFFFF to 0x0000
}

TEST(CpuInstructions, MultibyteInstructionStraddling64KBoundary) {
    CpuFixture fix;
    // LDI R0, 42 occupies 3 bytes:
    // opcode at 0xFFFE, dst register at 0xFFFF, imm8 at 0x0000
    fix.cpu.reset(0xFFFE);
    fix.memory.write_byte(0xFFFE, 0x10); // LDI
    fix.memory.write_byte(0xFFFF, 0x00); // R0
    fix.memory.write_byte(0x0000, 0x2A); // 42

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 42);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0001); // 0xFFFE + 3 wrapping = 0x0001
}

TEST(CpuInstructions, BranchToWrappedAddress) {
    CpuFixture fix;
    // JMP 0x0005 placed at 0xFFFE
    fix.cpu.reset(0xFFFE);
    fix.memory.write_byte(0xFFFE, 0x50); // JMP
    fix.memory.write_byte(0xFFFF, 0x00);
    fix.memory.write_byte(0x0000, 0x05);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0005);
}

TEST(CpuInstructions, BoundaryMemoryReadsAndWrites) {
    CpuFixture fix;
    // LDI R0, 0xBC (0x10, 0x00, 0xBC)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0xBC);
    // LDI R1, 0xDE (0x10, 0x01, 0xDE)
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0xDE);
    // ST [0x0000], R0
    fix.memory.write_byte(0x0206, 0x13); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x00); fix.memory.write_byte(0x0209, 0x00);
    // ST [0xFFFF], R1
    fix.memory.write_byte(0x020A, 0x13); fix.memory.write_byte(0x020B, 0xFF); fix.memory.write_byte(0x020C, 0xFF); fix.memory.write_byte(0x020D, 0x01);
    // LD R2, [0x0000]
    fix.memory.write_byte(0x020E, 0x12); fix.memory.write_byte(0x020F, 0x02); fix.memory.write_byte(0x0210, 0x00); fix.memory.write_byte(0x0211, 0x00);
    // LD R3, [0xFFFF]
    fix.memory.write_byte(0x0212, 0x12); fix.memory.write_byte(0x0213, 0x03); fix.memory.write_byte(0x0214, 0xFF); fix.memory.write_byte(0x0215, 0xFF);

    fix.cpu.step(); // LDI R0
    fix.cpu.step(); // LDI R1
    fix.cpu.step(); // ST [0x0000]
    EXPECT_EQ(fix.memory.read_byte(0x0000), 0xBC);

    fix.cpu.step(); // ST [0xFFFF]
    EXPECT_EQ(fix.memory.read_byte(0xFFFF), 0xDE);

    fix.cpu.step(); // LD R2
    EXPECT_EQ(fix.cpu.registers().r[2], 0xBC);

    fix.cpu.step(); // LD R3
    EXPECT_EQ(fix.cpu.registers().r[3], 0xDE);
}

// =============================================================
// 8. Machine State Transitions
// =============================================================

TEST(CpuInstructions, StateTransitionsFromReadyAndRunning) {
    CpuFixture fix;
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Ready);

    // 1. First instruction from Ready transitions to Running
    fix.memory.write_byte(0x0200, 0x00); // NOP
    auto r1 = fix.cpu.step();
    EXPECT_TRUE(r1.executed);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Running);

    // 2. Continuous instruction in Running remains Running
    fix.memory.write_byte(0x0201, 0x00); // NOP
    auto r2 = fix.cpu.step();
    EXPECT_TRUE(r2.executed);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Running);

    // 3. HALT instruction transitions Running to Halted
    fix.memory.write_byte(0x0202, 0x01); // HALT
    auto r3 = fix.cpu.step();
    EXPECT_TRUE(r3.executed);
    EXPECT_TRUE(r3.halted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);

    // 4. Reset returns state to Ready
    fix.cpu.reset(0x0300);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Ready);

    // 5. HALT directly from Ready transitions directly to Halted
    fix.memory.write_byte(0x0300, 0x01); // HALT
    auto r4 = fix.cpu.step();
    EXPECT_TRUE(r4.executed);
    EXPECT_TRUE(r4.halted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
}

// =============================================================
// 9. Raw-Byte Program Execution Conformance (Independent of Enums)
// =============================================================

TEST(CpuInstructions, RawByteProgramExecutionConformance) {
    CpuFixture fix;

    // Explicit raw byte program array with zero Opcode enum references
    // Verifies canonical opcode byte values directly from architectural specification v1.7:
    // NOP=0x00, HALT=0x01, LDI=0x10, MOV=0x11, LD=0x12, ST=0x13, LDI16=0x14,
    // LDX=0x15, STX=0x16, INX=0x17, DEX=0x18, ADD=0x20, SUB=0x21, INC=0x22,
    // DEC=0x23, CMP=0x40, JZ=0x51
    const std::vector<bitforge::byte_t> raw_program = {
        0x10, 0x00, 0x2A,       // LDI R0, 42
        0x10, 0x01, 0x10,       // LDI R1, 16
        0x20, 0x00, 0x01,       // ADD R0, R1     (42 + 16 = 58)
        0x10, 0x02, 0x08,       // LDI R2, 8
        0x21, 0x00, 0x02,       // SUB R0, R2     (58 - 8 = 50)
        0x14, 0x80, 0x00,       // LDI16 IX, 0x8000
        0x16, 0x00,             // STX [IX], R0   (store 50 at 0x8000)
        0x17,                   // INX            (IX becomes 0x8001)
        0x18,                   // DEX            (IX returns to 0x8000)
        0x15, 0x03,             // LDX R3, [IX]   (load 50 into R3)
        0x11, 0x01, 0x03,       // MOV R1, R3     (R1 = 50)
        0x13, 0x80, 0x10, 0x01, // ST [0x8010], R1 (store 50 at 0x8010)
        0x12, 0x02, 0x80, 0x10, // LD R2, [0x8010] (load 50 into R2)
        0x22, 0x02,             // INC R2         (51)
        0x23, 0x02,             // DEC R2         (50)
        0x40, 0x02, 0x03,       // CMP R2, R3     (50 == 50 -> Z=1, C=1)
        0x51, 0x02, 0x2E,       // JZ 0x022E      (jump to HALT at 0x022E)
        0x00,                   // NOP            (skipped by taken branch)
        0x01                    // HALT at 0x022E
    };

    fix.memory.load_segment(0x0200, raw_program.data(), raw_program.size());
    fix.cpu.reset(0x0200);

    auto result = bitforge::test::run_with_budget(fix.cpu, 100);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stop_reason, "Halted");
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(fix.cpu.registers().r[0], 50);
    EXPECT_EQ(fix.cpu.registers().r[1], 50);
    EXPECT_EQ(fix.cpu.registers().r[2], 50);
    EXPECT_EQ(fix.cpu.registers().r[3], 50);
    EXPECT_EQ(fix.cpu.registers().ix, 0x8000);
    EXPECT_EQ(fix.memory.read_byte(0x8000), 50);
    EXPECT_EQ(fix.memory.read_byte(0x8010), 50);
    EXPECT_TRUE(fix.cpu.flags().z);
    EXPECT_TRUE(fix.cpu.flags().c);
}

// =============================================================
// 10. Phase 2: Four-Byte Instruction 64 KiB Boundary Wraparound
// =============================================================

TEST(CpuInstructions, FourByteInstruction_LdStraddling64K_FFFD) {
    CpuFixture fix;
    // LD R1, [0x8050] occupies 4 bytes:
    // 0xFFFD: opcode 0x12 (LD)
    // 0xFFFE: dst reg 0x01 (R1)
    // 0xFFFF: addr_hi 0x80
    // 0x0000: addr_lo 0x50 (wraps across 64 KiB boundary)
    fix.memory.write_byte(0xFFFD, 0x12);
    fix.memory.write_byte(0xFFFE, 0x01);
    fix.memory.write_byte(0xFFFF, 0x80);
    fix.memory.write_byte(0x0000, 0x50);
    fix.memory.write_byte(0x8050, 0x42);

    fix.cpu.reset(0xFFFD);
    auto res = fix.cpu.step();

    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x42);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0001); // 0xFFFD + 4 mod 65536 = 0x0001
}

TEST(CpuInstructions, FourByteInstruction_LdStraddling64K_FFFE) {
    CpuFixture fix;
    // LD R2, [0x8055] occupies 4 bytes:
    // 0xFFFE: opcode 0x12 (LD)
    // 0xFFFF: dst reg 0x02 (R2)
    // 0x0000: addr_hi 0x80 (wraps)
    // 0x0001: addr_lo 0x55 (wraps)
    fix.memory.write_byte(0xFFFE, 0x12);
    fix.memory.write_byte(0xFFFF, 0x02);
    fix.memory.write_byte(0x0000, 0x80);
    fix.memory.write_byte(0x0001, 0x55);
    fix.memory.write_byte(0x8055, 0x77);

    fix.cpu.reset(0xFFFE);
    auto res = fix.cpu.step();

    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x77);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0002); // 0xFFFE + 4 mod 65536 = 0x0002
}

TEST(CpuInstructions, FourByteInstruction_LdStraddling64K_FFFF) {
    CpuFixture fix;
    // LD R3, [0x8070] occupies 4 bytes:
    // 0xFFFF: opcode 0x12 (LD)
    // 0x0000: dst reg 0x03 (R3) (wraps)
    // 0x0001: addr_hi 0x80 (wraps)
    // 0x0002: addr_lo 0x70 (wraps)
    fix.memory.write_byte(0xFFFF, 0x12);
    fix.memory.write_byte(0x0000, 0x03);
    fix.memory.write_byte(0x0001, 0x80);
    fix.memory.write_byte(0x0002, 0x70);
    fix.memory.write_byte(0x8070, 0x88);

    fix.cpu.reset(0xFFFF);
    auto res = fix.cpu.step();

    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x88);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0003); // 0xFFFF + 4 mod 65536 = 0x0003
}

TEST(CpuInstructions, FourByteInstruction_StStraddling64K_FFFD) {
    CpuFixture fix;
    // Pre-load R2 with 0x99 via LDI at 0xFFFA..0xFFFC
    fix.memory.write_byte(0xFFFA, 0x10); // LDI
    fix.memory.write_byte(0xFFFB, 0x02); // R2
    fix.memory.write_byte(0xFFFC, 0x99); // imm 0x99

    // ST [0x8060], R2 occupies 4 bytes:
    // 0xFFFD: opcode 0x13 (ST)
    // 0xFFFE: addr_hi 0x80
    // 0xFFFF: addr_lo 0x60
    // 0x0000: src reg 0x02 (R2) (wraps)
    fix.memory.write_byte(0xFFFD, 0x13);
    fix.memory.write_byte(0xFFFE, 0x80);
    fix.memory.write_byte(0xFFFF, 0x60);
    fix.memory.write_byte(0x0000, 0x02);

    fix.cpu.reset(0xFFFA);
    fix.cpu.step(); // Execute LDI, PC reaches 0xFFFD, R2 = 0x99

    auto res = fix.cpu.step(); // Execute ST across boundary
    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.memory.read_byte(0x8060), 0x99);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0001); // 0xFFFD + 4 mod 65536 = 0x0001
}

TEST(CpuInstructions, FourByteInstruction_StStraddling64K_FFFE) {
    CpuFixture fix;
    // Pre-load R3 with 0xAA via LDI at 0xFFFB..0xFFFD
    fix.memory.write_byte(0xFFFB, 0x10); // LDI
    fix.memory.write_byte(0xFFFC, 0x03); // R3
    fix.memory.write_byte(0xFFFD, 0xAA); // imm 0xAA

    // ST [0x8065], R3 occupies 4 bytes:
    // 0xFFFE: opcode 0x13 (ST)
    // 0xFFFF: addr_hi 0x80
    // 0x0000: addr_lo 0x65 (wraps)
    // 0x0001: src reg 0x03 (R3) (wraps)
    fix.memory.write_byte(0xFFFE, 0x13);
    fix.memory.write_byte(0xFFFF, 0x80);
    fix.memory.write_byte(0x0000, 0x65);
    fix.memory.write_byte(0x0001, 0x03);

    fix.cpu.reset(0xFFFB);
    fix.cpu.step(); // Execute LDI, PC reaches 0xFFFE, R3 = 0xAA

    auto res = fix.cpu.step(); // Execute ST across boundary
    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.memory.read_byte(0x8065), 0xAA);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0002); // 0xFFFE + 4 mod 65536 = 0x0002
}

TEST(CpuInstructions, FourByteInstruction_StStraddling64K_FFFF) {
    CpuFixture fix;
    // Pre-load R1 with 0x55 via LDI at 0xFFFC..0xFFFE
    fix.memory.write_byte(0xFFFC, 0x10); // LDI
    fix.memory.write_byte(0xFFFD, 0x01); // R1
    fix.memory.write_byte(0xFFFE, 0x55); // imm 0x55

    // ST [0x8075], R1 occupies 4 bytes:
    // 0xFFFF: opcode 0x13 (ST)
    // 0x0000: addr_hi 0x80 (wraps)
    // 0x0001: addr_lo 0x75 (wraps)
    // 0x0002: src reg 0x01 (R1) (wraps)
    fix.memory.write_byte(0xFFFF, 0x13);
    fix.memory.write_byte(0x0000, 0x80);
    fix.memory.write_byte(0x0001, 0x75);
    fix.memory.write_byte(0x0002, 0x01);

    fix.cpu.reset(0xFFFC);
    fix.cpu.step(); // Execute LDI, PC reaches 0xFFFF, R1 = 0x55

    auto res = fix.cpu.step(); // Execute ST across boundary
    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.memory.read_byte(0x8075), 0x55);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0003); // 0xFFFF + 4 mod 65536 = 0x0003
}

TEST(CpuInstructions, FourByteInstruction_TargetAddressAt64KBoundary) {
    CpuFixture fix;
    // LD R0, [0xFFFF]
    fix.memory.write_byte(0x0200, 0x12); // LD
    fix.memory.write_byte(0x0201, 0x00); // R0
    fix.memory.write_byte(0x0202, 0xFF); // addr_hi
    fix.memory.write_byte(0x0203, 0xFF); // addr_lo
    fix.memory.write_byte(0xFFFF, 0x3E);

    fix.cpu.reset(0x0200);
    auto res_ld = fix.cpu.step();
    EXPECT_TRUE(res_ld.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x3E);

    // ST [0xFFFF], R1 (modifies byte at 0xFFFF)
    fix.memory.write_byte(0x0204, 0x10); // LDI R1, 0x7B
    fix.memory.write_byte(0x0205, 0x01);
    fix.memory.write_byte(0x0206, 0x7B);
    fix.memory.write_byte(0x0207, 0x13); // ST [0xFFFF], R1
    fix.memory.write_byte(0x0208, 0xFF);
    fix.memory.write_byte(0x0209, 0xFF);
    fix.memory.write_byte(0x020A, 0x01);

    fix.cpu.step(); // LDI R1, 0x7B
    auto res_st = fix.cpu.step(); // ST
    EXPECT_TRUE(res_st.executed);
    EXPECT_EQ(fix.memory.read_byte(0xFFFF), 0x7B);
}

// =============================================================
// 11. Phase 2: Boundary Matrix on CPU Arithmetic Instructions
// =============================================================

TEST(CpuInstructions, ArithmeticInstructionBoundaryMatrix) {
    const bitforge::byte_t boundaries[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};

    for (const auto val_a : boundaries) {
        for (const auto val_b : boundaries) {
            CpuFixture fix;
            // 0x0200: LDI R0, val_a
            // 0x0203: LDI R1, val_b
            // 0x0206: ADD R0, R1
            fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, val_a);
            fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, val_b);
            fix.memory.write_byte(0x0206, 0x20); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 0x01);

            fix.cpu.reset(0x0200);
            fix.cpu.step(); // LDI R0
            fix.cpu.step(); // LDI R1
            auto res_add = fix.cpu.step(); // ADD

            const auto expected_add = bitforge::Alu::add(val_a, val_b);
            EXPECT_TRUE(res_add.executed);
            EXPECT_EQ(fix.cpu.registers().r[0], expected_add.result);
            EXPECT_EQ(fix.cpu.flags(), expected_add.flags);
        }
    }
}

// =============================================================
// 12. Phase 2: Register & Flag Isolation / Unaffected Integrity
// =============================================================

TEST(CpuInstructions, UnaffectedRegistersAndFlagsIntegrity) {
    CpuFixture fix;
    // Set R1, R2, R3, IX to non-zero known states
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x01); fix.memory.write_byte(0x0202, 0x11); // LDI R1, 0x11
    fix.memory.write_byte(0x0203, 0x10); fix.memory.write_byte(0x0204, 0x02); fix.memory.write_byte(0x0205, 0x22); // LDI R2, 0x22
    fix.memory.write_byte(0x0206, 0x10); fix.memory.write_byte(0x0207, 0x03); fix.memory.write_byte(0x0208, 0x33); // LDI R3, 0x33
    fix.memory.write_byte(0x0209, 0x14); fix.memory.write_byte(0x020A, 0x12); fix.memory.write_byte(0x020B, 0x34); // LDI16 IX, 0x1234
    // Set known flag state via SUB R1, R1 (Z=1, C=1, N=0, V=0)
    fix.memory.write_byte(0x020C, 0x21); fix.memory.write_byte(0x020D, 0x01); fix.memory.write_byte(0x020E, 0x01);
    // INX (must NOT affect R1, R2, R3, SP, or ANY flags)
    fix.memory.write_byte(0x020F, 0x17);
    // DEX (must NOT affect R1, R2, R3, SP, or ANY flags)
    fix.memory.write_byte(0x0210, 0x18);
    // JMP 0x0215 (must NOT affect ANY registers or flags)
    fix.memory.write_byte(0x0211, 0x50); fix.memory.write_byte(0x0212, 0x02); fix.memory.write_byte(0x0213, 0x15);
    fix.memory.write_byte(0x0214, 0x00); // NOP (skipped)
    // At 0x0215: HALT
    fix.memory.write_byte(0x0215, 0x01);

    fix.cpu.reset(0x0200);
    // Step through LDI R1, R2, R3, LDI16 IX, SUB
    for (int i = 0; i < 5; ++i) {
        fix.cpu.step();
    }
    const auto baseline_flags = fix.cpu.flags();
    EXPECT_TRUE(baseline_flags.z);
    EXPECT_TRUE(baseline_flags.c);

    // Step INX
    fix.cpu.step();
    EXPECT_EQ(fix.cpu.registers().ix, 0x1235);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x22);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x33);
    EXPECT_EQ(fix.cpu.flags(), baseline_flags);

    // Step DEX
    fix.cpu.step();
    EXPECT_EQ(fix.cpu.registers().ix, 0x1234);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x22);
    EXPECT_EQ(fix.cpu.registers().r[3], 0x33);
    EXPECT_EQ(fix.cpu.flags(), baseline_flags);

    // Step JMP
    fix.cpu.step();
    EXPECT_EQ(fix.cpu.registers().pc, 0x0215);
    EXPECT_EQ(fix.cpu.registers().ix, 0x1234);
    EXPECT_EQ(fix.cpu.flags(), baseline_flags);
}

// =============================================================
// 13. Phase 2: Comprehensive Register Encoding Fault Atomicity
// =============================================================

TEST(CpuInstructions, ExhaustiveRegisterEncodingValidationAndAtomicity) {
    // Check all instructions taking register operands
    // (both 1-reg and 2-reg instructions)
    struct RegTestCase {
        bitforge::byte_t opcode;
        std::vector<bitforge::byte_t> operands;
        const char* name;
    };

    const RegTestCase cases[] = {
        {0x10, {0x04, 0x42}, "LDI reg=4"},
        {0x11, {0x04, 0x00}, "MOV dst=4"},
        {0x11, {0x00, 0x04}, "MOV src=4"},
        {0x12, {0x04, 0x80, 0x00}, "LD reg=4"},
        {0x13, {0x80, 0x00, 0x04}, "ST reg=4"},
        {0x15, {0x04}, "LDX reg=4"},
        {0x16, {0x04}, "STX reg=4"},
        {0x20, {0x04, 0x00}, "ADD dst=4"},
        {0x20, {0x00, 0x04}, "ADD src=4"},
        {0x21, {0x04, 0x00}, "SUB dst=4"},
        {0x21, {0x00, 0x04}, "SUB src=4"},
        {0x22, {0x04}, "INC reg=4"},
        {0x23, {0x04}, "DEC reg=4"},
        {0x30, {0x04, 0x00}, "AND dst=4"},
        {0x30, {0x00, 0x04}, "AND src=4"},
        {0x31, {0x04, 0x00}, "OR dst=4"},
        {0x31, {0x00, 0x04}, "OR src=4"},
        {0x32, {0x04, 0x00}, "XOR dst=4"},
        {0x32, {0x00, 0x04}, "XOR src=4"},
        {0x33, {0x04}, "NOT reg=4"},
        {0x40, {0x04, 0x00}, "CMP left=4"},
        {0x40, {0x00, 0x04}, "CMP right=4"},
        {0x62, {0x04}, "PUSH reg=4"},
        {0x63, {0x04}, "POP reg=4"},
        {0x70, {0x04, 0x00}, "IN reg=4"},
        {0x71, {0x01, 0x04}, "OUT reg=4"}
    };

    for (const auto& tc : cases) {
        CpuFixture fix;
        fix.cpu.reset(0x0200);

        fix.memory.write_byte(0x0200, tc.opcode);
        for (std::size_t i = 0; i < tc.operands.size(); ++i) {
            fix.memory.write_byte(static_cast<bitforge::word_t>(0x0201 + i), tc.operands[i]);
        }

        const auto pre_regs = fix.cpu.registers();
        const auto pre_flags = fix.cpu.flags();

        auto res = fix.cpu.step();
        EXPECT_FALSE(res.executed);
        EXPECT_TRUE(res.faulted);
        EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
        EXPECT_EQ(res.instruction_address, 0x0200);
        EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
        // Universal PC preservation rule: PC remains at instruction start address
        EXPECT_EQ(fix.cpu.registers().pc, 0x0200);
        // State atomicity: zero register or flag mutations
        EXPECT_EQ(fix.cpu.registers().r[0], pre_regs.r[0]);
        EXPECT_EQ(fix.cpu.registers().r[1], pre_regs.r[1]);
        EXPECT_EQ(fix.cpu.registers().r[2], pre_regs.r[2]);
        EXPECT_EQ(fix.cpu.registers().r[3], pre_regs.r[3]);
        EXPECT_EQ(fix.cpu.registers().ix, pre_regs.ix);
        EXPECT_EQ(fix.cpu.registers().sp, pre_regs.sp);
        EXPECT_EQ(fix.cpu.flags(), pre_flags);
    }
}

// =============================================================
// 14. Phase 2: Comprehensive Invalid Opcode Fault Atomicity
// =============================================================

TEST(CpuInstructions, InvalidOpcodeFaultAtomicityComprehensive) {
    const bitforge::byte_t invalid_opcodes[] = {
        0x02, 0x0F, 0x19, 0x1A, 0x24, 0x2F, 0x34, 0x3F,
        0x41, 0x4E, 0x55, 0x5E, 0x64, 0x6F, 0x72, 0xFE, 0xFF
    };

    for (const auto bad_op : invalid_opcodes) {
        CpuFixture fix;
        fix.cpu.reset(0x0400);
        fix.memory.write_byte(0x0400, bad_op);

        const auto pre_regs = fix.cpu.registers();
        const auto pre_flags = fix.cpu.flags();

        auto res = fix.cpu.step();
        EXPECT_FALSE(res.executed);
        EXPECT_TRUE(res.faulted);
        EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidOpcode);
        EXPECT_EQ(res.instruction_address, 0x0400);
        EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
        // Universal PC preservation rule
        EXPECT_EQ(fix.cpu.registers().pc, 0x0400);
        // State atomicity
        EXPECT_EQ(fix.cpu.registers().r[0], pre_regs.r[0]);
        EXPECT_EQ(fix.cpu.registers().sp, pre_regs.sp);
        EXPECT_EQ(fix.cpu.flags(), pre_flags);
    }
}

// =============================================================
// 15. Phase 2: Self-Modifying Code Observability
// =============================================================

TEST(CpuInstructions, SelfModifyingCode_OverwritesSubsequentInstruction) {
    CpuFixture fix;
    // Program:
    // 0x0200: LDI R0, 0x01            (HALT opcode = 0x01)
    // 0x0203: ST [0x0207], R0         (dynamically overwrites byte at 0x0207 with HALT)
    // 0x0207: NOP                     (initially 0x00, will be overwritten with 0x01)
    // 0x0208: NOP                     (should never execute if HALT took effect)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x01);
    fix.memory.write_byte(0x0203, 0x13); fix.memory.write_byte(0x0204, 0x02); fix.memory.write_byte(0x0205, 0x07); fix.memory.write_byte(0x0206, 0x00);
    fix.memory.write_byte(0x0207, 0x00); // NOP initially
    fix.memory.write_byte(0x0208, 0x00); // NOP

    fix.cpu.reset(0x0200);

    // Step 1: LDI R0, 0x01
    auto r1 = fix.cpu.step();
    EXPECT_TRUE(r1.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0203);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x01);

    // Step 2: ST [0x0207], R0 -> overwrites memory location 0x0207
    auto r2 = fix.cpu.step();
    EXPECT_TRUE(r2.executed);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0207);
    EXPECT_EQ(fix.memory.read_byte(0x0207), 0x01); // Now contains HALT opcode!

    // Step 3: Fetch at 0x0207 must read the freshly modified byte (HALT)
    auto r3 = fix.cpu.step();
    EXPECT_TRUE(r3.executed);
    EXPECT_TRUE(r3.halted);
    EXPECT_EQ(r3.opcode, bitforge::Opcode::HALT);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0208); // Advanced past HALT
}

// =============================================================
// 16. Phase 2: CPU State Transitions & Reset Baseline Contract
// =============================================================

TEST(CpuInstructions, ResetFromFaultedAndHaltedRestoresDocumentedBaseline) {
    CpuFixture fix;

    // 1. Induce a fault
    fix.cpu.reset(0x0200);
    fix.memory.write_byte(0x0200, 0xFF); // Invalid opcode
    fix.cpu.step();
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);

    // Stepping from Faulted must be a no-op
    auto noop_res = fix.cpu.step();
    EXPECT_FALSE(noop_res.executed);
    EXPECT_TRUE(noop_res.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);

    // Reset restores to Ready and clears fault
    fix.cpu.reset(0x0300);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Ready);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0300);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(fix.cpu.registers().r[0], 0);
    EXPECT_EQ(fix.cpu.registers().r[1], 0);
    EXPECT_EQ(fix.cpu.registers().r[2], 0);
    EXPECT_EQ(fix.cpu.registers().r[3], 0);
    EXPECT_EQ(fix.cpu.registers().ix, 0);
    EXPECT_FALSE(fix.cpu.flags().z);
    EXPECT_FALSE(fix.cpu.flags().c);
    EXPECT_FALSE(fix.cpu.flags().n);
    EXPECT_FALSE(fix.cpu.flags().v);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::None);

    // 2. Transition to Halted
    fix.memory.write_byte(0x0300, 0x01); // HALT
    fix.cpu.step();
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);

    // Stepping from Halted must be a no-op
    auto noop_halt = fix.cpu.step();
    EXPECT_FALSE(noop_halt.executed);
    EXPECT_TRUE(noop_halt.halted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);

    // Reset again
    fix.cpu.reset(0x0000);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Ready);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0000);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF);
}
