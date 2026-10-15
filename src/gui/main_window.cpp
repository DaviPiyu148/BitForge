#include "bitforge/gui/main_window.hpp"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

namespace bitforge::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      controller_(new ExecutionController(this)) {
    setupUi();
    setupMenusAndToolbars();
    setupStatusBar();

    setCurrentFile(QString());
    updateUiState();

    resize(1200, 800);
}

void MainWindow::setupUi() {
    // Central Source Editor
    editor_ = new CodeEditor(this);
    editor_->setController(controller_);
    setCentralWidget(editor_);

    // Right Dock: CPU Registers & Status + Virtual Devices
    auto* right_dock = new QDockWidget("Registers & Virtual Devices", this);
    right_dock->setObjectName("RightDock");
    right_dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    auto* right_tabs = new QTabWidget(right_dock);
    registers_panel_ = new RegistersPanel(right_tabs);
    registers_panel_->setController(controller_);
    right_tabs->addTab(registers_panel_, "CPU Registers & Flags");

    devices_panel_ = new DevicesPanel(right_tabs);
    devices_panel_->setController(controller_);
    right_tabs->addTab(devices_panel_, "Virtual Devices & Consoles");

    right_dock->setWidget(right_tabs);
    addDockWidget(Qt::RightDockWidgetArea, right_dock);

    // Bottom Dock: Memory Viewer, Stack Viewer, Breakpoints, and Trace
    auto* bottom_dock = new QDockWidget("Memory, Stack & Debug Inspection", this);
    bottom_dock->setObjectName("BottomDock");
    bottom_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    auto* bottom_tabs = new QTabWidget(bottom_dock);
    memory_viewer_ = new MemoryViewer(bottom_tabs);
    memory_viewer_->setController(controller_);
    bottom_tabs->addTab(memory_viewer_, "Memory (64 KiB)");

    stack_viewer_ = new StackViewer(bottom_tabs);
    stack_viewer_->setController(controller_);
    bottom_tabs->addTab(stack_viewer_, "Stack (ADR-001)");

    breakpoints_panel_ = new BreakpointsPanel(bottom_tabs);
    breakpoints_panel_->setController(controller_);
    bottom_tabs->addTab(breakpoints_panel_, "Breakpoints");

    trace_panel_ = new TracePanel(bottom_tabs);
    trace_panel_->setController(controller_);
    bottom_tabs->addTab(trace_panel_, "Execution Trace");

    bottom_dock->setWidget(bottom_tabs);
    addDockWidget(Qt::BottomDockWidgetArea, bottom_dock);

    // Wire controller signals
    connect(controller_, &ExecutionController::stateChanged, this, &MainWindow::updateUiState);
    connect(controller_, &ExecutionController::staleChanged, this, &MainWindow::updateUiState);
    connect(controller_, &ExecutionController::assemblySucceeded, this, &MainWindow::updateUiState);
    connect(controller_, &ExecutionController::assemblyFailed, this, &MainWindow::updateUiState);
    connect(controller_, &ExecutionController::executionStopped, this, [this](debugger::StopReason reason) {
        updateUiState();
        if (reason == debugger::StopReason::BreakpointHit) {
            statusBar()->showMessage("Stopped at breakpoint.", 4000);
        } else if (reason == debugger::StopReason::Halted) {
            statusBar()->showMessage("CPU execution halted normally.", 4000);
        } else if (reason == debugger::StopReason::Faulted) {
            statusBar()->showMessage("CPU execution halted due to architectural fault.", 6000);
        } else if (reason == debugger::StopReason::BudgetExhausted) {
            statusBar()->showMessage("Execution budget exhausted.", 4000);
        }
    });

    // Wire editor modification to stale indicator
    connect(editor_->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (controller_->isAssembled()) {
            controller_->setStale(true);
        }
        updateWindowTitle();
    });

    // Wire diagnostic click to navigate editor
    connect(devices_panel_, &DevicesPanel::diagnosticClicked, editor_, &CodeEditor::gotoLine);
}

void MainWindow::setupMenusAndToolbars() {
    // -----------------------------------------------------------------
    // Actions: File
    // -----------------------------------------------------------------
    new_act_ = new QAction("&New File", this);
    new_act_->setShortcut(QKeySequence::New);
    new_act_->setStatusTip("Create a new BitForge assembly source file");
    connect(new_act_, &QAction::triggered, this, &MainWindow::newFile);

    open_act_ = new QAction("&Open File...", this);
    open_act_->setShortcut(QKeySequence::Open);
    open_act_->setStatusTip("Open an existing .asm file");
    connect(open_act_, &QAction::triggered, this, &MainWindow::openFile);

    save_act_ = new QAction("&Save", this);
    save_act_->setShortcut(QKeySequence::Save);
    save_act_->setStatusTip("Save current source file to disk");
    connect(save_act_, &QAction::triggered, this, &MainWindow::save);

    save_as_act_ = new QAction("Save &As...", this);
    save_as_act_->setShortcut(QKeySequence::SaveAs);
    save_as_act_->setStatusTip("Save current source file under a new name");
    connect(save_as_act_, &QAction::triggered, this, &MainWindow::saveAs);

    exit_act_ = new QAction("E&xit", this);
    exit_act_->setShortcut(QKeySequence::Quit);
    exit_act_->setStatusTip("Exit BitForge application");
    connect(exit_act_, &QAction::triggered, this, &MainWindow::close);

    // -----------------------------------------------------------------
    // Actions: Execution & Debug
    // -----------------------------------------------------------------
    assemble_act_ = new QAction("&Assemble", this);
    assemble_act_->setShortcut(QKeySequence(Qt::Key_F7));
    assemble_act_->setStatusTip("Assemble the current source text into executable program memory");
    connect(assemble_act_, &QAction::triggered, this, &MainWindow::onAssemble);

    run_act_ = new QAction("&Run", this);
    run_act_->setShortcut(QKeySequence(Qt::Key_F5));
    run_act_->setStatusTip("Run emulated execution until breakpoint, HALT, fault, or budget");
    connect(run_act_, &QAction::triggered, this, &MainWindow::onRun);

    continue_act_ = new QAction("&Continue", this);
    continue_act_->setShortcut(QKeySequence(Qt::Key_F8));
    continue_act_->setStatusTip("Resume execution past the active breakpoint");
    connect(continue_act_, &QAction::triggered, this, &MainWindow::onContinue);

    step_act_ = new QAction("&Step", this);
    step_act_->setShortcut(QKeySequence(Qt::Key_F10));
    step_act_->setStatusTip("Single-step execute at most one instruction");
    connect(step_act_, &QAction::triggered, this, &MainWindow::onStep);

    reset_act_ = new QAction("&Reset", this);
    reset_act_->setShortcut(QKeySequence(Qt::Key_F6));
    reset_act_->setStatusTip("Reset CPU registers and execution state back to entry point");
    connect(reset_act_, &QAction::triggered, this, &MainWindow::onReset);

    pause_act_ = new QAction("&Pause", this);
    pause_act_->setStatusTip("Pause active execution slice");
    connect(pause_act_, &QAction::triggered, this, &MainWindow::onPause);

    // -----------------------------------------------------------------
    // Menus
    // -----------------------------------------------------------------
    auto* file_menu = menuBar()->addMenu("&File");
    file_menu->addAction(new_act_);
    file_menu->addAction(open_act_);
    file_menu->addAction(save_act_);
    file_menu->addAction(save_as_act_);
    file_menu->addSeparator();
    file_menu->addAction(exit_act_);

    auto* exec_menu = menuBar()->addMenu("&Execution");
    exec_menu->addAction(assemble_act_);
    exec_menu->addSeparator();
    exec_menu->addAction(run_act_);
    exec_menu->addAction(continue_act_);
    exec_menu->addAction(step_act_);
    exec_menu->addAction(pause_act_);
    exec_menu->addAction(reset_act_);

    auto* help_menu = menuBar()->addMenu("&Help");
    help_menu->addAction("&About BitForge", this, [this]() {
        QMessageBox::about(this, "About BitForge",
                           "<b>BitForge</b> — Educational 8-Bit Computer Workbench<br>"
                           "Version 0.1.0<br><br>"
                           "Emulated 8-bit CPU, 64 KiB memory, virtual I/O devices, "
                           "two-pass assembler, and native Qt 6 desktop debugger.");
    });

    // -----------------------------------------------------------------
    // Toolbar
    // -----------------------------------------------------------------
    auto* toolbar = addToolBar("Execution Toolbar");
    toolbar->setObjectName("MainToolBar");
    toolbar->setMovable(false);

    toolbar->addAction(assemble_act_);
    toolbar->addSeparator();
    toolbar->addAction(run_act_);
    toolbar->addAction(continue_act_);
    toolbar->addAction(step_act_);
    toolbar->addAction(pause_act_);
    toolbar->addAction(reset_act_);
    toolbar->addSeparator();

    auto* budget_label = new QLabel(" Max Steps: ", toolbar);
    toolbar->addWidget(budget_label);

    budget_spinbox_ = new QSpinBox(toolbar);
    budget_spinbox_->setRange(1, 100000000);
    budget_spinbox_->setValue(1000000); // 1,000,000 default budget
    budget_spinbox_->setSingleStep(10000);
    toolbar->addWidget(budget_spinbox_);

    auto* spacer = new QWidget(toolbar);
    spacer->setFixedWidth(12);
    toolbar->addWidget(spacer);

    state_badge_ = new QLabel(" READY ", toolbar);
    state_badge_->setStyleSheet(
        "QLabel { background-color: #28a745; color: white; font-weight: bold; "
        "border-radius: 4px; padding: 3px 8px; }"
    );
    toolbar->addWidget(state_badge_);
}

void MainWindow::setupStatusBar() {
    status_file_label_ = new QLabel("[Untitled]", this);
    status_state_label_ = new QLabel("READY", this);
    status_pc_label_ = new QLabel("PC: 0x0200", this);
    status_steps_label_ = new QLabel("Steps: 0", this);

    statusBar()->addWidget(status_file_label_, 1);
    statusBar()->addPermanentWidget(status_state_label_);
    statusBar()->addPermanentWidget(status_pc_label_);
    statusBar()->addPermanentWidget(status_steps_label_);
}

void MainWindow::updateUiState() {
    const bool is_executing = controller_->isExecuting();
    const bool is_assembled = controller_->isAssembled();
    const MachineState st = controller_->session().state();
    const word_t pc = controller_->session().pc();
    const auto steps = controller_->session().total_steps_executed();
    const bool is_stale = controller_->isStale();

    // Enable/disable actions strictly by machine state
    assemble_act_->setEnabled(!is_executing);
    pause_act_->setEnabled(is_executing);

    if (is_executing) {
        run_act_->setEnabled(false);
        continue_act_->setEnabled(false);
        step_act_->setEnabled(false);
        reset_act_->setEnabled(false);
    } else if (!is_assembled) {
        run_act_->setEnabled(false);
        continue_act_->setEnabled(false);
        step_act_->setEnabled(false);
        reset_act_->setEnabled(false);
    } else {
        const bool can_execute = (st != MachineState::Halted && st != MachineState::Faulted);
        run_act_->setEnabled(can_execute);
        step_act_->setEnabled(can_execute);
        continue_act_->setEnabled(can_execute);
        reset_act_->setEnabled(true);
    }

    // Update state badge
    if (is_executing) {
        state_badge_->setText(" EXECUTING ");
        state_badge_->setStyleSheet(
            "QLabel { background-color: #007bff; color: white; font-weight: bold; "
            "border-radius: 4px; padding: 3px 8px; }"
        );
    } else if (is_stale) {
        state_badge_->setText(" STALE (Reassemble) ");
        state_badge_->setStyleSheet(
            "QLabel { background-color: #fd7e14; color: white; font-weight: bold; "
            "border-radius: 4px; padding: 3px 8px; }"
        );
    } else {
        switch (st) {
            case MachineState::Ready:
                state_badge_->setText(" READY ");
                state_badge_->setStyleSheet(
                    "QLabel { background-color: #28a745; color: white; font-weight: bold; "
                    "border-radius: 4px; padding: 3px 8px; }"
                );
                break;
            case MachineState::Running:
            case MachineState::Paused:
                state_badge_->setText(" PAUSED ");
                state_badge_->setStyleSheet(
                    "QLabel { background-color: #ffc107; color: black; font-weight: bold; "
                    "border-radius: 4px; padding: 3px 8px; }"
                );
                break;
            case MachineState::Halted:
                state_badge_->setText(" HALTED ");
                state_badge_->setStyleSheet(
                    "QLabel { background-color: #6c757d; color: white; font-weight: bold; "
                    "border-radius: 4px; padding: 3px 8px; }"
                );
                break;
            case MachineState::Faulted:
                state_badge_->setText(" FAULTED ");
                state_badge_->setStyleSheet(
                    "QLabel { background-color: #dc3545; color: white; font-weight: bold; "
                    "border-radius: 4px; padding: 3px 8px; }"
                );
                break;
        }
    }

    // Update status bar
    status_state_label_->setText(state_badge_->text().trimmed());
    status_pc_label_->setText(QString("PC: 0x%1").arg(pc, 4, 16, QLatin1Char('0')).toUpper());
    status_steps_label_->setText(QString("Steps: %1").arg(steps));
}

void MainWindow::updateWindowTitle() {
    QString title = "BitForge";
    if (!current_file_path_.isEmpty()) {
        title += " — " + QFileInfo(current_file_path_).fileName();
    } else {
        title += " — [Untitled]";
    }
    if (editor_->document()->isModified()) {
        title += "*";
    }
    setWindowTitle(title);

    status_file_label_->setText(current_file_path_.isEmpty() ? "[Untitled]" : current_file_path_);
    if (editor_->document()->isModified()) {
        status_file_label_->setText(status_file_label_->text() + " [Modified]");
    }
}

void MainWindow::setCurrentFile(const QString& filePath) {
    current_file_path_ = filePath;
    editor_->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::newFile() {
    if (maybeSave()) {
        editor_->clear();
        setCurrentFile(QString());
        controller_->reset();
    }
}

void MainWindow::openFile() {
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(
            this, "Open BitForge Assembly File", QString(),
            "Assembly Source (*.asm *.s);;All Files (*)"
        );
        if (!fileName.isEmpty()) {
            loadFile(fileName);
        }
    }
}

bool MainWindow::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
            this, "BitForge",
            QString("Cannot read file %1:\n%2").arg(filePath, file.errorString())
        );
        return false;
    }

    QTextStream in(&file);
    editor_->setPlainText(in.readAll());
    setCurrentFile(filePath);
    statusBar()->showMessage(QString("Loaded %1").arg(filePath), 3000);
    return true;
}

bool MainWindow::save() {
    if (current_file_path_.isEmpty()) {
        return saveAs();
    }
    return saveFile(current_file_path_);
}

bool MainWindow::saveAs() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Assembly File As", QString(),
        "Assembly Source (*.asm);;All Files (*)"
    );
    if (fileName.isEmpty()) {
        return false;
    }
    return saveFile(fileName);
}

bool MainWindow::saveFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(
            this, "BitForge",
            QString("Cannot write file %1:\n%2").arg(filePath, file.errorString())
        );
        return false;
    }

    QTextStream out(&file);
    out << editor_->toPlainText();
    setCurrentFile(filePath);
    statusBar()->showMessage(QString("Saved %1").arg(filePath), 3000);
    return true;
}

bool MainWindow::maybeSave() {
    if (!editor_->document()->isModified()) {
        return true;
    }
    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this, "BitForge",
        "The document has been modified.\nDo you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    if (ret == QMessageBox::Save) {
        return save();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::onAssemble() {
    const QString code = editor_->toPlainText();
    bool ok = controller_->assemble(code);
    if (ok) {
        statusBar()->showMessage("Assembly successful. Program loaded into emulated memory.", 4000);
    } else {
        statusBar()->showMessage("Assembly failed. See Diagnostics tab for details.", 6000);
    }
}

void MainWindow::onRun() {
    const std::uint64_t budget = static_cast<std::uint64_t>(budget_spinbox_->value());
    controller_->run(budget);
}

void MainWindow::onContinue() {
    const std::uint64_t budget = static_cast<std::uint64_t>(budget_spinbox_->value());
    controller_->continueExecution(budget);
}

void MainWindow::onStep() {
    controller_->step();
}

void MainWindow::onReset() {
    controller_->reset();
    statusBar()->showMessage("Execution reset to entry point.", 3000);
}

void MainWindow::onPause() {
    controller_->pause();
    statusBar()->showMessage("Execution paused by user.", 3000);
}

} // namespace bitforge::gui
