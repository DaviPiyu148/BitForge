#include "test_harness.hpp"
#include "bitforge/memory/memory.hpp"

#include <vector>
#include <limits>

TEST(MemoryTest, InitialStateIsZero) {
    bitforge::Memory mem;
    for (std::size_t i = 0; i < mem.size(); i += 256) {
        EXPECT_EQ(mem.read_byte(static_cast<bitforge::word_t>(i)), 0x00);
    }
}

TEST(MemoryTest, ByteReadWriteAndBoundaries) {
    bitforge::Memory mem;
    mem.write_byte(0x0000, 0xAA);
    mem.write_byte(0xFFFF, 0x55);
    mem.write_byte(0x8000, 0x42);

    EXPECT_EQ(mem.read_byte(0x0000), 0xAA);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x55);
    EXPECT_EQ(mem.read_byte(0x8000), 0x42);
}

TEST(MemoryTest, BigEndianWordReadWrite) {
    bitforge::Memory mem;
    mem.write_word(0x1000, 0x1234);

    EXPECT_EQ(mem.read_byte(0x1000), 0x12);
    EXPECT_EQ(mem.read_byte(0x1001), 0x34);
    EXPECT_EQ(mem.read_word(0x1000), 0x1234);
}

TEST(MemoryTest, WordBoundaryWrappingAcross64K) {
    bitforge::Memory mem;
    // Writing 16-bit word at 0xFFFF should place high byte at 0xFFFF and wrap low byte to 0x0000
    mem.write_word(0xFFFF, 0xA1B2);

    EXPECT_EQ(mem.read_byte(0xFFFF), 0xA1);
    EXPECT_EQ(mem.read_byte(0x0000), 0xB2);
    EXPECT_EQ(mem.read_word(0xFFFF), 0xA1B2);
}

TEST(MemoryTest, ResetZeroesAllStorage) {
    bitforge::Memory mem;
    mem.write_byte(0x0000, 0xFF);
    mem.write_byte(0x1234, 0x56);
    mem.write_byte(0xFFFF, 0xAA);

    mem.reset();

    EXPECT_EQ(mem.read_byte(0x0000), 0x00);
    EXPECT_EQ(mem.read_byte(0x1234), 0x00);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x00);
}

TEST(MemoryTest, LoadSegmentSuccess) {
    bitforge::Memory mem;
    const std::vector<bitforge::byte_t> code = {0x10, 0x00, 0x2A, 0x01}; // LDI R0, 42; HALT
    const bool ok = mem.load_segment(0x0200, code);

    EXPECT_TRUE(ok);
    EXPECT_EQ(mem.read_byte(0x0200), 0x10);
    EXPECT_EQ(mem.read_byte(0x0201), 0x00);
    EXPECT_EQ(mem.read_byte(0x0202), 0x2A);
    EXPECT_EQ(mem.read_byte(0x0203), 0x01);
}

TEST(MemoryTest, LoadSegmentFinalValidByte) {
    bitforge::Memory mem;
    const bitforge::byte_t byte_val = 0x7E;
    // Single byte loaded at 0xFFFF (the last physical byte in 64 KiB memory)
    const bool ok = mem.load_segment(0xFFFF, &byte_val, 1);

    EXPECT_TRUE(ok);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x7E);
    EXPECT_EQ(mem.read_byte(0xFFFE), 0x00);
}

TEST(MemoryTest, LoadSegmentEndingExactlyAt64KiB) {
    bitforge::Memory mem;
    // 1. Half memory segment from 0x8000 with length 0x8000 (32768) ends exactly at 0x10000
    std::vector<bitforge::byte_t> half_mem(0x8000, 0x33);
    const bool ok_half = mem.load_segment(0x8000, half_mem);
    EXPECT_TRUE(ok_half);
    EXPECT_EQ(mem.read_byte(0x8000), 0x33);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x33);
    EXPECT_EQ(mem.read_byte(0x7FFF), 0x00);

    // 2. Full 64 KiB memory image from 0x0000 with length 0x10000 (65536)
    std::vector<bitforge::byte_t> full_mem(65536, 0x5A);
    const bool ok_full = mem.load_segment(0x0000, full_mem);
    EXPECT_TRUE(ok_full);
    EXPECT_EQ(mem.read_byte(0x0000), 0x5A);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x5A);
}

TEST(MemoryTest, LoadSegmentOverrunRejection) {
    bitforge::Memory mem;
    const bitforge::byte_t overflow_data[] = {0x01, 0x02, 0x03};

    // 0xFFFE + 3 bytes = 0x10001 (1 byte overrun)
    EXPECT_FALSE(mem.load_segment(0xFFFE, overflow_data, 3));
    EXPECT_EQ(mem.read_byte(0xFFFE), 0x00);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x00);

    // 0xFFFF + 2 bytes = 0x10001 (1 byte overrun)
    EXPECT_FALSE(mem.load_segment(0xFFFF, overflow_data, 2));
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x00);

    // 0x0000 + 65537 bytes (1 byte overrun)
    EXPECT_FALSE(mem.load_segment(0x0000, overflow_data, 65537));
}

TEST(MemoryTest, LoadSegmentNullDataHandling) {
    bitforge::Memory mem;
    // Empty segment with null pointer is a valid no-op
    EXPECT_TRUE(mem.load_segment(0x1000, nullptr, 0));
    EXPECT_TRUE(mem.load_segment(0xFFFF, nullptr, 0));

    // Non-empty segment with null pointer must be rejected
    EXPECT_FALSE(mem.load_segment(0x1000, nullptr, 1));
    EXPECT_FALSE(mem.load_segment(0x1000, nullptr, 10));
}

TEST(MemoryTest, LoadSegmentVeryLargeLengthOverflowProtection) {
    bitforge::Memory mem;
    const bitforge::byte_t dummy = 0xFF;
    const std::size_t max_size = std::numeric_limits<std::size_t>::max();

    // Verify arithmetic overflow does not cause wrap-around acceptance
    EXPECT_FALSE(mem.load_segment(0x0000, &dummy, max_size));
    EXPECT_FALSE(mem.load_segment(0x8000, &dummy, max_size));
    EXPECT_FALSE(mem.load_segment(0xFFFF, &dummy, max_size));
}
