#ifndef BITFORGE_CORE_EXAMPLE_CATALOG_HPP
#define BITFORGE_CORE_EXAMPLE_CATALOG_HPP

#include "bitforge/core/types.hpp"
#include <string_view>
#include <array>
#include <optional>

namespace bitforge {

enum class ExampleKind {
    Runnable,        // Assembles cleanly and executes to HALT normally
    DebuggerDemo,    // Designed for stepping, breakpoint triggers, or budget bounds
    RuntimeFault,    // Demonstrates deliberate CPU architectural fault
    InvalidAssembly  // Intentionally invalid source fixture expecting diagnostic rejection
};

constexpr std::string_view to_string(ExampleKind kind) noexcept {
    switch (kind) {
        case ExampleKind::Runnable:        return "Runnable";
        case ExampleKind::DebuggerDemo:    return "Debugger Demo";
        case ExampleKind::RuntimeFault:    return "Runtime Fault";
        case ExampleKind::InvalidAssembly: return "Invalid Assembly";
        default:                           return "Unknown";
    }
}

struct ExampleEntry {
    std::string_view relative_path;
    std::string_view category;
    std::string_view title;
    ExampleKind kind;
    std::string_view description;
    std::string_view expected_stop;
    std::string_view expected_behavior;
    std::string_view recommended_breakpoints;
};

// Authoritative machine-readable catalog of all 51 BitForge assembly programs
inline constexpr std::array<ExampleEntry, 51> kExampleCatalog = {{
    // Quick Starts / Root
    {"examples/01_arithmetic.asm", "Quick Starts", "01 Arithmetic & Memory", ExampleKind::Runnable,
     "Fundamental addition and memory store to 0x8000.", "Halted", "R0=0x1E (30), Memory[0x8000]=0x1E", ""},
    {"examples/02_io_demo.asm", "Quick Starts", "02 Virtual I/O Demo", ExampleKind::Runnable,
     "Demonstrates text console, numeric display, and LED peripheral output.", "Halted", "Text: 'HI!', Num: 42, LEDs: 0xAA", ""},

    // 1. Fundamentals
    {"examples/fundamentals/01_nop_halt.asm", "Fundamentals", "01 NOP and HALT", ExampleKind::Runnable,
     "Three sequential NOPs followed by HALT.", "Halted", "R0-R3=0, PC=0x0204", ""},
    {"examples/fundamentals/02_registers_transfer.asm", "Fundamentals", "02 Register Transfer", ExampleKind::Runnable,
     "LDI and MOV data transfer across R0..R3.", "Halted", "R0=0x99, R1=0x42, R2=0x99, R3=0x99", ""},

    // 2. Arithmetic & Logic
    {"examples/arithmetic/01_addition_subtraction.asm", "Arithmetic & Logic", "01 Addition & Subtraction", ExampleKind::Runnable,
     "Basic addition and subtraction: (50 + 25) - 15 = 60.", "Halted", "R0=60 (0x3C), C=1", ""},
    {"examples/arithmetic/02_increment_decrement.asm", "Arithmetic & Logic", "02 Increment & Decrement", ExampleKind::Runnable,
     "INC past 0xFF (wraps to 0), DEC past 0x00. Preserves C flag.", "Halted", "R0=0x00, R1=0xFF, N=1", ""},
    {"examples/arithmetic/03_bitwise_logic.asm", "Arithmetic & Logic", "03 Bitwise Logic", ExampleKind::Runnable,
     "Bitwise AND, OR, XOR, NOT operations.", "Halted", "R0=0x5A, R1=0x0F, R2=0x50, R3=0xFF", ""},
    {"examples/arithmetic/04_comparison_flags.asm", "Arithmetic & Logic", "04 Comparison Flags", ExampleKind::Runnable,
     "CMP 10, 20 updates flags without altering registers.", "Halted", "R0=10, R1=20, N=1", ""},
    {"examples/arithmetic/05_carry_borrow_boundaries.asm", "Arithmetic & Logic", "05 Carry & Borrow Boundaries", ExampleKind::Runnable,
     "Unsigned carry generation (0xFF + 1) and borrow (4 - 6).", "Halted", "R0=0x00, R1=0x01, N=1", ""},
    {"examples/arithmetic/06_signed_overflow_boundaries.asm", "Arithmetic & Logic", "06 Signed Overflow Boundaries", ExampleKind::Runnable,
     "Signed overflow (+127 + 1 = -128) and underflow (-128 - 1 = +127).", "Halted", "R0=0x80, R1=0x01, V=1, C=1", ""},

    // 3. Control Flow
    {"examples/control_flow/01_unconditional_jump.asm", "Control Flow", "01 Unconditional Jump", ExampleKind::Runnable,
     "JMP over a trap instruction to target.", "Halted", "R0=1, R1=2, R2=0", ""},
    {"examples/control_flow/02_branch_zero.asm", "Control Flow", "02 Branch on Zero", ExampleKind::Runnable,
     "Tests JZ taken on zero, JNZ taken on non-zero.", "Halted", "R0=0x00, R1=0x01, R2=0xAA", ""},
    {"examples/control_flow/03_branch_carry.asm", "Control Flow", "03 Branch on Carry", ExampleKind::Runnable,
     "Tests JC taken on carry set, JNC taken on carry clear.", "Halted", "R0=0x20, R1=0xBB", ""},
    {"examples/control_flow/04_countdown_loop.asm", "Control Flow", "04 Countdown Loop", ExampleKind::Runnable,
     "Counted loop adding 5 + 4 + 3 + 2 + 1 = 15.", "Halted", "R0=0, R1=15 (0x0F), Z=1", ""},

    // 4. Memory & Pointers
    {"examples/memory/01_direct_load_store.asm", "Memory & Pointers", "01 Direct Load & Store", ExampleKind::Runnable,
     "ST to 0x8050, clear R0, LD back into R1.", "Halted", "R0=0x00, R1=0x7E, Memory[0x8050]=0x7E", ""},
    {"examples/memory/02_indexed_access_ix.asm", "Memory & Pointers", "02 Indexed Access (IX)", ExampleKind::Runnable,
     "LDI16 IX, STX, INX, DEX, LDX sequence.", "Halted", "R1=0x11, R2=0x22, IX=0x8001", ""},
    {"examples/memory/03_array_traversal.asm", "Memory & Pointers", "03 Array Traversal", ExampleKind::Runnable,
     "Traversing .BYTE 10, 20, 30, 40, 50 array via IX.", "Halted", "R0=50, R1=0, IX=0x0305", ""},
    {"examples/memory/04_array_summation.asm", "Memory & Pointers", "04 Array Summation", ExampleKind::Runnable,
     "Sums elements 5, 10, 15, 20, 25 to memory 0x8100.", "Halted", "R0=75 (0x4B), Memory[0x8100]=0x4B", ""},

    // 5. Stack & Subroutines
    {"examples/stack_subroutines/01_push_pop_basic.asm", "Stack & Subroutines", "01 Push & Pop Basic", ExampleKind::Runnable,
     "PUSH 0x11, 0x22, 0x33 and POP in reverse order.", "Halted", "R1=0x33, R2=0x22, R3=0x11, SP=0xFFFF", ""},
    {"examples/stack_subroutines/02_subroutine_call_ret.asm", "Stack & Subroutines", "02 Subroutine Call & Ret", ExampleKind::Runnable,
     "Simple CALL and RET doubling R0 from 7 to 14.", "Halted", "R0=14 (0x0E), SP=0xFFFF", ""},
    {"examples/stack_subroutines/03_register_preservation.asm", "Stack & Subroutines", "03 Register Preservation", ExampleKind::Runnable,
     "Callee-save convention: subroutine preserves R1 using PUSH/POP.", "Halted", "R0=30, R1=0xAA, SP=0xFFFF", ""},
    {"examples/stack_subroutines/04_nested_calls.asm", "Stack & Subroutines", "04 Nested Subroutine Calls", ExampleKind::Runnable,
     "Nested execution: main -> func_a -> func_b.", "Halted", "R0=16 (1 + 10 + 5), SP=0xFFFF", ""},

    // 6. Virtual I/O Devices
    {"examples/io_devices/01_text_console_output.asm", "Virtual I/O Devices", "01 Text Console Output", ExampleKind::Runnable,
     "Outputs 'BitForge!' string to Port 0x01.", "Halted", "Text Console: 'BitForge!'", ""},
    {"examples/io_devices/02_numeric_console_output.asm", "Virtual I/O Devices", "02 Numeric Console Output", ExampleKind::Runnable,
     "Outputs 0, 42, 100, 255 to Port 0x02.", "Halted", "Numeric Console: '0 42 100 255'", ""},
    {"examples/io_devices/03_virtual_led_patterns.asm", "Virtual I/O Devices", "03 Virtual LED Patterns", ExampleKind::Runnable,
     "Walking bit patterns and alternating LED displays on Port 0x10.", "Halted", "LEDs: 0xFF", ""},
    {"examples/io_devices/04_input_fifo_poll.asm", "Virtual I/O Devices", "04 Input FIFO Polling", ExampleKind::Runnable,
     "Reading Port 0x00 when FIFO is empty (non-fatal underflow).", "Halted", "R0=0x00, Underflows=1", ""},

    // 7. Algorithms
    {"examples/algorithms/01_multiplication_repeated_addition.asm", "Algorithms", "01 Repeated Addition Multiplication", ExampleKind::Runnable,
     "8-bit unsigned multiplication: 7 * 6 = 42.", "Halted", "R0=42 (0x2A), R1=7, R2=0", ""},
    {"examples/algorithms/02_division_repeated_subtraction.asm", "Algorithms", "02 Repeated Subtraction Division", ExampleKind::Runnable,
     "8-bit unsigned division: 45 / 7 = 6 remainder 3.", "Halted", "R0=6 (Quotient), R1=3 (Remainder)", ""},
    {"examples/algorithms/03_sum_first_n_integers.asm", "Algorithms", "03 Sum of First N Integers", ExampleKind::Runnable,
     "Triangular number calculation: Sum(1..10) = 55.", "Halted", "R0=55 (0x37), R1=0", ""},
    {"examples/algorithms/04_fibonacci_sequence.asm", "Algorithms", "04 Fibonacci Sequence", ExampleKind::Runnable,
     "Generates F(0)..F(8), writes to 0x8000, prints F(8).", "Halted", "R0=21 (0x15), Numeric: '21'", ""},
    {"examples/algorithms/05_greatest_common_divisor.asm", "Algorithms", "05 Euclidean GCD", ExampleKind::Runnable,
     "Euclidean GCD via repeated subtraction: GCD(48, 18) = 6.", "Halted", "R0=6, R1=6", ""},

    // 8. Debugger Demonstrations
    {"examples/debugger/01_stepping_sequential.asm", "Debugger Demos", "01 Stepping Sequential", ExampleKind::DebuggerDemo,
     "Demonstrates single-step execution and register highlighting.", "Halted", "Step through LDI sequence", "Line 24"},
    {"examples/debugger/02_branch_taken_not_taken.asm", "Debugger Demos", "02 Branch Taken / Not Taken", ExampleKind::DebuggerDemo,
     "Visualizing conditional branch jump vs. fallthrough.", "Halted", "Inspect branch preview indicator", "Line 28"},
    {"examples/debugger/03_breakpoint_target.asm", "Debugger Demos", "03 Breakpoint Target", ExampleKind::DebuggerDemo,
     "Demonstrates stopping cleanly before target breakpoint lines.", "BreakpointHit", "Breaks on configured breakpoint lines", "Line 34, Line 40, Line 46"},
    {"examples/debugger/04_self_loop_budget.asm", "Debugger Demos", "04 Self-Loop Budget", ExampleKind::DebuggerDemo,
     "Infinite self-loop demonstrating budget exhaustion and pause.", "BudgetExhausted", "Exhausts max step budget without freezing UI", ""},
    {"examples/debugger/05_disjoint_org_segments.asm", "Debugger Demos", "05 Disjoint ORG Segments", ExampleKind::DebuggerDemo,
     "Multi-segment memory image demonstrating CALL to distant segment.", "Halted", "Step across 0x0200 to 0x0500 boundary", "Line 24"},
    {"examples/debugger/06_memory_inspection_step.asm", "Debugger Demos", "06 Memory Inspection Step", ExampleKind::DebuggerDemo,
     "Sequential ST writes to 0x8000..0x8003 for live memory inspection.", "Halted", "Watch Memory tab highlight writes", "Line 26"},

    // 9. Runtime Fault Demonstrations
    {"examples/faults/01_invalid_opcode_fault.asm", "Runtime Faults", "01 Invalid Opcode Fault", ExampleKind::RuntimeFault,
     "Emits raw byte 0x03 to trigger an InvalidOpcode CPU fault.", "Faulted", "Fault: InvalidOpcode at 0x0200", ""},
    {"examples/faults/02_stack_underflow_fault.asm", "Runtime Faults", "02 Stack Underflow Fault", ExampleKind::RuntimeFault,
     "Executes POP when SP=0xFFFF to trigger ADR-001 StackUnderflow.", "Faulted", "Fault: StackUnderflow at 0x0200", ""},
    {"examples/faults/03_stack_overflow_fault.asm", "Runtime Faults", "03 Stack Overflow Fault", ExampleKind::RuntimeFault,
     "Infinite recursive CALL beyond stack limit 0xF000 triggering StackOverflow.", "Faulted", "Fault: StackOverflow", ""},
    {"examples/faults/04_invalid_port_fault.asm", "Runtime Faults", "04 Invalid Port Fault", ExampleKind::RuntimeFault,
     "Writes to unmapped I/O port 0x30 to trigger InvalidPort CPU fault.", "Faulted", "Fault: InvalidPort at 0x0203", ""},

    // 10. Invalid Assembly Fixtures
    {"examples/invalid/01_unknown_mnemonic.asm", "Invalid Syntax", "01 Unknown Mnemonic", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of unrecognized instruction mnemonic 'FOOBAR'.", "AssemblyError", "Diagnostic: Unknown instruction mnemonic 'FOOBAR'", ""},
    {"examples/invalid/02_invalid_register.asm", "Invalid Syntax", "02 Invalid Register", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of invalid register identifier 'R9'.", "AssemblyError", "Diagnostic: Invalid destination register 'R9'", ""},
    {"examples/invalid/03_missing_operand.asm", "Invalid Syntax", "03 Missing Operand", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of missing operand in ADD instruction.", "AssemblyError", "Diagnostic: Expected ',' between operands in ADD", ""},
    {"examples/invalid/04_extra_operand.asm", "Invalid Syntax", "04 Extra Operand", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of unexpected trailing operand on NOP.", "AssemblyError", "Diagnostic: Unexpected trailing token: 'R0'", ""},
    {"examples/invalid/05_duplicate_label.asm", "Invalid Syntax", "05 Duplicate Label", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of duplicate label definition.", "AssemblyError", "Diagnostic: Duplicate label definition: 'loop_point'", ""},
    {"examples/invalid/06_unresolved_label.asm", "Invalid Syntax", "06 Unresolved Label", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of branch to undefined symbol.", "AssemblyError", "Diagnostic: Unresolved label: 'nonexistent_target'", ""},
    {"examples/invalid/07_literal_overflow.asm", "Invalid Syntax", "07 Literal Overflow", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of 8-bit immediate exceeding range (999).", "AssemblyError", "Diagnostic: Immediate value out of range [-128, 255]", ""},
    {"examples/invalid/08_overlapping_segments.asm", "Invalid Syntax", "08 Overlapping Segments", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of overlapping .ORG segments.", "AssemblyError", "Diagnostic: Overlapping segment emission at address 0x0200", ""},
    {"examples/invalid/09_reserved_stack_emission.asm", "Invalid Syntax", "09 Reserved Stack Emission", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of .ORG in reserved hardware stack region.", "AssemblyError", "Diagnostic: Cannot set .ORG into reserved stack region", ""},
    {"examples/invalid/10_malformed_string.asm", "Invalid Syntax", "10 Malformed String", ExampleKind::InvalidAssembly,
     "Tests assembler rejection of unterminated string literal.", "AssemblyError", "Diagnostic: Unterminated string literal", ""}
}};

inline std::optional<ExampleEntry> find_example_by_path(std::string_view path) noexcept {
    for (const auto& ex : kExampleCatalog) {
        if (ex.relative_path == path) {
            return ex;
        }
    }
    return std::nullopt;
}

} // namespace bitforge

#endif // BITFORGE_CORE_EXAMPLE_CATALOG_HPP
