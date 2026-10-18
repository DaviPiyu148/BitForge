#include "bitforge/gui/examples_dialog.hpp"
#include "bitforge/gui/asm_highlighter.hpp"
#include "bitforge/gui/theme.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>

namespace bitforge::gui {

namespace {

QString resolveExamplePath(const QString& relPath) {
    // 1. Current working directory
    if (QFile::exists(relPath)) {
        return relPath;
    }
    // 2. Relative to application directory
    QString appRel = QDir(QCoreApplication::applicationDirPath()).filePath(relPath);
    if (QFile::exists(appRel)) {
        return appRel;
    }
    // 3. Parent of build directory
    QString repoRel = QDir(QCoreApplication::applicationDirPath() + "/../../").filePath(relPath);
    if (QFile::exists(repoRel)) {
        return QDir::cleanPath(repoRel);
    }
    return relPath;
}

} // namespace

ExamplesDialog::ExamplesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Browse Built-in Assembly Examples");
    resize(960, 600);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(10);

    auto* header_label = new QLabel("Select an assembly program to inspect, assemble, and execute in the workbench:", this);
    header_label->setStyleSheet("QLabel { font-weight: bold; color: #4ec9b0; font-size: 13px; }");
    main_layout->addWidget(header_label);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left pane: Categories
    auto* cat_container = new QWidget(splitter);
    auto* cat_layout = new QVBoxLayout(cat_container);
    cat_layout->setContentsMargins(0, 0, 0, 0);
    cat_layout->addWidget(new QLabel("Categories:", cat_container));
    category_list_ = new QListWidget(cat_container);
    cat_layout->addWidget(category_list_);
    splitter->addWidget(cat_container);

    // Center pane: Examples in Category
    auto* ex_container = new QWidget(splitter);
    auto* ex_layout = new QVBoxLayout(ex_container);
    ex_layout->setContentsMargins(0, 0, 0, 0);
    ex_layout->addWidget(new QLabel("Programs:", ex_container));
    example_list_ = new QListWidget(ex_container);
    ex_layout->addWidget(example_list_);
    splitter->addWidget(ex_container);

    // Right pane: Code Preview
    auto* prev_container = new QWidget(splitter);
    auto* prev_layout = new QVBoxLayout(prev_container);
    prev_layout->setContentsMargins(0, 0, 0, 0);
    info_label_ = new QLabel("Program Preview:", prev_container);
    prev_layout->addWidget(info_label_);

    preview_edit_ = new QPlainTextEdit(prev_container);
    preview_edit_->setReadOnly(true);
    preview_edit_->setFont(theme::monoFont(10));
    new AsmHighlighter(preview_edit_->document());
    prev_layout->addWidget(preview_edit_);
    splitter->addWidget(prev_container);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 3);
    main_layout->addWidget(splitter, 1);

    // Button bar
    auto* btn_bar = new QHBoxLayout();
    btn_bar->addStretch();

    open_btn_ = new QPushButton("Open in Editor", this);
    open_btn_->setStyleSheet("QPushButton { background-color: #00adb5; color: white; font-weight: bold; padding: 6px 16px; }");
    cancel_btn_ = new QPushButton("Cancel", this);

    btn_bar->addWidget(cancel_btn_);
    btn_bar->addWidget(open_btn_);
    main_layout->addLayout(btn_bar);

    connect(category_list_, &QListWidget::currentRowChanged, this, &ExamplesDialog::onCategoryChanged);
    connect(example_list_, &QListWidget::currentItemChanged, this, &ExamplesDialog::onExampleItemChanged);
    connect(example_list_, &QListWidget::itemDoubleClicked, this, &ExamplesDialog::onOpenClicked);
    connect(open_btn_, &QPushButton::clicked, this, &ExamplesDialog::onOpenClicked);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);

    populateCategories();
}

void ExamplesDialog::populateCategories() {
    catalog_["1. Quick Starts"] = {
        {"01 Arithmetic & Memory", "examples/01_arithmetic.asm", "Fundamental addition and memory store to 0x8000."},
        {"02 Virtual I/O Demo", "examples/02_io_demo.asm", "Text, numeric, and virtual LED peripheral output."},
        {"03 Flag Comparison", "examples/03_comparison.asm", "Comparing values and testing Z and C flags."},
        {"04 Bitwise Logic", "examples/04_bitwise.asm", "AND, OR, XOR, and NOT operations."},
        {"05 Loop Counter", "examples/05_jump_loop.asm", "Conditional jump loop with DEC and JNZ."},
        {"06 Subroutine Call", "examples/06_subroutine.asm", "CALL and RET subroutine execution."}
    };

    catalog_["2. Fundamentals"] = {
        {"01 NOP Sequence", "examples/fundamentals/01_nop_sequence.asm", "Pipeline padding and instruction sequencing."},
        {"02 Immediate Loading", "examples/fundamentals/02_immediate_loading.asm", "8-bit LDI and 16-bit LDI16 loading."},
        {"03 Register Transfer", "examples/fundamentals/03_register_transfer.asm", "Inter-register data transfers via MOV."}
    };

    catalog_["3. Arithmetic & Logic"] = {
        {"01 Addition & Subtraction", "examples/arithmetic/01_addition_subtraction.asm", "Basic arithmetic operations."},
        {"02 Increment & Decrement", "examples/arithmetic/02_increment_decrement.asm", "Single-cycle INC and DEC instructions."},
        {"03 Logic Bitwise", "examples/arithmetic/03_logic_bitwise.asm", "Bitwise masks and logical NOT."},
        {"04 Comparison Flags", "examples/arithmetic/04_comparison_flags.asm", "Signed and unsigned comparisons."},
        {"05 Carry & Borrow Boundaries", "examples/arithmetic/05_carry_borrow_boundaries.asm", "Boundary conditions on carry flag."},
        {"06 Signed Overflow", "examples/arithmetic/06_signed_overflow_boundaries.asm", "Testing the two's complement V flag."}
    };

    catalog_["4. Control Flow"] = {
        {"01 Unconditional Jumps", "examples/control_flow/01_unconditional_jumps.asm", "JMP instruction flow."},
        {"02 Zero Flag Branches", "examples/control_flow/02_zero_flag_branches.asm", "JZ and JNZ branches."},
        {"03 Carry Flag Branches", "examples/control_flow/03_carry_flag_branches.asm", "JC and JNC branches."},
        {"04 Complex Nested Loops", "examples/control_flow/04_nested_loops.asm", "Nested loops for multiplication/delay."}
    };

    catalog_["5. Memory & Pointers"] = {
        {"01 Direct Load & Store", "examples/memory/01_direct_load_store.asm", "Direct memory access via LD and ST."},
        {"02 Indexed Access (IX)", "examples/memory/02_indexed_access_ix.asm", "Indexed memory operations LDX and STX."},
        {"03 Array Traversal", "examples/memory/03_array_traversal.asm", "Pointer bumping using INX and DEX."},
        {"04 Array Summation", "examples/memory/04_array_summation.asm", "Summing consecutive memory words."}
    };

    catalog_["6. Stack & Subroutines"] = {
        {"01 Push & Pop Pairs", "examples/stack_subroutines/01_push_pop_pairs.asm", "Basic stack usage and register preservation."},
        {"02 Subroutine Calls", "examples/stack_subroutines/02_call_ret_subroutine.asm", "Modular code with CALL and RET."},
        {"03 Nested Subroutines", "examples/stack_subroutines/03_nested_calls.asm", "Multiple stack frame depths."},
        {"04 Recursive Factorial", "examples/stack_subroutines/04_recursion_factorial.asm", "Recursive algorithm using stack."}
    };

    catalog_["7. Virtual I/O Devices"] = {
        {"01 Text Console Output", "examples/io_devices/01_text_console_output.asm", "Character and string output on Port 0x01."},
        {"02 Numeric Console Output", "examples/io_devices/02_numeric_console_output.asm", "7-segment integer output on Port 0x02."},
        {"03 LED Patterns & Chasers", "examples/io_devices/03_led_register_patterns.asm", "Port 0x10 binary LED animations."},
        {"04 Input FIFO Reading", "examples/io_devices/04_input_fifo_reading.asm", "Reading characters from Port 0x00."}
    };

    catalog_["8. Algorithms & ADS"] = {
        {"01 Bubble Sort", "examples/algorithms/01_bubble_sort.asm", "In-place memory sorting algorithm."},
        {"02 Linear Search", "examples/algorithms/02_linear_search.asm", "Target search in memory array."},
        {"03 Fibonacci Sequence", "examples/algorithms/03_fibonacci_sequence.asm", "Generating Fibonacci numbers in RAM."},
        {"04 GCD Euclidean", "examples/algorithms/04_gcd_euclidean.asm", "Greatest common divisor calculation."}
    };

    catalog_["9. Debugger & Faults"] = {
        {"01 Step by Step", "examples/debugger/01_step_execution.asm", "Demonstrating single-step state transitions."},
        {"02 Breakpoints", "examples/debugger/02_breakpoint_halt.asm", "Breakpoint triggers and continuation."},
        {"01 Invalid Opcode", "examples/faults/01_invalid_opcode.asm", "CPU architectural fault handling."},
        {"02 Stack Underflow", "examples/faults/02_stack_underflow.asm", "ADR-001 stack underflow protection."}
    };

    category_list_->clear();
    for (const auto& [cat_name, _] : catalog_) {
        category_list_->addItem(cat_name);
    }

    if (category_list_->count() > 0) {
        category_list_->setCurrentRow(0);
    }
}

void ExamplesDialog::onCategoryChanged(int index) {
    if (index < 0 || index >= category_list_->count()) return;
    const QString cat_name = category_list_->item(index)->text();

    example_list_->clear();
    const auto it = catalog_.find(cat_name);
    if (it != catalog_.end()) {
        for (const auto& ex : it->second) {
            auto* item = new QListWidgetItem(ex.title, example_list_);
            item->setData(Qt::UserRole, ex.relativePath);
            item->setData(Qt::UserRole + 1, ex.description);
        }
    }

    if (example_list_->count() > 0) {
        example_list_->setCurrentRow(0);
    }
}

void ExamplesDialog::onExampleItemChanged(QListWidgetItem* current, QListWidgetItem* /* previous */) {
    if (!current) {
        preview_edit_->clear();
        info_label_->setText("Program Preview:");
        return;
    }

    const QString rel_path = current->data(Qt::UserRole).toString();
    const QString desc = current->data(Qt::UserRole + 1).toString();
    info_label_->setText(QString("<b>%1</b> — %2").arg(current->text(), desc));
    loadExamplePreview(rel_path);
}

void ExamplesDialog::loadExamplePreview(const QString& relPath) {
    const QString full_path = resolveExamplePath(relPath);
    QFile file(full_path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        preview_edit_->setPlainText(in.readAll());
    } else {
        preview_edit_->setPlainText("; File not found: " + full_path);
    }
}

void ExamplesDialog::onOpenClicked() {
    auto* item = example_list_->currentItem();
    if (!item) return;

    const QString rel_path = item->data(Qt::UserRole).toString();
    selected_path_ = resolveExamplePath(rel_path);
    emit exampleSelected(selected_path_);
    accept();
}

} // namespace bitforge::gui
