#include "bitforge/gui/breakpoints_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"

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

    table_ = new QTableWidget(0, 3, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Address" << "Source Location" << "Instruction";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);

    main_layout->addWidget(table_);

    auto* button_bar = new QHBoxLayout();
    remove_btn_ = new QPushButton("Remove Selected", this);
    clear_all_btn_ = new QPushButton("Clear All", this);

    button_bar->addWidget(remove_btn_);
    button_bar->addWidget(clear_all_btn_);
    button_bar->addStretch();

    main_layout->addLayout(button_bar);

    connect(remove_btn_, &QPushButton::clicked, this, &BreakpointsPanel::onRemoveSelected);
    connect(clear_all_btn_, &QPushButton::clicked, this, &BreakpointsPanel::onClearAll);
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

        auto* item_addr = new QTableWidgetItem(
            QString("0x%1").arg(addr, 4, 16, QLatin1Char('0')).toUpper()
        );
        item_addr->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 0, item_addr);

        QString loc_str = "-";
        QString mnem_str = "-";

        if (img != nullptr) {
            const auto* mapping = img->source_map().find_instruction_by_address(addr);
            if (mapping != nullptr) {
                loc_str = QString("Line %1, Col %2")
                              .arg(mapping->location.line)
                              .arg(mapping->location.column);
                mnem_str = QString::fromStdString(mapping->mnemonic);
            }
        }

        auto* item_loc = new QTableWidgetItem(loc_str);
        item_loc->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 1, item_loc);

        auto* item_mnem = new QTableWidgetItem(mnem_str);
        table_->setItem(row, 2, item_mnem);
    }
}

void BreakpointsPanel::onRemoveSelected() {
    if (controller_ == nullptr) return;
    auto selected_items = table_->selectedItems();
    if (selected_items.empty()) return;

    int row = selected_items.first()->row();
    auto* addr_item = table_->item(row, 0);
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
