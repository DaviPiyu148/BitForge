#include "bitforge/gui/devices_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QFontDatabase>
#include <QRegularExpression>

namespace bitforge::gui {

namespace {

std::vector<byte_t> parseHexByteSequence(const QString& input) {
    std::vector<byte_t> bytes;
    QString clean = input;
    clean.replace(',', ' ').replace(';', ' ');
    const QStringList tokens = clean.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString& tok : tokens) {
        QString s = tok.trimmed();
        bool is_explicit_hex = false;
        if (s.startsWith("0x", Qt::CaseInsensitive)) {
            s = s.mid(2);
            is_explicit_hex = true;
        } else if (s.startsWith("$")) {
            s = s.mid(1);
            is_explicit_hex = true;
        }
        if (s.isEmpty()) continue;
        bool ok = false;
        uint val = 0;
        if (is_explicit_hex || s.contains(QRegularExpression("[a-fA-F]"))) {
            val = s.toUInt(&ok, 16);
        } else {
            val = s.toUInt(&ok, 10);
        }
        if (ok && val <= 0xFF) {
            bytes.push_back(static_cast<byte_t>(val));
        }
    }
    return bytes;
}

} // namespace

DevicesPanel::DevicesPanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // 1. Group: Virtual LEDs (Port 0x10)
    auto* led_group = new QGroupBox("Virtual LEDs (Port 0x10)", this);
    auto* led_vbox = new QVBoxLayout(led_group);
    led_vbox->setContentsMargins(6, 4, 6, 4);
    led_vbox->setSpacing(3);

    auto* led_bank_layout = new QHBoxLayout();
    led_bank_layout->setContentsMargins(0, 0, 0, 0);
    led_bank_layout->setSpacing(6);

    for (int i = 7; i >= 0; --i) {
        auto* dot_container = new QVBoxLayout();
        dot_container->setContentsMargins(0, 0, 0, 0);
        dot_container->setSpacing(1);
        dot_container->setAlignment(Qt::AlignCenter);

        auto* bit_label = new QLabel(QString::number(i), led_group);
        bit_label->setAlignment(Qt::AlignCenter);
        bit_label->setStyleSheet("QLabel { color: #61afef; font-size: 10px; font-weight: bold; }");

        auto* dot = new QLabel(led_group);
        dot->setFixedSize(18, 18);
        dot->setAlignment(Qt::AlignCenter);
        dot->setStyleSheet("QLabel { background-color: #1b1d23; border-radius: 9px; border: 1px solid #3b4048; }");
        led_dots_[static_cast<std::size_t>(i)] = dot;

        dot_container->addWidget(bit_label, 0, Qt::AlignCenter);
        dot_container->addWidget(dot, 0, Qt::AlignCenter);
        led_bank_layout->addLayout(dot_container);
    }
    led_bank_layout->addStretch(1);
    led_vbox->addLayout(led_bank_layout);

    auto* val_layout = new QHBoxLayout();
    val_layout->setContentsMargins(0, 0, 0, 0);
    val_layout->setSpacing(4);

    led_value_label_ = new QLabel("0x00 (0b00000000)", led_group);
    led_value_label_->setFont(mono_font);
    led_value_label_->setStyleSheet(
        "QLabel { background-color: #1e1e1e; color: #abb2bf; border: 1px solid #282c34; border-radius: 4px; padding: 2px 6px; font-size: 10px; }"
    );
    val_layout->addWidget(led_value_label_, 0, Qt::AlignLeft);
    val_layout->addStretch(1);
    led_vbox->addLayout(val_layout);

    main_layout->addWidget(led_group);

    // 2. Group: 7-Segment Numeric Display (Port 0x02)
    auto* seg_group = new QGroupBox("Numeric Display (Port 0x02)", this);
    auto* seg_layout = new QVBoxLayout(seg_group);
    seg_layout->setContentsMargins(6, 4, 6, 4);
    seven_seg_display_ = new SevenSegmentDisplay(seg_group);
    seven_seg_display_->setMinimumHeight(66);
    seg_layout->addWidget(seven_seg_display_);

    main_layout->addWidget(seg_group);

    // 3. Group: Input FIFO (Port 0x00)
    auto* fifo_group = new QGroupBox("Input FIFO (Port 0x00)", this);
    auto* fifo_layout = new QVBoxLayout(fifo_group);
    fifo_layout->setContentsMargins(6, 4, 6, 4);
    fifo_layout->setSpacing(4);

    // Row 1: Hex Input + Enqueue Button + Clear Button
    auto* input_row = new QHBoxLayout();
    fifo_input_edit_ = new QLineEdit(fifo_group);
    fifo_input_edit_->setFont(mono_font);
    fifo_input_edit_->setPlaceholderText("Hex byte(s) (e.g. A5, 0x12, 42)...");
    input_row->addWidget(fifo_input_edit_, 1);

    fifo_enqueue_btn_ = new QPushButton("Enqueue", fifo_group);
    fifo_enqueue_btn_->setStyleSheet("QPushButton { font-weight: bold; }");
    input_row->addWidget(fifo_enqueue_btn_);

    fifo_clear_btn_ = new QPushButton("Clear FIFO", fifo_group);
    input_row->addWidget(fifo_clear_btn_);
    fifo_layout->addLayout(input_row);

    // Row 2: Status (Depth & Next) + Underflow Indicator
    auto* status_row = new QHBoxLayout();
    fifo_status_label_ = new QLabel("Buffer: Empty (0 bytes queued)", fifo_group);
    fifo_status_label_->setFont(mono_font);
    status_row->addWidget(fifo_status_label_, 1);

    fifo_underflow_label_ = new QLabel("Underflows: 0 (OK)", fifo_group);
    fifo_underflow_label_->setFont(mono_font);
    fifo_underflow_label_->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(theme::kTextMuted.name()));
    status_row->addWidget(fifo_underflow_label_);
    fifo_layout->addLayout(status_row);

    // Row 3: Queued Bytes Display
    fifo_contents_label_ = new QLabel("Queue: [ Empty ]", fifo_group);
    fifo_contents_label_->setFont(mono_font);
    fifo_contents_label_->setStyleSheet(
        QString("QLabel { color: %1; background-color: %2; padding: 4px 8px; border-radius: %3px; border: 1px solid %4; }")
            .arg(theme::kStateReady.name(), theme::kBgDark.name())
            .arg(theme::kRadiusControl)
            .arg(theme::kBorderMedium.name())
    );
    fifo_contents_label_->setWordWrap(true);
    fifo_layout->addWidget(fifo_contents_label_);

    connect(fifo_enqueue_btn_, &QPushButton::clicked, this, &DevicesPanel::onEnqueueInput);
    connect(fifo_input_edit_, &QLineEdit::returnPressed, this, &DevicesPanel::onEnqueueInput);
    connect(fifo_clear_btn_, &QPushButton::clicked, this, &DevicesPanel::onClearFifo);

    main_layout->addWidget(fifo_group);

    // 4. Tab Widget for Consoles and Diagnostics
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

    tabs->addTab(text_tab, "Text Console");

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

    tabs->addTab(num_tab, "Numeric Console");

    // Tab C: Diagnostics
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

    diag_empty_label_ = new QLabel("No assembler diagnostics or runtime faults reported.", diag_tab);
    diag_empty_label_->setStyleSheet(theme::emptyStateStyle(true));
    diag_layout->addWidget(diag_empty_label_);

    tabs->addTab(diag_tab, "Diagnostics");

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
                "QLabel { background-color: #4ec9b0; border-radius: 9px; border: 1px solid #7be5d0; }"
            );
        } else {
            led_dots_[static_cast<std::size_t>(i)]->setStyleSheet(
                "QLabel { background-color: #1b1d23; border-radius: 9px; border: 1px solid #3b4048; }"
            );
        }
    }

    QString bin_str;
    for (int i = 7; i >= 0; --i) {
        bin_str.append(((led_val & (1 << i)) != 0) ? '1' : '0');
    }
    QString hex_str = QString("%1").arg(led_val, 2, 16, QLatin1Char('0')).toUpper();
    led_value_label_->setText(QString("0x%1 (0b%2)").arg(hex_str, bin_str));

    // 2. Update 7-Segment Display
    if (seven_seg_display_ != nullptr) {
        const auto& num_vals = controller_->bus().numeric_values();
        if (num_vals.empty()) {
            seven_seg_display_->clearValue();
        } else {
            seven_seg_display_->setValue(num_vals.back());
        }
    }

    // 3. Update Input FIFO status
    if (fifo_status_label_ != nullptr) {
        const std::size_t fifo_sz = controller_->bus().input_fifo_size();
        const auto next_b = controller_->bus().peek_input_fifo();
        const std::size_t underflows = controller_->bus().input_underflow_count();
        const auto bytes = controller_->bus().input_fifo_contents();

        if (fifo_sz == 0) {
            fifo_status_label_->setText("Buffer: Empty (0 bytes queued)");
        } else if (next_b.has_value()) {
            QString next_hex = "0x" + QString("%1").arg(*next_b, 2, 16, QLatin1Char('0')).toUpper();
            fifo_status_label_->setText(
                QString("Buffer: %1 bytes queued | Next: %2 (%3)")
                    .arg(fifo_sz)
                    .arg(next_hex)
                    .arg(*next_b)
            );
        } else {
            fifo_status_label_->setText(QString("Buffer: %1 bytes queued").arg(fifo_sz));
        }

        if (fifo_underflow_label_ != nullptr) {
            if (underflows == 0) {
                fifo_underflow_label_->setText("Underflows: 0 (OK)");
                fifo_underflow_label_->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(theme::kTextMuted.name()));
            } else {
                fifo_underflow_label_->setText(QString("Underflows: %1 (Read Empty)").arg(underflows));
                fifo_underflow_label_->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; font-size: 11px; }").arg(theme::kStateFaulted.name()));
            }
        }

        if (fifo_contents_label_ != nullptr) {
            if (bytes.empty()) {
                fifo_contents_label_->setText("Queue: [ Empty ]");
            } else {
                QString qstr = "Queue: [ ";
                const std::size_t max_disp = std::min(bytes.size(), static_cast<std::size_t>(16));
                for (std::size_t i = 0; i < max_disp; ++i) {
                    if (i > 0) qstr += " ";
                    qstr += "0x" + QString("%1").arg(bytes[i], 2, 16, QLatin1Char('0')).toUpper();
                }
                if (bytes.size() > max_disp) {
                    qstr += QString(" ... (+%1 more)").arg(bytes.size() - max_disp);
                }
                qstr += " ]";
                fifo_contents_label_->setText(qstr);
            }
        }
    }

    // 4. Update Text Console
    text_console_->setPlainText(QString::fromStdString(controller_->bus().text_output()));
    text_console_->moveCursor(QTextCursor::End);

    // 5. Update Numeric Console
    numeric_console_->setPlainText(QString::fromStdString(controller_->bus().numeric_output()));
    numeric_console_->moveCursor(QTextCursor::End);

    // 6. Update Faults in Diagnostics if CPU is faulted
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
        QFont fault_font = item_sev->font();
        fault_font.setBold(true);
        item_sev->setFont(fault_font);
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

    if (diag_empty_label_ != nullptr) {
        diag_empty_label_->setVisible(diag_table_->rowCount() == 0);
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

void DevicesPanel::onEnqueueInput() {
    if (controller_ == nullptr || fifo_input_edit_ == nullptr) return;
    const QString text = fifo_input_edit_->text().trimmed();
    if (text.isEmpty()) return;

    const auto bytes = parseHexByteSequence(text);
    if (!bytes.empty()) {
        controller_->pushInputFifo(bytes);
        fifo_input_edit_->clear();
        refresh();
    }
}

void DevicesPanel::onClearFifo() {
    if (controller_ == nullptr) return;
    controller_->clearInputFifo();
    refresh();
}

} // namespace bitforge::gui
