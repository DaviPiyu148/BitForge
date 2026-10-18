#include "bitforge/gui/breakpoints_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/instruction_inspector.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontDatabase>

namespace bitforge::gui {

BreakpointsPanel::BreakpointsPanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    table_ = new QTableWidget(0, 4, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Line" << "Address" << "Instruction" << "Status";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);

    main_layout->addWidget(table_);

    auto* button_bar = new QHBoxLayout();
    remove_btn_ = new QPushButton("Remove Selected", this);
    clear_all_btn_ = new QPushButton("Clear All", this);
    empty_label_ = new QLabel("No breakpoints configured. Click editor gutter or press F9 to toggle.", this);
    empty_label_->setStyleSheet(theme::emptyStateStyle(true));

    button_bar->addWidget(remove_btn_);
    button_bar->addWidget(clear_all_btn_);
    button_bar->addSpacing(12);
    button_bar->addWidget(empty_label_);
    button_bar->addStretch();

    main_layout->addLayout(button_bar);

    connect(remove_btn_, &QPushButton::clicked, this, &BreakpointsPanel::onRemoveSelected);
    connect(clear_all_btn_, &QPushButton::clicked, this, &BreakpointsPanel::onClearAll);

    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int row, int /*col*/) {
        if (row < 0 || row >= table_->rowCount()) return;
        auto* line_item = table_->item(row, 0);
        if (line_item == nullptr) return;
        bool ok = false;
        std::size_t line = line_item->text().toULongLong(&ok);
        if (ok && line > 0) {
            emit breakpointDoubleClicked(line);
        }
    });
}

void BreakpointsPanel::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::breakpointsChanged, this, &BreakpointsPanel::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &BreakpointsPanel::refresh);
    }
    refresh();
}

void BreakpointsPanel::refresh() {
    if (controller_ == nullptr) return;

    table_->setRowCount(0);

    const auto bps = controller_->session().breakpoints();
    const auto* img = controller_->programImage();

    for (word_t addr : bps) {
        const int row = table_->rowCount();
        table_->insertRow(row);

        QString line_str = "-";
        QString mnem_str = "-";

        if (img != nullptr) {
            const auto* mapping = img->source_map().find_instruction_by_address(addr);
            if (mapping != nullptr) {
                line_str = QString::number(mapping->location.line);
                mnem_str = QString::fromStdString(mapping->mnemonic);
            }
        }

        auto decoded = InstructionInspector::decodeInstruction(addr, *controller_);
        if (!decoded.full_disassembly.isEmpty()) {
            mnem_str = decoded.full_disassembly;
        }

        auto* item_line = new QTableWidgetItem(line_str);
        item_line->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 0, item_line);

        auto* item_addr = new QTableWidgetItem(
            QString("0x%1").arg(addr, 4, 16, QLatin1Char('0')).toUpper()
        );
        item_addr->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 1, item_addr);

        auto* item_mnem = new QTableWidgetItem(mnem_str);
        table_->setItem(row, 2, item_mnem);

        auto* item_status = new QTableWidgetItem("Active");
        item_status->setTextAlignment(Qt::AlignCenter);
        item_status->setForeground(QColor(40, 167, 69));
        table_->setItem(row, 3, item_status);
    }

    if (empty_label_ != nullptr) {
        empty_label_->setVisible(bps.empty());
    }
    remove_btn_->setEnabled(!bps.empty());
    clear_all_btn_->setEnabled(!bps.empty());
}

void BreakpointsPanel::onRemoveSelected() {
    if (controller_ == nullptr) return;
    auto selected_items = table_->selectedItems();
    if (selected_items.empty()) return;

    int row = selected_items.first()->row();
    auto* addr_item = table_->item(row, 1);
    if (addr_item == nullptr) return;

    bool ok = false;
    uint addr_val = addr_item->text().toUInt(&ok, 0);
    if (ok) {
        controller_->removeBreakpoint(static_cast<word_t>(addr_val));
    }
}

void BreakpointsPanel::onClearAll() {
    if (controller_ == nullptr) return;
    controller_->clearAllBreakpoints();
}

} // namespace bitforge::gui
