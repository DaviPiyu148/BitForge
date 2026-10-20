#include "test_harness.hpp"
#include "bitforge/assembler/assembler.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/core/constants.hpp"

#include <vector>
#include <string>

// =============================================================
// Helper to extract a flat byte vector from all program segments
// =============================================================
static std::vector<bitforge::byte_t> extract_flat_bytes(const bitforge::assembler::ProgramImage& img) {
    std::vector<bitforge::byte_t> flat;
    for (const auto& seg : img.segments()) {
        flat.insert(flat.end(), seg.data.begin(), seg.data.end());
    }
    return flat;
}

// =============================================================
// 1. All 31 Instruction Encodings & Raw Byte Comparisons
// =============================================================

TEST(AssemblerTest, All31InstructionEncodings) {
    const std::string source = R"(
        NOP
        HALT
        LDI R0, 42
        MOV R1, R2
        LD R3, [0x8050]
        ST [0x8050], R0
        LDI16 IX, 0x1234
        LDX R1, [IX]
        STX [IX], R2
        INX
        DEX
        ADD R0, R1
        SUB R2, R3
        INC R1
        DEC R2
        AND R0, R3
        OR R1, R2
        XOR R3, R0
        NOT R2
        CMP R1, R3
        JMP 0x0500
        JZ 0x0600
        JNZ 0x0700
        JC 0x0800
        JNC 0x0900
        CALL 0x0A00
        RET
        PUSH R1
        POP R2
        IN R3, 0x00
        OUT 0x10, R0
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.diagnostics.size(), 0);

    const std::vector<bitforge::byte_t> expected = {
        // NOP, HALT
        0x00, 0x01,
        // LDI R0, 42
        0x10, 0x00, 42,
        // MOV R1, R2
        0x11, 0x01, 0x02,
        // LD R3, [0x8050]
        0x12, 0x03, 0x80, 0x50,
        // ST [0x8050], R0
        0x13, 0x80, 0x50, 0x00,
        // LDI16 IX, 0x1234
        0x14, 0x12, 0x34,
        // LDX R1, [IX]
        0x15, 0x01,
        // STX [IX], R2
        0x16, 0x02,
        // INX, DEX
        0x17, 0x18,
        // ADD R0, R1
        0x20, 0x00, 0x01,
        // SUB R2, R3
        0x21, 0x02, 0x03,
        // INC R1
        0x22, 0x01,
        // DEC R2
        0x23, 0x02,
        // AND R0, R3
        0x30, 0x00, 0x03,
        // OR R1, R2
        0x31, 0x01, 0x02,
        // XOR R3, R0
        0x32, 0x03, 0x00,
        // NOT R2
        0x33, 0x02,
        // CMP R1, R3
        0x40, 0x01, 0x03,
        // JMP 0x0500
        0x50, 0x05, 0x00,
        // JZ 0x0600
        0x51, 0x06, 0x00,
        // JNZ 0x0700
        0x52, 0x07, 0x00,
        // JC 0x0800
        0x53, 0x08, 0x00,
        // JNC 0x0900
        0x54, 0x09, 0x00,
        // CALL 0x0A00
        0x60, 0x0A, 0x00,
        // RET
        0x61,
        // PUSH R1
        0x62, 0x01,
        // POP R2
        0x63, 0x02,
        // IN R3, 0x00
        0x70, 0x03, 0x00,
        // OUT 0x10, R0
        0x71, 0x10, 0x00
    };

    auto flat = extract_flat_bytes(res.image);
    EXPECT_EQ(flat.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(flat[i], expected[i]);
    }
}

// =============================================================
// 2. Directives, Literals, Endianness, and Escape Sequences
// =============================================================

TEST(AssemblerTest, DirectiveByteAndWordAndAscii) {
    const std::string source = R"(
        .ORG 0x0200
        .BYTE 10, -5, 0xFF, 0b10101010
        .WORD 0x1234, -1, 300
        .ASCII "A\n\tB\"\\"
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0x0200);

    const std::vector<bitforge::byte_t> expected = {
        // .BYTE 10, -5, 0xFF, 0b10101010
        10, static_cast<bitforge::byte_t>(-5 & 0xFF), 0xFF, 0xAA,
        // .WORD 0x1234, -1, 300 (big-endian)
        0x12, 0x34,
        0xFF, 0xFF,
        0x01, 0x2C,
        // .ASCII "A\n\tB\"\\"
        'A', '\n', '\t', 'B', '"', '\\'
    };

    const auto& seg_data = res.image.segments()[0].data;
    EXPECT_EQ(seg_data.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(seg_data[i], expected[i]);
    }
}

TEST(AssemblerTest, CaseInsensitiveRegistersAndMnemonics) {
    const std::string source = R"(
        .org 0x0100
        ldi r0, 5
        mov R1, r0
        add r1, R0
        halt
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);

    const std::vector<bitforge::byte_t> expected = {
        0x10, 0x00, 5,       // LDI R0, 5
        0x11, 0x01, 0x00,    // MOV R1, R0
        0x20, 0x01, 0x00,    // ADD R1, R0
        0x01                 // HALT
    };

    auto flat = extract_flat_bytes(res.image);
    EXPECT_EQ(flat.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(flat[i], expected[i]);
    }
}

// =============================================================
// 3. Labels, Forward References, and .ENTRY Resolution
// =============================================================

TEST(AssemblerTest, LabelsAndForwardReferences) {
    const std::string source = R"(
        .ORG 0x0200
        .ENTRY start

        target:
            HALT

        start:
            LDI R0, 1
            JZ target
            JMP forward_target

        forward_target:
            INC R0
            JMP target
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.entry_point(), 0x0201); // start is at 0x0201 (target has HALT at 0x0200)

    EXPECT_TRUE(res.symbols.contains("target"));
    EXPECT_TRUE(res.symbols.contains("start"));
    EXPECT_TRUE(res.symbols.contains("forward_target"));

    EXPECT_EQ(res.symbols.lookup("target")->address, 0x0200);
    EXPECT_EQ(res.symbols.lookup("start")->address, 0x0201);
    EXPECT_EQ(res.symbols.lookup("forward_target")->address, 0x020A); // 0x0201 + 3(LDI) + 3(JZ) + 3(JMP) = 0x020A
}

// =============================================================
// 4. Multiple Segments and Overlap Rejection
// =============================================================

TEST(AssemblerTest, MultipleDisjointSegments) {
    const std::string source = R"(
        .ORG 0x0200
        start:
            LDI R0, 42
            HALT

        .ORG 0x8000
        data:
            .BYTE 1, 2, 3, 4
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.segments().size(), 2);

    EXPECT_EQ(res.image.segments()[0].start_address, 0x0200);
    EXPECT_EQ(res.image.segments()[0].data.size(), 4); // LDI(3) + HALT(1)

    EXPECT_EQ(res.image.segments()[1].start_address, 0x8000);
    EXPECT_EQ(res.image.segments()[1].data.size(), 4);
}

TEST(AssemblerTest, RejectOverlappingSegments) {
    const std::string source = R"(
        .ORG 0x0200
            .BYTE 1, 2, 3, 4, 5

        .ORG 0x0202
            .BYTE 99, 100
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.diagnostics.size() > 0);

    bool found_overlap_error = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("Overlapping segment") != std::string::npos) {
            found_overlap_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_overlap_error);
}

// =============================================================
// 5. Stack Region Emission Rejection (0xF000 and above)
// =============================================================

TEST(AssemblerTest, RejectStackRegionEmission) {
    const std::string source = R"(
        .ORG 0xF000
        start:
            NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_stack_error = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("stack") != std::string::npos ||
            diag.message.find("0xF000") != std::string::npos) {
            found_stack_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_stack_error);
}

TEST(AssemblerTest, RejectEntryInStackRegion) {
    const std::string source = R"(
        .ENTRY 0xF100
        start:
            NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_entry_error = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("stack") != std::string::npos ||
            diag.message.find("Entry") != std::string::npos) {
            found_entry_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_entry_error);
}

// =============================================================
// 6. Diagnostics, Error Reporting, and Source Positions
// =============================================================

TEST(AssemblerTest, DuplicateLabelError) {
    const std::string source = R"(
        my_label:
            NOP
        my_label:
            HALT
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_dup = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("Duplicate label") != std::string::npos) {
            found_dup = true;
            EXPECT_EQ(diag.location.line, 4);
            break;
        }
    }
    EXPECT_TRUE(found_dup);
}

TEST(AssemblerTest, UnresolvedLabelError) {
    const std::string source = R"(
        start:
            JMP nowhere
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_unres = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("Unresolved label") != std::string::npos) {
            found_unres = true;
            EXPECT_EQ(diag.location.line, 3);
            break;
        }
    }
    EXPECT_TRUE(found_unres);
}

TEST(AssemblerTest, InvalidRegisterDiagnostic) {
    const std::string source = R"(
        LDI R4, 10
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_reg_err = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("Invalid") != std::string::npos &&
            diag.message.find("register") != std::string::npos) {
            found_reg_err = true;
            EXPECT_EQ(diag.location.line, 2);
            break;
        }
    }
    EXPECT_TRUE(found_reg_err);
}

TEST(AssemblerTest, OutOfRangeImmediateDiagnostic) {
    const std::string source = R"(
        LDI R0, 300
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_range_err = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("out of range") != std::string::npos) {
            found_range_err = true;
            EXPECT_EQ(diag.location.line, 2);
            break;
        }
    }
    EXPECT_TRUE(found_range_err);
}

TEST(AssemblerTest, CrlfAndLfNormalizationIdenticalOutput) {
    const std::string lf_source = ".ORG 0x0200\nLDI R0, 10\nHALT\n";
    const std::string crlf_source = ".ORG 0x0200\r\nLDI R0, 10\r\nHALT\r\n";

    bitforge::assembler::Assembler assem;
    auto res_lf = assem.assemble(lf_source);
    auto res_crlf = assem.assemble(crlf_source);

    EXPECT_TRUE(res_lf.success);
    EXPECT_TRUE(res_crlf.success);

    auto bytes_lf = extract_flat_bytes(res_lf.image);
    auto bytes_crlf = extract_flat_bytes(res_crlf.image);

    EXPECT_EQ(bytes_lf.size(), bytes_crlf.size());
    for (std::size_t i = 0; i < bytes_lf.size(); ++i) {
        EXPECT_EQ(bytes_lf[i], bytes_crlf[i]);
    }
}

// =============================================================
// 7. End-to-End Program Assembly and Execution via CPU
// =============================================================

TEST(AssemblerTest, EndToEndArithmeticBenchmarkProgram) {
    // Arithmetic Baseline (spec Section 12.2.4: 10 + 20 = 30 stored at 0x8000)
    const std::string source = R"(
        .ORG 0x0200
        .ENTRY start

        start:
            LDI R0, 10
            LDI R1, 20
            ADD R0, R1
            ST [0x8000], R0
            HALT
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.entry_point(), 0x0200);

    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);
    bitforge::Cpu cpu(bus);

    EXPECT_TRUE(res.image.load_into(memory));
    cpu.reset(res.image.entry_point());

    auto run_res = bitforge::test::run_with_budget(cpu, 100);
    EXPECT_TRUE(run_res.success);
    EXPECT_EQ(run_res.stop_reason, "Halted");
    EXPECT_EQ(cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(cpu.registers().r[0], 0x1E); // 30
    EXPECT_EQ(memory.read_byte(0x8000), 0x1E);
    EXPECT_FALSE(cpu.flags().z);
    EXPECT_FALSE(cpu.flags().c);
    EXPECT_FALSE(cpu.flags().n);
    EXPECT_FALSE(cpu.flags().v);
}

TEST(AssemblerTest, EndToEndSubroutineAndIoProgram) {
    // Subroutine doubling a FIFO input byte and printing to consoles and LEDs
    const std::string source = R"(
        .ORG 0x0200
        .ENTRY main

        main:
            IN R0, 0x00        ; read from FIFO
            CALL double_val     ; double value via subroutine
            OUT 0x02, R0       ; numeric console
            OUT 0x10, R0       ; virtual LEDs
            LDI R1, '!'
            OUT 0x01, R1       ; text console
            HALT

        double_val:
            PUSH R1
            MOV R1, R0
            ADD R0, R1
            POP R1
            RET
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.entry_point(), 0x0200);

    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);
    bitforge::Cpu cpu(bus);

    bus.push_input_fifo(15); // Input: 15

    EXPECT_TRUE(res.image.load_into(memory));
    cpu.reset(res.image.entry_point());

    auto run_res = bitforge::test::run_with_budget(cpu, 100);
    EXPECT_TRUE(run_res.success);
    EXPECT_EQ(run_res.stop_reason, "Halted");
    EXPECT_EQ(cpu.state(), bitforge::MachineState::Halted);
    EXPECT_EQ(cpu.registers().r[0], 30); // 15 * 2 = 30
    EXPECT_EQ(cpu.registers().sp, 0xFFFF); // Stack fully balanced
    EXPECT_EQ(bus.led_state(), 30);
    EXPECT_EQ(bus.numeric_output(), "30");
    EXPECT_EQ(bus.text_output(), "!");
}

// =============================================================
// 7. Phase 3.1 Conformance and Edge-Case Audit Tests
// =============================================================

TEST(AssemblerTest, RejectExplicitEntryInUninitializedGap) {
    const std::string source = R"(
        .ORG 0x0100
        NOP
        .ENTRY 0x0300
        .ORG 0x0500
        NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_gap_error = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("uninitialized memory gap") != std::string::npos ||
            diag.message.find("does not point to an emitted byte") != std::string::npos) {
            found_gap_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_gap_error);
}

TEST(AssemblerTest, RejectExplicitLabelEntryAtEndOfFileGap) {
    const std::string source = R"(
        .ORG 0x0100
        NOP
        end_label:
        .ENTRY end_label
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_gap_error = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("uninitialized memory gap") != std::string::npos ||
            diag.message.find("does not point to an emitted byte") != std::string::npos) {
            found_gap_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_gap_error);
}

TEST(AssemblerTest, RejectDuplicateEntryDirective) {
    const std::string source = R"(
        .ORG 0x0100
        .ENTRY 0x0100
        .ENTRY 0x0100
        NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_dup_entry = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.message.find("Duplicate .ENTRY") != std::string::npos) {
            found_dup_entry = true;
            break;
        }
    }
    EXPECT_TRUE(found_dup_entry);
}

TEST(AssemblerTest, DefaultEntryBehaviorAtZero) {
    const std::string source = R"(
        NOP
        HALT
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.entry_point(), 0x0000);
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0x0000);
}

TEST(AssemblerTest, EmptyProgramAndWhitespaceDiagnostics) {
    bitforge::assembler::Assembler assem;

    // Completely empty
    auto res1 = assem.assemble("");
    EXPECT_FALSE(res1.success);
    bool found_empty1 = false;
    for (const auto& d : res1.diagnostics) {
        if (d.message.find("Empty program") != std::string::npos) found_empty1 = true;
    }
    EXPECT_TRUE(found_empty1);

    // Whitespace only
    auto res2 = assem.assemble("   \t  \r\n  \n  ");
    EXPECT_FALSE(res2.success);
    bool found_empty2 = false;
    for (const auto& d : res2.diagnostics) {
        if (d.message.find("Empty program") != std::string::npos) found_empty2 = true;
    }
    EXPECT_TRUE(found_empty2);

    // Comments only
    auto res3 = assem.assemble("; Just a comment\n; Another comment\n");
    EXPECT_FALSE(res3.success);
    bool found_empty3 = false;
    for (const auto& d : res3.diagnostics) {
        if (d.message.find("Empty program") != std::string::npos) found_empty3 = true;
    }
    EXPECT_TRUE(found_empty3);
}

TEST(AssemblerTest, ProgramContainingOnlyDataDiagnostic) {
    const std::string source = R"(
        .ORG 0x0000
        .BYTE 1, 2, 3
        .WORD 0x1234
        .ASCII "DATA"
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0x0000);

    bool found_data_warning = false;
    for (const auto& d : res.diagnostics) {
        if (d.severity == bitforge::assembler::DiagnosticSeverity::Warning &&
            d.message.find("only data declarations") != std::string::npos) {
            found_data_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_data_warning);
}

TEST(AssemblerTest, ForwardAndBackwardOrgSegments) {
    const std::string source = R"(
        .ORG 0x0500
        .BYTE 0xAA, 0xBB

        .ORG 0x0200
        .BYTE 0x11, 0x22

        .ORG 0x0800
        .BYTE 0x99
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);

    // Segments must be sorted in ascending address order
    const auto& segs = res.image.segments();
    EXPECT_EQ(segs.size(), 3);
    EXPECT_EQ(segs[0].start_address, 0x0200);
    EXPECT_EQ(segs[0].data.size(), 2);
    EXPECT_EQ(segs[0].data[0], 0x11);
    EXPECT_EQ(segs[0].data[1], 0x22);

    EXPECT_EQ(segs[1].start_address, 0x0500);
    EXPECT_EQ(segs[1].data.size(), 2);
    EXPECT_EQ(segs[1].data[0], 0xAA);
    EXPECT_EQ(segs[1].data[1], 0xBB);

    EXPECT_EQ(segs[2].start_address, 0x0800);
    EXPECT_EQ(segs[2].data.size(), 1);
    EXPECT_EQ(segs[2].data[0], 0x99);
}

TEST(AssemblerTest, SegmentEndingExactlyAtPermittedBoundary) {
    // 0xEFFF is the last valid address before reserved stack region (0xF000..0xFFFF)
    const std::string source = R"(
        .ORG 0xEFFF
        NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0xEFFF);
    EXPECT_EQ(res.image.segments()[0].data.size(), 1);
    EXPECT_EQ(res.image.segments()[0].data[0], 0x00); // NOP
}

TEST(AssemblerTest, EmissionCrossingIntoReservedStackRegion) {
    // Emitting a 2-byte word starting at 0xEFFF means byte 2 lands at 0xF000
    const std::string source = R"(
        .ORG 0xEFFF
        .WORD 0x1234
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_stack_error = false;
    for (const auto& d : res.diagnostics) {
        if (d.message.find("reserved stack region") != std::string::npos ||
            d.message.find("0xF000") != std::string::npos) {
            found_stack_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_stack_error);
}

TEST(AssemblerTest, InstructionCrossingIntoReservedStackRegion) {
    // LDI R0, 42 is 3 bytes (0xEFFF, 0xF000, 0xF001)
    const std::string source = R"(
        .ORG 0xEFFF
        LDI R0, 42
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_FALSE(res.success);

    bool found_stack_error = false;
    for (const auto& d : res.diagnostics) {
        if (d.message.find("reserved stack region") != std::string::npos ||
            d.message.find("0xF000") != std::string::npos) {
            found_stack_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_stack_error);
}

TEST(AssemblerTest, LabelsImmediatelyBeforeAndAfterDirectives) {
    const std::string source = R"(
        .ORG 0x0100
        label_before:
            .BYTE 0x42
        label_between:
            .ORG 0x0300
        label_after:
            NOP
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);

    const auto* sym_before = res.symbols.lookup("label_before");
    const auto* sym_between = res.symbols.lookup("label_between");
    const auto* sym_after = res.symbols.lookup("label_after");

    EXPECT_NE(sym_before, nullptr);
    EXPECT_NE(sym_between, nullptr);
    EXPECT_NE(sym_after, nullptr);

    EXPECT_EQ(sym_before->address, 0x0100);
    EXPECT_EQ(sym_between->address, 0x0101);
    EXPECT_EQ(sym_after->address, 0x0300);
}

TEST(AssemblerTest, UnterminatedStringDiagnostics) {
    bitforge::assembler::Assembler assem;

    // Unterminated with newline
    auto res1 = assem.assemble(".ASCII \"unterminated\nNOP\n");
    EXPECT_FALSE(res1.success);
    bool found_unterm1 = false;
    for (const auto& d : res1.diagnostics) {
        if (d.message.find("Unterminated string") != std::string::npos) found_unterm1 = true;
    }
    EXPECT_TRUE(found_unterm1);

    // Unterminated at EOF
    auto res2 = assem.assemble(".ASCII \"unterminated_at_eof");
    EXPECT_FALSE(res2.success);
    bool found_unterm2 = false;
    for (const auto& d : res2.diagnostics) {
        if (d.message.find("Unterminated string") != std::string::npos) found_unterm2 = true;
    }
    EXPECT_TRUE(found_unterm2);
}

TEST(AssemblerTest, InvalidEscapeSequenceDiagnostic) {
    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(".ASCII \"bad \\k escape\"\n");
    EXPECT_FALSE(res.success);

    bool found_escape_err = false;
    for (const auto& d : res.diagnostics) {
        if (d.message.find("Unknown escape sequence") != std::string::npos) {
            found_escape_err = true;
            break;
        }
    }
    EXPECT_TRUE(found_escape_err);
}

TEST(AssemblerTest, InvalidAndOverflowingNumericLiterals) {
    bitforge::assembler::Assembler assem;

    // Decimal overflow
    auto res1 = assem.assemble("LDI R0, 9999999999999999999999999999999999999999\n");
    EXPECT_FALSE(res1.success);
    bool found_ovf1 = false;
    for (const auto& d : res1.diagnostics) {
        if (d.message.find("representable integer range") != std::string::npos) found_ovf1 = true;
    }
    EXPECT_TRUE(found_ovf1);

    // Malformed hex
    auto res2 = assem.assemble("LDI R0, 0x\n");
    EXPECT_FALSE(res2.success);

    // Invalid hex digit
    auto res3 = assem.assemble("LDI R0, 0x12G4\n");
    EXPECT_FALSE(res3.success);

    // Malformed binary
    auto res4 = assem.assemble("LDI R0, 0b\n");
    EXPECT_FALSE(res4.success);

    // Invalid binary digit
    auto res5 = assem.assemble("LDI R0, 0b1021\n");
    EXPECT_FALSE(res5.success);

    // Invalid decimal suffix
    auto res6 = assem.assemble("LDI R0, 123abc\n");
    EXPECT_FALSE(res6.success);
}

TEST(AssemblerTest, MissingOperandsDiagnostics) {
    bitforge::assembler::Assembler assem;

    // Missing operands for instructions
    EXPECT_FALSE(assem.assemble("LDI\n").success);
    EXPECT_FALSE(assem.assemble("LDI R0,\n").success);
    EXPECT_FALSE(assem.assemble("MOV\n").success);
    EXPECT_FALSE(assem.assemble("MOV R0,\n").success);
    EXPECT_FALSE(assem.assemble("INC\n").success);
    EXPECT_FALSE(assem.assemble("IN\n").success);
    EXPECT_FALSE(assem.assemble("IN R0,\n").success);
    EXPECT_FALSE(assem.assemble("OUT\n").success);
    EXPECT_FALSE(assem.assemble("OUT 0x01,\n").success);
    EXPECT_FALSE(assem.assemble("JMP\n").success);

    // Missing operands for directives
    EXPECT_FALSE(assem.assemble(".ORG\n").success);
    EXPECT_FALSE(assem.assemble(".ENTRY\n").success);
    EXPECT_FALSE(assem.assemble(".BYTE\n").success);
    EXPECT_FALSE(assem.assemble(".WORD\n").success);
    EXPECT_FALSE(assem.assemble(".ASCII\n").success);
}

TEST(AssemblerTest, UnexpectedTrailingTokensDiagnostic) {
    bitforge::assembler::Assembler assem;

    auto res1 = assem.assemble("NOP HALT\n");
    EXPECT_FALSE(res1.success);
    bool found_trail1 = false;
    for (const auto& d : res1.diagnostics) {
        if (d.message.find("Unexpected trailing token") != std::string::npos) found_trail1 = true;
    }
    EXPECT_TRUE(found_trail1);

    auto res2 = assem.assemble("LDI R0, 42 extra_word\n");
    EXPECT_FALSE(res2.success);

    auto res3 = assem.assemble("RET 123\n");
    EXPECT_FALSE(res3.success);
}

TEST(AssemblerTest, StandaloneCarriageReturnLineEndings) {
    const std::string cr_source = ".ORG 0x0200\rLDI R0, 10\rHALT\r";
    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(cr_source);

    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0x0200);
    EXPECT_EQ(res.image.segments()[0].data.size(), 4); // LDI(3) + HALT(1)
}

TEST(AssemblerTest, DiagnosticSourceLocationsPrecision) {
    const std::string source = "NOP\nINVALID_MNEM\n";
    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);

    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.diagnostics.empty());
    EXPECT_EQ(res.diagnostics[0].location.line, 2);
    EXPECT_GT(res.diagnostics[0].location.column, 0);
}

TEST(AssemblerTest, ProgramImageLoadIntoSeparation) {
    const std::string source = R"(
        .ENTRY start
        .ORG 0x0100
        start:
            LDI R0, 99
            HALT
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);

    bitforge::Memory memory;
    bitforge::SystemBus bus(memory);
    bitforge::Cpu cpu(bus);

    // Initial CPU state before loading
    EXPECT_EQ(cpu.state(), bitforge::MachineState::Ready);
    EXPECT_EQ(cpu.registers().pc, 0x0000);
    EXPECT_EQ(cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(cpu.registers().r[0], 0);

    // load_into mutates ONLY memory
    EXPECT_TRUE(res.image.load_into(memory));

    // Verify memory was updated
    EXPECT_EQ(memory.read_byte(0x0100), 0x10); // LDI opcode

    // Verify CPU was NOT mutated by load_into
    EXPECT_EQ(cpu.state(), bitforge::MachineState::Ready);
    EXPECT_EQ(cpu.registers().pc, 0x0000);
    EXPECT_EQ(cpu.registers().sp, 0xFFFF);
    EXPECT_EQ(cpu.registers().r[0], 0);

    // Separate explicit CPU reset
    cpu.reset(res.image.entry_point());
    EXPECT_EQ(cpu.registers().pc, 0x0100);
}

// =============================================================
// 8. Phase 4 Comprehensive Verification Matrix
// =============================================================

TEST(AssemblerTest, InstructionEncodingRegisterMatrix) {
    const std::string source = R"(
        .ORG 0x0200
        ; Group 2: INC, DEC, NOT, PUSH, POP across R0..R3
        INC R0
        INC R1
        INC R2
        INC R3
        DEC R0
        DEC R1
        DEC R2
        DEC R3
        NOT R0
        NOT R1
        NOT R2
        NOT R3
        PUSH R0
        PUSH R1
        PUSH R2
        PUSH R3
        POP R0
        POP R1
        POP R2
        POP R3

        ; Group 3: MOV, ADD, SUB, AND, OR, XOR, CMP across register pairs
        MOV R0, R1
        MOV R2, R3
        MOV R3, R0
        ADD R0, R2
        ADD R1, R3
        SUB R2, R0
        SUB R3, R1
        AND R0, R1
        AND R2, R3
        OR R1, R0
        OR R3, R2
        XOR R0, R3
        XOR R2, R1
        CMP R1, R2
        CMP R3, R0

        ; Group 4: LDI Rd, imm8
        LDI R0, -128
        LDI R1, -1
        LDI R2, 0
        LDI R3, 255

        ; Group 5: LDI16 IX, addr16
        LDI16 IX, 0x0000
        LDI16 IX, 0xBEEF

        ; Group 6: LD Rd, [addr16]
        LD R0, [0x0000]
        LD R1, [0x1234]
        LD R2, [0x8000]
        LD R3, [0xEFFF]

        ; Group 7: ST [addr16], Rs
        ST [0x0000], R0
        ST [0x1234], R1
        ST [0x8000], R2
        ST [0xEFFF], R3

        ; Group 8: LDX Rd, [IX]
        LDX R0, [IX]
        LDX R1, [IX]
        LDX R2, [IX]
        LDX R3, [IX]

        ; Group 9: STX [IX], Rs
        STX [IX], R0
        STX [IX], R1
        STX [IX], R2
        STX [IX], R3

        ; Group 10: JMP, JZ, JNZ, JC, JNC, CALL
        JMP 0x1000
        JZ 0x2000
        JNZ 0x3000
        JC 0x4000
        JNC 0x5000
        CALL 0x6000

        ; Group 11: IN Rd, port8
        IN R0, 0x00
        IN R1, 0x01
        IN R2, 0x02
        IN R3, 0x10

        ; Group 12: OUT port8, Rs
        OUT 0x00, R0
        OUT 0x01, R1
        OUT 0x02, R2
        OUT 0x10, R3
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.diagnostics.size(), 0);

    const std::vector<bitforge::byte_t> expected = {
        // Group 2: INC, DEC, NOT, PUSH, POP
        0x22, 0x00,  0x22, 0x01,  0x22, 0x02,  0x22, 0x03,
        0x23, 0x00,  0x23, 0x01,  0x23, 0x02,  0x23, 0x03,
        0x33, 0x00,  0x33, 0x01,  0x33, 0x02,  0x33, 0x03,
        0x62, 0x00,  0x62, 0x01,  0x62, 0x02,  0x62, 0x03,
        0x63, 0x00,  0x63, 0x01,  0x63, 0x02,  0x63, 0x03,

        // Group 3: MOV, ADD, SUB, AND, OR, XOR, CMP
        0x11, 0x00, 0x01,
        0x11, 0x02, 0x03,
        0x11, 0x03, 0x00,
        0x20, 0x00, 0x02,
        0x20, 0x01, 0x03,
        0x21, 0x02, 0x00,
        0x21, 0x03, 0x01,
        0x30, 0x00, 0x01,
        0x30, 0x02, 0x03,
        0x31, 0x01, 0x00,
        0x31, 0x03, 0x02,
        0x32, 0x00, 0x03,
        0x32, 0x02, 0x01,
        0x40, 0x01, 0x02,
        0x40, 0x03, 0x00,

        // Group 4: LDI Rd, imm8
        0x10, 0x00, 0x80,
        0x10, 0x01, 0xFF,
        0x10, 0x02, 0x00,
        0x10, 0x03, 0xFF,

        // Group 5: LDI16 IX, addr16
        0x14, 0x00, 0x00,
        0x14, 0xBE, 0xEF,

        // Group 6: LD Rd, [addr16]
        0x12, 0x00, 0x00, 0x00,
        0x12, 0x01, 0x12, 0x34,
        0x12, 0x02, 0x80, 0x00,
        0x12, 0x03, 0xEF, 0xFF,

        // Group 7: ST [addr16], Rs
        0x13, 0x00, 0x00, 0x00,
        0x13, 0x12, 0x34, 0x01,
        0x13, 0x80, 0x00, 0x02,
        0x13, 0xEF, 0xFF, 0x03,

        // Group 8: LDX Rd, [IX]
        0x15, 0x00,
        0x15, 0x01,
        0x15, 0x02,
        0x15, 0x03,

        // Group 9: STX [IX], Rs
        0x16, 0x00,
        0x16, 0x01,
        0x16, 0x02,
        0x16, 0x03,

        // Group 10: JMP, JZ, JNZ, JC, JNC, CALL
        0x50, 0x10, 0x00,
        0x51, 0x20, 0x00,
        0x52, 0x30, 0x00,
        0x53, 0x40, 0x00,
        0x54, 0x50, 0x00,
        0x60, 0x60, 0x00,

        // Group 11: IN Rd, port8
        0x70, 0x00, 0x00,
        0x70, 0x01, 0x01,
        0x70, 0x02, 0x02,
        0x70, 0x03, 0x10,

        // Group 12: OUT port8, Rs
        0x71, 0x00, 0x00,
        0x71, 0x01, 0x01,
        0x71, 0x02, 0x02,
        0x71, 0x10, 0x03
    };

    auto flat = extract_flat_bytes(res.image);
    EXPECT_EQ(flat.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(flat[i], expected[i]);
    }
}

TEST(AssemblerTest, ExpressionsAndSymbolArithmetic) {
    const std::string source = R"(
        .ORG 0x0200
        base:
            NOP
        data_block:
            .BYTE 10 + 20, 50 - 8, -5 + 10, 0x10 + 0x20
            .WORD base + 0x100, 0x1000 - 0x0001
        start:
            LDI R0, 5 + 15
            LDI R1, 100 - 40
            LD R2, [data_block + 2]
            JMP start + 3
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.diagnostics.size(), 0);

    const std::vector<bitforge::byte_t> expected = {
        // NOP at 0x0200
        0x00,
        // .BYTE 30, 42, 5, 48 at 0x0201
        30, 42, 5, 0x30,
        // .WORD 0x0200 + 0x0100 = 0x0300 at 0x0205
        0x03, 0x00,
        // .WORD 0x1000 - 0x0001 = 0x0FFF at 0x0207
        0x0F, 0xFF,
        // LDI R0, 20 at 0x0209
        0x10, 0x00, 20,
        // LDI R1, 60 at 0x020C
        0x10, 0x01, 60,
        // LD R2, [data_block + 2] = [0x0203] at 0x020F
        0x12, 0x02, 0x02, 0x03,
        // JMP start + 3 = 0x0209 + 3 = 0x020C at 0x0213
        0x50, 0x02, 0x0C
    };

    auto flat = extract_flat_bytes(res.image);
    EXPECT_EQ(flat.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(flat[i], expected[i]);
    }
}

TEST(AssemblerTest, SymbolResolutionCaseSensitivityAndShadowing) {
    const std::string source = R"(
        .ORG 0x0200
        .ENTRY Start_Label

        Target_Label:
            NOP

        Start_Label:
            JZ TARGET_LABEL
            JNZ target_label
            JMP Target_Label
    )";

    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.image.entry_point(), 0x0201);
    EXPECT_TRUE(res.symbols.contains("TARGET_LABEL"));
    EXPECT_TRUE(res.symbols.contains("target_label"));
    EXPECT_TRUE(res.symbols.contains("Target_Label"));
    EXPECT_EQ(res.symbols.lookup("TARGET_LABEL")->address, 0x0200);
    EXPECT_EQ(res.symbols.lookup("target_label")->address, 0x0200);

    // Duplicate label with differing casing must be rejected
    const std::string dup_source = R"(
        my_loop:
            NOP
        MY_LOOP:
            HALT
    )";
    auto dup_res = assem.assemble(dup_source);
    EXPECT_FALSE(dup_res.success);
    bool found_dup = false;
    for (const auto& d : dup_res.diagnostics) {
        if (d.message.find("Duplicate label") != std::string::npos) {
            found_dup = true;
            break;
        }
    }
    EXPECT_TRUE(found_dup);
}

TEST(AssemblerTest, StackBoundaryCollisionExhaustiveMatrix) {
    bitforge::assembler::Assembler assem;

    // 1. Exact-fit tests ending exactly at 0xEFFF:
    // 1-byte instruction at 0xEFFF
    {
        auto res = assem.assemble(".ORG 0xEFFF\nNOP\n");
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.image.segments().size(), 1);
        EXPECT_EQ(res.image.segments()[0].start_address, 0xEFFF);
        EXPECT_EQ(res.image.segments()[0].data.size(), 1);
        EXPECT_EQ(res.image.segments()[0].data[0], 0x00);
    }
    // 2-byte instruction at 0xEFFE
    {
        auto res = assem.assemble(".ORG 0xEFFE\nINC R0\n");
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.image.segments().size(), 1);
        EXPECT_EQ(res.image.segments()[0].start_address, 0xEFFE);
        EXPECT_EQ(res.image.segments()[0].data.size(), 2);
    }
    // 3-byte instruction at 0xEFFD
    {
        auto res = assem.assemble(".ORG 0xEFFD\nLDI R0, 1\n");
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.image.segments().size(), 1);
        EXPECT_EQ(res.image.segments()[0].start_address, 0xEFFD);
        EXPECT_EQ(res.image.segments()[0].data.size(), 3);
    }
    // 4-byte instruction at 0xEFFC
    {
        auto res = assem.assemble(".ORG 0xEFFC\nLD R0, [0x1000]\n");
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.image.segments().size(), 1);
        EXPECT_EQ(res.image.segments()[0].start_address, 0xEFFC);
        EXPECT_EQ(res.image.segments()[0].data.size(), 4);
    }

    // 2. Overrun crossing into 0xF000:
    // 2-byte instruction at 0xEFFF (byte 2 lands at 0xF000)
    {
        auto res = assem.assemble(".ORG 0xEFFF\nINC R0\n");
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);
        bool found = false;
        for (const auto& d : res.diagnostics) {
            if (d.message.find("0xF000") != std::string::npos || d.message.find("stack") != std::string::npos) {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
    // 3-byte instruction at 0xEFFE (byte 3 lands at 0xF000)
    {
        auto res = assem.assemble(".ORG 0xEFFE\nLDI R0, 1\n");
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);
        bool found = false;
        for (const auto& d : res.diagnostics) {
            if (d.message.find("0xF000") != std::string::npos || d.message.find("stack") != std::string::npos) {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
    // 4-byte instruction at 0xEFFD (byte 4 lands at 0xF000)
    {
        auto res = assem.assemble(".ORG 0xEFFD\nLD R0, [0x1000]\n");
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);
        bool found = false;
        for (const auto& d : res.diagnostics) {
            if (d.message.find("0xF000") != std::string::npos || d.message.find("stack") != std::string::npos) {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
    // Direct .ORG at 0xF000
    {
        auto res = assem.assemble(".ORG 0xF000\nNOP\n");
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);
    }
    // Direct .ORG at 0xFFFF
    {
        auto res = assem.assemble(".ORG 0xFFFF\nNOP\n");
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);
    }
    // Earlier valid segment followed by stack overrun: verifies that no partial image is returned
    {
        const std::string multi_seg = R"(
            .ORG 0x0200
            start:
                NOP
            .ORG 0xEFFF
                INC R0
        )";
        auto res = assem.assemble(multi_seg);
        EXPECT_FALSE(res.success);
        EXPECT_TRUE(res.image.segments().empty());
        EXPECT_EQ(res.image.total_bytes(), 0);

        // Loading a failed assembly image into memory mutates nothing
        bitforge::Memory mem;
        EXPECT_TRUE(res.image.load_into(mem));
        EXPECT_EQ(mem.read_byte(0x0200), 0x00);
        EXPECT_EQ(mem.read_byte(0xEFFF), 0x00);
        EXPECT_EQ(mem.read_byte(0xF000), 0x00);

        bool found = false;
        for (const auto& d : res.diagnostics) {
            if (d.message.find("0xF000") != std::string::npos || d.message.find("stack") != std::string::npos) {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
}

TEST(AssemblerTest, MalformedInputComprehensiveMatrix) {
    bitforge::assembler::Assembler assem;
    const std::vector<std::string> malformed_snippets = {
        "INVALID_OPCODE\n",
        "MOV R0\n",
        "MOV R0, R4\n",
        "MOV R4, R0\n",
        "MOV R0 R1\n",
        "LDI R0\n",
        "LDI R0,\n",
        "LDI R0, 256\n",
        "LDI R0, -129\n",
        "LDI16 R0, 0x1000\n",
        "LDI16 IX\n",
        "LD R0, 0x1000\n",
        "LD R0, [0x1000\n",
        "LD R4, [0x1000]\n",
        "ST 0x1000, R0\n",
        "ST [0x1000], R4\n",
        "LDX R0, [R1]\n",
        "STX [R0], R1\n",
        "INC R4\n",
        "DEC R4\n",
        "PUSH R4\n",
        "POP R4\n",
        "IN R0, 256\n",
        "OUT 256, R0\n",
        "IN R4, 0x01\n",
        "OUT 0x01, R4\n",
        ".BYTE 256\n",
        ".BYTE -129\n",
        ".WORD 65536\n",
        ".WORD -32769\n",
        ".ORG 0x10000\n"
    };

    for (const auto& snippet : malformed_snippets) {
        auto res = assem.assemble(snippet);
        EXPECT_FALSE(res.success);
        EXPECT_FALSE(res.diagnostics.empty());
    }
}

TEST(AssemblerTest, ExamplesAssemblyAndExecutionVerification) {
    // 1. Example 01: 01_arithmetic.asm
    {
        const std::string ex01 = R"(
            ; BitForge Example 01: Arithmetic Baseline
            ; Computes 10 + 20 and stores the result (0x1E / 30) at address 0x8000

            .ORG 0x0200
            .ENTRY start

            start:
                LDI R0, 10         ; R0 = 10 (0x0A)
                LDI R1, 20         ; R1 = 20 (0x14)
                ADD R0, R1         ; R0 = R0 + R1 = 30 (0x1E)
                ST [0x8000], R0    ; memory[0x8000] = 30
                HALT               ; halt execution
        )";
        bitforge::assembler::Assembler assem;
        auto res = assem.assemble(ex01);
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.diagnostics.size(), 0);
        EXPECT_EQ(res.image.entry_point(), 0x0200);

        bitforge::Memory mem;
        bitforge::SystemBus bus(mem);
        bitforge::Cpu cpu(bus);

        EXPECT_TRUE(res.image.load_into(mem));
        cpu.reset(res.image.entry_point());

        auto run_res = bitforge::test::run_with_budget(cpu, 100);
        EXPECT_TRUE(run_res.success);
        EXPECT_EQ(cpu.state(), bitforge::MachineState::Halted);
        EXPECT_EQ(cpu.registers().r[0], 30);
        EXPECT_EQ(mem.read_byte(0x8000), 30);
    }

    // 2. Example 02: 02_io_demo.asm
    {
        const std::string ex02 = R"(
            ; BitForge Example 02: Virtual I/O Demonstration
            ; Demonstrates text output (Port 0x01), numeric output (Port 0x02), and LEDs (Port 0x10)

            .ORG 0x0200
            .ENTRY main

            main:
                ; Text console: Output "HI!"
                LDI R0, 'H'
                OUT 0x01, R0
                LDI R0, 'I'
                OUT 0x01, R0
                LDI R0, '!'
                OUT 0x01, R0

                ; Numeric console: Output 42
                LDI R0, 42
                OUT 0x02, R0

                ; Virtual LEDs: Set pattern 0x55 (0b01010101)
                LDI R0, 0x55
                OUT 0x10, R0

                HALT
        )";
        bitforge::assembler::Assembler assem;
        auto res = assem.assemble(ex02);
        EXPECT_TRUE(res.success);
        EXPECT_EQ(res.diagnostics.size(), 0);
        EXPECT_EQ(res.image.entry_point(), 0x0200);

        bitforge::Memory mem;
        bitforge::SystemBus bus(mem);
        bitforge::Cpu cpu(bus);

        EXPECT_TRUE(res.image.load_into(mem));
        cpu.reset(res.image.entry_point());

        auto run_res = bitforge::test::run_with_budget(cpu, 100);
        EXPECT_TRUE(run_res.success);
        EXPECT_EQ(cpu.state(), bitforge::MachineState::Halted);
        EXPECT_EQ(bus.text_output(), "HI!");
        EXPECT_EQ(bus.numeric_output(), "42");
        EXPECT_EQ(bus.led_state(), 0x55);
    }
}

TEST(AssemblerTest, ProgramImageAndSourceMapQueryAudit) {
    const std::string source = R"(
        .ORG 0x0100
        ; Comment line
        start:
            LDI R0, 42
            INC R0
            HALT
    )";
    bitforge::assembler::Assembler assem;
    auto res = assem.assemble(source);
    EXPECT_TRUE(res.success);

    // ProgramImage inspection
    EXPECT_EQ(res.image.total_bytes(), 3 + 2 + 1); // LDI(3) + INC(2) + HALT(1) = 6 bytes
    EXPECT_EQ(res.image.segments().size(), 1);
    EXPECT_EQ(res.image.segments()[0].start_address, 0x0100);
    EXPECT_EQ(res.image.segments()[0].data.size(), 6);

    // SourceMap inspection
    const auto& sm = res.image.source_map();
    EXPECT_GE(sm.instructions().size(), 3);

    // Find instructions by address
    const auto* inst1 = sm.find_instruction_by_address(0x0100);
    EXPECT_NE(inst1, nullptr);
    EXPECT_EQ(inst1->mnemonic, "LDI");
    EXPECT_EQ(inst1->size, 3);

    const auto* inst2 = sm.find_instruction_by_address(0x0103);
    EXPECT_NE(inst2, nullptr);
    EXPECT_EQ(inst2->mnemonic, "INC");
    EXPECT_EQ(inst2->size, 2);

    const auto* inst3 = sm.find_instruction_by_address(0x0105);
    EXPECT_NE(inst3, nullptr);
    EXPECT_EQ(inst3->mnemonic, "HALT");
    EXPECT_EQ(inst3->size, 1);

    // Non-instruction address (e.g. operand byte at 0x0101)
    EXPECT_EQ(sm.find_instruction_by_address(0x0101), nullptr);

    // Breakpoint line resolution
    // Line 5: LDI R0, 42
    auto bp_line5 = sm.resolve_line_breakpoint(5);
    EXPECT_TRUE(bp_line5.success);
    EXPECT_EQ(bp_line5.address, 0x0100);

    // Line 3: Comment line -> fails breakpoint resolution
    auto bp_line3 = sm.resolve_line_breakpoint(3);
    EXPECT_FALSE(bp_line3.success);
}
