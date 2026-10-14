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
