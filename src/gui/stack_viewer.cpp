#include "bitforge/gui/stack_viewer.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontDatabase>
#include <cctype>

namespace bitforge::gui {

StackViewer::StackViewer(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // Top Header info
    auto* top_layout = new QHBoxLayout();
    header_label_ = new QLabel("SP: 0xFFFF | Depth: 0 byte(s)", this);
    header_label_->setFont(mono_font);
    top_layout->addWidget(header_label_);

    status_label_ = new QLabel("Empty stack (SP = 0xFFFF sentinel per ADR-001; no items pushed)", this);
    status_label_->setStyleSheet("QLabel { color: #8a93a0; font-style: italic; }");
    top_layout->addWidget(status_label_);
    top_layout->addStretch();

    main_layout->addLayout(top_layout);

    // Stack Table
    table_ = new QTableWidget(0, 5, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Offset" << "Address" << "Hex" << "Dec" << "ASCII";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);

    main_layout->addWidget(table_);
}

void StackViewer::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &StackViewer::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &StackViewer::refresh);
    }
    refresh();
}

void StackViewer::refresh() {
    if (controller_ == nullptr) return;

    auto stack_info = controller_->session().inspect_stack();

    header_label_->setText(
        QString("SP: 0x%1 | Depth: %2 byte(s)")
            .arg(stack_info.sp, 4, 16, QLatin1Char('0')).toUpper()
            .arg(stack_info.depth)
    );

    if (stack_info.overflow_or_corrupt) {
        status_label_->setText("CORRUPT / OVERFLOW: Stack pointer out of bounds [0xF000, 0xFFFE]");
        status_label_->setStyleSheet("QLabel { color: #dc3545; font-weight: bold; }");
        table_->setRowCount(0);
        return;
    }

    if (stack_info.is_empty) {
        status_label_->setText("Empty stack (SP = 0xFFFF sentinel per ADR-001; no items pushed)");
        status_label_->setStyleSheet("QLabel { color: #8a93a0; font-style: italic; }");
        table_->setRowCount(0);
        return;
    }

    status_label_->setText(QString("Occupied (%1 byte(s))").arg(stack_info.depth));
    status_label_->setStyleSheet("QLabel { color: #28a745; font-weight: bold; }");

    table_->setRowCount(static_cast<int>(stack_info.entries.size()));

    for (int i = 0; i < static_cast<int>(stack_info.entries.size()); ++i) {
        const auto& entry = stack_info.entries[static_cast<std::size_t>(i)];

        QString offset_str = (i == 0) ? "TOS (SP)" : QString("TOS-%1").arg(i);
        auto* item_offset = new QTableWidgetItem(offset_str);
        item_offset->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 0, item_offset);

        auto* item_addr = new QTableWidgetItem(
            QString("0x%1").arg(entry.address, 4, 16, QLatin1Char('0')).toUpper()
        );
        item_addr->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 1, item_addr);

        auto* item_hex = new QTableWidgetItem(
            QString("0x%1").arg(entry.value, 2, 16, QLatin1Char('0')).toUpper()
        );
        item_hex->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 2, item_hex);

        auto* item_dec = new QTableWidgetItem(QString::number(entry.value));
        item_dec->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 3, item_dec);

        char ch = std::isprint(static_cast<unsigned char>(entry.value))
                      ? static_cast<char>(entry.value)
                      : '.';
        auto* item_ascii = new QTableWidgetItem(QString(ch));
        item_ascii->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, 4, item_ascii);
    }
}

} // namespace bitforge::gui
