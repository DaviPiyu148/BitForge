#include "test_harness.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/constants.hpp"

#include <vector>
#include <string>

// =============================================================
// Helper Fixture for I/O Instruction Tests
// =============================================================
struct IoFixture {
    bitforge::Memory memory;
    bitforge::SystemBus bus;
    bitforge::Cpu cpu;

    IoFixture()
        : memory(), bus(memory), cpu(bus) {
        cpu.reset(0x0200);
    }
};

// =============================================================
// 1. Input FIFO Operations (Port 0x00)
// =============================================================

TEST(IoTest, FifoInputSequentialConsumption) {
    IoFixture fix;
    // Preload FIFO with bytes: 0x12, 0x34, 0x56
    fix.bus.push_input_fifo({0x12, 0x34, 0x56});
    EXPECT_EQ(fix.bus.input_fifo_size(), 3);
    EXPECT_FALSE(fix.bus.has_input_underflow());

    // 0x0200: IN R0, 0x00 (0x70, 0x00, 0x00)
    // 0x0203: IN R1, 0x00 (0x70, 0x01, 0x00)
    // 0x0206: IN R2, 0x00 (0x70, 0x02, 0x00)
    fix.memory.write_byte(0x0200, 0x70); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0x00);
    fix.memory.write_byte(0x0203, 0x70); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x00);
    fix.memory.write_byte(0x0206, 0x70); fix.memory.write_byte(0x0207, 0x02); fix.memory.write_byte(0x0208, 0x00);

    auto r1 = fix.cpu.step();
    EXPECT_TRUE(r1.executed);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x12);
    EXPECT_EQ(fix.bus.input_fifo_size(), 2);

    auto r2 = fix.cpu.step();
    EXPECT_TRUE(r2.executed);
    EXPECT_EQ(fix.cpu.registers().r[1], 0x34);
    EXPECT_EQ(fix.bus.input_fifo_size(), 1);

    auto r3 = fix.cpu.step();
    EXPECT_TRUE(r3.executed);
    EXPECT_EQ(fix.cpu.registers().r[2], 0x56);
    EXPECT_EQ(fix.bus.input_fifo_size(), 0);
}

TEST(IoTest, EmptyFifoUnderflowDoesNotFault) {
    IoFixture fix;
    EXPECT_EQ(fix.bus.input_fifo_size(), 0);
    EXPECT_FALSE(fix.bus.has_input_underflow());

    // IN R0, 0x00 on empty FIFO
    fix.memory.write_byte(0x0200, 0x70);
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x00);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.executed);
    EXPECT_FALSE(res.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Running);
    EXPECT_EQ(fix.cpu.registers().r[0], 0x00); // Returns 0x00 on underflow
    EXPECT_TRUE(fix.bus.has_input_underflow());
    EXPECT_EQ(fix.bus.input_underflow_count(), 1);
}

TEST(IoTest, PermissionQueryIsSideEffectFree) {
    IoFixture fix;
    fix.bus.push_input_fifo(0x77);
    EXPECT_EQ(fix.bus.input_fifo_size(), 1);

    // Permission queries must be strictly const noexcept and must not consume data
    EXPECT_TRUE(fix.bus.can_read_io(0x00));
    EXPECT_FALSE(fix.bus.can_write_io(0x00));
    EXPECT_EQ(fix.bus.input_fifo_size(), 1);
    EXPECT_FALSE(fix.bus.has_input_underflow());
}

// =============================================================
// 2. Text Console Output (Port 0x01)
// =============================================================

TEST(IoTest, TextConsolePrintableCharacters) {
    IoFixture fix;
    // Print "Hi!" via OUT 0x01, Rs
    // LDI R0, 'H'; OUT 0x01, R0; LDI R0, 'i'; OUT 0x01, R0; LDI R0, '!'; OUT 0x01, R0;
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 'H');
    fix.memory.write_byte(0x0203, 0x71); fix.memory.write_byte(0x0204, 0x01); fix.memory.write_byte(0x0205, 0x00);
    fix.memory.write_byte(0x0206, 0x10); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 'i');
    fix.memory.write_byte(0x0209, 0x71); fix.memory.write_byte(0x020A, 0x01); fix.memory.write_byte(0x020B, 0x00);
    fix.memory.write_byte(0x020C, 0x10); fix.memory.write_byte(0x020D, 0x00); fix.memory.write_byte(0x020E, '!');
    fix.memory.write_byte(0x020F, 0x71); fix.memory.write_byte(0x0210, 0x01); fix.memory.write_byte(0x0211, 0x00);

    for (int i = 0; i < 6; ++i) {
        auto r = fix.cpu.step();
        EXPECT_TRUE(r.executed);
    }

    EXPECT_EQ(fix.bus.text_output(), "Hi!");
    EXPECT_EQ(fix.bus.text_raw_bytes().size(), 3);
    EXPECT_EQ(fix.bus.text_raw_bytes()[0], 'H');
    EXPECT_EQ(fix.bus.text_raw_bytes()[1], 'i');
    EXPECT_EQ(fix.bus.text_raw_bytes()[2], '!');
}

TEST(IoTest, TextConsoleNonPrintableEscapes) {
    IoFixture fix;
    // Output non-printable bytes: 0x00, 0x07 (bell), 0x1B (esc), 0xFF
    fix.bus.write_io(0x01, 0x00);
    fix.bus.write_io(0x01, 0x07);
    fix.bus.write_io(0x01, 0x1B);
    fix.bus.write_io(0x01, 0xFF);

    EXPECT_EQ(fix.bus.text_output(), "\\x00\\x07\\x1B\\xFF");
    EXPECT_EQ(fix.bus.text_raw_bytes().size(), 4);
}

// =============================================================
// 3. Numeric Console Stream (Port 0x02)
// =============================================================

TEST(IoTest, NumericConsoleUnsignedDecimalOutput) {
    IoFixture fix;
    // LDI R0, 0; OUT 0x02, R0; LDI R0, 42; OUT 0x02, R0; LDI R0, 255; OUT 0x02, R0
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0);
    fix.memory.write_byte(0x0203, 0x71); fix.memory.write_byte(0x0204, 0x02); fix.memory.write_byte(0x0205, 0x00);
    fix.memory.write_byte(0x0206, 0x10); fix.memory.write_byte(0x0207, 0x00); fix.memory.write_byte(0x0208, 42);
    fix.memory.write_byte(0x0209, 0x71); fix.memory.write_byte(0x020A, 0x02); fix.memory.write_byte(0x020B, 0x00);
    fix.memory.write_byte(0x020C, 0x10); fix.memory.write_byte(0x020D, 0x00); fix.memory.write_byte(0x020E, 255);
    fix.memory.write_byte(0x020F, 0x71); fix.memory.write_byte(0x0210, 0x02); fix.memory.write_byte(0x0211, 0x00);

    for (int i = 0; i < 6; ++i) {
        auto r = fix.cpu.step();
        EXPECT_TRUE(r.executed);
    }

    EXPECT_EQ(fix.bus.numeric_values().size(), 3);
    EXPECT_EQ(fix.bus.numeric_values()[0], 0);
    EXPECT_EQ(fix.bus.numeric_values()[1], 42);
    EXPECT_EQ(fix.bus.numeric_values()[2], 255);
    EXPECT_EQ(fix.bus.numeric_output(), "0 42 255");
}

// =============================================================
// 4. Virtual LEDs (Port 0x10)
// =============================================================

TEST(IoTest, VirtualLedWriteAndRead) {
    IoFixture fix;
    EXPECT_EQ(fix.bus.led_state(), 0x00);

    // LDI R0, 0xA5 (0x10, 0x00, 0xA5)
    // OUT 0x10, R0 (0x71, 0x10, 0x00)
    fix.memory.write_byte(0x0200, 0x10); fix.memory.write_byte(0x0201, 0x00); fix.memory.write_byte(0x0202, 0xA5);
    fix.memory.write_byte(0x0203, 0x71); fix.memory.write_byte(0x0204, 0x10); fix.memory.write_byte(0x0205, 0x00);

    fix.cpu.step(); // LDI R0
    auto r_out = fix.cpu.step(); // OUT 0x10, R0
    EXPECT_TRUE(r_out.executed);
    EXPECT_EQ(fix.bus.led_state(), 0xA5);
}

// =============================================================
// 5. Wrong-Direction and Unmapped Port Faults
// =============================================================

TEST(IoTest, WrongDirectionPortFaults) {
    IoFixture fix;

    // 1. OUT 0x00, R0 (writing to input-only FIFO must fault with InvalidPort)
    fix.memory.write_byte(0x0200, 0x71);
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x00);

    auto r1 = fix.cpu.step();
    EXPECT_FALSE(r1.executed);
    EXPECT_TRUE(r1.faulted);
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);

    // 2. IN R0, 0x01 (reading from output-only text console must fault)
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x70);
    fix.memory.write_byte(0x0301, 0x00);
    fix.memory.write_byte(0x0302, 0x01);

    auto r2 = fix.cpu.step();
    EXPECT_FALSE(r2.executed);
    EXPECT_TRUE(r2.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0300);

    // 3. IN R0, 0x02 (reading from output-only numeric console must fault)
    fix.cpu.reset(0x0400);
    fix.memory.write_byte(0x0400, 0x70);
    fix.memory.write_byte(0x0401, 0x00);
    fix.memory.write_byte(0x0402, 0x02);

    auto r3 = fix.cpu.step();
    EXPECT_FALSE(r3.executed);
    EXPECT_TRUE(r3.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);

    // 4. IN R0, 0x10 (reading from output-only virtual LED register must fault)
    fix.cpu.reset(0x0500);
    fix.memory.write_byte(0x0500, 0x70);
    fix.memory.write_byte(0x0501, 0x00);
    fix.memory.write_byte(0x0502, 0x10);

    auto r4 = fix.cpu.step();
    EXPECT_FALSE(r4.executed);
    EXPECT_TRUE(r4.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);
}

TEST(IoTest, UnmappedPortFaults) {
    IoFixture fix;

    // IN R0, 0x05 (unmapped port)
    fix.memory.write_byte(0x0200, 0x70);
    fix.memory.write_byte(0x0201, 0x00);
    fix.memory.write_byte(0x0202, 0x05);

    auto r1 = fix.cpu.step();
    EXPECT_FALSE(r1.executed);
    EXPECT_TRUE(r1.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);

    // OUT 0x05, R0 (unmapped port)
    fix.cpu.reset(0x0300);
    fix.memory.write_byte(0x0300, 0x71);
    fix.memory.write_byte(0x0301, 0x05);
    fix.memory.write_byte(0x0302, 0x00);

    auto r2 = fix.cpu.step();
    EXPECT_FALSE(r2.executed);
    EXPECT_TRUE(r2.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidPort);
}

// =============================================================
// 6. Fault Atomicity
// =============================================================

TEST(IoTest, FaultAtomicityPreservesDeviceState) {
    IoFixture fix;
    fix.bus.push_input_fifo(0x42);
    fix.bus.write_io(0x10, 0x55); // LED set to 0x55

    // IN R9, 0x00: invalid register R9 on valid port 0x00
    // Must fault with InvalidRegisterEncoding and NEVER consume from FIFO
    fix.memory.write_byte(0x0200, 0x70);
    fix.memory.write_byte(0x0201, 0x09); // Invalid register 9
    fix.memory.write_byte(0x0202, 0x00);

    auto res = fix.cpu.step();
    EXPECT_TRUE(res.faulted);
    EXPECT_EQ(fix.cpu.last_fault().type, bitforge::FaultType::InvalidRegisterEncoding);
    EXPECT_EQ(fix.cpu.registers().pc, 0x0200);

    // FIFO must still have its original byte untouched!
    EXPECT_EQ(fix.bus.input_fifo_size(), 1);
    EXPECT_EQ(fix.bus.led_state(), 0x55);
}

// =============================================================
// 7. End-to-End Program with Literal Encoded Bytes
// =============================================================

TEST(IoTest, EndToEndProgramCoveringPhase2Instructions) {
    IoFixture fix;
    // Preload FIFO with byte 10
    fix.bus.push_input_fifo(10);

    // Raw-byte program demonstrating:
    // 1. IN R0, 0x00     (reads 10 from FIFO)
    // 2. PUSH R0         (push 10 to stack)
    // 3. CALL 0x0211     (subroutine double_value)
    // 4. OUT 0x02, R0    (output result 20 to numeric console)
    // 5. OUT 0x10, R0    (set LEDs to 20)
    // 6. HALT
    // Subroutine double_value at 0x0211:
    // 7. POP R1          (pop 10 from stack)
    // 8. ADD R1, R1      (10 + 10 = 20)
    // 9. MOV R0, R1      (R0 = 20)
    // 10. RET
    const std::vector<bitforge::byte_t> program = {
        // Main at 0x0200:
        0x70, 0x00, 0x00,       // 0x0200: IN R0, 0x00
        0x62, 0x00,             // 0x0203: PUSH R0
        0x60, 0x02, 0x11,       // 0x0205: CALL 0x0211
        0x71, 0x02, 0x00,       // 0x0208: OUT 0x02, R0
        0x71, 0x10, 0x00,       // 0x020B: OUT 0x10, R0
        0x10, 0x01, 'O',        // 0x020E: LDI R1, 'O'
        0x71, 0x01, 0x01,       // 0x0211: OUT 0x01, R1 (Wait! Let's check address!)
    };

    // Exact instruction sequence:
    // 0x0200: 0x70, 0x00, 0x00 (IN R0, 0x00)   - len 3 -> next at 0x0203
    // 0x0203: 0x60, 0x02, 0x13 (CALL 0x0213)  - len 3 -> next at 0x0206
    // 0x0206: 0x71, 0x02, 0x00 (OUT 0x02, R0) - len 3 -> next at 0x0209
    // 0x0209: 0x71, 0x10, 0x00 (OUT 0x10, R0) - len 3 -> next at 0x020C
    // 0x020C: 0x10, 0x01, 'K'  (LDI R1, 'K')  - len 3 -> next at 0x020F
    // 0x020F: 0x71, 0x01, 0x01 (OUT 0x01, R1) - len 3 -> next at 0x0212
    // 0x0212: 0x01             (HALT)         - len 1 -> next at 0x0213
    // Subroutine double_value at 0x0213:
    // 0x0213: 0x62, 0x01       (PUSH R1)      - len 2 -> next at 0x0215
    // 0x0215: 0x11, 0x01, 0x00 (MOV R1, R0)   - len 3 -> next at 0x0218
    // 0x0218: 0x20, 0x00, 0x01 (ADD R0, R1)   - len 3 -> next at 0x021B
    // 0x021B: 0x63, 0x01       (POP R1)       - len 2 -> next at 0x021D
    // 0x021D: 0x61             (RET)          - len 1
    const std::vector<bitforge::byte_t> full_program = {
        0x70, 0x00, 0x00,       // 0x0200: IN R0, 0x00 (reads 10 from FIFO)
        0x60, 0x02, 0x13,       // 0x0203: CALL 0x0213 (double_value)
        0x71, 0x02, 0x00,       // 0x0206: OUT 0x02, R0 (outputs 20 to numeric console)
        0x71, 0x10, 0x00,       // 0x0209: OUT 0x10, R0 (sets LEDs to 20)
        0x10, 0x01, 'K',        // 0x020C: LDI R1, 'K'
        0x71, 0x01, 0x01,       // 0x020F: OUT 0x01, R1 (outputs 'K' to text console)
        0x01,                   // 0x0212: HALT
        // Subroutine at 0x0213:
        0x62, 0x01,             // 0x0213: PUSH R1 (preserve caller R1)
        0x11, 0x01, 0x00,       // 0x0215: MOV R1, R0 (R1 = 10)
        0x20, 0x00, 0x01,       // 0x0218: ADD R0, R1 (10 + 10 = 20)
        0x63, 0x01,             // 0x021B: POP R1 (restore caller R1)
        0x61                    // 0x021D: RET (return to 0x0206)
    };

    fix.memory.load_segment(0x0200, full_program.data(), full_program.size());
    fix.cpu.reset(0x0200);

    auto run_res = bitforge::test::run_with_budget(fix.cpu, 50);
    EXPECT_TRUE(run_res.success);
    EXPECT_EQ(run_res.stop_reason, "Halted");
    EXPECT_EQ(fix.cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(fix.cpu.registers().r[0], 20);
    EXPECT_EQ(fix.cpu.registers().sp, 0xFFFF); // Fully restored stack
    EXPECT_EQ(fix.bus.input_fifo_size(), 0);
    EXPECT_EQ(fix.bus.led_state(), 20);
    EXPECT_EQ(fix.bus.numeric_output(), "20");
    EXPECT_EQ(fix.bus.text_output(), "K");
}
