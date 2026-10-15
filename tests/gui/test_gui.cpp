#include "bitforge/gui/main_window.hpp"

#include <QTest>
#include <QSignalSpy>

using namespace bitforge;
using namespace bitforge::gui;

namespace {

QString resolveExamplePath(const QString& relPath) {
    if (QFile::exists(relPath)) return relPath;
    if (QFile::exists("../" + relPath)) return "../" + relPath;
    if (QFile::exists("../../" + relPath)) return "../../" + relPath;
    return relPath;
}

} // namespace

class GuiTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testApplicationStarts();
    void testAssembleValidSource();
    void testAssembleInvalidSource();
    void testArithmeticExampleExecution();
    void testVirtualDevicesExecution();
    void testSingleStepRefreshesState();
    void testBreakpointStopsBeforeExecution();
    void testContinueResumesPastBreakpoint();
    void testHaltAndFaultVisualDistinction();
    void testEditingSourceMarksProgramStale();
    void testDiagnosticNavigation();
    void testMultiSliceExecutionAcrossBoundaries();
    void testBudgetExhaustionVsHaltAtBoundary();
    void testBreakpointAtSliceBoundaryAndContinue();
    void testPauseAndResumeIntegrity();
    void testResetDuringExecution();
    void testStaleImageCannotBeExecuted();
    void testFileLifecycleAndUnloadProgram();
    void testBreakpointPlacementRestrictions();
    void testRealExample01Arithmetic();
    void testRealExample02IoDemoAndNoRunDuplication();
    void testMemoryAndStackViewersRefresh();
    void testOffscreenScreenshotCapture();
};

void GuiTest::initTestCase() {
    // Basic test case initialization
}

void GuiTest::testApplicationStarts() {
    MainWindow window;
    QVERIFY(window.controller() != nullptr);
    QVERIFY(window.editor() != nullptr);

    QCOMPARE(window.controller()->session().state(), MachineState::Ready);
    QCOMPARE(window.controller()->session().pc(), 0x0000);
    QVERIFY(!window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());
}

void GuiTest::testAssembleValidSource() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());
    QVERIFY(window.controller()->diagnostics().empty());
    QCOMPARE(window.controller()->session().state(), MachineState::Ready);
    QCOMPARE(window.controller()->session().pc(), 0x0200);
}

void GuiTest::testAssembleInvalidSource() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    NONEXISTENT_OPCODE_FOO R0\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(!window.controller()->isAssembled());
    QVERIFY(!window.controller()->diagnostics().empty());

    // Verify diagnostic points to line 4
    const auto& diag = window.controller()->diagnostics().front();
    QCOMPARE(diag.location.line, 4);
    QCOMPARE(diag.severity, assembler::DiagnosticSeverity::Error);
}

void GuiTest::testArithmeticExampleExecution() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    ADD R0, R1\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);
    QCOMPARE(window.controller()->session().registers().r[0], 30);
    QCOMPARE(window.controller()->session().registers().r[1], 20);
}

void GuiTest::testVirtualDevicesExecution() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    ; Text output 'H', 'I', '!'\n"
        "    LDI R0, 72\n"
        "    OUT 0x01, R0\n"
        "    LDI R0, 73\n"
        "    OUT 0x01, R0\n"
        "    LDI R0, 33\n"
        "    OUT 0x01, R0\n"
        "    ; Numeric output 42\n"
        "    LDI R0, 42\n"
        "    OUT 0x02, R0\n"
        "    ; LED output 0xAA (Port 0x10)\n"
        "    LDI R0, 170\n"
        "    OUT 0x10, R0\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("HI!"));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("42"));
    QCOMPARE(window.controller()->bus().numeric_values().size(), static_cast<std::size_t>(1));
    QCOMPARE(window.controller()->bus().numeric_values().front(), static_cast<byte_t>(42));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0xAA));
}

void GuiTest::testSingleStepRefreshesState() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 5\n"   // 0x0200..0x0202
        "    LDI R1, 10\n"  // 0x0203..0x0205
        "    HALT\n"        // 0x0206
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Step 1
    window.onStep();
    QCOMPARE(window.controller()->session().pc(), 0x0203);
    QCOMPARE(window.controller()->session().registers().r[0], 5);
    QCOMPARE(window.controller()->session().registers().r[1], 0);

    // Step 2
    window.onStep();
    QCOMPARE(window.controller()->session().pc(), 0x0206);
    QCOMPARE(window.controller()->session().registers().r[1], 10);

    // Step 3 (HALT)
    window.onStep();
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->session().pc(), 0x0207);
}

void GuiTest::testBreakpointStopsBeforeExecution() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"     // Line 1
        ".ENTRY start\n"    // Line 2
        "start:\n"          // Line 3
        "    LDI R0, 10\n"  // Line 4 (0x0200)
        "    LDI R1, 20\n"  // Line 5 (0x0203)
        "    HALT\n"        // Line 6 (0x0206)
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Toggle breakpoint on line 5 (LDI R1, 20)
    auto bp_res = window.controller()->toggleBreakpoint(5);
    QVERIFY(bp_res.success);
    QCOMPARE(bp_res.address, 0x0203);

    // Run execution
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().last_stop_reason(), debugger::StopReason::BreakpointHit, 2000);

    // Stopped before executing line 5: R0 is 10, R1 is NOT set yet
    QCOMPARE(window.controller()->session().pc(), 0x0203);
    QCOMPARE(window.controller()->session().registers().r[0], 10);
    QCOMPARE(window.controller()->session().registers().r[1], 0);
}

void GuiTest::testContinueResumesPastBreakpoint() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    HALT\n"
    );

    window.onAssemble();
    window.controller()->toggleBreakpoint(5);

    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().last_stop_reason(), debugger::StopReason::BreakpointHit, 2000);

    // Continue execution from breakpoint
    window.onContinue();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);
    QCOMPARE(window.controller()->session().registers().r[1], 20);
}

void GuiTest::testHaltAndFaultVisualDistinction() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    .BYTE 0xFF\n" // Invalid opcode
    );

    window.onAssemble();
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Faulted, 2000);
    QCOMPARE(window.controller()->session().last_stop_reason(), debugger::StopReason::Faulted);
    QVERIFY(window.controller()->cpu().last_fault().has_fault());
    QCOMPARE(window.controller()->cpu().last_fault().type, FaultType::InvalidOpcode);
}

void GuiTest::testEditingSourceMarksProgramStale() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());

    // Simulate user editing the source text
    window.editor()->insertPlainText("; modification\n");
    QVERIFY(window.controller()->isStale());
}

void GuiTest::testDiagnosticNavigation() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 1\n"
        "    INVALID_INSTRUCTION\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(!window.controller()->isAssembled());

    // Navigate to line 5, column 5
    window.editor()->gotoLine(5, 5);
    QCOMPARE(window.editor()->textCursor().blockNumber(), 4); // 0-indexed block 4 = line 5
}

void GuiTest::testMultiSliceExecutionAcrossBoundaries() {
    MainWindow window;
    // Outer loop 200 times, inner loop 50 times: 20,602 total instructions to HALT
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 200\n"
        "outer:\n"
        "    LDI R1, 50\n"
        "inner:\n"
        "    DEC R1\n"
        "    JNZ inner\n"
        "    DEC R0\n"
        "    JNZ outer\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Configure budget to 30,000 (greater than 20,602 steps)
    // Slices are 10,000 each: slice 1 (10,000), slice 2 (10,000), slice 3 (602 -> HALT)
    window.controller()->run(30000);
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 5000);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(20602));
    QCOMPARE(window.controller()->session().last_stop_reason(), debugger::StopReason::Halted);
    QVERIFY(!window.controller()->isExecuting());
}

void GuiTest::testBudgetExhaustionVsHaltAtBoundary() {
    MainWindow window;
    // 20,602 total steps to HALT
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 200\n"
        "outer:\n"
        "    LDI R1, 50\n"
        "inner:\n"
        "    DEC R1\n"
        "    JNZ inner\n"
        "    DEC R0\n"
        "    JNZ outer\n"
        "    HALT\n"
    );

    window.onAssemble();

    // 1. Budget of 15,000: exhausting budget across slice boundary (slice 1 = 10,000, slice 2 = 5,000)
    window.controller()->run(15000);
    QTRY_VERIFY_WITH_TIMEOUT(!window.controller()->isExecuting(), 3000);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::BudgetExhausted);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(15000));
    QVERIFY(!window.controller()->isExecuting());

    // Reset for exact boundary testing
    window.onReset();
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));

    // 2. Budget of exactly 20,601 steps (one step before HALT):
    window.controller()->run(20601);
    QTRY_VERIFY_WITH_TIMEOUT(!window.controller()->isExecuting(), 5000);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::BudgetExhausted);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(20601));

    // 3. One step from HALT: stepping executes HALT and reports Halted
    window.onStep();
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::Halted);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(20602));

    // 4. Exact budget = 20,602 (HALT on the last permitted instruction)
    window.onReset();
    window.controller()->run(20602);
    QTRY_VERIFY_WITH_TIMEOUT(!window.controller()->isExecuting(), 5000);
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::Halted);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(20602));
}

void GuiTest::testBreakpointAtSliceBoundaryAndContinue() {
    MainWindow window;
    // Loop of 200 outer * (1 + 60*2 + 2) + 1 LDI = 24,601 steps before HALT
    window.editor()->setPlainText(
        ".ORG 0x0200\n"     // Line 1
        ".ENTRY start\n"    // Line 2
        "start:\n"          // Line 3
        "    LDI R0, 200\n" // Line 4
        "outer:\n"          // Line 5
        "    LDI R1, 60\n"  // Line 6
        "inner:\n"          // Line 7
        "    DEC R1\n"      // Line 8
        "    JNZ inner\n"   // Line 9
        "    DEC R0\n"      // Line 10
        "    JNZ outer\n"   // Line 11
        "    HALT\n"        // Line 12
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    auto bp_res = window.controller()->toggleBreakpoint(12);
    QVERIFY(bp_res.success);

    // Run with budget 30,000. It crosses two 10,000 batch boundaries and hits the breakpoint before executing HALT
    window.controller()->run(30000);
    QTRY_VERIFY_WITH_TIMEOUT(!window.controller()->isExecuting(), 5000);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::BreakpointHit);
    QCOMPARE(window.controller()->session().pc(), bp_res.address);
    // 1 (LDI R0) + 200 * 123 (outer iterations) = 24,601 steps before breakpoint at HALT
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(24601));

    // Continue past breakpoint executes HALT and halts
    window.controller()->continueExecution(100);
    QTRY_VERIFY_WITH_TIMEOUT(!window.controller()->isExecuting(), 1000);
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->lastStopReason(), debugger::StopReason::Halted);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(24602));
}

void GuiTest::testPauseAndResumeIntegrity() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0\n"
        "loop:\n"
        "    INC R0\n"
        "    JMP loop\n"
    );

    window.onAssemble();
    window.controller()->run(1000000);
    QVERIFY(window.controller()->isExecuting());

    // Pause within reasonable interval
    QTest::qWait(10);
    window.onPause();
    QVERIFY(!window.controller()->isExecuting());
    QVERIFY(window.controller()->session().state() != MachineState::Halted);
    QVERIFY(window.controller()->session().state() != MachineState::Faulted);

    const word_t paused_pc = window.controller()->session().pc();
    const std::uint64_t paused_steps = window.controller()->session().total_steps_executed();
    QVERIFY(paused_steps > 0);

    // Single step resumes from paused_pc without skipping
    window.onStep();
    QCOMPARE(window.controller()->session().total_steps_executed(), paused_steps + 1);
    QVERIFY(window.controller()->session().pc() != paused_pc);
}

void GuiTest::testResetDuringExecution() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 1\n"
        "loop:\n"
        "    INC R0\n"
        "    JMP loop\n"
    );

    window.onAssemble();
    window.controller()->toggleBreakpoint(4); // Breakpoint on LDI R0, 1

    window.controller()->run(1000000);
    QTest::qWait(10);

    // Reset while timer is running
    window.onReset();
    QVERIFY(!window.controller()->isExecuting());
    QCOMPARE(window.controller()->session().state(), MachineState::Ready);
    QCOMPARE(window.controller()->session().pc(), 0x0200);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
    QCOMPARE(window.controller()->session().registers().r[0], 0);

    // Documented rule: Reset PRESERVES breakpoints
    QVERIFY(window.controller()->hasLineBreakpoint(4));
}

void GuiTest::testStaleImageCannotBeExecuted() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    HALT\n"
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());

    // Edit source
    window.editor()->insertPlainText("; edit source to make stale\n");
    QVERIFY(window.controller()->isStale());

    // Run must NOT execute stale image
    window.onRun();
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
    QCOMPARE(window.controller()->session().state(), MachineState::Ready);
    QCOMPARE(window.controller()->session().registers().r[0], 0);

    // Step must NOT execute stale image
    window.onStep();
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
    QCOMPARE(window.controller()->session().state(), MachineState::Ready);

    // Reassemble refreshes state and enables execution
    window.onAssemble();
    QVERIFY(!window.controller()->isStale());
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 1000);
    QCOMPARE(window.controller()->session().registers().r[0], 10);
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(2));
}

void GuiTest::testFileLifecycleAndUnloadProgram() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    HALT\n"
    );
    window.onAssemble();
    window.controller()->toggleBreakpoint(4);
    QVERIFY(window.controller()->hasLineBreakpoint(4));
    QVERIFY(window.controller()->isAssembled());

    // Document saved/clean: setModified(false) avoids modal prompt in automated test
    window.editor()->document()->setModified(false);
    window.newFile();
    QVERIFY(!window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());
    QVERIFY(!window.controller()->hasLineBreakpoint(4));
    QCOMPARE(window.controller()->session().breakpoints().size(), static_cast<std::size_t>(0));
    QVERIFY(window.controller()->diagnostics().empty());
}

void GuiTest::testBreakpointPlacementRestrictions() {
    MainWindow window;
    window.editor()->setPlainText(
        "; Line 1: Comment only\n"
        ".ORG 0x0200\n"          // Line 2: Directive
        ".ENTRY start\n"         // Line 3: Directive
        "start:\n"               // Line 4: Label only
        "    LDI R0, 1\n"        // Line 5: Executable instruction
        "    HALT\n"             // Line 6: Executable instruction
    );

    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Lines 1, 2, 3, 4 must be rejected
    QVERIFY(!window.controller()->toggleBreakpoint(1).success);
    QVERIFY(!window.controller()->toggleBreakpoint(2).success);
    QVERIFY(!window.controller()->toggleBreakpoint(3).success);
    QVERIFY(!window.controller()->toggleBreakpoint(4).success);

    // Lines 5 and 6 must succeed
    QVERIFY(window.controller()->toggleBreakpoint(5).success);
    QVERIFY(window.controller()->toggleBreakpoint(6).success);
}

void GuiTest::testRealExample01Arithmetic() {
    MainWindow window;
    QVERIFY(window.loadFile(resolveExamplePath("examples/01_arithmetic.asm")));
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);
    // Result R0 = 30 (0x1E), stored at memory address 0x8000
    QCOMPARE(window.controller()->session().registers().r[0], static_cast<byte_t>(0x1E));
    QCOMPARE(window.controller()->memory().read_byte(0x8000), static_cast<byte_t>(0x1E));
}

void GuiTest::testRealExample02IoDemoAndNoRunDuplication() {
    MainWindow window;
    QVERIFY(window.loadFile(resolveExamplePath("examples/02_io_demo.asm")));
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Run 1
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("HI!"));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("42"));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x55));

    // Reset execution
    window.onReset();
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString(""));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString(""));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x00));

    // Run 2: Output must NOT be duplicated as "HI!HI!" or "42 42"
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("HI!"));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("42"));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x55));
}

void GuiTest::testMemoryAndStackViewersRefresh() {
    MainWindow window;
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0xAB\n"
        "    PUSH R0\n"
        "    LDI R1, 0xCD\n"
        "    PUSH R1\n"
        "    HALT\n"
    );

    window.onAssemble();
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    // Stack inspection per ADR-001: 2 bytes pushed (0xAB then 0xCD), SP at 0xFFFD
    auto stack_info = window.controller()->session().inspect_stack();
    QCOMPARE(stack_info.depth, static_cast<std::size_t>(2));
    QCOMPARE(stack_info.sp, static_cast<word_t>(0xFFFD));
    QVERIFY(!stack_info.is_empty);
    QVERIFY(!stack_info.overflow_or_corrupt);

    // Read-only memory range inspection does not alter memory or trigger device faults
    auto mem_range = window.controller()->session().read_memory_range(0xFFFC, 4);
    QVERIFY(mem_range.success);
    QCOMPARE(mem_range.data.size(), static_cast<std::size_t>(4));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x00));
    QCOMPARE(window.controller()->bus().text_output(), std::string(""));
}

void GuiTest::testOffscreenScreenshotCapture() {
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    QVERIFY(window.loadFile(resolveExamplePath("examples/01_arithmetic.asm")));
    window.onAssemble();
    window.onStep(); // Step once so PC arrow appears

    // Render offscreen grab of MainWindow
    QPixmap pixmap = window.grab();
    QVERIFY(!pixmap.isNull());
    QVERIFY(pixmap.width() >= 1000);
    QVERIFY(pixmap.height() >= 600);

    // Save screenshots to artifact directory for visual inspection
    const QString artifact_dir = "C:/Users/Piyush/.gemini/antigravity/brain/3b875d0c-8f06-4ff1-9305-0a2eefba0b93";
    bool saved1 = pixmap.save(artifact_dir + "/gui_smoke_arithmetic.png");
    QVERIFY(saved1);

    // Load IO demo and run to halt
    QVERIFY(window.loadFile(resolveExamplePath("examples/02_io_demo.asm")));
    window.onAssemble();
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    QPixmap pixmap2 = window.grab();
    bool saved2 = pixmap2.save(artifact_dir + "/gui_smoke_io.png");
    QVERIFY(saved2);

    // Switch tabs to show Virtual Devices and Execution Trace
    auto tab_widgets = window.findChildren<QTabWidget*>();
    for (auto* tw : tab_widgets) {
        for (int i = 0; i < tw->count(); ++i) {
            if (tw->tabText(i).contains("Virtual Devices")) {
                tw->setCurrentIndex(i);
            } else if (tw->tabText(i).contains("Execution Trace")) {
                tw->setCurrentIndex(i);
            }
        }
    }

    QPixmap pixmap3 = window.grab();
    bool saved3 = pixmap3.save(artifact_dir + "/gui_smoke_devices.png");
    QVERIFY(saved3);
}

QTEST_MAIN(GuiTest)
#include "test_gui.moc"

