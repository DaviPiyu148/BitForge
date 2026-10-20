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

// =============================================================
// Phase 2: Exhaustive 256x256 Arithmetic & Flag Boundary Tests
// =============================================================

namespace {

// Portable mathematical mapping from 8-bit unsigned value to signed two's complement integer in [-128, 127].
// Maps 0x00..0x7F to 0..127 and 0x80..0xFF to -128..-1 without implementation-defined out-of-range conversions.
constexpr int to_signed_int8(unsigned int val) noexcept {
    return (val >= 0x80u) ? (static_cast<int>(val) - 256) : static_cast<int>(val);
}

} // namespace

TEST(AluTest, ExhaustiveAdd256x256) {
    std::size_t mismatches = 0;
    for (unsigned int a = 0; a < 256; ++a) {
        for (unsigned int b = 0; b < 256; ++b) {
            const auto res = bitforge::Alu::add(static_cast<bitforge::byte_t>(a),
                                                static_cast<bitforge::byte_t>(b));

            // Independent specification-derived reference (v1.7 §6.2)
            const unsigned int sum = a + b;
            const auto expected_r = static_cast<bitforge::byte_t>(sum & 0xFF);
            const bool expected_z = (expected_r == 0);
            const bool expected_c = (sum > 0xFF);
            const bool expected_n = (expected_r >= 0x80);

            // Signed overflow: true iff mathematically signed sum falls outside [-128, 127]
            const int sa = to_signed_int8(a);
            const int sb = to_signed_int8(b);
            const int s_sum = sa + sb;
            const bool expected_v = (s_sum < -128 || s_sum > 127);

            if (res.result != expected_r || res.flags.z != expected_z ||
                res.flags.c != expected_c || res.flags.n != expected_n ||
                res.flags.v != expected_v) {
                if (mismatches < 5) {
                    std::cerr << "ADD mismatch: a=" << a << " b=" << b
                              << " got r=" << static_cast<int>(res.result)
                              << " [Z=" << res.flags.z << " C=" << res.flags.c
                              << " N=" << res.flags.n << " V=" << res.flags.v << "]"
                              << " expected r=" << static_cast<int>(expected_r)
                              << " [Z=" << expected_z << " C=" << expected_c
                              << " N=" << expected_n << " V=" << expected_v << "]\n";
                }
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, ExhaustiveSub256x256) {
    std::size_t mismatches = 0;
    for (unsigned int a = 0; a < 256; ++a) {
        for (unsigned int b = 0; b < 256; ++b) {
            const auto res = bitforge::Alu::sub(static_cast<bitforge::byte_t>(a),
                                                static_cast<bitforge::byte_t>(b));

            // Independent specification-derived reference (v1.7 §6.2)
            const auto expected_r = static_cast<bitforge::byte_t>((static_cast<int>(a) - static_cast<int>(b)) & 0xFF);
            const bool expected_z = (expected_r == 0);
            const bool expected_c = (a >= b); // Carry/no-borrow convention
            const bool expected_n = (expected_r >= 0x80);

            // Signed overflow: true iff mathematically signed difference falls outside [-128, 127]
            const int sa = to_signed_int8(a);
            const int sb = to_signed_int8(b);
            const int s_diff = sa - sb;
            const bool expected_v = (s_diff < -128 || s_diff > 127);

            if (res.result != expected_r || res.flags.z != expected_z ||
                res.flags.c != expected_c || res.flags.n != expected_n ||
                res.flags.v != expected_v) {
                if (mismatches < 5) {
                    std::cerr << "SUB mismatch: a=" << a << " b=" << b
                              << " got r=" << static_cast<int>(res.result)
                              << " [Z=" << res.flags.z << " C=" << res.flags.c
                              << " N=" << res.flags.n << " V=" << res.flags.v << "]"
                              << " expected r=" << static_cast<int>(expected_r)
                              << " [Z=" << expected_z << " C=" << expected_c
                              << " N=" << expected_n << " V=" << expected_v << "]\n";
                }
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, ExhaustiveCmpMatchesSubFlags256x256) {
    std::size_t mismatches = 0;
    for (unsigned int a = 0; a < 256; ++a) {
        for (unsigned int b = 0; b < 256; ++b) {
            const auto sub_res = bitforge::Alu::sub(static_cast<bitforge::byte_t>(a),
                                                    static_cast<bitforge::byte_t>(b));
            const auto cmp_flags = bitforge::Alu::cmp(static_cast<bitforge::byte_t>(a),
                                                      static_cast<bitforge::byte_t>(b));

            if (sub_res.flags != cmp_flags) {
                if (mismatches < 5) {
                    std::cerr << "CMP mismatch: a=" << a << " b=" << b << "\n";
                }
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, ExhaustiveIncAllInputsAndCarryStates) {
    std::size_t mismatches = 0;
    for (int carry_state = 0; carry_state <= 1; ++carry_state) {
        bitforge::Flags in_flags;
        in_flags.c = (carry_state == 1);

        for (unsigned int a = 0; a < 256; ++a) {
            const auto res = bitforge::Alu::inc(static_cast<bitforge::byte_t>(a), in_flags);

            const auto expected_r = static_cast<bitforge::byte_t>((a + 1) & 0xFF);
            const bool expected_z = (expected_r == 0);
            const bool expected_n = (expected_r >= 0x80);
            const bool expected_v = (a == 0x7F); // 0x7F (+127) -> 0x80 (-128)
            const bool expected_c = (carry_state == 1); // C preserved

            if (res.result != expected_r || res.flags.z != expected_z ||
                res.flags.c != expected_c || res.flags.n != expected_n ||
                res.flags.v != expected_v) {
                if (mismatches < 5) {
                    std::cerr << "INC mismatch: a=" << a << " in_c=" << carry_state << "\n";
                }
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, ExhaustiveDecAllInputsAndCarryStates) {
    std::size_t mismatches = 0;
    for (int carry_state = 0; carry_state <= 1; ++carry_state) {
        bitforge::Flags in_flags;
        in_flags.c = (carry_state == 1);

        for (unsigned int a = 0; a < 256; ++a) {
            const auto res = bitforge::Alu::dec(static_cast<bitforge::byte_t>(a), in_flags);

            const auto expected_r = static_cast<bitforge::byte_t>((a + 0xFF) & 0xFF);
            const bool expected_z = (expected_r == 0);
            const bool expected_n = (expected_r >= 0x80);
            const bool expected_v = (a == 0x80); // 0x80 (-128) -> 0x7F (+127)
            const bool expected_c = (carry_state == 1); // C preserved

            if (res.result != expected_r || res.flags.z != expected_z ||
                res.flags.c != expected_c || res.flags.n != expected_n ||
                res.flags.v != expected_v) {
                if (mismatches < 5) {
                    std::cerr << "DEC mismatch: a=" << a << " in_c=" << carry_state << "\n";
                }
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, ExhaustiveBitwiseOperations256x256) {
    std::size_t mismatches = 0;
    for (unsigned int a = 0; a < 256; ++a) {
        const auto ba = static_cast<bitforge::byte_t>(a);

        // NOT (256 cases)
        const auto r_not = bitforge::Alu::not_op(ba);
        const auto exp_not_r = static_cast<bitforge::byte_t>((~a) & 0xFF);
        if (r_not.result != exp_not_r || r_not.flags.z != (exp_not_r == 0) ||
            r_not.flags.n != (exp_not_r >= 0x80) || r_not.flags.c || r_not.flags.v) {
            ++mismatches;
        }

        for (unsigned int b = 0; b < 256; ++b) {
            const auto bb = static_cast<bitforge::byte_t>(b);

            // AND
            const auto r_and = bitforge::Alu::and_op(ba, bb);
            const auto exp_and_r = static_cast<bitforge::byte_t>(a & b);
            if (r_and.result != exp_and_r || r_and.flags.z != (exp_and_r == 0) ||
                r_and.flags.n != (exp_and_r >= 0x80) || r_and.flags.c || r_and.flags.v) {
                ++mismatches;
            }

            // OR
            const auto r_or = bitforge::Alu::or_op(ba, bb);
            const auto exp_or_r = static_cast<bitforge::byte_t>(a | b);
            if (r_or.result != exp_or_r || r_or.flags.z != (exp_or_r == 0) ||
                r_or.flags.n != (exp_or_r >= 0x80) || r_or.flags.c || r_or.flags.v) {
                ++mismatches;
            }

            // XOR
            const auto r_xor = bitforge::Alu::xor_op(ba, bb);
            const auto exp_xor_r = static_cast<bitforge::byte_t>(a ^ b);
            if (r_xor.result != exp_xor_r || r_xor.flags.z != (exp_xor_r == 0) ||
                r_xor.flags.n != (exp_xor_r >= 0x80) || r_xor.flags.c || r_xor.flags.v) {
                ++mismatches;
            }
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

TEST(AluTest, DeterministicBoundaryPairsMatrix) {
    const bitforge::byte_t boundaries[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};

    for (const auto a : boundaries) {
        for (const auto b : boundaries) {
            auto add_res = bitforge::Alu::add(a, b);
            EXPECT_EQ(add_res.result, static_cast<bitforge::byte_t>((static_cast<unsigned int>(a) + static_cast<unsigned int>(b)) & 0xFF));

            auto sub_res = bitforge::Alu::sub(a, b);
            EXPECT_EQ(sub_res.result, static_cast<bitforge::byte_t>((static_cast<int>(a) - static_cast<int>(b)) & 0xFF));
            EXPECT_EQ(sub_res.flags.c, a >= b);

            auto cmp_flags = bitforge::Alu::cmp(a, b);
            EXPECT_EQ(cmp_flags, sub_res.flags);
        }
    }
}

TEST(AluTest, FlagChainingSequences) {
    // 1. ADD sets C=1 -> INC preserves C=1 -> DEC preserves C=1
    auto r1 = bitforge::Alu::add(0xFF, 0x01);
    EXPECT_TRUE(r1.flags.c);
    EXPECT_TRUE(r1.flags.z);

    auto r2 = bitforge::Alu::inc(r1.result, r1.flags);
    EXPECT_EQ(r2.result, 0x01);
    EXPECT_FALSE(r2.flags.z);
    EXPECT_TRUE(r2.flags.c); // C preserved

    auto r3 = bitforge::Alu::dec(r2.result, r2.flags);
    EXPECT_EQ(r3.result, 0x00);
    EXPECT_TRUE(r3.flags.z);
    EXPECT_TRUE(r3.flags.c); // C preserved

    // 2. SUB generates borrow (C=0) -> INC preserves C=0
    auto r4 = bitforge::Alu::sub(0x10, 0x20);
    EXPECT_FALSE(r4.flags.c); // borrow occurred
    EXPECT_TRUE(r4.flags.n);

    auto r5 = bitforge::Alu::inc(r4.result, r4.flags);
    EXPECT_FALSE(r5.flags.c); // C=0 preserved
}
