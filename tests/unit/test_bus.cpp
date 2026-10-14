#include "test_harness.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/core/constants.hpp"

TEST(BusTest, MemoryDelegationByteAndWord) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // Byte operations
    bus.write_byte(0x2000, 0x77);
    EXPECT_EQ(bus.read_byte(0x2000), 0x77);
    EXPECT_EQ(memory.read_byte(0x2000), 0x77);

    // Big-endian word operations
    bus.write_word(0x3000, 0xABCD);
    EXPECT_EQ(bus.read_word(0x3000), 0xABCD);
    EXPECT_EQ(memory.read_word(0x3000), 0xABCD);
    EXPECT_EQ(memory.read_byte(0x3000), 0xAB);
    EXPECT_EQ(memory.read_byte(0x3001), 0xCD);

    // Boundary word write/read across 0xFFFF / 0x0000 through the bus
    bus.write_word(0xFFFF, 0xCAFE);
    EXPECT_EQ(bus.read_word(0xFFFF), 0xCAFE);
    EXPECT_EQ(bus.read_byte(0xFFFF), 0xCA);
    EXPECT_EQ(bus.read_byte(0x0000), 0xFE);
}

TEST(BusTest, IoPermissionsSideEffectFree) {
    bitforge::Memory memory;
    const bitforge::SystemBus bus(memory);

    // Port 0x00 (FIFO) is input-only
    EXPECT_TRUE(bus.can_read_io(bitforge::kPortInputFifo));
    EXPECT_FALSE(bus.can_write_io(bitforge::kPortInputFifo));

    // Ports 0x01, 0x02, 0x10 are output-only
    EXPECT_FALSE(bus.can_read_io(bitforge::kPortTextConsole));
    EXPECT_TRUE(bus.can_write_io(bitforge::kPortTextConsole));

    EXPECT_FALSE(bus.can_read_io(bitforge::kPortNumericConsole));
    EXPECT_TRUE(bus.can_write_io(bitforge::kPortNumericConsole));

    EXPECT_FALSE(bus.can_read_io(bitforge::kPortLeds));
    EXPECT_TRUE(bus.can_write_io(bitforge::kPortLeds));

    // Unmapped ports return false for both read and write
    EXPECT_FALSE(bus.can_read_io(0x05));
    EXPECT_FALSE(bus.can_write_io(0x05));
    EXPECT_FALSE(bus.can_read_io(0xFF));
    EXPECT_FALSE(bus.can_write_io(0xFF));
}

TEST(BusTest, IoReadContractDistinguishesEmptyFifoFromInvalidPort) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // 1. Valid input port (Port 0x00): Empty FIFO returns std::optional containing byte 0x00
    const auto fifo_res = bus.read_io(bitforge::kPortInputFifo);
    EXPECT_TRUE(fifo_res.has_value());
    EXPECT_EQ(*fifo_res, 0x00);

    // 2. Output-only ports: read_io returns std::nullopt (cannot be read)
    EXPECT_FALSE(bus.read_io(bitforge::kPortTextConsole).has_value());
    EXPECT_FALSE(bus.read_io(bitforge::kPortNumericConsole).has_value());
    EXPECT_FALSE(bus.read_io(bitforge::kPortLeds).has_value());

    // 3. Unmapped ports: read_io returns std::nullopt
    EXPECT_FALSE(bus.read_io(0x05).has_value());
    EXPECT_FALSE(bus.read_io(0xFF).has_value());
}

TEST(BusTest, Phase1AOutputPortStubsValidatePermissionsWhileDiscardingPayload) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // 1. Valid output ports accept writes (returning true), confirming port validation.
    // In Phase 1A, payload byte is discarded because concrete virtual console and LED devices
    // attach to the bus in Phase 2.
    EXPECT_TRUE(bus.write_io(bitforge::kPortTextConsole, 'A'));
    EXPECT_TRUE(bus.write_io(bitforge::kPortNumericConsole, 42));
    EXPECT_TRUE(bus.write_io(bitforge::kPortLeds, 0xFF));

    // 2. Input-only port (Port 0x00) rejects writes and returns false
    EXPECT_FALSE(bus.write_io(bitforge::kPortInputFifo, 0x12));

    // 3. Unmapped ports reject writes and return false
    EXPECT_FALSE(bus.write_io(0x05, 0x34));
    EXPECT_FALSE(bus.write_io(0xFF, 0x56));
}
