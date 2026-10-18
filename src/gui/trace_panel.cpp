#include "bitforge/gui/trace_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontDatabase>

namespace bitforge::gui {

TracePanel::TracePanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    table_ = new QTableWidget(0, 7, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Step #" << "PC" << "Mnemonic" << "Raw Bytes" << "Source" << "State After" << "Execution Delta";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);

    main_layout->addWidget(table_);

    auto* button_bar = new QHBoxLayout();
    clear_btn_ = new QPushButton("Clear Trace", this);
    empty_label_ = new QLabel("Execution trace empty. Assemble and run/step to record instruction history.", this);
    empty_label_->setStyleSheet("QLabel { color: #8a93a0; font-style: italic; }");

    button_bar->addWidget(clear_btn_);
    button_bar->addSpacing(12);
    button_bar->addWidget(empty_label_);
    button_bar->addStretch();
    main_layout->addLayout(button_bar);

    connect(clear_btn_, &QPushButton::clicked, this, &TracePanel::onClearTrace);
}

void TracePanel::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &TracePanel::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &TracePanel::refresh);
    }
    refresh();
}

void TracePanel::refresh() {
    if (controller_ == nullptr) return;

    const auto& trace = controller_->session().trace();
    table_->setRowCount(static_cast<int>(trace.size()));

    for (int i = 0; i < static_cast<int>(trace.size()); ++i) {
        const auto& entry = trace[static_cast<std::size_t>(i)];

        // 0: Step #
        auto* item_step = new QTableWidgetItem(QString::number(entry.step_number));
        item_step->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 0, item_step);

        // 1: PC
        auto* item_pc = new QTableWidgetItem(
            QString("0x%1").arg(entry.instruction_address, 4, 16, QLatin1Char('0')).toUpper()
        );
        item_pc->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 1, item_pc);

        // 2: Mnemonic
        auto* item_mnem = new QTableWidgetItem(
            entry.mnemonic.empty() ? QString("0x%1").arg(entry.opcode_raw, 2, 16, QLatin1Char('0')).toUpper()
                                   : QString::fromStdString(entry.mnemonic)
        );
        item_mnem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 2, item_mnem);

        // 3: Raw bytes
        QString bytes_str;
        for (std::size_t b = 0; b < entry.instruction_bytes.size(); ++b) {
            bytes_str.append(QString("%1 ").arg(entry.instruction_bytes[b], 2, 16, QLatin1Char('0')).toUpper());
        }
        auto* item_bytes = new QTableWidgetItem(bytes_str.trimmed());
        item_bytes->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 3, item_bytes);

        // 4: Source
        QString src_str = "-";
        if (entry.source_location.has_value()) {
            src_str = QString("Line %1, Col %2")
                          .arg(entry.source_location->line)
                          .arg(entry.source_location->column);
        }
        auto* item_src = new QTableWidgetItem(src_str);
        item_src->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 4, item_src);

        // 5: State After
        QString state_str = QString::fromStdString(std::string(to_string(entry.state_after)));
        if (entry.faulted) {
            state_str = QString("FAULT: %1").arg(entry.fault.has_value() ? to_string(entry.fault->type).data() : "Unknown");
        }
        auto* item_state = new QTableWidgetItem(state_str);
        item_state->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 5, item_state);

        // 6: Execution Delta
        QString delta_str = "-";
        if (entry.delta.has_value()) {
            delta_str = QString::fromStdString(entry.delta->to_string());
        }
        auto* item_delta = new QTableWidgetItem(delta_str);
        item_delta->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table_->setItem(i, 6, item_delta);

        // Highlight HALT or Fault
        if (entry.state_after == MachineState::Halted) {
            for (int col = 0; col < 7; ++col) {
                auto* it = table_->item(i, col);
                if (it != nullptr) it->setBackground(QColor(230, 244, 234)); // Soft green
            }
        } else if (entry.faulted || entry.state_after == MachineState::Faulted) {
            for (int col = 0; col < 7; ++col) {
                auto* it = table_->item(i, col);
                if (it != nullptr) it->setBackground(QColor(252, 232, 230)); // Soft red
            }
        }
    }

    if (!trace.empty()) {
        table_->scrollToBottom();
    }

    if (empty_label_ != nullptr) {
        empty_label_->setVisible(trace.empty());
    }
    clear_btn_->setEnabled(!trace.empty());
}

void TracePanel::onClearTrace() {
    if (controller_ == nullptr) return;
    controller_->session().clear_trace();
    table_->setRowCount(0);
    if (empty_label_ != nullptr) {
        empty_label_->setVisible(true);
    }
    clear_btn_->setEnabled(false);
}

} // namespace bitforge::gui
