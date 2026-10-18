#include "bitforge/gui/welcome_widget.hpp"
#include "bitforge/gui/main_window.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>

namespace bitforge::gui {

namespace {

struct LessonData {
    QString title;
    QString subtitle;
    QString examplePath;
    QString buttonText;
    QString htmlContent;
};

const LessonData kLessons[11] = {
    // Lesson 1
    {
        "Lesson 1 — Getting Started with BitForge",
        "Architectural foundations, 8-bit registers, and CPU lifecycle",
        "examples/01_arithmetic.asm",
        "Open 01_arithmetic.asm in Editor",
        "<h3>Lesson 1 — Getting Started with BitForge</h3>"
        "<p>BitForge is an educational 8-bit digital-computer workbench designed to teach computer architecture, assembly programming, and systems engineering from first principles.</p>"
        "<h4>Architectural Foundations</h4>"
        "<ul>"
        "<li><b>Four 8-Bit General-Purpose Registers:</b> <code>R0</code>, <code>R1</code>, <code>R2</code>, and <code>R3</code> (each holds 0 to 255 / 0x00 to 0xFF).</li>"
        "<li><b>16-Bit Program Counter (PC):</b> Points to the address in memory of the next instruction byte to fetch (default entry is <code>0x0200</code>).</li>"
        "<li><b>16-Bit Stack Pointer (SP):</b> Points to the current top of the call stack (starts at <code>0xFFFF</code> and grows downward).</li>"
        "<li><b>8-Bit Status Register (Flags):</b> Zero (<code>Z</code>), Carry (<code>C</code>), Negative (<code>N</code>), and Overflow (<code>V</code>).</li>"
        "<li><b>64 KiB Linear Memory:</b> Byte-addressable address space from <code>0x0000</code> to <code>0xFFFF</code> (65,536 bytes).</li>"
        "</ul>"
        "<h4>CPU Execution Lifecycle</h4>"
        "<table cellpadding='4' cellspacing='0' border='1' style='border-color: #3b4048;'>"
        "<tr bgcolor='#21252b'><th>State</th><th>Description</th></tr>"
        "<tr><td><b>Ready</b></td><td>CPU is initialized or assembled, with PC pointing at the entry point.</td></tr>"
        "<tr><td><b>Running</b></td><td>CPU is executing instructions sequentially at the configured clock speed.</td></tr>"
        "<tr><td><b>Paused</b></td><td>Execution was paused by the user or halted cleanly at a breakpoint.</td></tr>"
        "<tr><td><b>Halted</b></td><td>CPU executed a <code>HALT</code> instruction cleanly.</td></tr>"
        "<tr><td><b>Faulted</b></td><td>An architectural violation occurred (e.g., illegal opcode, division by zero).</td></tr>"
        "</table>"
        "<p>Click the button below to load <b>examples/01_arithmetic.asm</b> into the editor to try your first program.</p>"
    },

    // Lesson 2
    {
        "Lesson 2 — The Assembly Editor",
        "Syntax formatting, directives, line numbers, and multi-document tabs",
        "examples/arithmetic/03_bitwise_logic.asm",
        "Open 03_bitwise_logic.asm in Editor",
        "<h3>Lesson 2 — The Assembly Editor</h3>"
        "<p>The BitForge multi-document assembly editor provides syntax highlighting, line numbers, a breakpoint gutter, search/replace, and tabbed file management.</p>"
        "<h4>Instruction Syntax</h4>"
        "<p>Every source line adheres to the format:</p>"
        "<pre style='background:#1b1d23; color:#abb2bf; padding:8px; border-radius:4px;'>"
        "[label:]   &lt;mnemonic&gt;  [operands]   [; comment]\n\n"
        "start:\n"
        "    LDI R0, 10      ; Load decimal 10 into Register R0\n"
        "    ADD R0, R1      ; Add R1 into R0\n"
        "</pre>"
        "<h4>Assembler Directives</h4>"
        "<ul>"
        "<li><code>.ORG &lt;address&gt;</code> — Set the assembler location counter (e.g. <code>.ORG 0x0200</code>).</li>"
        "<li><code>.ENTRY &lt;label&gt;</code> — Declare the program execution entry point.</li>"
        "<li><code>.BYTE &lt;val&gt;</code> — Allocate one byte in memory.</li>"
        "<li><code>.WORD &lt;val&gt;</code> — Allocate a 16-bit word (little-endian: low byte first).</li>"
        "<li><code>.SPACE &lt;count&gt;</code> — Reserve zero-initialized memory bytes.</li>"
        "<li><code>.EQU &lt;name&gt;, &lt;val&gt;</code> — Define a compile-time numeric constant symbol.</li>"
        "</ul>"
        "<h4>Multi-Document Workspace</h4>"
        "<p>Open multiple files using <b>Ctrl+O</b> or <b>Ctrl+N</b>. Files with unsaved modifications display an asterisk (<code>*</code>). Close tabs with <b>Ctrl+W</b> or right-click the tab bar.</p>"
    },

    // Lesson 3
    {
        "Lesson 3 — The Two-Pass Assembler & Diagnostics",
        "Symbol resolution, error diagnostics, and source-stale tracking",
        "examples/invalid/01_unknown_mnemonic.asm",
        "Open 01_unknown_mnemonic.asm in Editor",
        "<h3>Lesson 3 — The Two-Pass Assembler & Diagnostics</h3>"
        "<p>BitForge uses a native two-pass assembler to transform human-readable assembly into executable machine bytecode.</p>"
        "<h4>Two-Pass Operation</h4>"
        "<ul>"
        "<li><b>Pass 1 (Symbol Discovery):</b> Scans each statement, calculates instruction and directive sizes, and populates the symbol table with label addresses.</li>"
        "<li><b>Pass 2 (Code Emission):</b> Encodes opcodes, resolves symbol references to absolute addresses, and loads bytecode segments into the emulated memory image.</li>"
        "</ul>"
        "<h4>Diagnostics Panel</h4>"
        "<p>When syntax errors or semantic faults occur (e.g., duplicate label, undefined symbol, immediate out of range), the <b>Diagnostics</b> panel displays the exact error message, line number, and column. Double-clicking any diagnostic jumps the editor cursor straight to the error line.</p>"
        "<h4>The Stale Indicator</h4>"
        "<p>Modifying assembly code after assembly flags the memory image as <b>STALE</b>. This prevents running out-of-sync machine code until you press <b>Assemble (F7)</b> again.</p>"
    },

    // Lesson 4
    {
        "Lesson 4 — Execution Controls",
        "Running, single-stepping, continuing, resetting, and clock speeds",
        "examples/01_arithmetic.asm",
        "Open 01_arithmetic.asm in Editor",
        "<h3>Lesson 4 — Execution Controls</h3>"
        "<p>BitForge provides granular hardware execution controls in the top toolbar and <b>Execution</b> menu:</p>"
        "<h4>Core Controls</h4>"
        "<ul>"
        "<li><b>Assemble (F7):</b> Compiles the active editor source and loads bytecode into emulated memory.</li>"
        "<li><b>Run from Entry (F5):</b> Resets CPU registers, memory, stack, and virtual devices to the initial assembled state, then runs continuously until a breakpoint or halt.</li>"
        "<li><b>Continue (F8):</b> Resumes execution from the current PC without resetting any CPU registers, memory, or devices.</li>"
        "<li><b>Step (F10):</b> Executes exactly one CPU instruction and immediately pauses.</li>"
        "<li><b>Pause:</b> Cleanly interrupts a continuous execution slice at the next instruction boundary.</li>"
        "<li><b>Reset (F6):</b> Restores CPU registers, stack, memory, and devices back to the post-assembly initial snapshot.</li>"
        "</ul>"
        "<h4>Execution Speed & Budget</h4>"
        "<p>Select <b>Slow (500 ms)</b>, <b>Normal (100 ms)</b>, <b>Fast (50 ms / 5 steps)</b>, or <b>Turbo (unthrottled)</b> from the toolbar. The safety step budget prevents runaway infinite loops from freezing your workbench.</p>"
    },

    // Lesson 5
    {
        "Lesson 5 — CPU Registers & Flags",
        "General-purpose registers, program counter, and status flags (Z, C, N, V)",
        "examples/control_flow/02_branch_zero.asm",
        "Open 02_branch_zero.asm in Editor",
        "<h3>Lesson 5 — CPU Registers & Flags</h3>"
        "<p>The <b>CPU Registers & Flags</b> dock displays real-time state for all architectural registers.</p>"
        "<h4>Registers</h4>"
        "<ul>"
        "<li><code>R0, R1, R2, R3</code>: 8-bit registers used for arithmetic, logic, and I/O.</li>"
        "<li><code>PC</code> (Program Counter): 16-bit address of the next instruction byte to fetch.</li>"
        "<li><code>SP</code> (Stack Pointer): 16-bit pointer to the current top of the stack (0xFFFF down).</li>"
        "</ul>"
        "<h4>Status Register Flags</h4>"
        "<ul>"
        "<li><b>Z (Zero):</b> Set to 1 if the last ALU operation result was exactly 0x00.</li>"
        "<li><b>C (Carry):</b> Set to 1 on unsigned overflow or carry-out from bit 7.</li>"
        "<li><b>N (Negative):</b> Set to 1 if the MSB (bit 7) of the result is 1 (two's complement negative).</li>"
        "<li><b>V (Overflow):</b> Set to 1 if a signed arithmetic operation overflowed.</li>"
        "</ul>"
        "<h4>Bit-Level Visualization</h4>"
        "<p>Switch to the <b>Bit Visualization</b> tab in the right dock to inspect active 8-bit binary LED rows for each register in real time!</p>"
    },

    // Lesson 6
    {
        "Lesson 6 — Memory and the Hardware Stack",
        "64 KiB memory space, memory mapping, and stack call frame mechanics",
        "examples/stack_subroutines/02_subroutine_call_ret.asm",
        "Open 02_subroutine_call_ret.asm in Editor",
        "<h3>Lesson 6 — Memory and the Hardware Stack</h3>"
        "<p>BitForge features a uniform 16-bit address space encompassing 65,536 individual 8-bit bytes.</p>"
        "<h4>Memory Map</h4>"
        "<ul>"
        "<li><code>0x0000 – 0x01FF</code>: Reserved system vectors and boot space.</li>"
        "<li><code>0x0200 – 0x7FFF</code>: Executable program code (default <code>.ORG 0x0200</code>).</li>"
        "<li><code>0x8000 – 0xEFFF</code>: Static and dynamic application data.</li>"
        "<li><code>0xF000 – 0xFFFF</code>: Hardware stack region (grows downward from 0xFFFF).</li>"
        "</ul>"
        "<h4>Hardware Stack</h4>"
        "<p>The stack is manipulated using <code>PUSH R</code>, <code>POP R</code>, <code>CALL addr</code>, and <code>RET</code>.</p>"
        "<p>When a subroutine is invoked with <code>CALL</code>, the 16-bit return address is pushed onto the stack (low byte first, then high byte). When <code>RET</code> is executed, the return address is popped back into PC.</p>"
        "<p>The <b>Hardware Stack</b> viewer tab shows live call frames and stack depth.</p>"
    },

    // Lesson 7
    {
        "Lesson 7 — Breakpoints, Tracing, and the Instruction Inspector",
        "Interactive debugging, execution history, and opcode decoding",
        "examples/debugger/03_breakpoint_target.asm",
        "Open 03_breakpoint_target.asm in Editor",
        "<h3>Lesson 7 — Breakpoints, Tracing, and the Instruction Inspector</h3>"
        "<p>Professional debugging requires observability into past, present, and future machine states.</p>"
        "<h4>Breakpoints</h4>"
        "<p>Click the editor line number gutter or press <b>F9</b> to set a breakpoint. A red circle appears. When executing, the CPU will pause <i>immediately before</i> executing that instruction.</p>"
        "<h4>Execution Trace Panel</h4>"
        "<p>The <b>Execution Trace</b> tab records a chronological log of executed instructions, including the instruction mnemonic, addressing mode, operands, step counter, and register mutations.</p>"
        "<h4>Instruction Inspector</h4>"
        "<p>The <b>Instruction Inspector</b> decodes the opcode byte at the current PC, displaying the mnemonic, cycle timing, addressing mode, byte length, and architectural specification details.</p>"
    },

    // Lesson 8
    {
        "Lesson 8 — Watch Expressions",
        "Monitoring registers, flags, and memory addresses dynamically",
        "examples/debugger/06_memory_inspection_step.asm",
        "Open 06_memory_inspection_step.asm in Editor",
        "<h3>Lesson 8 — Watch Expressions</h3>"
        "<p>The <b>Watch Expressions</b> dock enables continuous tracking of arbitrary machine state during stepping and execution.</p>"
        "<h4>Supported Watch Targets</h4>"
        "<ul>"
        "<li><b>Registers:</b> <code>R0</code>, <code>R1</code>, <code>R2</code>, <code>R3</code>, <code>PC</code>, <code>SP</code></li>"
        "<li><b>Status Flags:</b> <code>Z</code>, <code>C</code>, <code>N</code>, <code>V</code></li>"
        "<li><b>Memory Addresses:</b> <code>[0x8000]</code>, <code>[0x8001]</code>, <code>[0x0200]</code></li>"
        "</ul>"
        "<h4>Adding Watches from Context Menus</h4>"
        "<p>Right-click any register in the Registers panel, or right-click any memory cell in the Memory Viewer, and choose <b>Add to Watch</b>. Watches update dynamically on every step, flashing amber whenever their value mutates.</p>"
    },

    // Lesson 9
    {
        "Lesson 9 — Virtual Devices and Input/Output",
        "Text console, numeric display, virtual LEDs, and interactive input FIFO",
        "examples/02_io_demo.asm",
        "Open 02_io_demo.asm in Editor",
        "<h3>Lesson 9 — Virtual Devices and Input/Output</h3>"
        "<p>The CPU communicates with peripherals using the <code>IN R, port</code> and <code>OUT port, R</code> instructions.</p>"
        "<h4>Architectural Port Map</h4>"
        "<table cellpadding='4' cellspacing='0' border='1' style='border-color: #3b4048;'>"
        "<tr bgcolor='#21252b'><th>Port</th><th>Direction</th><th>Device</th><th>Description</th></tr>"
        "<tr><td><code>0x00</code></td><td>IN</td><td><b>Input FIFO</b></td><td>Read queued byte from the interactive input buffer.</td></tr>"
        "<tr><td><code>0x01</code></td><td>OUT</td><td><b>Text Console</b></td><td>Writes an ASCII character to the text terminal.</td></tr>"
        "<tr><td><code>0x02</code></td><td>OUT</td><td><b>Numeric Console</b></td><td>Outputs a decimal byte value.</td></tr>"
        "<tr><td><code>0x10</code></td><td>OUT</td><td><b>Virtual LEDs</b></td><td>Updates the 8-bit LED bank bit-pattern.</td></tr>"
        "<tr><td><code>0x03</code></td><td>IN/OUT</td><td><b>Unmapped Port</b></td><td>Reads return 0x00; writes are safely ignored.</td></tr>"
        "</table>"
        "<h4>Seven-Segment Display</h4>"
        "<p>The Virtual Devices panel features a hardware-accurate seven-segment display showing the latest output byte.</p>"
    },

    // Lesson 10
    {
        "Lesson 10 — Examples and Further Practice",
        "51 curated assembly programs covering algorithms, subroutines, and fault recovery",
        "examples/control_flow/04_countdown_loop.asm",
        "Open 04_countdown_loop.asm in Editor",
        "<h3>Lesson 10 — Examples and Further Practice</h3>"
        "<p>BitForge ships with 51 comprehensive, self-contained assembly examples organized into clear educational categories:</p>"
        "<ul>"
        "<li><b>Fundamentals:</b> Basic arithmetic, register swapping, logic, conditional branching.</li>"
        "<li><b>Algorithms:</b> Bubble sort, Fibonacci series, binary search, string reversal, GCD.</li>"
        "<li><b>Subroutines:</b> Stack parameter passing, recursive factorial, preserved register conventions.</li>"
        "<li><b>Peripherals:</b> LED counters, ASCII echo loops, string printing to console.</li>"
        "<li><b>Fault Recovery:</b> Intentional illegal opcodes, stack overflow, division by zero, unmapped memory.</li>"
        "</ul>"
        "<p>Open the <b>Example Browser</b> from the toolbar or Help menu to filter programs by category, preview their source code, and open them in tabs.</p>"
    },

    // Lesson 11
    {
        "Lesson 11 — Keyboard Shortcuts and Workspace Customization",
        "Workbench efficiency, dock management, zoom controls, and layout reset",
        "",
        "",
        "<h3>Lesson 11 — Keyboard Shortcuts and Workspace Customization</h3>"
        "<p>Master BitForge by using standard keyboard shortcuts for rapid editing and debugging.</p>"
        "<h4>Keyboard Shortcuts</h4>"
        "<table cellpadding='4' cellspacing='0' border='1' style='border-color: #3b4048;'>"
        "<tr bgcolor='#21252b'><th>Shortcut</th><th>Action</th><th>Description</th></tr>"
        "<tr><td><b>F5</b></td><td>Run from Entry</td><td>Resets CPU to entry point and runs until breakpoint/halt.</td></tr>"
        "<tr><td><b>F6</b></td><td>Reset</td><td>Resets registers, memory, and devices to initial state.</td></tr>"
        "<tr><td><b>F7</b></td><td>Assemble</td><td>Assembles current source editor text into memory.</td></tr>"
        "<tr><td><b>F8</b></td><td>Continue</td><td>Resumes execution from current PC without resetting.</td></tr>"
        "<tr><td><b>F9</b></td><td>Toggle Breakpoint</td><td>Toggles breakpoint on line at cursor / gutter.</td></tr>"
        "<tr><td><b>F10</b></td><td>Step</td><td>Single-steps exactly one instruction from current PC.</td></tr>"
        "<tr><td><b>Ctrl+N</b></td><td>New File</td><td>Creates a new empty assembly document tab.</td></tr>"
        "<tr><td><b>Ctrl+O</b></td><td>Open File</td><td>Opens an existing assembly file in a tab.</td></tr>"
        "<tr><td><b>Ctrl+S</b></td><td>Save</td><td>Saves active document to disk.</td></tr>"
        "<tr><td><b>Ctrl+Shift+S</b></td><td>Save As</td><td>Saves active document under a new path.</td></tr>"
        "<tr><td><b>Ctrl+W</b></td><td>Close Tab</td><td>Closes active document tab (prompts if modified).</td></tr>"
        "<tr><td><b>Ctrl+Tab</b></td><td>Next Tab</td><td>Switches to next document tab.</td></tr>"
        "<tr><td><b>Ctrl+F</b></td><td>Find</td><td>Opens the inline search bar.</td></tr>"
        "<tr><td><b>Ctrl+G</b></td><td>Go to Line</td><td>Jump directly to a specific line number.</td></tr>"
        "<tr><td><b>Ctrl++ / Ctrl+-</b></td><td>Zoom</td><td>Adjust editor font size.</td></tr>"
        "</table>"
        "<h4>Customizing Layout</h4>"
        "<p>Docks can be dragged, tabified, or detached into floating windows. If your layout ever becomes cluttered, select <b>View → Restore Default Layout</b> to instantly reset all panels.</p>"
    }
};

} // namespace

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent) {
    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(14, 10, 14, 10);
    root_layout->setSpacing(8);

    // 1. Header Banner
    auto* header_container = new QWidget(this);
    auto* header_layout = new QHBoxLayout(header_container);
    header_layout->setContentsMargins(0, 0, 0, 0);
    header_layout->setSpacing(12);

    auto* title_box = new QVBoxLayout();
    title_box->setContentsMargins(0, 0, 0, 0);
    title_box->setSpacing(2);

    auto* title_label = new QLabel("⚡ BitForge Workbench", header_container);
    title_label->setStyleSheet("QLabel { font-size: 22px; font-weight: bold; color: #4ec9b0; }");
    title_box->addWidget(title_label);

    auto* subtitle_label = new QLabel(
        "8-Bit Educational Computer Emulator, Two-Pass Assembler & Interactive Debugger",
        header_container
    );
    subtitle_label->setStyleSheet("QLabel { font-size: 12px; color: #abb2bf; }");
    title_box->addWidget(subtitle_label);

    header_layout->addLayout(title_box, 1);

    // Top-Right: Show on Startup CheckBox
    show_on_startup_cb_ = new QCheckBox("Show Welcome on Startup", header_container);
    show_on_startup_cb_->setChecked(true);
    show_on_startup_cb_->setStyleSheet("QCheckBox { color: #abb2bf; font-size: 11px; }");
    connect(show_on_startup_cb_, &QCheckBox::toggled, this, &WelcomeWidget::showOnStartupChanged);
    header_layout->addWidget(show_on_startup_cb_, 0, Qt::AlignRight | Qt::AlignVCenter);

    root_layout->addWidget(header_container);

    // 2. Multi-Section Tab Widget (Quick Start, Curriculum, First Program)
    welcome_tabs_ = new QTabWidget(this);
    welcome_tabs_->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #282c34; background-color: #21252b; border-radius: 4px; }"
    );

    setupQuickStartTab();
    setupCurriculumTab();
    setupFirstProgramTab();

    root_layout->addWidget(welcome_tabs_, 1);
}

void WelcomeWidget::setupQuickStartTab() {
    auto* quick_page = new QWidget(welcome_tabs_);
    auto* content_layout = new QHBoxLayout(quick_page);
    content_layout->setContentsMargins(16, 16, 16, 16);
    content_layout->setSpacing(20);

    // Left Column: Primary Actions
    auto* actions_box = new QGroupBox("Start Here", quick_page);
    auto* actions_layout = new QVBoxLayout(actions_box);
    actions_layout->setContentsMargins(14, 14, 14, 14);
    actions_layout->setSpacing(8);

    auto makeActionBtn = [this](const QString& title, const QString& subtitle, const QString& shortcut, const QString& color = "#282c34") {
        auto* btn = new QPushButton(this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setMinimumHeight(34);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid #3b4048; border-radius: 6px; text-align: left; padding: 4px 10px; }"
            "QPushButton:hover { background-color: #3e4451; border-color: #4ec9b0; }"
            "QPushButton:pressed { background-color: #181a1f; }"
        ).arg(color));

        auto* btn_layout = new QVBoxLayout(btn);
        btn_layout->setContentsMargins(0, 0, 0, 0);
        btn_layout->setSpacing(1);

        auto* t_lbl = new QLabel(QString("<b>%1</b> <span style='color: #61afef; font-size: 11px;'>%2</span>").arg(title, shortcut), btn);
        t_lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        btn_layout->addWidget(t_lbl);

        auto* s_lbl = new QLabel(subtitle, btn);
        s_lbl->setStyleSheet("QLabel { color: #8a93a0; font-size: 11px; }");
        s_lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        btn_layout->addWidget(s_lbl);

        return btn;
    };

    auto* btn_guided_tour = makeActionBtn("Start Guided Tour", "Take a step-by-step 9-stage interactive walkthrough", "", "#243336");
    auto* btn_first_program = makeActionBtn("First Program Tutorial", "Hands-on beginner guide: assemble, run, and inspect LEDs", "", "#2c313a");
    auto* btn_curriculum = makeActionBtn("Interactive Curriculum", "Browse 11 comprehensive lessons on CPU architecture & tools", "");
    auto* btn_new = makeActionBtn("New Assembly File", "Create a blank assembly program with .ENTRY template", "[Ctrl+N]");
    auto* btn_open = makeActionBtn("Open Assembly File...", "Load an existing .asm source file from disk into a tab", "[Ctrl+O]");
    auto* btn_examples = makeActionBtn("Browse Example Library", "Explore 51 categorized programs with live syntax previews", "");
    auto* btn_editor = makeActionBtn("Switch to Code Editor", "Jump directly into the active editing & debugging tabs", "[Esc]");

    actions_layout->addWidget(btn_guided_tour);
    actions_layout->addWidget(btn_first_program);
    actions_layout->addWidget(btn_curriculum);
    actions_layout->addWidget(btn_new);
    actions_layout->addWidget(btn_open);
    actions_layout->addWidget(btn_examples);
    actions_layout->addWidget(btn_editor);
    actions_layout->addStretch();

    content_layout->addWidget(actions_box, 3);

    // Right Column: Recent Files & Quick Shortcuts
    auto* right_col = new QVBoxLayout();
    right_col->setSpacing(12);

    // Recent Files
    auto* recent_box = new QGroupBox("Recent Files", quick_page);
    auto* recent_layout = new QVBoxLayout(recent_box);
    recent_layout->setContentsMargins(12, 12, 12, 12);
    recent_layout->setSpacing(8);

    recent_list_ = new QListWidget(recent_box);
    recent_list_->setAlternatingRowColors(true);
    recent_layout->addWidget(recent_list_, 1);

    auto* recent_btn_bar = new QHBoxLayout();
    recent_btn_bar->addStretch();
    clear_recent_btn_ = new QPushButton("Clear Recent", recent_box);
    clear_recent_btn_->setStyleSheet("QPushButton { font-size: 11px; padding: 3px 8px; }");
    recent_btn_bar->addWidget(clear_recent_btn_);
    recent_layout->addLayout(recent_btn_bar);

    right_col->addWidget(recent_box, 3);

    // Quick Reference
    auto* ref_box = new QGroupBox("Quick Reference", quick_page);
    auto* ref_layout = new QGridLayout(ref_box);
    ref_layout->setContentsMargins(12, 10, 12, 10);
    ref_layout->setSpacing(6);

    auto addRef = [&](int row, const QString& key, const QString& desc) {
        auto* k = new QLabel(QString("<b>%1</b>").arg(key), ref_box);
        k->setStyleSheet("QLabel { color: #4ec9b0; }");
        auto* d = new QLabel(desc, ref_box);
        d->setStyleSheet("QLabel { color: #abb2bf; font-size: 11px; }");
        ref_layout->addWidget(k, row, 0);
        ref_layout->addWidget(d, row, 1);
    };

    addRef(0, "F5", "Run from Entry (Fresh execution)");
    addRef(1, "F8", "Continue (Resume without reset)");
    addRef(2, "F10", "Step (Execute single instruction)");
    addRef(3, "F7", "Assemble (Compile active buffer)");
    addRef(4, "F9", "Toggle Breakpoint at cursor");
    addRef(5, "F6", "Reset CPU, stack, and peripherals");
    addRef(6, "Ctrl+W", "Close active document tab");
    addRef(7, "Ctrl+Tab", "Switch to next document tab");

    right_col->addWidget(ref_box, 2);
    content_layout->addLayout(right_col, 4);

    auto* scroll = new QScrollArea(welcome_tabs_);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(quick_page);
    welcome_tabs_->addTab(scroll, "🚀 Quick Start");

    // Connect button signals
    connect(btn_guided_tour, &QPushButton::clicked, this, &WelcomeWidget::startTutorialRequested);
    connect(btn_first_program, &QPushButton::clicked, this, [this]() {
        selectTab(2); // Switch to First Program tab
    });
    connect(btn_curriculum, &QPushButton::clicked, this, [this]() {
        selectTab(1); // Switch to Curriculum tab
    });
    connect(btn_new, &QPushButton::clicked, this, &WelcomeWidget::newFileRequested);
    connect(btn_open, &QPushButton::clicked, this, &WelcomeWidget::openFileRequested);
    connect(btn_examples, &QPushButton::clicked, this, &WelcomeWidget::browseExamplesRequested);
    connect(btn_editor, &QPushButton::clicked, this, &WelcomeWidget::openEditorRequested);

    connect(recent_list_, &QListWidget::itemDoubleClicked, this, &WelcomeWidget::onRecentItemDoubleClicked);
    connect(clear_recent_btn_, &QPushButton::clicked, this, &WelcomeWidget::onClearRecentClicked);
}

void WelcomeWidget::setupCurriculumTab() {
    auto* curr_page = new QWidget(welcome_tabs_);
    auto* page_layout = new QHBoxLayout(curr_page);
    page_layout->setContentsMargins(12, 12, 12, 12);
    page_layout->setSpacing(12);

    // Left Column: Lesson Navigation List
    auto* left_box = new QGroupBox("Curriculum Lessons", curr_page);
    auto* left_layout = new QVBoxLayout(left_box);
    left_layout->setContentsMargins(8, 8, 8, 8);
    left_layout->setSpacing(6);

    lesson_list_ = new QListWidget(left_box);
    lesson_list_->setAlternatingRowColors(true);
    for (int i = 0; i < 11; ++i) {
        auto* item = new QListWidgetItem(kLessons[i].title, lesson_list_);
        item->setToolTip(kLessons[i].subtitle);
    }
    left_layout->addWidget(lesson_list_, 1);

    page_layout->addWidget(left_box, 3);

    // Right Column: Lesson Content and Action Bar
    auto* right_box = new QGroupBox("Lesson Details", curr_page);
    auto* right_layout = new QVBoxLayout(right_box);
    right_layout->setContentsMargins(12, 12, 12, 12);
    right_layout->setSpacing(8);

    lesson_content_ = new QTextBrowser(right_box);
    lesson_content_->setOpenExternalLinks(false);

    // Set document default stylesheet for typography, tables, lists, and code blocks
    lesson_content_->document()->setDefaultStyleSheet(
        "body {"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
        "  font-size: 13px;"
        "  color: #abb2bf;"
        "  line-height: 1.45;"
        "}"
        "h3 {"
        "  color: #4ec9b0;"
        "  font-size: 15px;"
        "  margin-top: 0px;"
        "  margin-bottom: 8px;"
        "  font-weight: 600;"
        "}"
        "h4 {"
        "  color: #e5c07b;"
        "  font-size: 13px;"
        "  margin-top: 12px;"
        "  margin-bottom: 6px;"
        "  font-weight: 600;"
        "}"
        "p {"
        "  margin-top: 3px;"
        "  margin-bottom: 8px;"
        "  color: #abb2bf;"
        "}"
        "ul, ol {"
        "  margin-top: 3px;"
        "  margin-bottom: 8px;"
        "  padding-left: 20px;"
        "}"
        "li {"
        "  margin-bottom: 4px;"
        "  color: #abb2bf;"
        "}"
        "code {"
        "  font-family: 'Cascadia Code', 'Consolas', monospace;"
        "  font-size: 12px;"
        "  background-color: #1b1d23;"
        "  color: #98c379;"
        "  padding: 1px 4px;"
        "  border-radius: 3px;"
        "}"
        "pre {"
        "  font-family: 'Cascadia Code', 'Consolas', monospace;"
        "  font-size: 12px;"
        "  background-color: #1b1d23;"
        "  color: #abb2bf;"
        "  padding: 8px 10px;"
        "  border: 1px solid #282c34;"
        "  border-radius: 4px;"
        "  margin-top: 6px;"
        "  margin-bottom: 8px;"
        "}"
        "table {"
        "  border-collapse: collapse;"
        "  width: 100%;"
        "  margin-top: 6px;"
        "  margin-bottom: 8px;"
        "  border: 1px solid #3b4048;"
        "}"
        "th {"
        "  background-color: #282c34;"
        "  color: #61afef;"
        "  padding: 5px 8px;"
        "  border: 1px solid #3b4048;"
        "  text-align: left;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "td {"
        "  padding: 4px 8px;"
        "  border: 1px solid #3b4048;"
        "  color: #abb2bf;"
        "  font-size: 12px;"
        "}"
    );

    lesson_content_->setStyleSheet(
        "QTextBrowser {"
        "  background-color: transparent;"
        "  border: none;"
        "  color: #abb2bf;"
        "  selection-background-color: #264f78;"
        "  selection-color: #ffffff;"
        "  padding: 0px 4px 2px 0px;"
        "}"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: #1b1d23;"
        "  width: 8px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #3e4451;"
        "  min-height: 24px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #4ec9b0;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "  background: #528bff;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "  background: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"
    );
    right_layout->addWidget(lesson_content_, 1);

    // Anchored bottom action bar
    lesson_action_bar_ = new QFrame(right_box);
    lesson_action_bar_->setObjectName("lessonActionBar");
    lesson_action_bar_->setStyleSheet(
        "#lessonActionBar { border-top: 1px solid #282c34; background: transparent; padding-top: 8px; }"
    );
    auto* action_layout = new QHBoxLayout(lesson_action_bar_);
    action_layout->setContentsMargins(0, 4, 0, 0);
    action_layout->setSpacing(8);

    open_lesson_example_btn_ = new QPushButton(lesson_action_bar_);
    open_lesson_example_btn_->setCursor(Qt::PointingHandCursor);
    open_lesson_example_btn_->setStyleSheet(
        "QPushButton { background-color: #00adb5; color: white; font-weight: bold; padding: 7px 16px; border-radius: 4px; border: none; font-size: 12px; }"
        "QPushButton:hover { background-color: #00c4cc; }"
        "QPushButton:pressed { background-color: #00969d; }"
    );
    connect(open_lesson_example_btn_, &QPushButton::clicked, this, [this]() {
        if (current_lesson_idx_ >= 0 && current_lesson_idx_ < 11) {
            const QString path = kLessons[current_lesson_idx_].examplePath;
            if (!path.isEmpty()) {
                emit openExampleRequested(path);
            }
        }
    });
    action_layout->addWidget(open_lesson_example_btn_, 0, Qt::AlignLeft);
    action_layout->addStretch(1);

    right_layout->addWidget(lesson_action_bar_, 0);

    page_layout->addWidget(right_box, 5);

    welcome_tabs_->addTab(curr_page, "📚 Curriculum (11 Lessons)");

    connect(lesson_list_, &QListWidget::currentRowChanged, this, &WelcomeWidget::onLessonSelected);
    lesson_list_->setCurrentRow(0);
}

void WelcomeWidget::setupFirstProgramTab() {
    auto* fp_page = new QWidget(welcome_tabs_);
    auto* fp_layout = new QVBoxLayout(fp_page);
    fp_layout->setContentsMargins(20, 16, 20, 16);
    fp_layout->setSpacing(12);

    auto* title = new QLabel("<h3>First Program Tutorial — Adding Numbers and Driving LEDs</h3>", fp_page);
    title->setStyleSheet("QLabel { color: #4ec9b0; }");
    fp_layout->addWidget(title);

    auto* desc = new QLabel(
        "<p>This beginner tutorial introduces you to writing your first assembly program in BitForge. "
        "You will see how to declare the program origin, load numbers into CPU registers, perform addition, "
        "output the binary result to the Virtual LEDs, and store it into memory.</p>"
        "<h4>Step-by-Step Instructions:</h4>"
        "<ol>"
        "<li>Click <b>'Open First Program in Editor'</b> below. BitForge creates a dedicated tab <code>tutorial_first_program.asm</code> without touching any existing source files.</li>"
        "<li>Press <b>Assemble (F7)</b> in the toolbar. The assembler will compile the instructions into machine code at address <code>0x0200</code>.</li>"
        "<li>Click the editor gutter next to <code>ADD R0, R1</code> (or press <b>F9</b>) to set a breakpoint. A red circle will appear.</li>"
        "<li>Press <b>Run from Entry (F5)</b>. Execution starts from <code>0x0200</code> and pauses right before executing the <code>ADD</code> instruction.</li>"
        "<li>Inspect the <b>CPU Registers & Flags</b> panel on the right. Notice <code>R0 = 10</code> (0x0A) and <code>R1 = 20</code> (0x14).</li>"
        "<li>Press <b>Step (F10)</b>. Observe Register <code>R0</code> mutate to <code>30</code> (0x1E), highlighted in amber!</li>"
        "<li>Press <b>Continue (F8)</b>. The program outputs the sum to the Virtual LEDs (Port 0x10), stores it in memory at address 0x8000, and halts cleanly!</li>"
        "</ol>",
        fp_page
    );
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color: #abb2bf; font-size: 12px; line-height: 1.4; }");
    fp_layout->addWidget(desc, 1);

    first_program_btn_ = new QPushButton("🚀 Open First Program in Editor (tutorial_first_program.asm)", fp_page);
    first_program_btn_->setCursor(Qt::PointingHandCursor);
    first_program_btn_->setStyleSheet(
        "QPushButton { background-color: #4ec9b0; color: #1e1e1e; font-size: 13px; font-weight: bold; padding: 10px 16px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #5edbb8; }"
        "QPushButton:pressed { background-color: #3cb89a; }"
    );
    connect(first_program_btn_, &QPushButton::clicked, this, &WelcomeWidget::openFirstProgramTutorialRequested);
    fp_layout->addWidget(first_program_btn_, 0, Qt::AlignLeft);

    auto* scroll = new QScrollArea(welcome_tabs_);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(fp_page);
    welcome_tabs_->addTab(scroll, "🎓 First Program Tutorial");
}

void WelcomeWidget::onLessonSelected(int row) {
    if (row < 0 || row >= 11) return;
    current_lesson_idx_ = row;
    const auto& lesson = kLessons[row];
    lesson_content_->setHtml(lesson.htmlContent);

    // Reset vertical scrollbar to top whenever a new lesson is selected
    if (lesson_content_->verticalScrollBar()) {
        lesson_content_->verticalScrollBar()->setValue(0);
    }

    if (!lesson.buttonText.isEmpty() && !lesson.examplePath.isEmpty()) {
        open_lesson_example_btn_->setText(lesson.buttonText);
        open_lesson_example_btn_->show();
        if (lesson_action_bar_ != nullptr) {
            lesson_action_bar_->show();
        }
    } else {
        open_lesson_example_btn_->hide();
        if (lesson_action_bar_ != nullptr) {
            lesson_action_bar_->hide();
        }
    }
}

void WelcomeWidget::selectTab(int index) {
    if (welcome_tabs_ && index >= 0 && index < welcome_tabs_->count()) {
        welcome_tabs_->setCurrentIndex(index);
    }
}

void WelcomeWidget::selectLesson(int lessonIndex) {
    selectTab(1); // Curriculum tab
    if (lesson_list_ && lessonIndex >= 0 && lessonIndex < lesson_list_->count()) {
        lesson_list_->setCurrentRow(lessonIndex);
    }
}

int WelcomeWidget::currentLessonIndex() const {
    return current_lesson_idx_;
}

void WelcomeWidget::setShowOnStartup(bool show) {
    if (show_on_startup_cb_) {
        if (show_on_startup_cb_->isChecked() != show) {
            show_on_startup_cb_->setChecked(show);
        }
    }
}

bool WelcomeWidget::showOnStartup() const {
    return show_on_startup_cb_ ? show_on_startup_cb_->isChecked() : true;
}

void WelcomeWidget::setRecentFiles(const QStringList& files) {
    recent_paths_.clear();
    recent_list_->clear();

    for (const auto& file : files) {
        if (file.trimmed().isEmpty()) continue;
        if (!QFile::exists(file)) continue;
        if (!recent_paths_.contains(file)) {
            recent_paths_.append(file);
        }
    }

    if (recent_paths_.empty()) {
        auto* item = new QListWidgetItem("No recent files yet. Open or create an assembly file to begin.", recent_list_);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor(108, 117, 125));
        clear_recent_btn_->setEnabled(false);
    } else {
        clear_recent_btn_->setEnabled(true);
        for (const auto& path : recent_paths_) {
            QFileInfo fi(path);
            auto* item = new QListWidgetItem(recent_list_);
            item->setText(fi.fileName());
            item->setToolTip(path);
            item->setData(Qt::UserRole, path);
        }
    }
}

QStringList WelcomeWidget::recentFiles() const {
    return recent_paths_;
}

void WelcomeWidget::onRecentItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    if (!QFile::exists(path)) {
        QMessageBox::warning(
            this, "File Not Found",
            QString("The file was not found at the specified path:\n%1\n\nIt will be removed from recent files.").arg(path)
        );
        recent_paths_.removeAll(path);
        setRecentFiles(recent_paths_);
        return;
    }

    emit openRecentRequested(path);
}

void WelcomeWidget::onClearRecentClicked() {
    recent_paths_.clear();
    setRecentFiles(recent_paths_);
    QSettings settings(MainWindow::settingsOrganization(), MainWindow::settingsApplication());
    settings.remove("recentFiles");
}

} // namespace bitforge::gui
