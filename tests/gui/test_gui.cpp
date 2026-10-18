#include "bitforge/gui/main_window.hpp"

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QPushButton>
#include <QCloseEvent>
#include <QTimer>
#include <QDockWidget>
#include <QTabWidget>
#include <QSettings>

using namespace bitforge;
using namespace bitforge::gui;

namespace {

QString resolveExamplePath(const QString& relPath) {
    if (QFile::exists(relPath)) return relPath;
    if (QFile::exists("../" + relPath)) return "../" + relPath;
    if (QFile::exists("../../" + relPath)) return "../../" + relPath;
    return relPath;
}

void dismissModalMessageBox(QMessageBox::StandardButton button, int delay_ms = 25) {
    QTimer::singleShot(delay_ms, [button]() {
        auto* modal = QApplication::activeModalWidget();
        auto* mb = qobject_cast<QMessageBox*>(modal);
        if (!mb) {
            for (auto* top : QApplication::topLevelWidgets()) {
                mb = qobject_cast<QMessageBox*>(top);
                if (mb && mb->isVisible()) break;
            }
        }
        if (mb) {
            auto* btn = mb->button(button);
            if (btn) {
                btn->click();
            } else {
                mb->accept();
            }
        }
    });
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
    void testDirtyStateTracking();
    void testFileSaveAndReloadCycle();
    void testUnwritableSavePreservesEditorContent();
    void testUnsavedChangesPromptBranches();
    void testNewFileCancellationPreservesState();
    void testFileSwitchingPurgesAllState();
    void testStartupFileLoading();
    void testSevenSegmentDisplayVectorRendering();
    void testSevenSegmentDisplayInDevicesPanel();
    void testBitViewWidgetVisualization();
    void testInstructionInspectorDecoding();
    void testExecutionSpeedModesAndBatching();
    void testRunFromEntryVsContinueSemantics();
    void testBreakpointErgonomicsAndGutterF9();
    void testVirtualDevicesTabTitleNoScrollClipping();
    void testFreshRunMemoryResetAndIoContract();
    void testWelcomeDashboardAndNavigation();
    void testFindReplaceBarOperations();
    void testEditorZoomControls();
    void testTutorialGuideWalkthrough();
    void testExamplesCatalogBrowser();
    void testLayoutPersistenceAndRestoration();
    void testADR001StackViewerEmptySentinelAndBreakpointsEmptyState();
    void testInteractiveInputFifoOperations();
    void testWatchPanelInspectionAndHighlighting();
    void testPreferencesPersistenceAcrossSessions();
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

    // Switch tabs to show Bit Visualization and Instruction Inspector
    for (auto* tw : tab_widgets) {
        for (int i = 0; i < tw->count(); ++i) {
            if (tw->tabText(i).contains("CPU Registers")) {
                tw->setCurrentIndex(i);
            } else if (tw->tabText(i).contains("Bit Visualization")) {
                tw->setCurrentIndex(i);
            } else if (tw->tabText(i).contains("Instruction Inspector")) {
                tw->setCurrentIndex(i);
            }
        }
    }

    QPixmap pixmap4 = window.grab();
    bool saved4 = pixmap4.save(artifact_dir + "/gui_smoke_visualization.png");
    QVERIFY(saved4);

    // Capture Phase 9 Welcome Dashboard
    window.showWelcomeScreen();
    QPixmap pixmap_welcome = window.grab();
    bool saved_welcome = pixmap_welcome.save(artifact_dir + "/gui_smoke_welcome.png");
    QVERIFY(saved_welcome);

    // Capture Phase 9 Dark Workbench with active Find bar
    window.showEditor();
    window.onFind();
    window.findBar()->searchInput()->setText("LDI");
    QPixmap pixmap_workbench = window.grab();
    bool saved_workbench = pixmap_workbench.save(artifact_dir + "/gui_smoke_dark_workbench.png");
    QVERIFY(saved_workbench);
    window.findBar()->closeButton()->click();
}

void GuiTest::testDirtyStateTracking() {
    MainWindow window;
    QVERIFY(window.currentFilePath().isEmpty());
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().contains("[Untitled]"));
    QVERIFY(!window.windowTitle().contains("*"));

    // 1. Text edit marks document modified and adds asterisk to title
    window.editor()->insertPlainText("; Comment\n");
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().endsWith("*"));

    // 2. Assembling code
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    NOP\n"
        "    HALT\n"
    );
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());

    // 3. Modifying source code after assembly marks document modified and controller stale
    window.editor()->insertPlainText("    NOP\n");
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.controller()->isStale());
    QVERIFY(window.windowTitle().endsWith("*"));

    // 4. Setting modified(false) clears asterisk on save
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString temp_file = temp_dir.filePath("dirty_test.asm");
    QVERIFY(window.saveFile(temp_file));
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(!window.windowTitle().endsWith("*"));
    QCOMPARE(window.currentFilePath(), temp_file);
}

void GuiTest::testFileSaveAndReloadCycle() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    const QString path_a = temp_dir.filePath("program_a.asm");
    const QString path_b = temp_dir.filePath("program_b.asm");

    MainWindow window;
    const QString code_a =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    HALT\n";

    window.editor()->insertPlainText(code_a);
    QVERIFY(window.editor()->document()->isModified());

    // Save to path_a
    QVERIFY(window.saveFile(path_a));
    QCOMPARE(window.currentFilePath(), path_a);
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().contains("program_a.asm"));
    QVERIFY(!window.windowTitle().endsWith("*"));

    // Verify file content on disk
    QFile file_a(path_a);
    QVERIFY(file_a.open(QFile::ReadOnly | QFile::Text));
    QCOMPARE(QString::fromUtf8(file_a.readAll()), code_a);
    file_a.close();

    // Now edit and save under path_b (Save As simulation)
    window.editor()->appendPlainText("    ; additional comment\n");
    const QString code_b = window.editor()->toPlainText();
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().endsWith("*"));

    QVERIFY(window.saveFile(path_b));
    QCOMPARE(window.currentFilePath(), path_b);
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().contains("program_b.asm"));
    QVERIFY(!window.windowTitle().endsWith("*"));

    // Verify path_b has code_b on disk
    QFile file_b(path_b);
    QVERIFY(file_b.open(QFile::ReadOnly | QFile::Text));
    QCOMPARE(QString::fromUtf8(file_b.readAll()), code_b);
    file_b.close();

    // Now reload path_a and verify editor restores code_a and sets path_a
    QVERIFY(window.loadFile(path_a));
    QCOMPARE(window.currentFilePath(), path_a);
    QCOMPARE(window.editor()->toPlainText(), code_a);
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().contains("program_a.asm"));
    QVERIFY(!window.windowTitle().endsWith("*"));

    // Now test save() directly when currentFilePath() is already set
    window.editor()->appendPlainText("    ; quick edit\n");
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.save());
    QVERIFY(!window.editor()->document()->isModified());
    QCOMPARE(window.currentFilePath(), path_a);

    QFile file_a_updated(path_a);
    QVERIFY(file_a_updated.open(QFile::ReadOnly | QFile::Text));
    QVERIFY(QString::fromUtf8(file_a_updated.readAll()).contains("; quick edit"));
    file_a_updated.close();
}

void GuiTest::testUnwritableSavePreservesEditorContent() {
    MainWindow window;
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 99\n"
        "    HALT\n";

    window.editor()->setPlainText(code);
    window.editor()->document()->setModified(true);

    // Save to an impossible / unwritable directory path
    dismissModalMessageBox(QMessageBox::Ok);
    const QString invalid_path = "C:/nonexistent_dir_bitforge_12345/impossible_sub/file.asm";
    const bool save_res = window.saveFile(invalid_path);
    QVERIFY(!save_res);

    // Verify editor contents, dirty state, and current file path are completely preserved
    QCOMPARE(window.editor()->toPlainText(), code);
    QVERIFY(window.currentFilePath().isEmpty());
    QVERIFY(window.editor()->document()->isModified());

    // Similarly, loading from a nonexistent file must fail gracefully and preserve contents
    dismissModalMessageBox(QMessageBox::Ok);
    const bool load_res = window.loadFile("C:/nonexistent_dir_bitforge_12345/missing.asm");
    QVERIFY(!load_res);
    QCOMPARE(window.editor()->toPlainText(), code);
    QVERIFY(window.currentFilePath().isEmpty());
}

void GuiTest::testUnsavedChangesPromptBranches() {
    MainWindow window;

    // 1. Clean document -> maybeSave() immediately returns true
    window.editor()->document()->setModified(false);
    QVERIFY(window.maybeSave());

    // 2. Modified document + Discard -> returns true without saving
    window.editor()->insertPlainText("; unsaved changes\n");
    QVERIFY(window.editor()->document()->isModified());

    dismissModalMessageBox(QMessageBox::Discard);
    bool proceed_discard = window.maybeSave();
    QVERIFY(proceed_discard);
    QVERIFY(window.editor()->document()->isModified()); // Contents not saved, still dirty

    // 3. Modified document + Cancel -> returns false
    dismissModalMessageBox(QMessageBox::Cancel);
    bool proceed_cancel = window.maybeSave();
    QVERIFY(!proceed_cancel);

    // 4. Modified document with known file path + Save -> saves file and returns true
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString temp_file = temp_dir.filePath("save_branch.asm");
    QVERIFY(window.saveFile(temp_file));
    QVERIFY(!window.editor()->document()->isModified());

    window.editor()->appendPlainText("; added line\n");
    QVERIFY(window.editor()->document()->isModified());

    dismissModalMessageBox(QMessageBox::Save);
    bool proceed_save = window.maybeSave();
    QVERIFY(proceed_save);
    QVERIFY(!window.editor()->document()->isModified()); // Successfully saved to disk!

    // Verify saved file on disk contains added line
    QFile f(temp_file);
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    QVERIFY(QString::fromUtf8(f.readAll()).contains("; added line"));
    f.close();

    // 5. Close event rejection on Cancel vs acceptance on clean
    window.editor()->appendPlainText("; new dirty line\n");
    QVERIFY(window.editor()->document()->isModified());

    dismissModalMessageBox(QMessageBox::Cancel);
    bool closed_cancel = window.close();
    QVERIFY(!closed_cancel); // Window rejected close event

    dismissModalMessageBox(QMessageBox::Discard);
    bool closed_discard = window.close();
    QVERIFY(closed_discard); // Window accepted close event on Discard
}

void GuiTest::testNewFileCancellationPreservesState() {
    MainWindow window;
    const QString initial_code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 77\n"
        "    HALT\n";
    window.editor()->setPlainText(initial_code);
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Make editor modified
    window.editor()->insertPlainText("    ; new comment\n");
    QVERIFY(window.editor()->document()->isModified());

    // Calling newFile() and selecting Cancel in the prompt must preserve editor and assembled state
    dismissModalMessageBox(QMessageBox::Cancel);
    window.newFile();

    QVERIFY(window.editor()->toPlainText().contains("LDI R0, 77"));
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.controller()->isAssembled());

    // Calling newFile() and selecting Discard clears editor and unloads program
    dismissModalMessageBox(QMessageBox::Discard);
    window.newFile();

    QVERIFY(window.editor()->toPlainText().isEmpty());
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(!window.controller()->isAssembled());
    QVERIFY(window.currentFilePath().isEmpty());
}

void GuiTest::testFileSwitchingPurgesAllState() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    const QString path_a = temp_dir.filePath("file_a.asm");
    const QString path_b = temp_dir.filePath("file_b.asm");

    // Write file_a
    {
        QFile fa(path_a);
        QVERIFY(fa.open(QFile::WriteOnly | QFile::Text));
        QTextStream out(&fa);
        out << ".ORG 0x0200\n"
            << ".ENTRY start\n"
            << "start:\n"
            << "    LDI R0, 15\n"
            << "    HALT\n";
    }

    // Write file_b
    {
        QFile fb(path_b);
        QVERIFY(fb.open(QFile::WriteOnly | QFile::Text));
        QTextStream out(&fb);
        out << ".ORG 0x0300\n"
            << ".ENTRY start_b\n"
            << "start_b:\n"
            << "    LDI R1, 30\n"
            << "    HALT\n";
    }

    MainWindow window;
    QVERIFY(window.loadFile(path_a));
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());
    QVERIFY(window.controller()->programImage() != nullptr);

    // Set breakpoint on Line 4 (LDI R0, 15)
    auto bp_res = window.controller()->toggleBreakpoint(4);
    QVERIFY(bp_res.success);
    QVERIFY(window.controller()->hasLineBreakpoint(4));
    QCOMPARE(window.controller()->session().breakpoints().size(), static_cast<std::size_t>(1));

    // Single step to execute LDI R0, 15 and change machine state
    window.onStep();
    QCOMPARE(window.controller()->session().registers().r[0], static_cast<byte_t>(15));
    QVERIFY(window.controller()->session().total_steps_executed() > 0);

    // Mark stale by typing
    window.editor()->insertPlainText("    ; modification\n");
    QVERIFY(window.controller()->isStale());

    // Switch to file_b: clean document first to prevent save prompt
    window.editor()->document()->setModified(false);
    QVERIFY(window.loadFile(path_b));

    // VERIFY ALL STATE FROM FILE A IS COMPLETELY PURGED:
    // 1. New file is not assembled yet
    QVERIFY(!window.controller()->isAssembled());
    // 2. Program image and source mappings are gone
    QVERIFY(window.controller()->programImage() == nullptr);
    // 3. Breakpoints from file A are purged
    QCOMPARE(window.controller()->session().breakpoints().size(), static_cast<std::size_t>(0));
    QVERIFY(!window.controller()->hasLineBreakpoint(4));
    // 4. Stale flag is reset to false
    QVERIFY(!window.controller()->isStale());
    // 5. Diagnostics are empty
    QVERIFY(window.controller()->diagnostics().empty());
    // 6. CPU state and registers are reset
    QCOMPARE(window.controller()->session().registers().r[0], static_cast<byte_t>(0));
    QCOMPARE(window.controller()->session().pc(), static_cast<word_t>(0x0000));
    QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
    QCOMPARE(window.controller()->session().trace().size(), static_cast<std::size_t>(0));

    // 7. Verify diagnostic purging: assemble invalid source in file_b
    window.editor()->setPlainText("INVALID_DIRECTIVE 123\n");
    window.onAssemble();
    QVERIFY(!window.controller()->diagnostics().empty());

    // Call newFile() to verify diagnostics table and controller diagnostics are cleared
    window.editor()->document()->setModified(false);
    window.newFile();
    QVERIFY(window.controller()->diagnostics().empty());
    QVERIFY(!window.controller()->isAssembled());
    QVERIFY(!window.controller()->isStale());
    QVERIFY(window.currentFilePath().isEmpty());
    QVERIFY(window.editor()->toPlainText().isEmpty());
}

void GuiTest::testStartupFileLoading() {
    // 1. Empty arguments: no file passed -> empty untitled editor, clean document
    {
        MainWindow window;
        const bool loaded = window.loadStartupFile({ "bitforge_gui" });
        QVERIFY(!loaded);
        QVERIFY(window.currentFilePath().isEmpty());
        QVERIFY(window.editor()->toPlainText().isEmpty());
        QVERIFY(!window.editor()->document()->isModified());
        QVERIFY(window.windowTitle().contains("[Untitled]"));
        QVERIFY(!window.windowTitle().endsWith("*"));
        QVERIFY(!window.controller()->isAssembled());
    }

    // 2. Direct valid assembly file argument:
    // e.g. launch_bitforge.bat examples/02_io_demo.asm
    {
        MainWindow window;
        const QString io_path = resolveExamplePath("examples/02_io_demo.asm");
        const bool loaded = window.loadStartupFile({ "bitforge_gui", io_path });
        QVERIFY(loaded);

        // Specified file appears in the editor
        QVERIFY(!window.editor()->toPlainText().isEmpty());
        QVERIFY(window.editor()->toPlainText().contains("OUT 0x01") ||
                 window.editor()->toPlainText().contains("02_io_demo"));

        // Path and dirty-state information are correct
        QCOMPARE(window.currentFilePath(), io_path);
        QVERIFY(!window.editor()->document()->isModified());
        QVERIFY(window.windowTitle().contains("02_io_demo.asm"));
        QVERIFY(!window.windowTitle().endsWith("*"));

        // Controller state is reset / unloaded program ready for assemble
        QVERIFY(!window.controller()->isAssembled());
        QVERIFY(!window.controller()->isStale());
        QCOMPARE(window.controller()->session().state(), MachineState::Ready);
        QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
    }

    // 3. File path with Windows-style backslashes (drag-and-drop / Windows CLI simulation)
    {
        MainWindow window;
        const QString io_path = resolveExamplePath("examples/02_io_demo.asm");
        const QString win_path = QDir::toNativeSeparators(io_path);
        const bool loaded = window.loadStartupFile({ "bitforge_gui", win_path });
        QVERIFY(loaded);
        QVERIFY(!window.editor()->toPlainText().isEmpty());
        QCOMPARE(window.currentFilePath(), win_path);
        QVERIFY(!window.editor()->document()->isModified());
        QVERIFY(!window.windowTitle().endsWith("*"));
    }

    // 4. Arguments preceded by Qt flags (e.g. -platform offscreen)
    {
        MainWindow window;
        const QString arith_path = resolveExamplePath("examples/01_arithmetic.asm");
        const bool loaded = window.loadStartupFile({ "bitforge_gui", "-platform", "offscreen", arith_path });
        QVERIFY(loaded);
        QVERIFY(window.editor()->toPlainText().contains("01_arithmetic") ||
                 window.editor()->toPlainText().contains("ADD"));
        QCOMPARE(window.currentFilePath(), arith_path);
        QVERIFY(!window.editor()->document()->isModified());
    }

    // 5. Argument preceded by '--' delimiter
    {
        MainWindow window;
        const QString io_path = resolveExamplePath("examples/02_io_demo.asm");
        const bool loaded = window.loadStartupFile({ "bitforge_gui", "--", io_path });
        QVERIFY(loaded);
        QCOMPARE(window.currentFilePath(), io_path);
        QVERIFY(!window.editor()->document()->isModified());
    }

    // 6. Nonexistent file argument: fails gracefully with message box dismissed
    {
        MainWindow window;
        dismissModalMessageBox(QMessageBox::Ok);
        const bool loaded = window.loadStartupFile({ "bitforge_gui", "C:/nonexistent_dir_bitforge_12345/missing.asm" });
        QVERIFY(!loaded);
        QVERIFY(window.currentFilePath().isEmpty());
        QVERIFY(window.editor()->toPlainText().isEmpty());
        QVERIFY(!window.editor()->document()->isModified());
    }

    // 7. Reset of prior execution session when loading new startup file
    {
        MainWindow window;
        const QString arith_path = resolveExamplePath("examples/01_arithmetic.asm");
        QVERIFY(window.loadFile(arith_path));
        window.onAssemble();
        QVERIFY(window.controller()->isAssembled());
        window.onRun();
        QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);
        QVERIFY(window.controller()->session().total_steps_executed() > 0);

        // Now load another file via startup file API
        const QString io_path = resolveExamplePath("examples/02_io_demo.asm");
        const bool loaded = window.loadStartupFile({ "bitforge_gui", io_path });
        QVERIFY(loaded);

        // Previous execution session must be completely reset
        QVERIFY(!window.controller()->isAssembled());
        QVERIFY(!window.controller()->isStale());
        QCOMPARE(window.controller()->session().state(), MachineState::Ready);
        QCOMPARE(window.controller()->session().total_steps_executed(), static_cast<std::uint64_t>(0));
        QCOMPARE(window.currentFilePath(), io_path);
        QVERIFY(!window.editor()->document()->isModified());
        QVERIFY(!window.windowTitle().endsWith("*"));
    }
}

void GuiTest::testSevenSegmentDisplayVectorRendering() {
    SevenSegmentDisplay display;
    QVERIFY(!display.hasValue());
    QCOMPARE(display.value(), static_cast<byte_t>(0));

    // Off-state rendering
    QPixmap pixmap(200, 80);
    pixmap.fill(Qt::black);
    display.resize(200, 80);
    display.render(&pixmap);
    QVERIFY(!pixmap.isNull());

    // Genuine zero
    display.setValue(0);
    QVERIFY(display.hasValue());
    QCOMPARE(display.value(), static_cast<byte_t>(0));
    display.render(&pixmap);

    // Single digit 7
    display.setValue(7);
    QVERIFY(display.hasValue());
    QCOMPARE(display.value(), static_cast<byte_t>(7));

    // Two digits 42
    display.setValue(42);
    QVERIFY(display.hasValue());
    QCOMPARE(display.value(), static_cast<byte_t>(42));

    // Three digits 255
    display.setValue(255);
    QVERIFY(display.hasValue());
    QCOMPARE(display.value(), static_cast<byte_t>(255));

    // Clear value
    display.clearValue();
    QVERIFY(!display.hasValue());
}

void GuiTest::testSevenSegmentDisplayInDevicesPanel() {
    MainWindow window;
    auto* dev_panel = window.devicesPanel();
    QVERIFY(dev_panel != nullptr);
    auto* seg_display = dev_panel->sevenSegmentDisplay();
    QVERIFY(seg_display != nullptr);
    QVERIFY(!seg_display->hasValue());

    // Assemble program that outputs 123 to Port 0x02
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 123\n"
        "    OUT 0x02, R0\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Step 1: LDI
    window.onStep();
    QVERIFY(!seg_display->hasValue());

    // Step 2: OUT 0x02, R0
    window.onStep();
    QVERIFY(seg_display->hasValue());
    QCOMPARE(seg_display->value(), static_cast<byte_t>(123));

    // Input FIFO status check
    QVERIFY(dev_panel->fifoStatusLabel() != nullptr);
    QVERIFY(dev_panel->fifoStatusLabel()->text().contains("Empty"));

    // Reset CPU
    window.onReset();
    QVERIFY(!seg_display->hasValue());
}

void GuiTest::testBitViewWidgetVisualization() {
    MainWindow window;
    auto* bit_view = window.registersPanel()->bitView();
    QVERIFY(bit_view != nullptr);

    // Offscreen render check
    QPixmap pixmap(300, 400);
    pixmap.fill(Qt::black);
    bit_view->resize(300, 400);
    bit_view->render(&pixmap);
    QVERIFY(!pixmap.isNull());

    // Assemble and run
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 0xAA\n"
        "    LDI R1, 0x55\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 1000);

    // Refresh and check values match registers
    QCOMPARE(window.controller()->session().r(0), static_cast<byte_t>(0xAA));
    QCOMPARE(window.controller()->session().r(1), static_cast<byte_t>(0x55));
    bit_view->render(&pixmap);
}

void GuiTest::testInstructionInspectorDecoding() {
    MainWindow window;
    auto* inspector = window.instructionInspector();
    QVERIFY(inspector != nullptr);

    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    ADD R0, R1\n"
        "    JZ start\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Inspector decoding at initial PC (0x0200)
    inspector->refresh();
    auto info = inspector->decodedInfo();
    QCOMPARE(info.mnemonic, QString("LDI"));
    QCOMPARE(info.instruction_size, static_cast<std::size_t>(3));
    QVERIFY(info.full_disassembly.contains("LDI"));
    QCOMPARE(info.group, QString("Data Movement"));
    QVERIFY(info.description.contains("Load 8-bit immediate"));
    QCOMPARE(inspector->pcGroupLabel()->text(), QString("Data Movement"));
    QVERIFY(inspector->pcDescriptionLabel()->text().contains("Load 8-bit immediate"));
    QVERIFY(inspector->statusBannerLabel()->text().contains("Ready"));

    // Step once (LDI)
    window.onStep();
    inspector->refresh();
    info = inspector->decodedInfo();
    QCOMPARE(info.mnemonic, QString("ADD"));
    QCOMPARE(info.group, QString("Arithmetic"));
    QVERIFY(inspector->statusBannerLabel()->text().contains("Step #1 Completed"));

    // Check last executed step trace summary
    auto last_step = inspector->lastExecutedStep();
    QVERIFY(last_step.has_value());
    QCOMPARE(last_step->mnemonic, QString("LDI"));
    QCOMPARE(last_step->step_number, static_cast<std::uint64_t>(1));

    // Step again to ADD
    window.onStep();
    inspector->refresh();
    info = inspector->decodedInfo();
    QCOMPARE(info.mnemonic, QString("JZ"));
    QCOMPARE(info.group, QString("Branch"));
    QVERIFY(info.branch_evaluation.toLower().contains("taken"));

    // Step to HALT (0x0207)
    window.onStep();
    window.onStep();
    inspector->refresh();
    QVERIFY(inspector->statusBannerLabel()->text().contains("Halted"));
}

void GuiTest::testExecutionSpeedModesAndBatching() {
    MainWindow window;
    auto* combo = window.speedComboBox();
    QVERIFY(combo != nullptr);
    QCOMPARE(combo->count(), 4);
    QCOMPARE(combo->currentIndex(), 3); // Turbo by default

    // Verify unified speed labels across combo box and Execution menu actions
    QCOMPARE(combo->itemText(0), QString("Slow (500 ms / step)"));
    QCOMPARE(combo->itemText(1), QString("Normal (100 ms / step)"));
    QCOMPARE(combo->itemText(2), QString("Fast (50 ms / 5 steps)"));
    QCOMPARE(combo->itemText(3), QString("Turbo (Full Speed)"));

    QCOMPARE(window.speedSlowAction()->text().remove('&'), QString("Slow (500 ms / step)"));
    QCOMPARE(window.speedNormalAction()->text().remove('&'), QString("Normal (100 ms / step)"));
    QCOMPARE(window.speedFastAction()->text().remove('&'), QString("Fast (50 ms / 5 steps)"));
    QCOMPARE(window.speedTurboAction()->text().remove('&'), QString("Turbo (Full Speed)"));

    QSignalSpy speedSpy(window.controller(), &ExecutionController::speedChanged);

    // Change to Slow
    combo->setCurrentIndex(0);
    QCOMPARE(window.controller()->speed(), ExecutionSpeed::Slow);
    QCOMPARE(speedSpy.count(), 1);

    // Change to Normal
    combo->setCurrentIndex(1);
    QCOMPARE(window.controller()->speed(), ExecutionSpeed::Normal);
    QCOMPARE(speedSpy.count(), 2);

    // Change to Fast
    combo->setCurrentIndex(2);
    QCOMPARE(window.controller()->speed(), ExecutionSpeed::Fast);
    QCOMPARE(speedSpy.count(), 3);

    // Change to Turbo
    combo->setCurrentIndex(3);
    QCOMPARE(window.controller()->speed(), ExecutionSpeed::Turbo);
    QCOMPARE(speedSpy.count(), 4);

    // Fast program run under Fast speed with cumulative budget
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "loop:\n"
        "    DEC R0\n"
        "    JNZ loop\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();
    window.controller()->setSpeed(ExecutionSpeed::Fast);
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 3000);
    QCOMPARE(window.controller()->session().r(0), static_cast<byte_t>(0));

    // Reset back to Turbo
    window.controller()->setSpeed(ExecutionSpeed::Turbo);
}

void GuiTest::testRunFromEntryVsContinueSemantics() {
    MainWindow window;
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 10\n"
        "    LDI R1, 20\n"
        "    ADD R0, R1\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Place breakpoint on line 5 (LDI R1, 20)
    auto bp_res = window.controller()->toggleBreakpoint(5);
    QVERIFY(bp_res.success);

    // Run from Entry (F5): should execute line 4 (LDI R0, 10) and stop before line 5
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->lastStopReason(), debugger::StopReason::BreakpointHit, 1000);
    QVERIFY(!window.controller()->isExecuting());
    QCOMPARE(window.controller()->session().r(0), static_cast<byte_t>(10));
    QCOMPARE(window.controller()->session().r(1), static_cast<byte_t>(0));

    // Continue (F8): strictly resumes from current PC without resetting
    window.onContinue();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->lastStopReason(), debugger::StopReason::Halted, 1000);
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->session().r(0), static_cast<byte_t>(30));
    QCOMPARE(window.controller()->session().r(1), static_cast<byte_t>(20));

    // Continue while halted: must not advance
    const auto steps_halted = window.controller()->session().total_steps_executed();
    window.onContinue();
    QCOMPARE(window.controller()->session().total_steps_executed(), steps_halted);
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);

    // Run from Entry again (F5): resets CPU to entry point and runs up to the breakpoint again!
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->lastStopReason(), debugger::StopReason::BreakpointHit, 1000);
    QVERIFY(!window.controller()->isExecuting());
    QCOMPARE(window.controller()->session().r(0), static_cast<byte_t>(10));
}

void GuiTest::testBreakpointErgonomicsAndGutterF9() {
    MainWindow window;
    const QString code =
        "; Comment line 1\n"
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 42\n"
        "    HALT\n";
    window.editor()->setPlainText(code);
    window.onAssemble();

    // 1. Line 1 is a comment -> reject breakpoint
    auto res_comment = window.controller()->toggleBreakpoint(1);
    QVERIFY(!res_comment.success);

    // 2. Line 2 is .ORG directive -> reject breakpoint
    auto res_org = window.controller()->toggleBreakpoint(2);
    QVERIFY(!res_org.success);

    // 3. Line 5 is instruction -> accept breakpoint
    window.editor()->gotoLine(5);
    window.toggleBreakpointAction()->trigger();
    QVERIFY(window.controller()->hasLineBreakpoint(5));

    // Check BreakpointsPanel has the row with 4 columns
    auto* bp_panel = window.breakpointsPanel();
    QVERIFY(bp_panel != nullptr);
    QCOMPARE(bp_panel->table()->rowCount(), 1);
    QCOMPARE(bp_panel->table()->item(0, 0)->text(), QString("5"));
    QCOMPARE(bp_panel->table()->item(0, 1)->text().toUpper(), QString("0X0200"));
    QVERIFY(bp_panel->table()->item(0, 2)->text().contains("LDI"));
    QCOMPARE(bp_panel->table()->item(0, 3)->text(), QString("Active"));

    // Double-click breakpoint jumps editor to line 5
    QSignalSpy jumpSpy(bp_panel, &BreakpointsPanel::breakpointDoubleClicked);
    emit bp_panel->table()->cellDoubleClicked(0, 0);
    QCOMPARE(jumpSpy.count(), 1);
    QCOMPARE(jumpSpy.takeFirst().at(0).toULongLong(), static_cast<unsigned long long>(5));

    // 4. Clear all breakpoints action
    window.clearBreakpointsAction()->trigger();
    QVERIFY(!window.controller()->hasLineBreakpoint(5));
    QCOMPARE(bp_panel->table()->rowCount(), 0);
}

void GuiTest::testVirtualDevicesTabTitleNoScrollClipping() {
    MainWindow window;

    // Verify dock title escaping
    auto* right_dock = window.findChild<QDockWidget*>("RightDock");
    QVERIFY(right_dock != nullptr);
    QCOMPARE(right_dock->windowTitle(), QString("Registers && Virtual Devices"));

    auto* bottom_dock = window.findChild<QDockWidget*>("BottomDock");
    QVERIFY(bottom_dock != nullptr);
    QCOMPARE(bottom_dock->windowTitle(), QString("Memory, Stack && Debug Inspection"));

    // Verify DevicesPanel tab titles do not include & accelerator or long text
    auto* dev_panel = window.devicesPanel();
    QVERIFY(dev_panel != nullptr);
    auto* tabs = dev_panel->findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 3);
    QCOMPARE(tabs->tabText(0), QString("Text Console"));
    QCOMPARE(tabs->tabText(1), QString("Numeric Console"));
    QCOMPARE(tabs->tabText(2), QString("Diagnostics"));

    // Verify shortcuts dialog action exists and works
    dismissModalMessageBox(QMessageBox::Ok);
    window.showKeyboardShortcuts();
}

void GuiTest::testFreshRunMemoryResetAndIoContract() {
    MainWindow window;
    const QString code =
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LD R0, [counter]\n"
        "    LDI R1, 3\n"
        "    ADD R0, R1\n"
        "    ST [counter], R0\n"
        "    LDI R2, 0x41\n"
        "    OUT 0x01, R2\n"
        "    OUT 0x02, R0\n"
        "    LDI R3, 0x0F\n"
        "    OUT 0x10, R3\n"
        "    HALT\n"
        ".ORG 0x0300\n"
        "counter:\n"
        "    .BYTE 5\n";

    window.editor()->setPlainText(code);
    window.onAssemble();
    QVERIFY(window.controller()->isAssembled());

    // Initial state after assemble: counter is 5
    QCOMPARE(window.controller()->memory().read_byte(0x0300), static_cast<byte_t>(5));
    QCOMPARE(window.controller()->bus().text_output(), std::string(""));
    QCOMPARE(window.controller()->bus().numeric_output(), std::string(""));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x00));

    // Run 1: Run from Entry (F5)
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    // Verify Run 1 results
    QCOMPARE(window.controller()->memory().read_byte(0x0300), static_cast<byte_t>(8));
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("A"));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("8"));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x0F));
    const auto steps_run1 = window.controller()->session().total_steps_executed();
    QVERIFY(steps_run1 > 0);
    QCOMPARE(window.controller()->session().trace().size(), static_cast<std::size_t>(steps_run1));

    // Continue while halted: must do nothing and preserve state
    window.onContinue();
    QCOMPARE(window.controller()->session().state(), MachineState::Halted);
    QCOMPARE(window.controller()->session().total_steps_executed(), steps_run1);
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("A"));

    // Run 2: Run from Entry (F5) without manual reset or reassemble
    // Contract: Fresh execution restores initial program image (counter=5), resets devices, step count, and trace
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);

    // Verify Run 2 fresh run contract
    QCOMPARE(window.controller()->memory().read_byte(0x0300), static_cast<byte_t>(8)); // 5 + 3 = 8 (NOT 8 + 3 = 11)
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("A")); // NOT "AA"
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("8")); // NOT "8 8" or "11"
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x0F));
    QCOMPARE(window.controller()->session().total_steps_executed(), steps_run1); // Exact same step count as single run
    QCOMPARE(window.controller()->session().trace().size(), static_cast<std::size_t>(steps_run1));

    // Now test Breakpoint + Continue preserving execution state and memory
    // Place breakpoint on line 8 (LDI R2, 0x41)
    auto bp_res = window.controller()->toggleBreakpoint(8);
    QVERIFY(bp_res.success);

    // Run from Entry with breakpoint active
    window.onRun();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->lastStopReason(), debugger::StopReason::BreakpointHit, 2000);
    QVERIFY(!window.controller()->isExecuting());

    // Counter already stored as 8, but OUT instructions have not executed yet
    QCOMPARE(window.controller()->memory().read_byte(0x0300), static_cast<byte_t>(8));
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString(""));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString(""));

    // Continue (F8) resumes and finishes execution without wiping memory or device state
    window.onContinue();
    QTRY_COMPARE_WITH_TIMEOUT(window.controller()->session().state(), MachineState::Halted, 2000);
    QCOMPARE(window.controller()->memory().read_byte(0x0300), static_cast<byte_t>(8));
    QCOMPARE(QString::fromStdString(window.controller()->bus().text_output()), QString("A"));
    QCOMPARE(QString::fromStdString(window.controller()->bus().numeric_output()), QString("8"));
    QCOMPARE(window.controller()->bus().led_state(), static_cast<byte_t>(0x0F));
}

void GuiTest::testWelcomeDashboardAndNavigation() {
    MainWindow window;
    QVERIFY(window.centralStack() != nullptr);
    QVERIFY(window.welcomeWidget() != nullptr);
    // At startup with no file loaded, central stack displays welcome screen (index 0)
    QCOMPARE(window.centralStack()->currentIndex(), 0);

    // Switching to editor via action or welcome widget
    window.showEditor();
    QCOMPARE(window.centralStack()->currentIndex(), 1);

    // Return to welcome screen
    window.showWelcomeScreen();
    QCOMPARE(window.centralStack()->currentIndex(), 0);

    // Loading a file switches to editor automatically
    window.loadFile(resolveExamplePath("examples/01_arithmetic.asm"));
    QCOMPARE(window.centralStack()->currentIndex(), 1);
    QVERIFY(window.editor()->toPlainText().contains(".ENTRY start"));

    // Welcome widget recent files: add paths and ensure pruning works
    QStringList recents;
    recents << resolveExamplePath("examples/01_arithmetic.asm")
            << resolveExamplePath("examples/02_io_demo.asm")
            << "D:/nonexistent_file_xyz.asm";
    window.welcomeWidget()->setRecentFiles(recents);

    // Recent files in widget should only contain existing files
    const auto active_recents = window.welcomeWidget()->recentFiles();
    QVERIFY(active_recents.contains(resolveExamplePath("examples/01_arithmetic.asm")));
    QVERIFY(!active_recents.contains("D:/nonexistent_file_xyz.asm"));
}

void GuiTest::testFindReplaceBarOperations() {
    MainWindow window;
    window.show();
    window.loadFile(resolveExamplePath("examples/01_arithmetic.asm"));
    window.showEditor();

    auto* fb = window.findBar();
    QVERIFY(fb != nullptr);
    QVERIFY(!fb->isVisible());

    // Open find bar
    window.onFind();
    QVERIFY(fb->isVisible());

    // Find "LDI" in 01_arithmetic.asm (which has 2 instances: LDI R0, 10 and LDI R1, 20)
    fb->searchInput()->setText("LDI");
    fb->triggerFindNext();

    int match_idx = 0;
    int total_matches = 0;
    bool found = window.editor()->findText("LDI", true, false, &match_idx, &total_matches);
    QVERIFY(found);
    QCOMPARE(total_matches, 2);
    QVERIFY(match_idx >= 1 && match_idx <= 2);

    // Navigate next
    fb->triggerFindNext();
    QVERIFY(window.editor()->textCursor().hasSelection());
    QCOMPARE(window.editor()->textCursor().selectedText(), QString("LDI"));

    // Case sensitive search for non-matching case
    bool found_case = window.editor()->findText("ldi", true, true, &match_idx, &total_matches);
    QVERIFY(!found_case);
    QCOMPARE(total_matches, 0);

    // Close find bar via close button
    fb->closeButton()->click();
    QVERIFY(!fb->isVisible());
}

void GuiTest::testEditorZoomControls() {
    MainWindow window;
    auto* ed = window.editor();
    QVERIFY(ed != nullptr);

    const int initial_size = ed->currentFontSize();
    QVERIFY(initial_size >= 8 && initial_size <= 24);

    // Zoom in
    ed->zoomIn(2);
    QCOMPARE(ed->currentFontSize(), initial_size + 2);

    // Zoom out
    ed->zoomOut(1);
    QCOMPARE(ed->currentFontSize(), initial_size + 1);

    // Reset zoom
    ed->resetZoom();
    QCOMPARE(ed->currentFontSize(), initial_size);
}

void GuiTest::testTutorialGuideWalkthrough() {
    MainWindow window;
    TutorialGuide tutorial(&window);
    tutorial.setController(window.controller());

    QCOMPARE(tutorial.currentStage(), 0);

    // Test stage progression through all 9 stages (0 to 8)
    for (int s = 0; s < 8; ++s) {
        QCOMPARE(tutorial.currentStage(), s);
        tutorial.setStage(s + 1);
        QCOMPARE(tutorial.currentStage(), s + 1);
    }

    QCOMPARE(tutorial.currentStage(), 8);

    // Reset back to stage 0
    tutorial.setStage(0);
    QCOMPARE(tutorial.currentStage(), 0);
}

void GuiTest::testExamplesCatalogBrowser() {
    ExamplesDialog dialog;
    // Dialog should populate categories without crashing
    QVERIFY(dialog.layout() != nullptr);
}

void GuiTest::testLayoutPersistenceAndRestoration() {
    MainWindow window;
    window.show();
    QVERIFY(window.rightDock() != nullptr);
    QVERIFY(window.bottomDock() != nullptr);

    // Close right dock
    window.rightDock()->close();
    QVERIFY(!window.rightDock()->isVisible());

    // Restore default layout
    window.onRestoreDefaultLayout();
    QVERIFY(window.rightDock()->isVisible());
    QVERIFY(window.bottomDock()->isVisible());
    QCOMPARE(window.centralStack()->currentIndex(), 1); // Switched to editor
}

void GuiTest::testADR001StackViewerEmptySentinelAndBreakpointsEmptyState() {
    MainWindow window;
    window.show();

    // 1. Stack viewer empty state
    auto* sv = window.stackViewer();
    QVERIFY(sv != nullptr);
    window.controller()->reset();
    sv->refresh();
    QVERIFY(sv->statusLabel()->text().contains("Empty stack (SP = 0xFFFF sentinel per ADR-001"));

    // 2. Breakpoints panel empty state
    auto* bp = window.breakpointsPanel();
    QVERIFY(bp != nullptr);
    bp->refresh();
    QVERIFY(bp->emptyLabel() != nullptr);
    QVERIFY(!bp->emptyLabel()->isHidden());
    QVERIFY(!bp->removeButton()->isEnabled());
    QVERIFY(!bp->clearAllButton()->isEnabled());

    // Add breakpoint
    window.loadFile(resolveExamplePath("examples/01_arithmetic.asm"));
    window.onAssemble();
    window.controller()->toggleBreakpoint(10);
    bp->refresh();
    QVERIFY(bp->emptyLabel()->isHidden());
    QVERIFY(bp->removeButton()->isEnabled());
    QVERIFY(bp->clearAllButton()->isEnabled());

    // Clear breakpoints
    window.controller()->clearAllBreakpoints();
    bp->refresh();
    QVERIFY(!bp->emptyLabel()->isHidden());
    QVERIFY(!bp->removeButton()->isEnabled());
    QVERIFY(!bp->clearAllButton()->isEnabled());

    // 3. Trace panel empty state
    auto* tp = window.tracePanel();
    QVERIFY(tp != nullptr);
    tp->refresh();
    QVERIFY(tp->emptyLabel() != nullptr);
    QVERIFY(!tp->emptyLabel()->isHidden());
}

void GuiTest::testInteractiveInputFifoOperations() {
    MainWindow window;
    auto* dp = window.devicesPanel();
    QVERIFY(dp != nullptr);
    QVERIFY(dp->fifoInputField() != nullptr);
    QVERIFY(dp->fifoEnqueueButton() != nullptr);
    QVERIFY(dp->fifoClearButton() != nullptr);
    QVERIFY(dp->fifoContentsLabel() != nullptr);
    QVERIFY(dp->fifoUnderflowLabel() != nullptr);

    // Initial state: empty FIFO
    QVERIFY(dp->fifoStatusLabel()->text().contains("Empty"));
    QVERIFY(dp->fifoContentsLabel()->text().contains("Empty"));
    QVERIFY(dp->fifoUnderflowLabel()->text().contains("0"));

    // Enter hex bytes and click Enqueue
    dp->fifoInputField()->setText("0xA5 0x5A 42");
    dp->fifoEnqueueButton()->click();

    QCOMPARE(window.controller()->bus().input_fifo_size(), static_cast<std::size_t>(3));
    QVERIFY(dp->fifoStatusLabel()->text().contains("3 bytes queued"));
    QVERIFY(dp->fifoContentsLabel()->text().contains("0xA5"));
    QVERIFY(dp->fifoContentsLabel()->text().contains("0x5A"));
    QVERIFY(dp->fifoContentsLabel()->text().contains("0x2A")); // 42 in hex is 0x2A

    // Reading 1 byte via bus pops from FIFO
    auto val = window.controller()->bus().read_io(0x00);
    QVERIFY(val.has_value());
    QCOMPARE(*val, static_cast<byte_t>(0xA5));
    dp->refresh();
    QCOMPARE(window.controller()->bus().input_fifo_size(), static_cast<std::size_t>(2));

    // Clear button clears queue
    dp->fifoClearButton()->click();
    QCOMPARE(window.controller()->bus().input_fifo_size(), static_cast<std::size_t>(0));
    QVERIFY(dp->fifoStatusLabel()->text().contains("Empty"));

    // Reading from empty FIFO increments underflow counter and returns 0x00
    auto underflow_val = window.controller()->bus().read_io(0x00);
    QVERIFY(underflow_val.has_value());
    QCOMPARE(*underflow_val, static_cast<byte_t>(0x00));
    QCOMPARE(window.controller()->bus().input_underflow_count(), static_cast<std::size_t>(1));
    dp->refresh();
    QVERIFY(dp->fifoUnderflowLabel()->text().contains("1"));
}

void GuiTest::testWatchPanelInspectionAndHighlighting() {
    MainWindow window;
    auto* wp = window.watchPanel();
    QVERIFY(wp != nullptr);
    QVERIFY(wp->table() != nullptr);
    QVERIFY(wp->addButton() != nullptr);
    QVERIFY(wp->removeButton() != nullptr);
    QVERIFY(wp->clearButton() != nullptr);

    // Initial state: empty watches
    QCOMPARE(wp->watchCount(), static_cast<std::size_t>(0));
    QVERIFY(!wp->emptyLabel()->isHidden());

    // Add watches
    QVERIFY(wp->addWatch("R0"));
    QVERIFY(wp->addWatch("IX"));
    QVERIFY(wp->addWatch("Flag Z"));
    QVERIFY(wp->addWatch("0x8000"));
    QCOMPARE(wp->watchCount(), static_cast<std::size_t>(4));
    QCOMPARE(wp->table()->rowCount(), 4);
    QVERIFY(wp->emptyLabel()->isHidden());

    // Assemble program that modifies R0 and Memory[0x8000]
    window.editor()->setPlainText(
        ".ORG 0x0200\n"
        ".ENTRY start\n"
        "start:\n"
        "    LDI R0, 99\n"
        "    ST [0x8000], R0\n"
        "    HALT\n"
    );
    QVERIFY(window.controller()->assemble(window.editor()->toPlainText()));

    // Execute step 1: LDI R0, 99
    window.controller()->step();
    wp->refresh();

    // Verify row 0 (R0) value is 0x63 / 99 and highlighted as changed
    auto* r0_hex = wp->table()->item(0, 2);
    QVERIFY(r0_hex != nullptr);
    QCOMPARE(r0_hex->text(), QString("0x63"));
    QCOMPARE(r0_hex->background().color(), theme::kChangedBg);

    // Remove row 1 (IX)
    wp->removeWatch(1);
    QCOMPARE(wp->watchCount(), static_cast<std::size_t>(3));

    // Clear all watches
    wp->clearWatches();
    QCOMPARE(wp->watchCount(), static_cast<std::size_t>(0));
    QCOMPARE(wp->table()->rowCount(), 0);
    QVERIFY(!wp->emptyLabel()->isHidden());
}

void GuiTest::testPreferencesPersistenceAcrossSessions() {
    {
        MainWindow window;
        window.editor()->setFontSize(16);
        window.controller()->setSpeed(ExecutionSpeed::Fast);
        window.watchPanel()->clearWatches();
        window.watchPanel()->addWatch("R1");
        window.watchPanel()->addWatch("0x5000");

        // Close/save settings
        QCloseEvent ce;
        window.close();
    }

    {
        // Re-open fresh instance
        MainWindow window2;
        // Verify restored preferences
        QCOMPARE(window2.editor()->currentFontSize(), 16);
        QCOMPARE(window2.controller()->speed(), ExecutionSpeed::Fast);
        QCOMPARE(window2.watchPanel()->watchCount(), static_cast<std::size_t>(2));

        // Clean up settings
        QSettings settings("BitForge", "BitForge");
        settings.remove("editorFontSize");
        settings.remove("executionSpeed");
        settings.remove("watchList");
    }
}

QTEST_MAIN(GuiTest)
#include "test_gui.moc"



