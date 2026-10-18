#include "bitforge/gui/main_window.hpp"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QSignalBlocker>
#include <QSettings>
#include <QApplication>
#include <QShortcut>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>

namespace bitforge::gui {

namespace {

QString canonicalizePath(const QString& path) {
    if (path.isEmpty()) return QString();
    QFileInfo fi(path);
    QString canonical = fi.canonicalFilePath();
    if (canonical.isEmpty()) {
        canonical = fi.absoluteFilePath();
    }
    return QDir::cleanPath(canonical);
}

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

const char* kFirstProgramTutorialAsm =
    "; ==============================================================================\n"
    "; BitForge Tutorial — First Assembly Program\n"
    "; ==============================================================================\n"
    "; This beginner program loads two 8-bit numbers into registers R0 and R1,\n"
    "; computes their sum, writes the sum to the Virtual LEDs (Port 0x10),\n"
    "; stores the result in memory at address 0x8000, and halts execution.\n"
    ";\n"
    "; Instructions to Try:\n"
    "; 1. Press F7 to Assemble this program into emulated memory.\n"
    "; 2. Place cursor on Line 20 (ADD R0, R1) and press F9 to set a Breakpoint.\n"
    "; 3. Press F5 (Run from Entry). Notice execution stops BEFORE the ADD executes!\n"
    "; 4. Check R0 (10) and R1 (20) in the CPU Registers panel on the right.\n"
    "; 5. Press F10 (Step). Observe R0 mutate to 30 (0x1E) with amber highlight!\n"
    "; 6. Press F8 (Continue). Execution completes and the CPU enters HALTED state.\n"
    "; 7. Look at the Virtual LEDs panel to see the glowing binary pattern of 30!\n"
    "; ==============================================================================\n\n"
    ".ORG 0x0200             ; Default program execution origin\n"
    ".ENTRY start            ; Execution entry point label\n\n"
    "start:\n"
    "    LDI R0, 10          ; Load immediate decimal value 10 into Register R0\n"
    "    LDI R1, 20          ; Load immediate decimal value 20 into Register R1\n"
    "    ADD R0, R1          ; Add R1 into R0: R0 = 10 + 20 = 30 (0x1E)\n"
    "    OUT 0x10, R0        ; Output sum to Virtual LEDs (Port 0x10)\n"
    "    ST  [0x8000], R0    ; Store sum into memory address 0x8000\n"
    "    HALT                ; Stop CPU execution cleanly\n";

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

    updateUiState();
    updateWindowTitle();

    if (size().width() < 600 || size().height() < 400) {
        resize(1240, 820);
    }
}

void MainWindow::setupUi() {
    // Central Container: Find/Replace Bar on top, Tabbed Document Workspace below
    central_container_ = new QWidget(this);
    auto* central_layout = new QVBoxLayout(central_container_);
    central_layout->setContentsMargins(0, 0, 0, 0);
    central_layout->setSpacing(0);

    find_bar_ = new FindReplaceBar(central_container_);
    find_bar_->setVisible(false);
    central_layout->addWidget(find_bar_);

    document_tabs_ = new QTabWidget(central_container_);
    document_tabs_->setTabsClosable(true);
    document_tabs_->setMovable(true);
    document_tabs_->setElideMode(Qt::ElideRight);
    document_tabs_->setDocumentMode(true);
    central_layout->addWidget(document_tabs_);
    setCentralWidget(central_container_);

    // 1. Tab 0: Welcome & Tutorial Center
    welcome_widget_ = new WelcomeWidget(document_tabs_);
    document_tabs_->addTab(welcome_widget_, "⚡ Welcome");
    document_tabs_->setTabToolTip(0, "BitForge Welcome & Tutorial Center");

    // 2. Tab 1: Default initial untitled editor document
    createEditorTab(); // Creates Untitled-1

    // Default tab at startup: Welcome tab (index 0)
    document_tabs_->setCurrentIndex(0);

    // Wire tab signals
    connect(document_tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(document_tabs_, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    document_tabs_->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(document_tabs_->tabBar(), &QWidget::customContextMenuRequested, this, &MainWindow::onTabContextMenu);

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
    connect(welcome_widget_, &WelcomeWidget::openFirstProgramTutorialRequested, this, &MainWindow::onFirstProgramTutorial);
    connect(welcome_widget_, &WelcomeWidget::openExampleRequested, this, [this](const QString& relPath) {
        QString path = resolveExamplePath(relPath);
        if (loadFile(path)) {
            showEditor();
        }
    });
    connect(welcome_widget_, &WelcomeWidget::showOnStartupChanged, this, [](bool show) {
        QSettings settings("BitForge", "BitForge");
        settings.setValue("showWelcomeOnStartup", show);
    });

    // Wire FindReplaceBar actions to active editor
    connect(find_bar_, &FindReplaceBar::findNext, this, [this](const QString& query, bool match_case) {
        auto* ed = currentEditor();
        if (ed) {
            int match_idx = 0;
            int total_matches = 0;
            ed->findText(query, true, match_case, &match_idx, &total_matches);
            find_bar_->setMatchStatus(match_idx, total_matches);
        }
    });
    connect(find_bar_, &FindReplaceBar::findPrevious, this, [this](const QString& query, bool match_case) {
        auto* ed = currentEditor();
        if (ed) {
            int match_idx = 0;
            int total_matches = 0;
            ed->findText(query, false, match_case, &match_idx, &total_matches);
            find_bar_->setMatchStatus(match_idx, total_matches);
        }
    });
    connect(find_bar_, &FindReplaceBar::closed, this, [this]() {
        auto* ed = currentEditor();
        if (ed) {
            ed->clearFindMatches();
            ed->setFocus();
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
    bottom_tabs->addTab(stack_viewer_, "Hardware Stack");

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

    // Wire diagnostic click to navigate editor
    connect(devices_panel_, &DevicesPanel::diagnosticClicked, this, [this](std::size_t line, std::size_t col) {
        if (executing_editor_ != nullptr) {
            int tab_idx = document_tabs_->indexOf(executing_editor_);
            if (tab_idx >= 0) {
                document_tabs_->setCurrentIndex(tab_idx);
            }
            executing_editor_->gotoLine(line, col);
        } else if (currentEditor() != nullptr) {
            showEditor();
            currentEditor()->gotoLine(line, col);
        }
    });

    // Wire breakpoint double-click to navigate editor
    connect(breakpoints_panel_, &BreakpointsPanel::breakpointDoubleClicked, this, [this](std::size_t line) {
        if (executing_editor_ != nullptr) {
            int tab_idx = document_tabs_->indexOf(executing_editor_);
            if (tab_idx >= 0) {
                document_tabs_->setCurrentIndex(tab_idx);
            }
            executing_editor_->gotoLine(line, 1);
        } else if (currentEditor() != nullptr) {
            showEditor();
            currentEditor()->gotoLine(line, 1);
        }
    });
}

void MainWindow::setupMenusAndToolbars() {
    // -----------------------------------------------------------------
    // Actions: File
    // -----------------------------------------------------------------
    new_act_ = new QAction("&New File", this);
    new_act_->setShortcut(QKeySequence::New);
    new_act_->setStatusTip("Create a new BitForge assembly source document tab (Ctrl+N)");
    connect(new_act_, &QAction::triggered, this, &MainWindow::newFile);

    open_act_ = new QAction("&Open File...", this);
    open_act_->setShortcut(QKeySequence::Open);
    open_act_->setStatusTip("Open an existing .asm file into a tab (Ctrl+O)");
    connect(open_act_, &QAction::triggered, this, &MainWindow::openFile);

    save_act_ = new QAction("&Save", this);
    save_act_->setShortcut(QKeySequence::Save);
    save_act_->setStatusTip("Save current active document to disk (Ctrl+S)");
    connect(save_act_, &QAction::triggered, this, &MainWindow::save);

    save_as_act_ = new QAction("Save &As...", this);
    save_as_act_->setShortcut(QKeySequence::SaveAs);
    save_as_act_->setStatusTip("Save current document under a new name (Ctrl+Shift+S)");
    connect(save_as_act_, &QAction::triggered, this, &MainWindow::saveAs);

    close_tab_act_ = new QAction("&Close Tab", this);
    close_tab_act_->setShortcut(QKeySequence(tr("Ctrl+W")));
    close_tab_act_->setStatusTip("Close the active document tab (Ctrl+W)");
    connect(close_tab_act_, &QAction::triggered, this, &MainWindow::closeActiveTab);

    close_all_tabs_act_ = new QAction("Close &All Tabs", this);
    close_all_tabs_act_->setStatusTip("Close all open document tabs");
    connect(close_all_tabs_act_, &QAction::triggered, this, &MainWindow::closeAllTabs);

    close_other_tabs_act_ = new QAction("Close &Other Tabs", this);
    close_other_tabs_act_->setStatusTip("Close all document tabs except the active one");
    connect(close_other_tabs_act_, &QAction::triggered, this, [this]() {
        closeOtherTabs(document_tabs_->currentIndex());
    });

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
    connect(undo_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->undo();
    });

    redo_act_ = new QAction("&Redo", this);
    redo_act_->setShortcut(QKeySequence::Redo);
    redo_act_->setStatusTip("Redo last undone source edit (Ctrl+Y)");
    connect(redo_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->redo();
    });

    cut_act_ = new QAction("Cu&t", this);
    cut_act_->setShortcut(QKeySequence::Cut);
    cut_act_->setStatusTip("Cut selected text to clipboard (Ctrl+X)");
    connect(cut_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->cut();
    });

    copy_act_ = new QAction("&Copy", this);
    copy_act_->setShortcut(QKeySequence::Copy);
    copy_act_->setStatusTip("Copy selected text to clipboard (Ctrl+C)");
    connect(copy_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->copy();
    });

    paste_act_ = new QAction("&Paste", this);
    paste_act_->setShortcut(QKeySequence::Paste);
    paste_act_->setStatusTip("Paste text from clipboard (Ctrl+V)");
    connect(paste_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->paste();
    });

    select_all_act_ = new QAction("Select &All", this);
    select_all_act_->setShortcut(QKeySequence::SelectAll);
    select_all_act_->setStatusTip("Select all text in editor (Ctrl+A)");
    connect(select_all_act_, &QAction::triggered, this, [this]() {
        if (auto* ed = currentEditor()) ed->selectAll();
    });

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
    connect(zoom_in_act_, &QAction::triggered, this, [this]() {
        current_font_size_ = std::min(48, current_font_size_ + 1);
        for (const auto& doc : editor_tabs_) {
            if (doc.editor) doc.editor->setFontSize(current_font_size_);
        }
    });

    zoom_out_act_ = new QAction("Zoom &Out", this);
    zoom_out_act_->setShortcut(QKeySequence::ZoomOut);
    zoom_out_act_->setStatusTip("Decrease editor font size (Ctrl+-)");
    connect(zoom_out_act_, &QAction::triggered, this, [this]() {
        current_font_size_ = std::max(6, current_font_size_ - 1);
        for (const auto& doc : editor_tabs_) {
            if (doc.editor) doc.editor->setFontSize(current_font_size_);
        }
    });

    reset_zoom_act_ = new QAction("&Reset Zoom", this);
    reset_zoom_act_->setShortcut(QKeySequence(tr("Ctrl+0")));
    reset_zoom_act_->setStatusTip("Reset editor font size to default (Ctrl+0)");
    connect(reset_zoom_act_, &QAction::triggered, this, [this]() {
        current_font_size_ = 10;
        for (const auto& doc : editor_tabs_) {
            if (doc.editor) doc.editor->resetZoom();
        }
    });

    // -----------------------------------------------------------------
    // Actions: View
    // -----------------------------------------------------------------
    welcome_screen_act_ = new QAction("&Welcome Screen", this);
    welcome_screen_act_->setStatusTip("Open the Welcome & Tutorial Center tab");
    connect(welcome_screen_act_, &QAction::triggered, this, &MainWindow::showWelcomeScreen);

    show_editor_act_ = new QAction("Code &Editor", this);
    show_editor_act_->setStatusTip("Switch focus to the active assembly code editor tab");
    connect(show_editor_act_, &QAction::triggered, this, &MainWindow::showEditor);

    next_tab_act_ = new QAction("&Next Document Tab", this);
    next_tab_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
    next_tab_act_->setStatusTip("Switch to the next document tab (Ctrl+Tab)");
    connect(next_tab_act_, &QAction::triggered, this, &MainWindow::nextTab);

    prev_tab_act_ = new QAction("&Previous Document Tab", this);
    prev_tab_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    prev_tab_act_->setStatusTip("Switch to the previous document tab (Ctrl+Shift+Tab)");
    connect(prev_tab_act_, &QAction::triggered, this, &MainWindow::previousTab);

    restore_default_layout_act_ = new QAction("&Restore Default Layout", this);
    restore_default_layout_act_->setStatusTip("Reset window docks and proportions to default workbench layout");
    connect(restore_default_layout_act_, &QAction::triggered, this, &MainWindow::onRestoreDefaultLayout);

    // -----------------------------------------------------------------
    // Actions: Execution & Debug
    // -----------------------------------------------------------------
    assemble_act_ = new QAction("&Assemble", this);
    assemble_act_->setShortcut(QKeySequence(Qt::Key_F7));
    assemble_act_->setStatusTip("Assemble the active document into executable program memory (F7)");
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
    // Actions: Help & Onboarding
    // -----------------------------------------------------------------
    tutorial_act_ = new QAction("Interactive &Guided Tour...", this);
    tutorial_act_->setStatusTip("Take a step-by-step interactive 9-stage guided tour of BitForge features");
    connect(tutorial_act_, &QAction::triggered, this, &MainWindow::onStartTutorial);

    first_program_act_ = new QAction("&First Program Tutorial...", this);
    first_program_act_->setStatusTip("Open hands-on beginner tutorial document: tutorial_first_program.asm");
    connect(first_program_act_, &QAction::triggered, this, &MainWindow::onFirstProgramTutorial);

    curriculum_act_ = new QAction("Browse &Curriculum (11 Lessons)...", this);
    curriculum_act_->setStatusTip("Browse 11 structured lessons covering CPU architecture and workbench tools");
    connect(curriculum_act_, &QAction::triggered, this, &MainWindow::onCurriculumRequested);

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
                           "multi-document editor tabs, two-pass assembler, and native Qt 6 desktop debugger.");
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
    file_menu->addAction(close_tab_act_);
    file_menu->addAction(close_other_tabs_act_);
    file_menu->addAction(close_all_tabs_act_);
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
    view_menu->addAction(next_tab_act_);
    view_menu->addAction(prev_tab_act_);
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
    help_menu->addAction(welcome_screen_act_);
    help_menu->addAction(tutorial_act_);
    help_menu->addAction(first_program_act_);
    help_menu->addAction(curriculum_act_);
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
    status_file_label_ = new QLabel("[Untitled-1]", this);
    status_file_label_->setMinimumWidth(0);
    status_file_label_->setToolTip("[Untitled-1]");
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

CodeEditor* MainWindow::currentEditor() const noexcept {
    if (current_editor_ != nullptr) return current_editor_;
    for (const auto& doc : editor_tabs_) {
        if (doc.editor != nullptr) return doc.editor;
    }
    return nullptr;
}

CodeEditor* MainWindow::editor() const noexcept {
    return currentEditor();
}

QString MainWindow::currentFilePath() const noexcept {
    if (current_editor_ != nullptr) {
        int idx = findTabByEditor(current_editor_);
        if (idx >= 0 && idx < editor_tabs_.size()) {
            return editor_tabs_[idx].file_path;
        }
    }
    return QString();
}

int MainWindow::createEditorTab(const QString& filePath, const QString& initialContent) {
    auto* ed = new CodeEditor(document_tabs_);
    ed->setController(controller_);
    ed->setFontSize(current_font_size_);

    EditorDocumentInfo info;
    info.editor = ed;

    if (filePath.isEmpty()) {
        info.title = QString("Untitled-%1").arg(untitled_counter_++);
        info.file_path = QString();
        info.is_untitled = true;
    } else {
        info.file_path = filePath;
        info.title = QFileInfo(filePath).fileName();
        info.is_untitled = false;
    }

    if (!initialContent.isEmpty()) {
        ed->setPlainText(initialContent);
    }
    if (!filePath.isEmpty()) {
        ed->document()->setModified(false);
    }

    int tab_idx = document_tabs_->addTab(ed, info.title);
    document_tabs_->setTabToolTip(tab_idx, info.is_untitled ? QString("[%1] — Unsaved Document").arg(info.title) : info.file_path);

    editor_tabs_.append(info);

    // Wire editor signals
    connect(ed->document(), &QTextDocument::contentsChanged, this, [this, ed]() {
        onDocumentContentsChanged(ed);
    });
    connect(ed->document(), &QTextDocument::modificationChanged, this, [this, ed](bool) {
        onDocumentContentsChanged(ed);
    });
    connect(ed, &CodeEditor::cursorLineColumnChanged, this, [this, ed](int line, int col) {
        onEditorCursorChanged(ed, line, col);
    });

    current_editor_ = ed;
    document_tabs_->setCurrentIndex(tab_idx);
    updateWindowTitle();
    return tab_idx;
}

int MainWindow::findTabByPath(const QString& filePath) const {
    if (filePath.isEmpty()) return -1;
    const QString target_canonical = canonicalizePath(filePath);
    for (int i = 0; i < editor_tabs_.size(); ++i) {
        if (!editor_tabs_[i].is_untitled && !editor_tabs_[i].file_path.isEmpty()) {
            if (QString::compare(canonicalizePath(editor_tabs_[i].file_path), target_canonical, Qt::CaseInsensitive) == 0) {
                return document_tabs_->indexOf(editor_tabs_[i].editor);
            }
        }
    }
    return -1;
}

int MainWindow::findTabByEditor(const CodeEditor* editor) const {
    if (!editor) return -1;
    for (int i = 0; i < editor_tabs_.size(); ++i) {
        if (editor_tabs_[i].editor == editor) {
            return i;
        }
    }
    return -1;
}

void MainWindow::updateTabTitle(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= document_tabs_->count()) return;
    QWidget* w = document_tabs_->widget(tabIndex);
    if (w == welcome_widget_) return;

    auto* ed = qobject_cast<CodeEditor*>(w);
    if (!ed) return;

    int info_idx = findTabByEditor(ed);
    if (info_idx < 0) return;

    QString display_title = editor_tabs_[info_idx].title;
    if (ed->document()->isModified()) {
        display_title += " *";
    }
    document_tabs_->setTabText(tabIndex, display_title);
}

void MainWindow::onDocumentContentsChanged(CodeEditor* editor) {
    if (!editor) return;
    int tab_idx = document_tabs_->indexOf(editor);
    if (tab_idx >= 0) {
        updateTabTitle(tab_idx);
    }
    if (editor == executing_editor_ && controller_->isAssembled()) {
        controller_->setStale(true);
    }
    if (editor == currentEditor()) {
        updateWindowTitle();
    }
}

void MainWindow::onEditorCursorChanged(CodeEditor* editor, int line, int col) {
    if (editor == currentEditor() && status_cursor_label_ != nullptr) {
        status_cursor_label_->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    }
}

void MainWindow::onTabCloseRequested(int index) {
    closeTab(index);
}

bool MainWindow::closeTab(int index) {
    if (index < 0 || index >= document_tabs_->count()) return false;
    QWidget* w = document_tabs_->widget(index);

    if (w == welcome_widget_) {
        document_tabs_->removeTab(index);
        if (document_tabs_->count() == 0) {
            showWelcomeScreen();
        }
        return true;
    }

    auto* ed = qobject_cast<CodeEditor*>(w);
    if (!ed) {
        document_tabs_->removeTab(index);
        return true;
    }

    int info_idx = findTabByEditor(ed);
    if (info_idx < 0) {
        document_tabs_->removeTab(index);
        return true;
    }

    if (ed->document()->isModified()) {
        const QMessageBox::StandardButton ret = QMessageBox::warning(
            this, "BitForge",
            QString("The document '%1' has unsaved changes.\nDo you want to save before closing?").arg(editor_tabs_[info_idx].title),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
        if (ret == QMessageBox::Save) {
            if (!saveTab(index)) {
                return false; // User cancelled save or write failed
            }
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }

    if (ed == executing_editor_) {
        executing_editor_ = nullptr;
    }
    if (ed == current_editor_) {
        current_editor_ = nullptr;
    }

    editor_tabs_.removeAt(info_idx);
    document_tabs_->removeTab(index);
    ed->deleteLater();

    if (document_tabs_->count() == 0) {
        showWelcomeScreen();
    } else {
        auto* active_ed = currentEditor();
        if (active_ed != nullptr) {
            current_editor_ = active_ed;
        }
        updateWindowTitle();
        updateUiState();
    }
    return true;
}

bool MainWindow::closeActiveTab() {
    return closeTab(document_tabs_->currentIndex());
}

bool MainWindow::closeAllTabs() {
    // Check all dirty editor tabs first
    for (int i = document_tabs_->count() - 1; i >= 0; --i) {
        QWidget* w = document_tabs_->widget(i);
        if (w == welcome_widget_) continue;
        auto* ed = qobject_cast<CodeEditor*>(w);
        if (ed && ed->document()->isModified()) {
            document_tabs_->setCurrentIndex(i);
            int info_idx = findTabByEditor(ed);
            QString docTitle = (info_idx >= 0) ? editor_tabs_[info_idx].title : "Document";
            const QMessageBox::StandardButton ret = QMessageBox::warning(
                this, "BitForge",
                QString("The document '%1' has unsaved changes.\nDo you want to save before closing?").arg(docTitle),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );
            if (ret == QMessageBox::Save) {
                if (!saveTab(i)) return false;
            } else if (ret == QMessageBox::Cancel) {
                return false;
            }
        }
    }

    // Now close all editor tabs cleanly
    while (!editor_tabs_.isEmpty()) {
        auto doc = editor_tabs_.takeLast();
        int tab_idx = document_tabs_->indexOf(doc.editor);
        if (tab_idx >= 0) {
            document_tabs_->removeTab(tab_idx);
        }
        doc.editor->deleteLater();
    }
    current_editor_ = nullptr;
    executing_editor_ = nullptr;

    if (document_tabs_->count() == 0) {
        showWelcomeScreen();
    }
    updateWindowTitle();
    updateUiState();
    return true;
}

void MainWindow::closeOtherTabs(int keepIndex) {
    if (keepIndex < 0 || keepIndex >= document_tabs_->count()) return;
    QWidget* keepWidget = document_tabs_->widget(keepIndex);

    for (int i = document_tabs_->count() - 1; i >= 0; --i) {
        if (document_tabs_->widget(i) == keepWidget) continue;
        closeTab(i);
    }
}

void MainWindow::nextTab() {
    if (document_tabs_->count() > 1) {
        int next = (document_tabs_->currentIndex() + 1) % document_tabs_->count();
        document_tabs_->setCurrentIndex(next);
    }
}

void MainWindow::previousTab() {
    if (document_tabs_->count() > 1) {
        int prev = (document_tabs_->currentIndex() - 1 + document_tabs_->count()) % document_tabs_->count();
        document_tabs_->setCurrentIndex(prev);
    }
}

void MainWindow::onCurrentTabChanged(int index) {
    if (index < 0 || index >= document_tabs_->count()) return;
    QWidget* w = document_tabs_->widget(index);

    if (w == welcome_widget_) {
        updateWindowTitle();
        if (status_cursor_label_) status_cursor_label_->setText("");
    } else if (auto* ed = qobject_cast<CodeEditor*>(w)) {
        current_editor_ = ed;
        updateWindowTitle();
        if (status_cursor_label_) {
            status_cursor_label_->setText(QString("Ln %1, Col %2")
                .arg(ed->textCursor().blockNumber() + 1)
                .arg(ed->textCursor().columnNumber() + 1));
        }
        if (find_bar_->isVisible()) {
            find_bar_->searchInput()->setFocus();
        } else {
            ed->setFocus();
        }
    }
    updateUiState();
}

void MainWindow::onTabContextMenu(const QPoint& pos) {
    int tab_idx = document_tabs_->tabBar()->tabAt(pos);
    if (tab_idx < 0) return;

    QWidget* w = document_tabs_->widget(tab_idx);
    QMenu menu(this);

    if (w != welcome_widget_) {
        menu.addAction("&Save", this, [this, tab_idx]() { saveTab(tab_idx); });
        menu.addAction("Save &As...", this, [this, tab_idx]() {
            document_tabs_->setCurrentIndex(tab_idx);
            saveAs();
        });
        menu.addSeparator();
    }

    menu.addAction("&Close Tab", this, [this, tab_idx]() { closeTab(tab_idx); });
    menu.addAction("Close &Others", this, [this, tab_idx]() { closeOtherTabs(tab_idx); });
    menu.addAction("Close &All Tabs", this, &MainWindow::closeAllTabs);

    if (w != welcome_widget_) {
        auto* ed = qobject_cast<CodeEditor*>(w);
        int info_idx = findTabByEditor(ed);
        if (info_idx >= 0 && !editor_tabs_[info_idx].is_untitled) {
            menu.addSeparator();
            const QString path = editor_tabs_[info_idx].file_path;
            menu.addAction("Copy Full Path", this, [path]() {
                QApplication::clipboard()->setText(path);
            });
            menu.addAction("Reveal in Explorer", this, [path]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
            });
        }
    }

    menu.exec(document_tabs_->tabBar()->mapToGlobal(pos));
}

void MainWindow::showWelcomeScreen() {
    int welcome_idx = document_tabs_->indexOf(welcome_widget_);
    if (welcome_idx >= 0) {
        document_tabs_->setCurrentIndex(welcome_idx);
    } else {
        welcome_idx = document_tabs_->insertTab(0, welcome_widget_, "⚡ Welcome");
        document_tabs_->setTabToolTip(welcome_idx, "BitForge Welcome & Tutorial Center");
        document_tabs_->setCurrentIndex(welcome_idx);
    }
    updateWindowTitle();
}

void MainWindow::showEditor() {
    if (current_editor_ != nullptr) {
        int idx = document_tabs_->indexOf(current_editor_);
        if (idx >= 0) {
            document_tabs_->setCurrentIndex(idx);
            current_editor_->setFocus();
            return;
        }
    }
    for (int i = 0; i < document_tabs_->count(); ++i) {
        if (document_tabs_->widget(i) != welcome_widget_) {
            document_tabs_->setCurrentIndex(i);
            if (auto* ed = qobject_cast<CodeEditor*>(document_tabs_->widget(i))) {
                current_editor_ = ed;
                ed->setFocus();
            }
            return;
        }
    }
    // No editor tab currently open, create one!
    createEditorTab();
    showEditor();
}

void MainWindow::newFile() {
    if (maybeSave()) {
        if (current_editor_ != nullptr && currentFilePath().isEmpty()) {
            current_editor_->clear();
            current_editor_->document()->setModified(false);
            controller_->unloadProgram();
            updateWindowTitle();
            showEditor();
        } else {
            createEditorTab();
            controller_->unloadProgram();
            showEditor();
        }
    }
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open BitForge Assembly File", QString(),
        "Assembly Source (*.asm *.s);;All Files (*)"
    );
    if (!fileName.isEmpty()) {
        loadFile(fileName);
    }
}

bool MainWindow::loadFile(const QString& filePath) {
    const QString canonical = canonicalizePath(filePath);

    // Check if already open
    int existing_tab = findTabByPath(canonical);
    if (existing_tab >= 0) {
        document_tabs_->setCurrentIndex(existing_tab);
        if (auto* ed = qobject_cast<CodeEditor*>(document_tabs_->widget(existing_tab))) {
            current_editor_ = ed;
            ed->setFocus();
        }
        controller_->unloadProgram();
        updateRecentFilesList(filePath);
        statusBar()->showMessage(QString("Activated already open file: %1").arg(QFileInfo(filePath).fileName()), 3000);
        return true;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
            this, "BitForge",
            QString("Cannot read file %1:\n%2").arg(filePath, file.errorString())
        );
        return false;
    }

    QTextStream in(&file);
    const QString content = in.readAll();

    // If current tab is an untouched, empty, unmodified untitled document, reuse it!
    if (current_editor_ != nullptr) {
        int info_idx = findTabByEditor(current_editor_);
        if (info_idx >= 0 && editor_tabs_[info_idx].is_untitled &&
            !current_editor_->document()->isModified() &&
            current_editor_->toPlainText().isEmpty()) {
            editor_tabs_[info_idx].is_untitled = false;
            editor_tabs_[info_idx].file_path = filePath;
            editor_tabs_[info_idx].title = QFileInfo(filePath).fileName();
            current_editor_->setPlainText(content);
            current_editor_->document()->setModified(false);

            int tab_idx = document_tabs_->indexOf(current_editor_);
            if (tab_idx >= 0) {
                document_tabs_->setTabText(tab_idx, editor_tabs_[info_idx].title);
                document_tabs_->setTabToolTip(tab_idx, canonical);
            }

            controller_->unloadProgram();
            updateRecentFilesList(filePath);
            updateWindowTitle();
            showEditor();
            statusBar()->showMessage(QString("Loaded %1").arg(filePath), 3000);
            return true;
        }
    }

    // Otherwise create a new tab
    createEditorTab(filePath, content);
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
    auto* ed = currentEditor();
    if (!ed) return false;
    int tab_idx = document_tabs_->indexOf(ed);
    if (tab_idx < 0) return false;
    return saveTab(tab_idx);
}

bool MainWindow::saveAs() {
    auto* ed = currentEditor();
    if (!ed) return false;
    int info_idx = findTabByEditor(ed);
    if (info_idx < 0) return false;

    QString default_name = editor_tabs_[info_idx].title;
    if (editor_tabs_[info_idx].is_untitled && !default_name.endsWith(".asm")) {
        default_name += ".asm";
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Assembly File As", default_name,
        "Assembly Source (*.asm);;All Files (*)"
    );
    if (fileName.isEmpty()) {
        return false;
    }
    return saveFile(fileName);
}

bool MainWindow::saveTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= document_tabs_->count()) return false;
    auto* ed = qobject_cast<CodeEditor*>(document_tabs_->widget(tabIndex));
    if (!ed) return false;

    int info_idx = findTabByEditor(ed);
    if (info_idx < 0) return false;

    QString target_path = editor_tabs_[info_idx].file_path;
    if (editor_tabs_[info_idx].is_untitled || target_path.isEmpty()) {
        QString default_name = editor_tabs_[info_idx].title;
        if (!default_name.endsWith(".asm")) default_name += ".asm";

        QString fileName = QFileDialog::getSaveFileName(
            this, "Save Assembly File As", default_name,
            "Assembly Source (*.asm);;All Files (*)"
        );
        if (fileName.isEmpty()) return false;
        target_path = fileName;
    }

    QFile file(target_path);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(
            this, "BitForge",
            QString("Cannot write file %1:\n%2").arg(target_path, file.errorString())
        );
        return false;
    }

    QTextStream out(&file);
    out << ed->toPlainText();
    ed->document()->setModified(false);

    editor_tabs_[info_idx].file_path = canonicalizePath(target_path);
    editor_tabs_[info_idx].title = QFileInfo(target_path).fileName();
    editor_tabs_[info_idx].is_untitled = false;

    document_tabs_->setTabText(tabIndex, editor_tabs_[info_idx].title);
    document_tabs_->setTabToolTip(tabIndex, editor_tabs_[info_idx].file_path);

    updateRecentFilesList(target_path);
    updateWindowTitle();
    statusBar()->showMessage(QString("Saved %1").arg(target_path), 3000);
    return true;
}

bool MainWindow::saveDocumentInfo(EditorDocumentInfo& info) {
    int tab_idx = document_tabs_->indexOf(info.editor);
    if (tab_idx >= 0) {
        return saveTab(tab_idx);
    }
    return false;
}

bool MainWindow::saveFile(const QString& filePath) {
    auto* ed = currentEditor();
    if (!ed) return false;

    int info_idx = findTabByEditor(ed);
    if (info_idx < 0) return false;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(
            this, "BitForge",
            QString("Cannot write file %1:\n%2").arg(filePath, file.errorString())
        );
        return false;
    }

    QTextStream out(&file);
    out << ed->toPlainText();
    ed->document()->setModified(false);

    editor_tabs_[info_idx].file_path = filePath;
    editor_tabs_[info_idx].title = QFileInfo(filePath).fileName();
    editor_tabs_[info_idx].is_untitled = false;

    int tab_idx = document_tabs_->indexOf(ed);
    if (tab_idx >= 0) {
        document_tabs_->setTabText(tab_idx, editor_tabs_[info_idx].title);
        document_tabs_->setTabToolTip(tab_idx, canonicalizePath(filePath));
    }

    updateRecentFilesList(filePath);
    updateWindowTitle();
    statusBar()->showMessage(QString("Saved %1").arg(filePath), 3000);
    return true;
}

bool MainWindow::maybeSave() {
    auto* ed = currentEditor();
    if (!ed || !ed->document()->isModified()) {
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
    for (int i = document_tabs_->count() - 1; i >= 0; --i) {
        QWidget* w = document_tabs_->widget(i);
        if (w == welcome_widget_) continue;
        auto* ed = qobject_cast<CodeEditor*>(w);
        if (ed && ed->document()->isModified()) {
            document_tabs_->setCurrentIndex(i);
            int info_idx = findTabByEditor(ed);
            QString docTitle = (info_idx >= 0) ? editor_tabs_[info_idx].title : "Document";
            const QMessageBox::StandardButton ret = QMessageBox::warning(
                this, "BitForge",
                QString("The document '%1' has unsaved changes.\nDo you want to save before closing?").arg(docTitle),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );
            if (ret == QMessageBox::Save) {
                if (!saveTab(i)) {
                    event->ignore();
                    return;
                }
            } else if (ret == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
        }
    }
    saveSettings();
    event->accept();
}

void MainWindow::updateWindowTitle() {
    if (!document_tabs_) return;
    CodeEditor* ed = currentEditor();

    int info_idx = findTabByEditor(ed);
    QString title = "BitForge";
    QString file_display = "[Untitled]";

    if (info_idx >= 0) {
        if (editor_tabs_[info_idx].is_untitled) {
            title += " — [Untitled]";
            file_display = "[Untitled]";
        } else {
            title += " — " + editor_tabs_[info_idx].title;
            file_display = editor_tabs_[info_idx].file_path;
        }
    } else {
        title += " — [Untitled]";
    }

    if (ed != nullptr && ed->document()->isModified()) {
        title += "*";
        file_display += " [Modified]";
    }

    setWindowTitle(title);
    if (status_file_label_) {
        status_file_label_->setText(file_display);
        status_file_label_->setToolTip(file_display);
    }
}

void MainWindow::updateUiState() {
    if (!controller_) return;
    const bool is_executing = controller_->isExecuting();
    const bool is_assembled = controller_->isAssembled();
    const MachineState st = controller_->session().state();
    const word_t pc = controller_->session().pc();
    const auto steps = controller_->session().total_steps_executed();
    const bool is_stale = controller_->isStale();

    // Enable/disable actions strictly by machine state
    if (assemble_act_) assemble_act_->setEnabled(!is_executing);
    if (pause_act_) pause_act_->setEnabled(is_executing);

    if (is_executing) {
        if (run_act_) run_act_->setEnabled(false);
        if (continue_act_) continue_act_->setEnabled(false);
        if (step_act_) step_act_->setEnabled(false);
        if (reset_act_) reset_act_->setEnabled(false);
    } else if (!is_assembled) {
        if (run_act_) run_act_->setEnabled(false);
        if (continue_act_) continue_act_->setEnabled(false);
        if (step_act_) step_act_->setEnabled(false);
        if (reset_act_) reset_act_->setEnabled(false);
    } else {
        const bool can_execute = !is_stale && (st != MachineState::Halted && st != MachineState::Faulted);
        if (run_act_) run_act_->setEnabled(!is_stale);
        if (step_act_) step_act_->setEnabled(can_execute);
        if (continue_act_) continue_act_->setEnabled(can_execute);
        if (reset_act_) reset_act_->setEnabled(true);
    }

    // Update state badge
    if (state_badge_) {
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
    }

    // Update status bar
    if (status_state_label_) {
        if (is_assembled && executing_editor_ != nullptr && executing_editor_ != currentEditor()) {
            QString exec_name = executing_document_path_.isEmpty() ? "[Untitled]" : QFileInfo(executing_document_path_).fileName();
            QString base_text = state_badge_ ? state_badge_->text().trimmed() : "READY";
            status_state_label_->setText(QString("%1 (Exec: %2)").arg(base_text, exec_name));
        } else if (state_badge_) {
            status_state_label_->setText(state_badge_->text().trimmed());
        }
    }

    if (status_pc_label_) {
        status_pc_label_->setText(QString("PC: 0x%1").arg(QString::number(pc, 16).toUpper().rightJustified(4, '0')));
    }
    if (status_steps_label_) {
        status_steps_label_->setText(QString("Steps: %1").arg(steps));
    }
}

void MainWindow::onAssemble() {
    auto* ed = currentEditor();
    if (!ed) return;
    showEditor();

    const QString code = ed->toPlainText();
    bool ok = controller_->assemble(code);
    if (ok) {
        executing_editor_ = ed;
        executing_document_path_ = currentFilePath();
        statusBar()->showMessage("Assembly successful. Program loaded into emulated memory.", 4000);
    } else {
        statusBar()->showMessage("Assembly failed. See Diagnostics tab for details.", 6000);
    }
    updateUiState();
}

void MainWindow::onRun() {
    auto* ed = currentEditor();
    if (!ed) return;
    showEditor();

    if (controller_->isStale()) {
        statusBar()->showMessage("Source modified since last assembly. Please reassemble (F7) before running.", 5000);
        return;
    }

    if (!controller_->isAssembled() || executing_editor_ != ed) {
        onAssemble();
        if (!controller_->isAssembled()) return;
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
    if (auto* ed = currentEditor()) {
        ed->toggleBreakpointAtCursor();
    }
}

void MainWindow::onClearBreakpoints() {
    controller_->clearAllBreakpoints();
    statusBar()->showMessage("All breakpoints cleared.", 3000);
}

void MainWindow::onClearTrace() {
    controller_->clearTrace();
    statusBar()->showMessage("Execution trace cleared.", 3000);
}

void MainWindow::onFind() {
    showEditor();
    find_bar_->showAndFocus();
    if (auto* ed = currentEditor()) {
        QTextCursor cursor = ed->textCursor();
        if (cursor.hasSelection() && !cursor.selectedText().isEmpty()) {
            find_bar_->searchInput()->setText(cursor.selectedText());
        }
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
    auto* ed = currentEditor();
    if (!ed) return;

    int total_lines = std::max(1, ed->document()->blockCount());
    int current_line = ed->textCursor().blockNumber() + 1;
    bool ok = false;
    int target = QInputDialog::getInt(
        this, "Go to Line",
        QString("Enter line number (1 - %1):").arg(total_lines),
        current_line, 1, total_lines, 1, &ok
    );
    if (ok) {
        ed->gotoLine(static_cast<std::size_t>(target), 1);
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

void MainWindow::onFirstProgramTutorial() {
    // Check if tutorial tab already open
    for (int i = 0; i < document_tabs_->count(); ++i) {
        if (document_tabs_->tabText(i).startsWith("tutorial_first_program.asm")) {
            document_tabs_->setCurrentIndex(i);
            showEditor();
            return;
        }
    }

    createEditorTab(QString(), QString::fromUtf8(kFirstProgramTutorialAsm));
    int cur_idx = document_tabs_->currentIndex();
    document_tabs_->setTabText(cur_idx, "tutorial_first_program.asm");
    document_tabs_->setTabToolTip(cur_idx, "tutorial_first_program.asm [Tutorial Document]");
    if (current_editor_ != nullptr) {
        int info_idx = findTabByEditor(current_editor_);
        if (info_idx >= 0) {
            editor_tabs_[info_idx].title = "tutorial_first_program.asm";
            editor_tabs_[info_idx].is_untitled = true;
        }
    }
    showEditor();
    statusBar()->showMessage("Opened First Program Tutorial: tutorial_first_program.asm", 4000);
}

void MainWindow::onCurriculumRequested() {
    showWelcomeScreen();
    if (welcome_widget_) {
        welcome_widget_->selectTab(1); // Select Curriculum tab
    }
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
    if (settings.contains("editorFontSize")) {
        bool ok_font = false;
        int fontSize = settings.value("editorFontSize", 11).toInt(&ok_font);
        if (ok_font && fontSize >= 6 && fontSize <= 48) {
            current_font_size_ = fontSize;
            for (const auto& doc : editor_tabs_) {
                if (doc.editor) doc.editor->setFontSize(fontSize);
            }
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

    // 6. Show Welcome on Startup preference
    bool show_welcome = settings.value("showWelcomeOnStartup", true).toBool();
    if (welcome_widget_) {
        welcome_widget_->setShowOnStartup(show_welcome);
    }
    if (show_welcome) {
        document_tabs_->setCurrentWidget(welcome_widget_);
    } else {
        showEditor();
    }
}

void MainWindow::saveSettings() {
    QSettings settings("BitForge", "BitForge");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    auto* ed = currentEditor();
    const int font_size = ed ? ed->currentFontSize() : current_font_size_;
    settings.setValue("editorFontSize", font_size);
    if (controller_ != nullptr) {
        settings.setValue("executionSpeed", static_cast<int>(controller_->speed()));
    }
    if (watch_panel_ != nullptr) {
        settings.setValue("watchList", watch_panel_->watchExpressions());
    }
    if (welcome_widget_ != nullptr) {
        settings.setValue("showWelcomeOnStartup", welcome_widget_->showOnStartup());
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
        "<tr><td><b>Ctrl+N</b></td><td>New File</td><td>Creates a new empty assembly file tab</td></tr>"
        "<tr><td><b>Ctrl+O</b></td><td>Open File</td><td>Opens an existing assembly file in a tab</td></tr>"
        "<tr><td><b>Ctrl+S</b></td><td>Save</td><td>Saves the current file to disk</td></tr>"
        "<tr><td><b>Ctrl+Shift+S</b></td><td>Save As</td><td>Saves current file to a new path</td></tr>"
        "<tr><td><b>Ctrl+W</b></td><td>Close Tab</td><td>Closes the active document tab</td></tr>"
        "<tr><td><b>Ctrl+Tab</b></td><td>Next Tab</td><td>Switches to next document tab</td></tr>"
        "<tr><td><b>Ctrl+F</b></td><td>Find</td><td>Opens the inline find/search bar</td></tr>"
        "<tr><td><b>F3 / Shift+F3</b></td><td>Find Next / Prev</td><td>Navigates between search matches</td></tr>"
        "<tr><td><b>Ctrl+G</b></td><td>Go to Line</td><td>Jump directly to a line number</td></tr>"
        "<tr><td><b>Ctrl++ / Ctrl+-</b></td><td>Zoom In / Out</td><td>Adjust editor font size</td></tr>"
        "<tr><td><b>Ctrl+0</b></td><td>Reset Zoom</td><td>Restore default editor font size</td></tr>"
        "</table>"
    );
}

} // namespace bitforge::gui
