#include "test_harness.hpp"
#include "bitforge/core/alu.hpp"

TEST(AluTest, AddBasicAndFlags) {
    // Basic addition without carry or overflow
    auto r1 = bitforge::Alu::add(10, 20);
    EXPECT_EQ(r1.result, 30);
    EXPECT_FALSE(r1.flags.z);
    EXPECT_FALSE(r1.flags.c);
    EXPECT_FALSE(r1.flags.n);
    EXPECT_FALSE(r1.flags.v);

    // Zero result
    auto r2 = bitforge::Alu::add(0, 0);
    EXPECT_EQ(r2.result, 0);
    EXPECT_TRUE(r2.flags.z);
    EXPECT_FALSE(r2.flags.c);
    EXPECT_FALSE(r2.flags.n);
    EXPECT_FALSE(r2.flags.v);

    // Unsigned carry out
    auto r3 = bitforge::Alu::add(0xFF, 0x01);
    EXPECT_EQ(r3.result, 0x00);
    EXPECT_TRUE(r3.flags.z);
    EXPECT_TRUE(r3.flags.c);
    EXPECT_FALSE(r3.flags.n);
    EXPECT_FALSE(r3.flags.v);

    // Negative result without overflow
    auto r4 = bitforge::Alu::add(0x80, 0x05);
    EXPECT_EQ(r4.result, 0x85);
    EXPECT_FALSE(r4.flags.z);
    EXPECT_FALSE(r4.flags.c);
    EXPECT_TRUE(r4.flags.n);
    EXPECT_FALSE(r4.flags.v);
}

TEST(AluTest, AddSignedOverflowEdgeCases) {
    // 0x7F (+127) + 0x01 (+1) = 0x80 (-128) -> Overflow!
    auto r1 = bitforge::Alu::add(0x7F, 0x01);
    EXPECT_EQ(r1.result, 0x80);
    EXPECT_FALSE(r1.flags.z);
    EXPECT_FALSE(r1.flags.c);
    EXPECT_TRUE(r1.flags.n);
    EXPECT_TRUE(r1.flags.v);

    // 0x80 (-128) + 0x80 (-128) = 0x00 (0) with Carry -> Overflow!
    auto r2 = bitforge::Alu::add(0x80, 0x80);
    EXPECT_EQ(r2.result, 0x00);
    EXPECT_TRUE(r2.flags.z);
    EXPECT_TRUE(r2.flags.c);
    EXPECT_FALSE(r2.flags.n);
    EXPECT_TRUE(r2.flags.v);

    // 0x7F (+127) + 0x80 (-128) = 0xFF (-1) -> No overflow (operands have opposite signs)
    auto r3 = bitforge::Alu::add(0x7F, 0x80);
    EXPECT_EQ(r3.result, 0xFF);
    EXPECT_FALSE(r3.flags.z);
    EXPECT_FALSE(r3.flags.c);
    EXPECT_TRUE(r3.flags.n);
    EXPECT_FALSE(r3.flags.v);
}

TEST(AluTest, SubBasicAndFlags) {
    // Equal operands: zero result, no borrow (C=1)
    auto r1 = bitforge::Alu::sub(0x42, 0x42);
    EXPECT_EQ(r1.result, 0x00);
    EXPECT_TRUE(r1.flags.z);
    EXPECT_TRUE(r1.flags.c); // C = (a >= b)
    EXPECT_FALSE(r1.flags.n);
    EXPECT_FALSE(r1.flags.v);

    // Greater - Lesser: no borrow (C=1)
    auto r2 = bitforge::Alu::sub(0x50, 0x10);
    EXPECT_EQ(r2.result, 0x40);
    EXPECT_FALSE(r2.flags.z);
    EXPECT_TRUE(r2.flags.c);
    EXPECT_FALSE(r2.flags.n);
    EXPECT_FALSE(r2.flags.v);

    // Lesser - Greater: borrow required (C=0)
    auto r3 = bitforge::Alu::sub(0x10, 0x50);
    EXPECT_EQ(r3.result, 0xC0);
    EXPECT_FALSE(r3.flags.z);
    EXPECT_FALSE(r3.flags.c); // Borrow occurred
    EXPECT_TRUE(r3.flags.n);
    EXPECT_FALSE(r3.flags.v);
}

TEST(AluTest, SubSignedOverflowEdgeCases) {
    // 0x80 (-128) - 0x01 (+1) = 0x7F (+127) -> Signed Underflow!
    auto r1 = bitforge::Alu::sub(0x80, 0x01);
    EXPECT_EQ(r1.result, 0x7F);
    EXPECT_FALSE(r1.flags.z);
    EXPECT_TRUE(r1.flags.c);
    EXPECT_FALSE(r1.flags.n);
    EXPECT_TRUE(r1.flags.v);

    // 0x7F (+127) - 0x80 (-128) = 0xFF (-1) -> Signed Overflow!
    auto r2 = bitforge::Alu::sub(0x7F, 0x80);
    EXPECT_EQ(r2.result, 0xFF);
    EXPECT_FALSE(r2.flags.z);
    EXPECT_FALSE(r2.flags.c); // 0x7F < 0x80 unsigned
    EXPECT_TRUE(r2.flags.n);
    EXPECT_TRUE(r2.flags.v);
}

TEST(AluTest, CmpProducesIdenticalFlagsAsSub) {
    const bitforge::byte_t test_pairs[][2] = {
        {0x42, 0x42},
        {0x50, 0x10},
        {0x10, 0x50},
        {0x80, 0x01},
        {0x7F, 0x80},
        {0x00, 0xFF},
        {0xFF, 0x00}
    };

    for (const auto& pair : test_pairs) {
        auto sub_res = bitforge::Alu::sub(pair[0], pair[1]);
        auto cmp_flags = bitforge::Alu::cmp(pair[0], pair[1]);
        EXPECT_EQ(sub_res.flags, cmp_flags);
    }
}

TEST(AluTest, IncSemanticsAndCarryPreservation) {
    bitforge::Flags f_carry_on;
    f_carry_on.c = true;

    bitforge::Flags f_carry_off;
    f_carry_off.c = false;

    // Normal increment
    auto r1 = bitforge::Alu::inc(0x10, f_carry_on);
    EXPECT_EQ(r1.result, 0x11);
    EXPECT_FALSE(r1.flags.z);
    EXPECT_TRUE(r1.flags.c); // Preserved
    EXPECT_FALSE(r1.flags.n);
    EXPECT_FALSE(r1.flags.v);

    // Wrap 0xFF -> 0x00
    auto r2 = bitforge::Alu::inc(0xFF, f_carry_off);
    EXPECT_EQ(r2.result, 0x00);
    EXPECT_TRUE(r2.flags.z);
    EXPECT_FALSE(r2.flags.c); // Preserved
    EXPECT_FALSE(r2.flags.n);
    EXPECT_FALSE(r2.flags.v);

    // Signed overflow 0x7F -> 0x80
    auto r3 = bitforge::Alu::inc(0x7F, f_carry_on);
    EXPECT_EQ(r3.result, 0x80);
    EXPECT_FALSE(r3.flags.z);
    EXPECT_TRUE(r3.flags.c); // Preserved
    EXPECT_TRUE(r3.flags.n);
    EXPECT_TRUE(r3.flags.v); // V set on 0x7F -> 0x80
}

TEST(AluTest, DecSemanticsAndCarryPreservation) {
    bitforge::Flags f_carry_on;
    f_carry_on.c = true;

    bitforge::Flags f_carry_off;
    f_carry_off.c = false;

    // Normal decrement
    auto r1 = bitforge::Alu::dec(0x11, f_carry_on);
    EXPECT_EQ(r1.result, 0x10);
    EXPECT_FALSE(r1.flags.z);
    EXPECT_TRUE(r1.flags.c); // Preserved
    EXPECT_FALSE(r1.flags.n);
    EXPECT_FALSE(r1.flags.v);

    // Decrement to zero
    auto r2 = bitforge::Alu::dec(0x01, f_carry_off);
    EXPECT_EQ(r2.result, 0x00);
    EXPECT_TRUE(r2.flags.z);
    EXPECT_FALSE(r2.flags.c); // Preserved
    EXPECT_FALSE(r2.flags.n);
    EXPECT_FALSE(r2.flags.v);

    // Wrap 0x00 -> 0xFF
    auto r3 = bitforge::Alu::dec(0x00, f_carry_on);
    EXPECT_EQ(r3.result, 0xFF);
    EXPECT_FALSE(r3.flags.z);
    EXPECT_TRUE(r3.flags.c); // Preserved
    EXPECT_TRUE(r3.flags.n);
    EXPECT_FALSE(r3.flags.v);

    // Signed overflow 0x80 -> 0x7F
    auto r4 = bitforge::Alu::dec(0x80, f_carry_off);
    EXPECT_EQ(r4.result, 0x7F);
    EXPECT_FALSE(r4.flags.z);
    EXPECT_FALSE(r4.flags.c); // Preserved
    EXPECT_FALSE(r4.flags.n);
    EXPECT_TRUE(r4.flags.v); // V set on 0x80 -> 0x7F
}

TEST(AluTest, BitwiseOperationsClearCarryAndOverflow) {
    // AND
    auto r_and = bitforge::Alu::and_op(0xFF, 0x80);
    EXPECT_EQ(r_and.result, 0x80);
    EXPECT_FALSE(r_and.flags.z);
    EXPECT_TRUE(r_and.flags.n);
    EXPECT_FALSE(r_and.flags.c);
    EXPECT_FALSE(r_and.flags.v);

    auto r_and_z = bitforge::Alu::and_op(0xAA, 0x55);
    EXPECT_EQ(r_and_z.result, 0x00);
    EXPECT_TRUE(r_and_z.flags.z);
    EXPECT_FALSE(r_and_z.flags.n);
    EXPECT_FALSE(r_and_z.flags.c);
    EXPECT_FALSE(r_and_z.flags.v);

    // OR
    auto r_or = bitforge::Alu::or_op(0x40, 0x80);
    EXPECT_EQ(r_or.result, 0xC0);
    EXPECT_FALSE(r_or.flags.z);
    EXPECT_TRUE(r_or.flags.n);
    EXPECT_FALSE(r_or.flags.c);
    EXPECT_FALSE(r_or.flags.v);

    // XOR
    auto r_xor = bitforge::Alu::xor_op(0xAA, 0xAA);
    EXPECT_EQ(r_xor.result, 0x00);
    EXPECT_TRUE(r_xor.flags.z);
    EXPECT_FALSE(r_xor.flags.n);
    EXPECT_FALSE(r_xor.flags.c);
    EXPECT_FALSE(r_xor.flags.v);

    // NOT
    auto r_not = bitforge::Alu::not_op(0x7F);
    EXPECT_EQ(r_not.result, 0x80);
    EXPECT_FALSE(r_not.flags.z);
    EXPECT_TRUE(r_not.flags.n);
    EXPECT_FALSE(r_not.flags.c);
    EXPECT_FALSE(r_not.flags.v);

    auto r_not_ff = bitforge::Alu::not_op(0xFF);
    EXPECT_EQ(r_not_ff.result, 0x00);
    EXPECT_TRUE(r_not_ff.flags.z);
    EXPECT_FALSE(r_not_ff.flags.n);
    EXPECT_FALSE(r_not_ff.flags.c);
    EXPECT_FALSE(r_not_ff.flags.v);
}
