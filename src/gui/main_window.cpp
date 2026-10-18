#include "bitforge/gui/main_window.hpp"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QSettings>
#include <QApplication>

namespace bitforge::gui {

namespace {

QString resolveExamplePath(const QString& relPath) {
    if (QFile::exists(relPath)) return QFileInfo(relPath).absoluteFilePath();
    if (QFile::exists("../" + relPath)) return QFileInfo("../" + relPath).absoluteFilePath();
    if (QFile::exists("../../" + relPath)) return QFileInfo("../../" + relPath).absoluteFilePath();
    QString appPath = QCoreApplication::applicationDirPath() + "/" + relPath;
    if (QFile::exists(appPath)) return QFileInfo(appPath).absoluteFilePath();
    QString appParentPath = QCoreApplication::applicationDirPath() + "/../" + relPath;
    if (QFile::exists(appParentPath)) return QFileInfo(appParentPath).absoluteFilePath();
    return relPath;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      controller_(new ExecutionController(this)) {
    // Apply centralized dark design system stylesheet across application
    if (qApp) {
        qApp->setStyleSheet(theme::applicationStyleSheet());
    }

    setupUi();
    setupMenusAndToolbars();
    setupStatusBar();

    loadSettings();

    setCurrentFile(QString());
    updateUiState();

    if (size().width() < 600 || size().height() < 400) {
        resize(1240, 820);
    }
}

void MainWindow::setupUi() {
    // Central Stack: Page 0 = Welcome Dashboard, Page 1 = Code Editor + Find/Replace Bar
    central_stack_ = new QStackedWidget(this);

    welcome_widget_ = new WelcomeWidget(central_stack_);
    central_stack_->addWidget(welcome_widget_); // Page 0

    editor_container_ = new QWidget(central_stack_);
    auto* editor_layout = new QVBoxLayout(editor_container_);
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_layout->setSpacing(0);

    find_bar_ = new FindReplaceBar(editor_container_);
    find_bar_->setVisible(false);
    editor_layout->addWidget(find_bar_);

    editor_ = new CodeEditor(editor_container_);
    editor_->setController(controller_);
    editor_layout->addWidget(editor_);

    central_stack_->addWidget(editor_container_); // Page 1
    central_stack_->setCurrentIndex(0); // Start on welcome screen by default
    setCentralWidget(central_stack_);

    // Wire WelcomeWidget actions
    connect(welcome_widget_, &WelcomeWidget::newFileRequested, this, [this]() {
        newFile();
        showEditor();
    });
    connect(welcome_widget_, &WelcomeWidget::openFileRequested, this, &MainWindow::openFile);
    connect(welcome_widget_, &WelcomeWidget::openRecentRequested, this, [this](const QString& path) {
        if (loadFile(path)) {
            showEditor();
        }
    });
    connect(welcome_widget_, &WelcomeWidget::browseExamplesRequested, this, &MainWindow::onBrowseExamples);
    connect(welcome_widget_, &WelcomeWidget::startTutorialRequested, this, &MainWindow::onStartTutorial);
    connect(welcome_widget_, &WelcomeWidget::openEditorRequested, this, &MainWindow::showEditor);

    // Wire FindReplaceBar actions to editor
    connect(find_bar_, &FindReplaceBar::findNext, this, [this](const QString& query, bool match_case) {
        int match_idx = 0;
        int total_matches = 0;
        editor_->findText(query, true, match_case, &match_idx, &total_matches);
        find_bar_->setMatchStatus(match_idx, total_matches);
    });
    connect(find_bar_, &FindReplaceBar::findPrevious, this, [this](const QString& query, bool match_case) {
        int match_idx = 0;
        int total_matches = 0;
        editor_->findText(query, false, match_case, &match_idx, &total_matches);
        find_bar_->setMatchStatus(match_idx, total_matches);
    });
    connect(find_bar_, &FindReplaceBar::closed, this, [this]() {
        editor_->clearFindMatches();
        editor_->setFocus();
    });

    // Wire editor cursor position changes to status bar
    connect(editor_, &CodeEditor::cursorLineColumnChanged, this, [this](int line, int col) {
        if (status_cursor_label_) {
            status_cursor_label_->setText(QString("Ln %1, Col %2").arg(line).arg(col));
        }
    });

    // Right Dock: CPU Registers & Status + Virtual Devices
    right_dock_ = new QDockWidget("Registers && Virtual Devices", this);
    right_dock_->setObjectName("RightDock");
    right_dock_->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    auto* right_tabs = new QTabWidget(right_dock_);
    registers_panel_ = new RegistersPanel(right_tabs);
    registers_panel_->setController(controller_);
    right_tabs->addTab(registers_panel_, "CPU Registers && Flags");

    devices_panel_ = new DevicesPanel(right_tabs);
    devices_panel_->setController(controller_);
    right_tabs->addTab(devices_panel_, "Virtual Devices");

    right_dock_->setWidget(right_tabs);
    addDockWidget(Qt::RightDockWidgetArea, right_dock_);

    // Bottom Dock: Memory Viewer, Stack Viewer, Breakpoints, Trace, and Instruction Inspector
    bottom_dock_ = new QDockWidget("Memory, Stack && Debug Inspection", this);
    bottom_dock_->setObjectName("BottomDock");
    bottom_dock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    auto* bottom_tabs = new QTabWidget(bottom_dock_);
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

    instruction_inspector_ = new InstructionInspector(bottom_tabs);
    instruction_inspector_->setController(controller_);
    bottom_tabs->addTab(instruction_inspector_, "Instruction Inspector");

    bottom_dock_->setWidget(bottom_tabs);
    addDockWidget(Qt::BottomDockWidgetArea, bottom_dock_);

    // Watch Dock: Dedicated dock widget for watch expressions
    watch_dock_ = new QDockWidget("Watch Expressions", this);
    watch_dock_->setObjectName("WatchDock");
    watch_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);

    watch_panel_ = new WatchPanel(watch_dock_);
    watch_panel_->setController(controller_);
    watch_dock_->setWidget(watch_panel_);
    addDockWidget(Qt::BottomDockWidgetArea, watch_dock_);
    tabifyDockWidget(bottom_dock_, watch_dock_);

    // Wire right-click "Add to Watch" signals from panels to watch_panel_
    connect(registers_panel_, &RegistersPanel::requestAddWatch, this, [this](const QString& sym) {
        if (watch_panel_ != nullptr) {
            watch_panel_->addWatch(sym);
            if (watch_dock_ != nullptr) {
                watch_dock_->show();
                watch_dock_->raise();
            }
        }
    });

    connect(memory_viewer_, &MemoryViewer::requestAddWatch, this, [this](word_t addr) {
        if (watch_panel_ != nullptr) {
            watch_panel_->addMemoryWatch(addr);
            if (watch_dock_ != nullptr) {
                watch_dock_->show();
                watch_dock_->raise();
            }
        }
    });

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
    connect(devices_panel_, &DevicesPanel::diagnosticClicked, this, [this](std::size_t line, std::size_t col) {
        showEditor();
        editor_->gotoLine(line, col);
    });

    // Wire breakpoint double-click to navigate editor
    connect(breakpoints_panel_, &BreakpointsPanel::breakpointDoubleClicked, this, [this](std::size_t line) {
        showEditor();
        editor_->gotoLine(line, 1);
    });
}

void MainWindow::setupMenusAndToolbars() {
    // -----------------------------------------------------------------
    // Actions: File
    // -----------------------------------------------------------------
    new_act_ = new QAction("&New File", this);
    new_act_->setShortcut(QKeySequence::New);
    new_act_->setStatusTip("Create a new BitForge assembly source file (Ctrl+N)");
    connect(new_act_, &QAction::triggered, this, &MainWindow::newFile);

    open_act_ = new QAction("&Open File...", this);
    open_act_->setShortcut(QKeySequence::Open);
    open_act_->setStatusTip("Open an existing .asm file (Ctrl+O)");
    connect(open_act_, &QAction::triggered, this, &MainWindow::openFile);

    save_act_ = new QAction("&Save", this);
    save_act_->setShortcut(QKeySequence::Save);
    save_act_->setStatusTip("Save current source file to disk (Ctrl+S)");
    connect(save_act_, &QAction::triggered, this, &MainWindow::save);

    save_as_act_ = new QAction("Save &As...", this);
    save_as_act_->setShortcut(QKeySequence::SaveAs);
    save_as_act_->setStatusTip("Save current source file under a new name (Ctrl+Shift+S)");
    connect(save_as_act_, &QAction::triggered, this, &MainWindow::saveAs);

    exit_act_ = new QAction("E&xit", this);
    exit_act_->setShortcut(QKeySequence::Quit);
    exit_act_->setStatusTip("Exit BitForge application");
    connect(exit_act_, &QAction::triggered, this, &MainWindow::close);

    // -----------------------------------------------------------------
    // Actions: Edit
    // -----------------------------------------------------------------
    undo_act_ = new QAction("&Undo", this);
    undo_act_->setShortcut(QKeySequence::Undo);
    undo_act_->setStatusTip("Undo last source edit (Ctrl+Z)");
    connect(undo_act_, &QAction::triggered, editor_, &CodeEditor::undo);

    redo_act_ = new QAction("&Redo", this);
    redo_act_->setShortcut(QKeySequence::Redo);
    redo_act_->setStatusTip("Redo last undone source edit (Ctrl+Y)");
    connect(redo_act_, &QAction::triggered, editor_, &CodeEditor::redo);

    cut_act_ = new QAction("Cu&t", this);
    cut_act_->setShortcut(QKeySequence::Cut);
    cut_act_->setStatusTip("Cut selected text to clipboard (Ctrl+X)");
    connect(cut_act_, &QAction::triggered, editor_, &CodeEditor::cut);

    copy_act_ = new QAction("&Copy", this);
    copy_act_->setShortcut(QKeySequence::Copy);
    copy_act_->setStatusTip("Copy selected text to clipboard (Ctrl+C)");
    connect(copy_act_, &QAction::triggered, editor_, &CodeEditor::copy);

    paste_act_ = new QAction("&Paste", this);
    paste_act_->setShortcut(QKeySequence::Paste);
    paste_act_->setStatusTip("Paste text from clipboard (Ctrl+V)");
    connect(paste_act_, &QAction::triggered, editor_, &CodeEditor::paste);

    select_all_act_ = new QAction("Select &All", this);
    select_all_act_->setShortcut(QKeySequence::SelectAll);
    select_all_act_->setStatusTip("Select all text in editor (Ctrl+A)");
    connect(select_all_act_, &QAction::triggered, editor_, &CodeEditor::selectAll);

    find_act_ = new QAction("&Find...", this);
    find_act_->setShortcut(QKeySequence::Find);
    find_act_->setStatusTip("Find text in the assembly source editor (Ctrl+F)");
    connect(find_act_, &QAction::triggered, this, &MainWindow::onFind);

    find_next_act_ = new QAction("Find &Next", this);
    find_next_act_->setShortcut(QKeySequence::FindNext);
    find_next_act_->setStatusTip("Jump to next match (F3)");
    connect(find_next_act_, &QAction::triggered, this, &MainWindow::onFindNext);

    find_prev_act_ = new QAction("Find &Previous", this);
    find_prev_act_->setShortcut(QKeySequence::FindPrevious);
    find_prev_act_->setStatusTip("Jump to previous match (Shift+F3)");
    connect(find_prev_act_, &QAction::triggered, this, &MainWindow::onFindPrevious);

    goto_line_act_ = new QAction("&Go to Line...", this);
    goto_line_act_->setShortcut(QKeySequence(tr("Ctrl+G")));
    goto_line_act_->setStatusTip("Go to specified line number (Ctrl+G)");
    connect(goto_line_act_, &QAction::triggered, this, &MainWindow::onGoToLine);

    zoom_in_act_ = new QAction("Zoom &In", this);
    zoom_in_act_->setShortcut(QKeySequence::ZoomIn);
    zoom_in_act_->setStatusTip("Increase editor font size (Ctrl++)");
    connect(zoom_in_act_, &QAction::triggered, editor_, &CodeEditor::zoomIn);

    zoom_out_act_ = new QAction("Zoom &Out", this);
    zoom_out_act_->setShortcut(QKeySequence::ZoomOut);
    zoom_out_act_->setStatusTip("Decrease editor font size (Ctrl+-)");
    connect(zoom_out_act_, &QAction::triggered, editor_, &CodeEditor::zoomOut);

    reset_zoom_act_ = new QAction("&Reset Zoom", this);
    reset_zoom_act_->setShortcut(QKeySequence(tr("Ctrl+0")));
    reset_zoom_act_->setStatusTip("Reset editor font size to default (Ctrl+0)");
    connect(reset_zoom_act_, &QAction::triggered, editor_, &CodeEditor::resetZoom);

    // -----------------------------------------------------------------
    // Actions: View
    // -----------------------------------------------------------------
    welcome_screen_act_ = new QAction("&Welcome Screen", this);
    welcome_screen_act_->setStatusTip("Show the Welcome dashboard with recent files and quick start options");
    connect(welcome_screen_act_, &QAction::triggered, this, &MainWindow::showWelcomeScreen);

    show_editor_act_ = new QAction("Code &Editor", this);
    show_editor_act_->setStatusTip("Switch view to the main assembly code editor");
    connect(show_editor_act_, &QAction::triggered, this, &MainWindow::showEditor);

    restore_default_layout_act_ = new QAction("&Restore Default Layout", this);
    restore_default_layout_act_->setStatusTip("Reset window docks and proportions to default workbench layout");
    connect(restore_default_layout_act_, &QAction::triggered, this, &MainWindow::onRestoreDefaultLayout);

    // -----------------------------------------------------------------
    // Actions: Execution & Debug
    // -----------------------------------------------------------------
    assemble_act_ = new QAction("&Assemble", this);
    assemble_act_->setShortcut(QKeySequence(Qt::Key_F7));
    assemble_act_->setStatusTip("Assemble the current source text into executable program memory (F7)");
    connect(assemble_act_, &QAction::triggered, this, &MainWindow::onAssemble);

    run_act_ = new QAction("&Run from Entry", this);
    run_act_->setShortcut(QKeySequence(Qt::Key_F5));
    run_act_->setStatusTip("Reset CPU and devices to entry point and run (F5)");
    connect(run_act_, &QAction::triggered, this, &MainWindow::onRun);

    continue_act_ = new QAction("&Continue", this);
    continue_act_->setShortcut(QKeySequence(Qt::Key_F8));
    continue_act_->setStatusTip("Resume execution from current stopped PC without resetting CPU or devices (F8)");
    connect(continue_act_, &QAction::triggered, this, &MainWindow::onContinue);

    step_act_ = new QAction("&Step", this);
    step_act_->setShortcut(QKeySequence(Qt::Key_F10));
    step_act_->setStatusTip("Single-step execute at most one instruction from current PC (F10)");
    connect(step_act_, &QAction::triggered, this, &MainWindow::onStep);

    reset_act_ = new QAction("&Reset", this);
    reset_act_->setShortcut(QKeySequence(Qt::Key_F6));
    reset_act_->setStatusTip("Reset CPU registers, memory, and devices back to initial assembled state (F6)");
    connect(reset_act_, &QAction::triggered, this, &MainWindow::onReset);

    pause_act_ = new QAction("&Pause", this);
    pause_act_->setStatusTip("Pause active execution slice at next instruction boundary");
    connect(pause_act_, &QAction::triggered, this, &MainWindow::onPause);

    toggle_bp_act_ = new QAction("Toggle &Breakpoint", this);
    toggle_bp_act_->setShortcut(QKeySequence(Qt::Key_F9));
    toggle_bp_act_->setStatusTip("Toggle breakpoint on current cursor line (F9)");
    connect(toggle_bp_act_, &QAction::triggered, this, &MainWindow::onToggleBreakpoint);

    clear_bps_act_ = new QAction("Clear All Break&points", this);
    clear_bps_act_->setStatusTip("Remove all configured breakpoints");
    connect(clear_bps_act_, &QAction::triggered, this, &MainWindow::onClearBreakpoints);

    clear_trace_act_ = new QAction("Clear Execution &Trace", this);
    clear_trace_act_->setStatusTip("Clear chronological execution trace history");
    connect(clear_trace_act_, &QAction::triggered, this, &MainWindow::onClearTrace);

    shortcuts_act_ = new QAction("&Keyboard Shortcuts...", this);
    shortcuts_act_->setStatusTip("Show all BitForge keyboard shortcuts and workbench controls");
    connect(shortcuts_act_, &QAction::triggered, this, &MainWindow::showKeyboardShortcuts);

    // Speed submenu actions
    speed_action_group_ = new QActionGroup(this);

    speed_slow_act_ = new QAction("&Slow (500 ms / step)", this);
    speed_slow_act_->setCheckable(true);
    speed_action_group_->addAction(speed_slow_act_);
    connect(speed_slow_act_, &QAction::triggered, this, [this]() {
        controller_->setSpeed(ExecutionSpeed::Slow);
    });

    speed_normal_act_ = new QAction("&Normal (100 ms / step)", this);
    speed_normal_act_->setCheckable(true);
    speed_action_group_->addAction(speed_normal_act_);
    connect(speed_normal_act_, &QAction::triggered, this, [this]() {
        controller_->setSpeed(ExecutionSpeed::Normal);
    });

    speed_fast_act_ = new QAction("&Fast (50 ms / 5 steps)", this);
    speed_fast_act_->setCheckable(true);
    speed_action_group_->addAction(speed_fast_act_);
    connect(speed_fast_act_, &QAction::triggered, this, [this]() {
        controller_->setSpeed(ExecutionSpeed::Fast);
    });

    speed_turbo_act_ = new QAction("&Turbo (Full Speed)", this);
    speed_turbo_act_->setCheckable(true);
    speed_turbo_act_->setChecked(true);
    speed_action_group_->addAction(speed_turbo_act_);
    connect(speed_turbo_act_, &QAction::triggered, this, [this]() {
        controller_->setSpeed(ExecutionSpeed::Turbo);
    });

    // -----------------------------------------------------------------
    // Actions: Help
    // -----------------------------------------------------------------
    tutorial_act_ = new QAction("Interactive &Tutorial...", this);
    tutorial_act_->setStatusTip("Take a step-by-step interactive guided tour of BitForge features");
    connect(tutorial_act_, &QAction::triggered, this, &MainWindow::onStartTutorial);

    browse_examples_act_ = new QAction("&Browse Example Library...", this);
    browse_examples_act_->setStatusTip("Browse 51 categorized assembly programs with syntax previews");
    connect(browse_examples_act_, &QAction::triggered, this, &MainWindow::onBrowseExamples);

    about_act_ = new QAction("&About BitForge", this);
    about_act_->setStatusTip("About BitForge educational emulator");
    connect(about_act_, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About BitForge",
                           "<b>BitForge</b> — Educational 8-Bit Computer Workbench<br>"
                           "Version 0.1.0<br><br>"
                           "Emulated 8-bit CPU, 64 KiB memory, virtual I/O devices, "
                           "two-pass assembler, and native Qt 6 desktop debugger.");
    });

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

    auto* edit_menu = menuBar()->addMenu("&Edit");
    edit_menu->addAction(undo_act_);
    edit_menu->addAction(redo_act_);
    edit_menu->addSeparator();
    edit_menu->addAction(cut_act_);
    edit_menu->addAction(copy_act_);
    edit_menu->addAction(paste_act_);
    edit_menu->addAction(select_all_act_);
    edit_menu->addSeparator();
    edit_menu->addAction(find_act_);
    edit_menu->addAction(find_next_act_);
    edit_menu->addAction(find_prev_act_);
    edit_menu->addAction(goto_line_act_);
    edit_menu->addSeparator();
    edit_menu->addAction(zoom_in_act_);
    edit_menu->addAction(zoom_out_act_);
    edit_menu->addAction(reset_zoom_act_);

    auto* view_menu = menuBar()->addMenu("&View");
    view_menu->addAction(welcome_screen_act_);
    view_menu->addAction(show_editor_act_);
    view_menu->addSeparator();
    view_menu->addAction(right_dock_->toggleViewAction());
    view_menu->addAction(bottom_dock_->toggleViewAction());
    view_menu->addAction(watch_dock_->toggleViewAction());
    view_menu->addSeparator();
    view_menu->addAction(restore_default_layout_act_);

    auto* exec_menu = menuBar()->addMenu("&Execution");
    exec_menu->addAction(assemble_act_);
    exec_menu->addSeparator();
    exec_menu->addAction(run_act_);
    exec_menu->addAction(continue_act_);
    exec_menu->addAction(step_act_);
    exec_menu->addAction(pause_act_);
    exec_menu->addAction(reset_act_);
    exec_menu->addSeparator();
    exec_menu->addAction(toggle_bp_act_);
    exec_menu->addAction(clear_bps_act_);
    exec_menu->addSeparator();
    exec_menu->addAction(clear_trace_act_);
    exec_menu->addSeparator();

    auto* speed_submenu = exec_menu->addMenu("Execution &Speed");
    speed_submenu->addAction(speed_slow_act_);
    speed_submenu->addAction(speed_normal_act_);
    speed_submenu->addAction(speed_fast_act_);
    speed_submenu->addAction(speed_turbo_act_);

    exec_menu->addSeparator();
    exec_menu->addAction(shortcuts_act_);

    auto* help_menu = menuBar()->addMenu("&Help");
    help_menu->addAction(tutorial_act_);
    help_menu->addAction(browse_examples_act_);
    help_menu->addSeparator();
    help_menu->addAction(shortcuts_act_);
    help_menu->addSeparator();
    help_menu->addAction(about_act_);

    // -----------------------------------------------------------------
    // Toolbar: Primary Execution Controls
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

    auto* speed_label = new QLabel(" Speed: ", toolbar);
    toolbar->addWidget(speed_label);

    speed_combo_ = new QComboBox(toolbar);
    speed_combo_->addItem("Slow (500 ms / step)");
    speed_combo_->addItem("Normal (100 ms / step)");
    speed_combo_->addItem("Fast (50 ms / 5 steps)");
    speed_combo_->addItem("Turbo (Full Speed)");
    speed_combo_->setCurrentIndex(3); // Default Turbo
    toolbar->addWidget(speed_combo_);
    connect(speed_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSpeedComboChanged);

    connect(controller_, &ExecutionController::speedChanged, this, [this](ExecutionSpeed spd) {
        int idx = 3;
        switch (spd) {
            case ExecutionSpeed::Slow: idx = 0; speed_slow_act_->setChecked(true); break;
            case ExecutionSpeed::Normal: idx = 1; speed_normal_act_->setChecked(true); break;
            case ExecutionSpeed::Fast: idx = 2; speed_fast_act_->setChecked(true); break;
            case ExecutionSpeed::Turbo: idx = 3; speed_turbo_act_->setChecked(true); break;
        }
        if (speed_combo_->currentIndex() != idx) {
            const QSignalBlocker blocker(speed_combo_);
            speed_combo_->setCurrentIndex(idx);
        }
    });

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
    state_badge_->setStyleSheet(theme::stateBadgeStyle(MachineState::Ready, false, false));
    toolbar->addWidget(state_badge_);
}

void MainWindow::setupStatusBar() {
    status_file_label_ = new QLabel("[Untitled]", this);
    status_cursor_label_ = new QLabel("Ln 1, Col 1", this);
    status_state_label_ = new QLabel("READY", this);
    status_pc_label_ = new QLabel("PC: 0x0200", this);
    status_steps_label_ = new QLabel("Steps: 0", this);

    statusBar()->addWidget(status_file_label_, 1);
    statusBar()->addPermanentWidget(status_cursor_label_);
    statusBar()->addPermanentWidget(status_state_label_);
    statusBar()->addPermanentWidget(status_pc_label_);
    statusBar()->addPermanentWidget(status_steps_label_);
}

void MainWindow::loadSettings() {
    QSettings settings("BitForge", "BitForge");

    // 1. Window geometry and dock layout state
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    if (settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }

    // 2. Editor font size / zoom level (safe fallback clamp [6, 48])
    if (editor_ != nullptr && settings.contains("editorFontSize")) {
        bool ok_font = false;
        int fontSize = settings.value("editorFontSize", 11).toInt(&ok_font);
        if (ok_font && fontSize >= 6 && fontSize <= 48) {
            editor_->setFontSize(fontSize);
        }
    }

    // 3. Execution speed preset (safe fallback clamp [0, 3])
    if (controller_ != nullptr && settings.contains("executionSpeed")) {
        bool ok_speed = false;
        int speedVal = settings.value("executionSpeed", static_cast<int>(ExecutionSpeed::Turbo)).toInt(&ok_speed);
        if (ok_speed && speedVal >= 0 && speedVal <= 3) {
            auto sp = static_cast<ExecutionSpeed>(speedVal);
            controller_->setSpeed(sp);
            if (speed_combo_ != nullptr) {
                speed_combo_->setCurrentIndex(speedVal);
            }
        }
    }

    // 4. Watch expressions list
    if (watch_panel_ != nullptr && settings.contains("watchList")) {
        QStringList watches = settings.value("watchList").toStringList();
        if (!watches.isEmpty()) {
            watch_panel_->loadWatchExpressions(watches);
        }
    }

    // 5. Recent files list
    QStringList recents = settings.value("recentFiles").toStringList();
    QStringList valid_recents;
    for (const auto& path : recents) {
        if (QFile::exists(path)) {
            valid_recents.append(path);
        }
    }
    if (welcome_widget_) {
        welcome_widget_->setRecentFiles(valid_recents);
    }
}

void MainWindow::saveSettings() {
    QSettings settings("BitForge", "BitForge");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    if (editor_ != nullptr) {
        settings.setValue("editorFontSize", editor_->currentFontSize());
    }
    if (controller_ != nullptr) {
        settings.setValue("executionSpeed", static_cast<int>(controller_->speed()));
    }
    if (watch_panel_ != nullptr) {
        settings.setValue("watchList", watch_panel_->watchExpressions());
    }
}

void MainWindow::updateRecentFilesList(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;
    QSettings settings("BitForge", "BitForge");
    QStringList recents = settings.value("recentFiles").toStringList();
    QString abs_path = QFileInfo(filePath).absoluteFilePath();
    recents.removeAll(abs_path);
    recents.prepend(abs_path);
    while (recents.size() > 10) {
        recents.removeLast();
    }
    settings.setValue("recentFiles", recents);
    if (welcome_widget_) {
        welcome_widget_->setRecentFiles(recents);
    }
}

void MainWindow::showWelcomeScreen() {
    central_stack_->setCurrentWidget(welcome_widget_);
}

void MainWindow::showEditor() {
    central_stack_->setCurrentWidget(editor_container_);
    editor_->setFocus();
}

void MainWindow::onFind() {
    showEditor();
    find_bar_->showAndFocus();
    QTextCursor cursor = editor_->textCursor();
    if (cursor.hasSelection() && !cursor.selectedText().isEmpty()) {
        find_bar_->searchInput()->setText(cursor.selectedText());
    }
    find_bar_->searchInput()->setFocus();
    find_bar_->searchInput()->selectAll();
}

void MainWindow::onFindNext() {
    if (!find_bar_->isVisible()) {
        onFind();
    } else {
        find_bar_->triggerFindNext();
    }
}

void MainWindow::onFindPrevious() {
    if (!find_bar_->isVisible()) {
        onFind();
    } else {
        find_bar_->triggerFindPrevious();
    }
}

void MainWindow::onGoToLine() {
    showEditor();
    int total_lines = std::max(1, editor_->document()->blockCount());
    int current_line = editor_->textCursor().blockNumber() + 1;
    bool ok = false;
    int target = QInputDialog::getInt(
        this, "Go to Line",
        QString("Enter line number (1 - %1):").arg(total_lines),
        current_line, 1, total_lines, 1, &ok
    );
    if (ok) {
        editor_->gotoLine(static_cast<std::size_t>(target), 1);
    }
}

void MainWindow::onRestoreDefaultLayout() {
    addDockWidget(Qt::RightDockWidgetArea, right_dock_);
    addDockWidget(Qt::BottomDockWidgetArea, bottom_dock_);
    if (watch_dock_ != nullptr) {
        addDockWidget(Qt::BottomDockWidgetArea, watch_dock_);
        tabifyDockWidget(bottom_dock_, watch_dock_);
        watch_dock_->show();
        watch_dock_->setFloating(false);
    }
    right_dock_->show();
    bottom_dock_->show();
    right_dock_->setFloating(false);
    bottom_dock_->setFloating(false);
    resize(1240, 820);
    showEditor();
    statusBar()->showMessage("Workbench layout restored to default proportions.", 3000);
}

void MainWindow::onBrowseExamples() {
    ExamplesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.selectedExamplePath();
        if (!path.isEmpty()) {
            if (loadFile(path)) {
                showEditor();
            }
        }
    }
}

void MainWindow::onStartTutorial() {
    if (!active_tutorial_) {
        active_tutorial_ = new TutorialGuide(this);
        active_tutorial_->setController(controller_);
        connect(active_tutorial_, &TutorialGuide::loadTutorialExampleRequested, this, [this]() {
            QString path = resolveExamplePath("examples/01_arithmetic.asm");
            if (loadFile(path)) {
                showEditor();
            }
        });
    }
    active_tutorial_->show();
    active_tutorial_->raise();
    active_tutorial_->activateWindow();
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
        const bool can_execute = !is_stale && (st != MachineState::Halted && st != MachineState::Faulted);
        run_act_->setEnabled(!is_stale);
        step_act_->setEnabled(can_execute);
        continue_act_->setEnabled(can_execute);
        reset_act_->setEnabled(true);
    }

    // Update state badge
    if (is_executing) {
        state_badge_->setText(" EXECUTING ");
    } else if (is_stale) {
        state_badge_->setText(" STALE (Reassemble) ");
    } else {
        switch (st) {
            case MachineState::Ready:
                state_badge_->setText(" READY ");
                break;
            case MachineState::Running:
            case MachineState::Paused:
                state_badge_->setText(" PAUSED ");
                break;
            case MachineState::Halted:
                state_badge_->setText(" HALTED ");
                break;
            case MachineState::Faulted:
                state_badge_->setText(" FAULTED ");
                break;
        }
    }
    state_badge_->setStyleSheet(theme::stateBadgeStyle(st, is_executing, is_stale));

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
        controller_->unloadProgram();
        showEditor();
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
    controller_->unloadProgram();
    updateRecentFilesList(filePath);
    showEditor();
    statusBar()->showMessage(QString("Loaded %1").arg(filePath), 3000);
    return true;
}

bool MainWindow::loadStartupFile(const QStringList& args) {
    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args[i];
        if (arg == "--") {
            if (i + 1 < args.size()) {
                return loadFile(args[i + 1]);
            }
            break;
        }
        if (arg.startsWith("-")) {
            // Skip common Qt command-line options that take a subsequent argument
            if ((arg == "-platform" || arg == "-style" || arg == "-geometry" ||
                 arg == "-plugin" || arg == "-session" || arg == "-display" ||
                 arg == "-font") &&
                i + 1 < args.size()) {
                ++i;
            }
            continue;
        }
        return loadFile(arg);
    }
    return false;
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
    updateRecentFilesList(filePath);
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
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::onAssemble() {
    showEditor();
    const QString code = editor_->toPlainText();
    bool ok = controller_->assemble(code);
    if (ok) {
        statusBar()->showMessage("Assembly successful. Program loaded into emulated memory.", 4000);
    } else {
        statusBar()->showMessage("Assembly failed. See Diagnostics tab for details.", 6000);
    }
}

void MainWindow::onRun() {
    showEditor();
    if (controller_->isStale()) {
        statusBar()->showMessage("Source modified since last assembly. Please reassemble (F7) before running.", 5000);
        return;
    }
    const std::uint64_t budget = static_cast<std::uint64_t>(budget_spinbox_->value());
    controller_->runFromEntry(budget);
}

void MainWindow::onContinue() {
    showEditor();
    if (controller_->isStale()) {
        statusBar()->showMessage("Source modified since last assembly. Please reassemble (F7) before continuing.", 5000);
        return;
    }
    const auto st = controller_->session().state();
    if (st == MachineState::Halted || st == MachineState::Faulted) {
        statusBar()->showMessage("Cannot continue: CPU is halted or faulted. Use Reset (F6) or Run from Entry (F5).", 4000);
        return;
    }
    const std::uint64_t budget = static_cast<std::uint64_t>(budget_spinbox_->value());
    controller_->continueExecution(budget);
}

void MainWindow::onStep() {
    showEditor();
    if (controller_->isStale()) {
        statusBar()->showMessage("Source modified since last assembly. Please reassemble (F7) before stepping.", 5000);
        return;
    }
    controller_->step();
}

void MainWindow::onReset() {
    showEditor();
    controller_->reset();
    statusBar()->showMessage("Execution reset to entry point.", 3000);
}

void MainWindow::onPause() {
    controller_->pause();
    statusBar()->showMessage("Execution paused by user.", 3000);
}

void MainWindow::onSpeedComboChanged(int index) {
    switch (index) {
        case 0: controller_->setSpeed(ExecutionSpeed::Slow); break;
        case 1: controller_->setSpeed(ExecutionSpeed::Normal); break;
        case 2: controller_->setSpeed(ExecutionSpeed::Fast); break;
        case 3: controller_->setSpeed(ExecutionSpeed::Turbo); break;
        default: break;
    }
}

void MainWindow::onToggleBreakpoint() {
    showEditor();
    editor_->toggleBreakpointAtCursor();
}

void MainWindow::onClearBreakpoints() {
    controller_->clearAllBreakpoints();
    statusBar()->showMessage("All breakpoints cleared.", 3000);
}

void MainWindow::onClearTrace() {
    controller_->clearTrace();
    statusBar()->showMessage("Execution trace cleared.", 3000);
}

void MainWindow::showKeyboardShortcuts() {
    QMessageBox::information(
        this, "BitForge Keyboard Shortcuts",
        "<h3>BitForge Workbench Shortcuts</h3>"
        "<table cellpadding='4' cellspacing='0' border='1'>"
        "<tr><th>Key</th><th>Action</th><th>Description</th></tr>"
        "<tr><td><b>F5</b></td><td>Run from Entry</td><td>Resets CPU to entry point and runs until breakpoint/halt</td></tr>"
        "<tr><td><b>F6</b></td><td>Reset</td><td>Resets registers, memory, and devices to initial state</td></tr>"
        "<tr><td><b>F7</b></td><td>Assemble</td><td>Assembles current source editor text into memory</td></tr>"
        "<tr><td><b>F8</b></td><td>Continue</td><td>Resumes execution from current PC without resetting</td></tr>"
        "<tr><td><b>F9</b></td><td>Toggle Breakpoint</td><td>Toggles breakpoint on line at cursor / gutter</td></tr>"
        "<tr><td><b>F10</b></td><td>Step</td><td>Single-steps exactly one instruction from current PC</td></tr>"
        "<tr><td><b>Ctrl+N</b></td><td>New File</td><td>Creates a new empty assembly file</td></tr>"
        "<tr><td><b>Ctrl+O</b></td><td>Open File</td><td>Opens an existing assembly file</td></tr>"
        "<tr><td><b>Ctrl+S</b></td><td>Save</td><td>Saves the current file to disk</td></tr>"
        "<tr><td><b>Ctrl+Shift+S</b></td><td>Save As</td><td>Saves current file to a new path</td></tr>"
        "<tr><td><b>Ctrl+F</b></td><td>Find</td><td>Opens the inline find/search bar</td></tr>"
        "<tr><td><b>F3 / Shift+F3</b></td><td>Find Next / Prev</td><td>Navigates between search matches</td></tr>"
        "<tr><td><b>Ctrl+G</b></td><td>Go to Line</td><td>Jump directly to a line number</td></tr>"
        "<tr><td><b>Ctrl++ / Ctrl+-</b></td><td>Zoom In / Out</td><td>Adjust editor font size</td></tr>"
        "<tr><td><b>Ctrl+0</b></td><td>Reset Zoom</td><td>Restore default editor font size</td></tr>"
        "</table>"
    );
}

} // namespace bitforge::gui
