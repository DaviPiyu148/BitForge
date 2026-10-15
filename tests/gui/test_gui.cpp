#include "bitforge/gui/main_window.hpp"

#include <QTest>
#include <QSignalSpy>

using namespace bitforge;
using namespace bitforge::gui;

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

QTEST_MAIN(GuiTest)
#include "test_gui.moc"
