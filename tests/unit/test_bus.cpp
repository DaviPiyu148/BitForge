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

TEST(BusTest, InputFifoContentsAndInspection) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    EXPECT_TRUE(bus.input_fifo_contents().empty());
    bus.push_input_fifo(0x42);
    bus.push_input_fifo(0x55);
    bus.push_input_fifo(0xAA);

    auto contents = bus.input_fifo_contents();
    ASSERT_EQ(contents.size(), 3);
    EXPECT_EQ(contents[0], 0x42);
    EXPECT_EQ(contents[1], 0x55);
    EXPECT_EQ(contents[2], 0xAA);

    // Reading pops from FIFO
    EXPECT_EQ(bus.read_io(bitforge::kPortInputFifo).value_or(0), 0x42);
    EXPECT_EQ(bus.input_fifo_size(), 2);

    bus.clear_input_fifo();
    EXPECT_EQ(bus.input_fifo_size(), 0);
    EXPECT_TRUE(bus.input_fifo_contents().empty());
}

TEST(BusTest, MemoryAndIoEventJournaling) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // Memory write byte
    bus.write_byte(0x8000, 0x12);
    auto mw = bus.take_memory_write_events();
    ASSERT_EQ(mw.size(), 1);
    EXPECT_EQ(mw[0].address, 0x8000);
    EXPECT_EQ(mw[0].old_value, 0x00);
    EXPECT_EQ(mw[0].new_value, 0x12);
    EXPECT_TRUE(bus.memory_write_events().empty());

    // Memory write word
    bus.write_word(0x8010, 0xABCD);
    auto mw_word = bus.take_memory_write_events();
    ASSERT_EQ(mw_word.size(), 2);
    EXPECT_EQ(mw_word[0].address, 0x8010);
    EXPECT_EQ(mw_word[0].new_value, 0xAB);
    EXPECT_EQ(mw_word[1].address, 0x8011);
    EXPECT_EQ(mw_word[1].new_value, 0xCD);

    // IO events
    bus.write_io(bitforge::kPortLeds, 0xFF);
    bus.write_io(bitforge::kPortNumericConsole, 42);
    auto io_events = bus.take_io_events();
    ASSERT_EQ(io_events.size(), 2);
    EXPECT_EQ(io_events[0].port, bitforge::kPortLeds);
    EXPECT_EQ(io_events[0].value, 0xFF);
    EXPECT_EQ(io_events[1].port, bitforge::kPortNumericConsole);
    EXPECT_EQ(io_events[1].value, 42);
}

TEST(BusTest, BusResetDevicesClearsPeripheralsAndPreservesMemory) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // Populate device state across all virtual devices
    bus.push_input_fifo(0x11);
    bus.push_input_fifo(0x22);
    EXPECT_EQ(bus.read_io(bitforge::kPortInputFifo).value_or(0), 0x11);
    EXPECT_EQ(bus.read_io(bitforge::kPortInputFifo).value_or(0), 0x22);
    // Trigger underflow on empty FIFO
    EXPECT_EQ(bus.read_io(bitforge::kPortInputFifo).value_or(0xFF), 0x00);
    EXPECT_TRUE(bus.has_input_underflow());
    EXPECT_EQ(bus.input_underflow_count(), 1);

    // Populate text console
    bus.write_io(bitforge::kPortTextConsole, 'A');
    bus.write_io(bitforge::kPortTextConsole, 0x07); // bell escape
    EXPECT_FALSE(bus.text_output().empty());
    EXPECT_FALSE(bus.text_raw_bytes().empty());

    // Populate numeric console
    bus.write_io(bitforge::kPortNumericConsole, 42);
    bus.write_io(bitforge::kPortNumericConsole, 99);
    EXPECT_FALSE(bus.numeric_output().empty());
    EXPECT_FALSE(bus.numeric_values().empty());

    // Populate virtual LEDs
    bus.write_io(bitforge::kPortLeds, 0xAA);
    EXPECT_EQ(bus.led_state(), 0xAA);

    // Populate memory through bus
    bus.write_byte(0x2000, 0x55);
    bus.write_word(0x3000, 0x1234);
    EXPECT_FALSE(bus.memory_write_events().empty());
    EXPECT_FALSE(bus.io_events().empty());

    // Perform reset_devices()
    bus.reset_devices();

    // Verify all virtual peripheral buffers and flags are reset
    EXPECT_EQ(bus.input_fifo_size(), 0);
    EXPECT_TRUE(bus.input_fifo_contents().empty());
    EXPECT_FALSE(bus.has_input_underflow());
    EXPECT_EQ(bus.input_underflow_count(), 0);
    EXPECT_TRUE(bus.text_output().empty());
    EXPECT_TRUE(bus.text_raw_bytes().empty());
    EXPECT_TRUE(bus.numeric_output().empty());
    EXPECT_TRUE(bus.numeric_values().empty());
    EXPECT_EQ(bus.led_state(), 0x00);
    EXPECT_TRUE(bus.memory_write_events().empty());
    EXPECT_TRUE(bus.io_events().empty());

    // Separation-of-concerns: verify underlying RAM was NOT cleared by reset_devices()
    EXPECT_EQ(memory.read_byte(0x2000), 0x55);
    EXPECT_EQ(memory.read_word(0x3000), 0x1234);
}

TEST(BusTest, BusBoundaryMemoryDelegationAndJournaling) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // Boundary addresses to audit through SystemBus
    const bitforge::word_t boundaries[] = {
        0x0000, 0x7FFF, 0x8000, 0xEFFF, 0xF000, 0xFFFE, 0xFFFF
    };

    // 1. Single byte delegation and write event journaling
    for (std::size_t i = 0; i < 7; ++i) {
        const bitforge::word_t addr = boundaries[i];
        const bitforge::byte_t val = static_cast<bitforge::byte_t>(0x10 + i);

        bus.write_byte(addr, val);
        EXPECT_EQ(bus.read_byte(addr), val);
        EXPECT_EQ(memory.read_byte(addr), val);

        auto events = bus.take_memory_write_events();
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].address, addr);
        EXPECT_EQ(events[0].old_value, 0x00);
        EXPECT_EQ(events[0].new_value, val);
    }

    // 2. 16-bit word delegation across 0xFFFF / 0x0000 wrapping boundary
    bus.write_word(0xFFFF, 0xA1B2);
    EXPECT_EQ(bus.read_word(0xFFFF), 0xA1B2);
    EXPECT_EQ(bus.read_byte(0xFFFF), 0xA1);
    EXPECT_EQ(bus.read_byte(0x0000), 0xB2);
    EXPECT_EQ(memory.read_word(0xFFFF), 0xA1B2);

    auto wrap_events = bus.take_memory_write_events();
    ASSERT_EQ(wrap_events.size(), 2);
    EXPECT_EQ(wrap_events[0].address, 0xFFFF);
    EXPECT_EQ(wrap_events[0].new_value, 0xA1);
    EXPECT_EQ(wrap_events[1].address, 0x0000);
    EXPECT_EQ(wrap_events[1].new_value, 0xB2);
}

TEST(BusTest, UnmappedPortRejectionAndAtomicityMatrix) {
    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);

    // Exhaustively scan all 256 possible 8-bit I/O ports
    for (int p = 0; p <= 0xFF; ++p) {
        const auto port = static_cast<bitforge::port_t>(p);

        if (port == bitforge::kPortInputFifo) {
            // Port 0x00: input-only
            EXPECT_TRUE(bus.can_read_io(port));
            EXPECT_FALSE(bus.can_write_io(port));
        } else if (port == bitforge::kPortTextConsole ||
                   port == bitforge::kPortNumericConsole ||
                   port == bitforge::kPortLeds) {
            // Ports 0x01, 0x02, 0x10: output-only
            EXPECT_FALSE(bus.can_read_io(port));
            EXPECT_TRUE(bus.can_write_io(port));
        } else {
            // All other 252 ports are unmapped
            EXPECT_FALSE(bus.can_read_io(port));
            EXPECT_FALSE(bus.can_write_io(port));

            // Attempts to read must return std::nullopt without logging events
            EXPECT_FALSE(bus.read_io(port).has_value());

            // Attempts to write must return false without logging events
            EXPECT_FALSE(bus.write_io(port, 0x55));
        }
    }

    // Verify rejected unmapped accesses produced zero I/O events
    EXPECT_TRUE(bus.take_io_events().empty());
}

