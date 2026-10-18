#include "test_harness.hpp"
#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/opcodes.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/core/registers.hpp"
#include "bitforge/core/cpu_state.hpp"
#include "bitforge/core/faults.hpp"
#include "bitforge/core/instruction_info.hpp"

#include <type_traits>
#include <string_view>

TEST(TypesTest, IntegerWidthsAndSignedness) {
    EXPECT_EQ(sizeof(bitforge::byte_t), 1);
    EXPECT_EQ(sizeof(bitforge::word_t), 2);
    EXPECT_EQ(sizeof(bitforge::sbyte_t), 1);
    EXPECT_EQ(sizeof(bitforge::sword_t), 2);
    EXPECT_EQ(sizeof(bitforge::reg_id_t), 1);
    EXPECT_EQ(sizeof(bitforge::port_t), 1);

    EXPECT_TRUE(std::is_unsigned_v<bitforge::byte_t>);
    EXPECT_TRUE(std::is_unsigned_v<bitforge::word_t>);
    EXPECT_TRUE(std::is_signed_v<bitforge::sbyte_t>);
    EXPECT_TRUE(std::is_signed_v<bitforge::sword_t>);
}

TEST(TypesTest, BigEndianWordCompositionAndDecomposition) {
    EXPECT_EQ(bitforge::make_word(0x12, 0x34), 0x1234);
    EXPECT_EQ(bitforge::make_word(0x00, 0x00), 0x0000);
    EXPECT_EQ(bitforge::make_word(0xFF, 0xEE), 0xFFEE);
    EXPECT_EQ(bitforge::make_word(0x80, 0x01), 0x8001);

    EXPECT_EQ(bitforge::high_byte(0x1234), 0x12);
    EXPECT_EQ(bitforge::low_byte(0x1234), 0x34);

    EXPECT_EQ(bitforge::high_byte(0xFFFF), 0xFF);
    EXPECT_EQ(bitforge::low_byte(0xFFFF), 0xFF);

    EXPECT_EQ(bitforge::high_byte(0x0001), 0x00);
    EXPECT_EQ(bitforge::low_byte(0x0001), 0x01);

    // Roundtrip verification
    for (int hi = 0; hi <= 0xFF; hi += 17) {
        for (int lo = 0; lo <= 0xFF; lo += 19) {
            const auto h = static_cast<bitforge::byte_t>(hi);
            const auto l = static_cast<bitforge::byte_t>(lo);
            const bitforge::word_t w = bitforge::make_word(h, l);
            EXPECT_EQ(bitforge::high_byte(w), h);
            EXPECT_EQ(bitforge::low_byte(w), l);
        }
    }
}

TEST(TypesTest, Exact31OpcodeValuesAndLengths) {
    // Structure holding canonical definition from specification Section 6.1
    struct CanonicalOp {
        bitforge::byte_t opcode;
        std::string_view mnemonic;
        std::size_t length;
    };

    const CanonicalOp canonical_table[] = {
        // Group 1: Basic Control & Data Movement (11 mnemonics)
        {0x00, "NOP",   1},
        {0x01, "HALT",  1},
        {0x10, "LDI",   3},
        {0x11, "MOV",   3},
        {0x12, "LD",    4},
        {0x13, "ST",    4},
        {0x14, "LDI16", 3},
        {0x15, "LDX",   2},
        {0x16, "STX",   2},
        {0x17, "INX",   1},
        {0x18, "DEX",   1},

        // Group 2: Arithmetic & Logic (9 mnemonics)
        {0x20, "ADD",   3},
        {0x21, "SUB",   3},
        {0x22, "INC",   2},
        {0x23, "DEC",   2},
        {0x30, "AND",   3},
        {0x31, "OR",    3},
        {0x32, "XOR",   3},
        {0x33, "NOT",   2},
        {0x40, "CMP",   3},

        // Group 3: Branch, Stack & Call (9 mnemonics)
        {0x50, "JMP",   3},
        {0x51, "JZ",    3},
        {0x52, "JNZ",   3},
        {0x53, "JC",    3},
        {0x54, "JNC",   3},
        {0x60, "CALL",  3},
        {0x61, "RET",   1},
        {0x62, "PUSH",  2},
        {0x63, "POP",   2},

        // Group 4: Input/Output (2 mnemonics)
        {0x70, "IN",    3},
        {0x71, "OUT",   3}
    };

    // Verify exactly 31 definitions in canonical table
    EXPECT_EQ(sizeof(canonical_table) / sizeof(canonical_table[0]), 31);

    for (const auto& op : canonical_table) {
        EXPECT_TRUE(bitforge::is_valid_opcode(op.opcode));
        EXPECT_EQ(bitforge::instruction_length(op.opcode), op.length);
        EXPECT_EQ(bitforge::opcode_to_mnemonic(op.opcode), op.mnemonic);
    }
}

TEST(TypesTest, OpcodeLengthDistribution) {
    std::size_t len1_count = 0;
    std::size_t len2_count = 0;
    std::size_t len3_count = 0;
    std::size_t len4_count = 0;

    for (int b = 0; b <= 0xFF; ++b) {
        const auto op = static_cast<bitforge::byte_t>(b);
        if (bitforge::is_valid_opcode(op)) {
            const std::size_t len = bitforge::instruction_length(op);
            if (len == 1) len1_count++;
            else if (len == 2) len2_count++;
            else if (len == 3) len3_count++;
            else if (len == 4) len4_count++;
        }
    }

    // 5 one-byte instructions: NOP, HALT, INX, DEX, RET
    EXPECT_EQ(len1_count, 5);
    // 7 two-byte instructions: LDX, STX, INC, DEC, NOT, PUSH, POP
    EXPECT_EQ(len2_count, 7);
    // 17 three-byte instructions: LDI, MOV, LDI16, ADD, SUB, AND, OR, XOR, CMP, JMP, JZ, JNZ, JC, JNC, CALL, IN, OUT
    EXPECT_EQ(len3_count, 17);
    // 2 four-byte instructions: LD, ST
    EXPECT_EQ(len4_count, 2);

    EXPECT_EQ(len1_count + len2_count + len3_count + len4_count, 31);
}

TEST(TypesTest, ExhaustiveInvalidOpcodeRejection) {
    std::size_t valid_count = 0;
    std::size_t invalid_count = 0;

    for (int b = 0; b <= 0xFF; ++b) {
        const auto op = static_cast<bitforge::byte_t>(b);
        if (bitforge::is_valid_opcode(op)) {
            valid_count++;
        } else {
            invalid_count++;
            EXPECT_EQ(bitforge::instruction_length(op), 0);
            EXPECT_EQ(bitforge::opcode_to_mnemonic(op), "UNKNOWN");
        }
    }

    EXPECT_EQ(valid_count, 31);
    EXPECT_EQ(invalid_count, 256 - 31); // 225 undefined opcodes
    EXPECT_EQ(invalid_count, 225);

    // Explicit sample checks on gap boundaries
    EXPECT_FALSE(bitforge::is_valid_opcode(0x02));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x0F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x19));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x1F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x24));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x2F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x34));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x3F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x41));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x4F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x55));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x5F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x64));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x6F));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x72));
    EXPECT_FALSE(bitforge::is_valid_opcode(0x80));
    EXPECT_FALSE(bitforge::is_valid_opcode(0xFF));
}

TEST(TypesTest, ArchitecturalConstants) {
    EXPECT_EQ(bitforge::kMemorySize, 65536);
    EXPECT_EQ(bitforge::kMinAddress, 0x0000);
    EXPECT_EQ(bitforge::kMaxAddress, 0xFFFF);
    EXPECT_EQ(bitforge::kStackReservedStart, 0xF000);
    EXPECT_EQ(bitforge::kStackReservedEnd, 0xFFFE);
    EXPECT_EQ(bitforge::kStackResetValue, 0xFFFF);

    EXPECT_EQ(bitforge::kGeneralRegisterCount, 4);
    EXPECT_EQ(bitforge::kRegR0, 0);
    EXPECT_EQ(bitforge::kRegR1, 1);
    EXPECT_EQ(bitforge::kRegR2, 2);
    EXPECT_EQ(bitforge::kRegR3, 3);
    EXPECT_EQ(bitforge::kMaxRegisterId, 3);

    EXPECT_EQ(bitforge::kPortInputFifo, 0x00);
    EXPECT_EQ(bitforge::kPortTextConsole, 0x01);
    EXPECT_EQ(bitforge::kPortNumericConsole, 0x02);
    EXPECT_EQ(bitforge::kPortLeds, 0x10);

    EXPECT_EQ(bitforge::kDefaultInstructionBudget, 1'000'000);
    EXPECT_EQ(bitforge::kBenchmarkInstructionBudget, 100'000);
    EXPECT_EQ(bitforge::kMicroInstructionBudget, 1'000);

    EXPECT_EQ(bitforge::kCanonicalInstructionCount, 31);
    EXPECT_EQ(bitforge::kControlDataInstructionCount, 11);
    EXPECT_EQ(bitforge::kArithmeticLogicInstructionCount, 9);
    EXPECT_EQ(bitforge::kBranchStackCallInstructionCount, 9);
    EXPECT_EQ(bitforge::kIoInstructionCount, 2);
}

TEST(TypesTest, FlagsInitialStateAndEquality) {
    bitforge::Flags f1;
    EXPECT_FALSE(f1.z);
    EXPECT_FALSE(f1.c);
    EXPECT_FALSE(f1.n);
    EXPECT_FALSE(f1.v);

    f1.z = true;
    f1.c = true;
    bitforge::Flags f2;
    EXPECT_NE(f1, f2);

    f1.reset();
    EXPECT_EQ(f1, f2);
}

TEST(TypesTest, RegistersInitialStateAndReset) {
    bitforge::Registers regs;
    EXPECT_EQ(regs.r[0], 0);
    EXPECT_EQ(regs.r[1], 0);
    EXPECT_EQ(regs.r[2], 0);
    EXPECT_EQ(regs.r[3], 0);
    EXPECT_EQ(regs.ix, 0);
    EXPECT_EQ(regs.pc, 0x0000);
    EXPECT_EQ(regs.sp, 0xFFFF);

    regs.r[0] = 42;
    regs.ix = 0x1234;
    regs.pc = 0x5678;
    regs.sp = 0xF500;

    regs.reset(0x0200);
    EXPECT_EQ(regs.r[0], 0);
    EXPECT_EQ(regs.ix, 0);
    EXPECT_EQ(regs.pc, 0x0200);
    EXPECT_EQ(regs.sp, 0xFFFF);
}

TEST(TypesTest, EnumStrings) {
    EXPECT_EQ(bitforge::to_string(bitforge::MachineState::Ready), "Ready");
    EXPECT_EQ(bitforge::to_string(bitforge::MachineState::Running), "Running");
    EXPECT_EQ(bitforge::to_string(bitforge::MachineState::Paused), "Paused");
    EXPECT_EQ(bitforge::to_string(bitforge::MachineState::Halted), "Halted");
    EXPECT_EQ(bitforge::to_string(bitforge::MachineState::Faulted), "Faulted");

    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::None), "None");
    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::InvalidOpcode), "InvalidOpcode");
    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::InvalidRegisterEncoding), "InvalidRegisterEncoding");
    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::InvalidPort), "InvalidPort");
    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::StackOverflow), "StackOverflow");
    EXPECT_EQ(bitforge::to_string(bitforge::FaultType::StackUnderflow), "StackUnderflow");
}

TEST(TypesTest, InstructionInfoMetadata) {
    EXPECT_EQ(bitforge::kInstructionDescriptors.size(), 31u);

    // Verify all 31 descriptors have valid length, mnemonic, and description
    for (const auto& desc : bitforge::kInstructionDescriptors) {
        EXPECT_TRUE(bitforge::is_valid_opcode(desc.opcode));
        EXPECT_EQ(desc.length, bitforge::instruction_length(desc.opcode));
        EXPECT_EQ(desc.mnemonic, bitforge::opcode_to_mnemonic(desc.opcode));
        EXPECT_FALSE(desc.description.empty());
        EXPECT_FALSE(bitforge::to_string(desc.group).empty());

        auto found = bitforge::get_instruction_info(desc.opcode);
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->mnemonic, desc.mnemonic);

        auto found_by_mnem = bitforge::find_instruction_info_by_mnemonic(desc.mnemonic);
        ASSERT_TRUE(found_by_mnem.has_value());
        EXPECT_EQ(found_by_mnem->opcode, desc.opcode);
    }

    // Case-insensitive lookup check
    auto ldi_lower = bitforge::find_instruction_info_by_mnemonic("ldi");
    ASSERT_TRUE(ldi_lower.has_value());
    EXPECT_EQ(ldi_lower->opcode, 0x10);

    // Invalid opcode lookup returns nullopt
    EXPECT_FALSE(bitforge::get_instruction_info(0x02).has_value());
    EXPECT_FALSE(bitforge::get_instruction_info(0xFF).has_value());
    EXPECT_FALSE(bitforge::find_instruction_info_by_mnemonic("NONEXISTENT").has_value());
}
