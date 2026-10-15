#include "bitforge/gui/devices_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QFontDatabase>

namespace bitforge::gui {

DevicesPanel::DevicesPanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // 1. Group: Virtual LEDs (Port 0x10)
    auto* led_group = new QGroupBox("Virtual LEDs (Port 0x10)", this);
    auto* led_layout = new QHBoxLayout(led_group);
    led_layout->setContentsMargins(8, 8, 8, 8);

    for (int i = 7; i >= 0; --i) {
        auto* dot_container = new QVBoxLayout();
        auto* bit_label = new QLabel(QString::number(i), led_group);
        bit_label->setAlignment(Qt::AlignCenter);

        auto* dot = new QLabel(led_group);
        dot->setFixedSize(16, 16);
        dot->setStyleSheet("QLabel { background-color: #6c757d; border-radius: 8px; border: 1px solid #495057; }");
        led_dots_[static_cast<std::size_t>(i)] = dot;

        dot_container->addWidget(bit_label);
        dot_container->addWidget(dot);
        led_layout->addLayout(dot_container);
    }

    led_layout->addSpacing(16);
    led_value_label_ = new QLabel("0x00 (0b00000000)", led_group);
    led_value_label_->setFont(mono_font);
    led_layout->addWidget(led_value_label_);
    led_layout->addStretch();

    main_layout->addWidget(led_group);

    // 2. Tab Widget for Consoles and Diagnostics
    auto* tabs = new QTabWidget(this);

    // Tab A: Text Console (Port 0x01)
    auto* text_tab = new QWidget(tabs);
    auto* text_layout = new QVBoxLayout(text_tab);
    text_layout->setContentsMargins(4, 4, 4, 4);

    text_console_ = new QPlainTextEdit(text_tab);
    text_console_->setFont(mono_font);
    text_console_->setReadOnly(true);
    text_layout->addWidget(text_console_);

    auto* btn_clear_text = new QPushButton("Clear Output", text_tab);
    connect(btn_clear_text, &QPushButton::clicked, text_console_, &QPlainTextEdit::clear);
    text_layout->addWidget(btn_clear_text, 0, Qt::AlignRight);

    tabs->addTab(text_tab, "Text Console (0x01)");

    // Tab B: Numeric Console (Port 0x02)
    auto* num_tab = new QWidget(tabs);
    auto* num_layout = new QVBoxLayout(num_tab);
    num_layout->setContentsMargins(4, 4, 4, 4);

    numeric_console_ = new QPlainTextEdit(num_tab);
    numeric_console_->setFont(mono_font);
    numeric_console_->setReadOnly(true);
    num_layout->addWidget(numeric_console_);

    auto* btn_clear_num = new QPushButton("Clear Output", num_tab);
    connect(btn_clear_num, &QPushButton::clicked, numeric_console_, &QPlainTextEdit::clear);
    num_layout->addWidget(btn_clear_num, 0, Qt::AlignRight);

    tabs->addTab(num_tab, "Numeric Console (0x02)");

    // Tab C: Diagnostics & Faults
    auto* diag_tab = new QWidget(tabs);
    auto* diag_layout = new QVBoxLayout(diag_tab);
    diag_layout->setContentsMargins(4, 4, 4, 4);

    diag_table_ = new QTableWidget(0, 4, diag_tab);
    diag_table_->setFont(mono_font);
    diag_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diag_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    diag_table_->verticalHeader()->setVisible(false);

    QStringList diag_headers;
    diag_headers << "Severity" << "Line" << "Col" << "Message";
    diag_table_->setHorizontalHeaderLabels(diag_headers);
    diag_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    diag_table_->horizontalHeader()->setStretchLastSection(true);

    diag_layout->addWidget(diag_table_);
    tabs->addTab(diag_tab, "Diagnostics & Faults");

    connect(diag_table_, &QTableWidget::cellDoubleClicked, this, &DevicesPanel::onDiagnosticTableClicked);
    connect(diag_table_, &QTableWidget::cellClicked, this, &DevicesPanel::onDiagnosticTableClicked);

    main_layout->addWidget(tabs);
}

void DevicesPanel::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &DevicesPanel::refresh);
        connect(controller_, &ExecutionController::stateChanged, this, &DevicesPanel::showDiagnostics);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &DevicesPanel::showDiagnostics);
        connect(controller_, &ExecutionController::assemblyFailed, this, &DevicesPanel::showDiagnostics);
    }
    refresh();
}

void DevicesPanel::refresh() {
    if (controller_ == nullptr) return;

    // 1. Update LEDs
    const byte_t led_val = controller_->bus().led_state();
    for (int i = 0; i < 8; ++i) {
        bool bit_set = (led_val & (1 << i)) != 0;
        if (bit_set) {
            led_dots_[static_cast<std::size_t>(i)]->setStyleSheet(
                "QLabel { background-color: #28a745; border-radius: 8px; border: 1px solid #1e7e34; }"
            );
        } else {
            led_dots_[static_cast<std::size_t>(i)]->setStyleSheet(
                "QLabel { background-color: #6c757d; border-radius: 8px; border: 1px solid #495057; }"
            );
        }
    }

    QString bin_str;
    for (int i = 7; i >= 0; --i) {
        bin_str.append(((led_val & (1 << i)) != 0) ? '1' : '0');
    }
    led_value_label_->setText(
        QString("0x%1 (0b%2)").arg(led_val, 2, 16, QLatin1Char('0')).toUpper().arg(bin_str)
    );

    // 2. Update Text Console
    text_console_->setPlainText(QString::fromStdString(controller_->bus().text_output()));
    text_console_->moveCursor(QTextCursor::End);

    // 3. Update Numeric Console
    numeric_console_->setPlainText(QString::fromStdString(controller_->bus().numeric_output()));
    numeric_console_->moveCursor(QTextCursor::End);

    // 4. Update Faults in Diagnostics if CPU is faulted
    if (controller_->session().state() == MachineState::Faulted &&
        controller_->cpu().last_fault().has_fault()) {
        showDiagnostics();
    }
}

void DevicesPanel::showDiagnostics() {
    if (controller_ == nullptr) return;

    diag_table_->setRowCount(0);

    // Add Assembler diagnostics
    const auto& diags = controller_->diagnostics();
    for (const auto& d : diags) {
        const int row = diag_table_->rowCount();
        diag_table_->insertRow(row);

        QString sev_str = (d.severity == assembler::DiagnosticSeverity::Error) ? "Error" : "Warning";
        auto* item_sev = new QTableWidgetItem(sev_str);
        item_sev->setForeground((d.severity == assembler::DiagnosticSeverity::Error) ? QColor(220, 53, 69) : QColor(255, 193, 7));
        diag_table_->setItem(row, 0, item_sev);

        auto* item_line = new QTableWidgetItem(QString::number(d.location.line));
        item_line->setTextAlignment(Qt::AlignCenter);
        diag_table_->setItem(row, 1, item_line);

        auto* item_col = new QTableWidgetItem(QString::number(d.location.column));
        item_col->setTextAlignment(Qt::AlignCenter);
        diag_table_->setItem(row, 2, item_col);

        auto* item_msg = new QTableWidgetItem(QString::fromStdString(d.message));
        diag_table_->setItem(row, 3, item_msg);
    }

    // Add CPU fault if present
    if (controller_->session().state() == MachineState::Faulted &&
        controller_->cpu().last_fault().has_fault()) {
        const auto& fault = controller_->cpu().last_fault();
        const int row = diag_table_->rowCount();
        diag_table_->insertRow(row);

        auto* item_sev = new QTableWidgetItem("CPU Fault");
        item_sev->setForeground(QColor(220, 53, 69));
        item_sev->setFont(QFont(item_sev->font().family(), -1, QFont::Bold));
        diag_table_->setItem(row, 0, item_sev);

        // Map fault PC to source line if possible
        auto loc = controller_->session().resolve_source_location(fault.instruction_address);
        if (loc.has_value()) {
            auto* item_line = new QTableWidgetItem(QString::number(loc->line));
            item_line->setTextAlignment(Qt::AlignCenter);
            diag_table_->setItem(row, 1, item_line);

            auto* item_col = new QTableWidgetItem(QString::number(loc->column));
            item_col->setTextAlignment(Qt::AlignCenter);
            diag_table_->setItem(row, 2, item_col);
        } else {
            auto* item_addr = new QTableWidgetItem(QString("0x%1").arg(fault.instruction_address, 4, 16, QLatin1Char('0')).toUpper());
            item_addr->setTextAlignment(Qt::AlignCenter);
            diag_table_->setItem(row, 1, item_addr);

            auto* item_dash = new QTableWidgetItem("-");
            item_dash->setTextAlignment(Qt::AlignCenter);
            diag_table_->setItem(row, 2, item_dash);
        }

        QString fault_msg = QString("%1: %2").arg(to_string(fault.type).data()).arg(QString::fromStdString(fault.message));
        auto* item_msg = new QTableWidgetItem(fault_msg);
        diag_table_->setItem(row, 3, item_msg);
    }
}

void DevicesPanel::onDiagnosticTableClicked(int row, int /* column */) {
    if (row < 0 || row >= diag_table_->rowCount()) return;

    auto* line_item = diag_table_->item(row, 1);
    auto* col_item = diag_table_->item(row, 2);
    if (line_item == nullptr) return;

    bool ok_line = false;
    std::size_t line = line_item->text().toULongLong(&ok_line);
    if (!ok_line) return;

    std::size_t col = 1;
    if (col_item != nullptr) {
        bool ok_col = false;
        col = col_item->text().toULongLong(&ok_col);
        if (!ok_col) col = 1;
    }

    emit diagnosticClicked(line, col);
}

} // namespace bitforge::gui
