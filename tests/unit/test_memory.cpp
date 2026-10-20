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

TEST(MemoryTest, BoundaryReadWriteAndNeighborIsolation) {
    bitforge::Memory mem;

    // Boundary addresses to audit: 0x0000, 0x7FFF, 0x8000, 0xEFFF, 0xF000, 0xFFFE, 0xFFFF
    struct BoundarySpec {
        bitforge::word_t addr;
        bitforge::byte_t val;
        bitforge::word_t left_neighbor;
        bitforge::word_t right_neighbor;
    };

    const BoundarySpec boundaries[] = {
        {0x0000, 0x11, 0xFFFF, 0x0001},
        {0x7FFF, 0x22, 0x7FFE, 0x8000},
        {0x8000, 0x33, 0x7FFF, 0x8001},
        {0xEFFF, 0x44, 0xEFFE, 0xF000},
        {0xF000, 0x55, 0xEFFF, 0xF001},
        {0xFFFE, 0x66, 0xFFFD, 0xFFFF},
        {0xFFFF, 0x77, 0xFFFE, 0x0000}
    };

    // 1. Isolated write and neighbor verification for each individual boundary
    for (const auto& b : boundaries) {
        mem.reset();
        mem.write_byte(b.addr, b.val);

        EXPECT_EQ(mem.read_byte(b.addr), b.val);
        EXPECT_EQ(mem.read_byte(b.left_neighbor), 0x00);
        EXPECT_EQ(mem.read_byte(b.right_neighbor), 0x00);
    }

    // 2. Simultaneous write of distinct values to all 7 boundaries
    mem.reset();
    for (const auto& b : boundaries) {
        mem.write_byte(b.addr, b.val);
    }

    // Assert each boundary holds its specific value
    for (const auto& b : boundaries) {
        EXPECT_EQ(mem.read_byte(b.addr), b.val);
    }

    // Assert non-boundary neighboring bytes remain 0x00
    EXPECT_EQ(mem.read_byte(0x0001), 0x00);
    EXPECT_EQ(mem.read_byte(0x7FFE), 0x00);
    EXPECT_EQ(mem.read_byte(0x8001), 0x00);
    EXPECT_EQ(mem.read_byte(0xEFFE), 0x00);
    EXPECT_EQ(mem.read_byte(0xF001), 0x00);
    EXPECT_EQ(mem.read_byte(0xFFFD), 0x00);
}

TEST(MemoryTest, WordAccessAcrossAllArchitecturalBoundaries) {
    bitforge::Memory mem;

    // 1. Boundary word at 0x0000 (spans 0x0000 and 0x0001)
    mem.write_word(0x0000, 0x1234);
    EXPECT_EQ(mem.read_word(0x0000), 0x1234);
    EXPECT_EQ(mem.read_byte(0x0000), 0x12);
    EXPECT_EQ(mem.read_byte(0x0001), 0x34);

    // 2. Boundary word at 0x7FFF (spans 0x7FFF and 0x8000 across 32K halves)
    mem.write_word(0x7FFF, 0x2345);
    EXPECT_EQ(mem.read_word(0x7FFF), 0x2345);
    EXPECT_EQ(mem.read_byte(0x7FFF), 0x23);
    EXPECT_EQ(mem.read_byte(0x8000), 0x45);

    // 3. Boundary word at 0x8000 (spans 0x8000 and 0x8001)
    mem.write_word(0x8000, 0x3456);
    EXPECT_EQ(mem.read_word(0x8000), 0x3456);
    EXPECT_EQ(mem.read_byte(0x8000), 0x34);
    EXPECT_EQ(mem.read_byte(0x8001), 0x56);

    // 4. Boundary word at 0xEFFF (spans 0xEFFF and 0xF000 into stack region)
    mem.write_word(0xEFFF, 0x4567);
    EXPECT_EQ(mem.read_word(0xEFFF), 0x4567);
    EXPECT_EQ(mem.read_byte(0xEFFF), 0x45);
    EXPECT_EQ(mem.read_byte(0xF000), 0x67);

    // 5. Boundary word at 0xF000 (spans 0xF000 and 0xF001)
    mem.write_word(0xF000, 0x5678);
    EXPECT_EQ(mem.read_word(0xF000), 0x5678);
    EXPECT_EQ(mem.read_byte(0xF000), 0x56);
    EXPECT_EQ(mem.read_byte(0xF001), 0x78);

    // 6. Boundary word at 0xFFFE (spans 0xFFFE and 0xFFFF at physical top)
    mem.write_word(0xFFFE, 0x6789);
    EXPECT_EQ(mem.read_word(0xFFFE), 0x6789);
    EXPECT_EQ(mem.read_byte(0xFFFE), 0x67);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x89);

    // 7. Boundary word at 0xFFFF (spans 0xFFFF and wraps to 0x0000 across 64K space)
    mem.write_word(0xFFFF, 0x789A);
    EXPECT_EQ(mem.read_word(0xFFFF), 0x789A);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x78);
    EXPECT_EQ(mem.read_byte(0x0000), 0x9A);
}

TEST(MemoryTest, LoadSegmentBoundaryExactFitAndOverflowAtomicity) {
    bitforge::Memory mem;

    // 1. Exact-fit at 0xFFFE with length 2 (occupies 0xFFFE and 0xFFFF)
    const bitforge::byte_t exact_fit_2[] = {0xCA, 0xFE};
    const bool ok_2 = mem.load_segment(0xFFFE, exact_fit_2, 2);
    EXPECT_TRUE(ok_2);
    EXPECT_EQ(mem.read_byte(0xFFFE), 0xCA);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0xFE);
    EXPECT_EQ(mem.read_byte(0xFFFD), 0x00);
    EXPECT_EQ(mem.read_byte(0x0000), 0x00);

    // 2. 1-byte overflow at 0xFFFE with length 3 (would need 0xFFFE, 0xFFFF, 0x10000)
    // Atomicity assertion: rejected load must NOT mutate existing storage at 0xFFFE or 0xFFFF
    const bitforge::byte_t overrun_3[] = {0xAA, 0xBB, 0xCC};
    const bool ok_overrun_3 = mem.load_segment(0xFFFE, overrun_3, 3);
    EXPECT_FALSE(ok_overrun_3);
    EXPECT_EQ(mem.read_byte(0xFFFE), 0xCA); // Preserved
    EXPECT_EQ(mem.read_byte(0xFFFF), 0xFE); // Preserved

    // 3. Exact-fit at 0xFFFF with length 1 (occupies 0xFFFF)
    const bitforge::byte_t exact_fit_1[] = {0x99};
    const bool ok_1 = mem.load_segment(0xFFFF, exact_fit_1, 1);
    EXPECT_TRUE(ok_1);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x99);

    // 4. 1-byte overflow at 0xFFFF with length 2 (would need 0xFFFF, 0x10000)
    // Atomicity assertion: rejected load must NOT mutate existing storage at 0xFFFF or wrap to 0x0000
    const bitforge::byte_t overrun_2[] = {0x11, 0x22};
    const bool ok_overrun_2 = mem.load_segment(0xFFFF, overrun_2, 2);
    EXPECT_FALSE(ok_overrun_2);
    EXPECT_EQ(mem.read_byte(0xFFFF), 0x99); // Preserved
    EXPECT_EQ(mem.read_byte(0x0000), 0x00); // Untouched
}

TEST(MemoryTest, ResetComprehensiveStorageClear) {
    bitforge::Memory mem;

    // Populate every single address (all 65,536 bytes) with a deterministic non-zero byte pattern.
    // (i % 255) + 1 produces values strictly within [1, 255], ensuring no byte is zero before reset.
    for (std::size_t i = 0; i < mem.size(); ++i) {
        const auto val = static_cast<bitforge::byte_t>((i % 255) + 1);
        mem.write_byte(static_cast<bitforge::word_t>(i), val);
    }

    // Verify non-zero values are present across boundaries before reset
    EXPECT_NE(mem.read_byte(0x0000), 0x00);
    EXPECT_NE(mem.read_byte(0x7FFF), 0x00);
    EXPECT_NE(mem.read_byte(0x8000), 0x00);
    EXPECT_NE(mem.read_byte(0xEFFF), 0x00);
    EXPECT_NE(mem.read_byte(0xF000), 0x00);
    EXPECT_NE(mem.read_byte(0xFFFE), 0x00);
    EXPECT_NE(mem.read_byte(0xFFFF), 0x00);

    // Perform reset and verify the entire 64 KiB address space is strictly 0x00
    mem.reset();

    for (std::size_t i = 0; i < mem.size(); ++i) {
        ASSERT_EQ(mem.read_byte(static_cast<bitforge::word_t>(i)), 0x00);
    }
}
