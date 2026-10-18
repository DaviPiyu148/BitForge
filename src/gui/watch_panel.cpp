#include "bitforge/gui/watch_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QRegularExpression>
#include <algorithm>

namespace bitforge::gui {

WatchPanel::WatchPanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = theme::monoFont(10);

    // Top action bar: Expression input, Add, Remove, Clear
    auto* top_bar = new QHBoxLayout();
    expr_input_ = new QLineEdit(this);
    expr_input_->setFont(mono_font);
    expr_input_->setPlaceholderText("Enter watch expression (e.g. R0, IX, Z, 0x8000, [0x0200])...");
    top_bar->addWidget(expr_input_, 1);

    add_btn_ = new QPushButton("Add Watch", this);
    add_btn_->setStyleSheet("QPushButton { font-weight: bold; }");
    top_bar->addWidget(add_btn_);

    remove_btn_ = new QPushButton("Remove", this);
    top_bar->addWidget(remove_btn_);

    clear_btn_ = new QPushButton("Clear All", this);
    top_bar->addWidget(clear_btn_);
    main_layout->addLayout(top_bar);

    // Table: 5 columns
    table_ = new QTableWidget(0, 5, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);
    table_->setContextMenuPolicy(Qt::CustomContextMenu);

    QStringList headers;
    headers << "Target" << "Type" << "Hex Value" << "Decimal" << "Binary";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    main_layout->addWidget(table_);

    // Empty state label
    empty_label_ = new QLabel("No watch expressions active. Add registers, flags, or memory addresses above.", this);
    empty_label_->setStyleSheet("QLabel { color: #8a93a0; font-style: italic; padding: 4px; }");
    main_layout->addWidget(empty_label_);

    // Signals & connections
    connect(add_btn_, &QPushButton::clicked, this, &WatchPanel::onAddClicked);
    connect(expr_input_, &QLineEdit::returnPressed, this, &WatchPanel::onAddClicked);
    connect(remove_btn_, &QPushButton::clicked, this, &WatchPanel::onRemoveClicked);
    connect(clear_btn_, &QPushButton::clicked, this, &WatchPanel::onClearClicked);
    connect(table_, &QWidget::customContextMenuRequested, this, &WatchPanel::onCustomContextMenu);

    updateEmptyState();
}

void WatchPanel::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &WatchPanel::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &WatchPanel::refresh);
    }
    refresh();
}

std::optional<WatchItem> WatchPanel::parseExpression(const QString& rawExpr) const {
    QString s = rawExpr.trimmed();
    if (s.isEmpty()) return std::nullopt;

    WatchItem item;
    item.expression = s;

    QString upper = s.toUpper();

    // 1. General-purpose 8-bit registers (R0-R3)
    if (upper == "R0" || upper == "R1" || upper == "R2" || upper == "R3") {
        item.type = WatchType::Register8;
        item.reg_index = upper.mid(1).toInt();
        item.displayName = upper;
        return item;
    }

    // 2. 16-bit registers (IX, PC, SP)
    if (upper == "IX" || upper == "PC" || upper == "SP") {
        item.type = WatchType::Register16;
        item.displayName = upper;
        return item;
    }

    // 3. Flags (Z, C, N, V or FLAG Z)
    if (upper.startsWith("FLAG ") || upper.startsWith("FLAG_")) {
        upper = upper.mid(5).trimmed();
    }
    if (upper == "Z" || upper == "C" || upper == "N" || upper == "V") {
        item.type = WatchType::Flag;
        item.flag_name = upper;
        item.displayName = QString("Flag %1").arg(upper);
        return item;
    }

    // 4. Memory Byte address: [0x8000], 0x8000, [$8000], 32768, etc.
    QString addrStr = s;
    if (addrStr.startsWith('[') && addrStr.endsWith(']')) {
        addrStr = addrStr.mid(1, addrStr.length() - 2).trimmed();
    }
    if (addrStr.startsWith('$')) {
        addrStr = "0x" + addrStr.mid(1);
    }

    bool ok = false;
    uint val = addrStr.toUInt(&ok, 0); // Handles hex prefix 0x and decimal
    if (ok && val <= 0xFFFF) {
        item.type = WatchType::MemoryByte;
        item.address = static_cast<word_t>(val);
        item.displayName = QString("[0x%1]").arg(item.address, 4, 16, QLatin1Char('0')).toUpper();
        return item;
    }

    return std::nullopt;
}

bool WatchPanel::addWatch(const QString& expression) {
    auto parsed = parseExpression(expression);
    if (!parsed.has_value()) {
        return false;
    }

    // Check for duplicate
    for (const auto& w : watches_) {
        if (w.displayName.compare(parsed->displayName, Qt::CaseInsensitive) == 0) {
            return true; // Already exists
        }
    }

    watches_.push_back(*parsed);
    last_step_values_.push_back(0xFFFFFFFF);
    current_step_values_.push_back(0xFFFFFFFF);
    updateEmptyState();
    refresh();
    return true;
}

void WatchPanel::addRegisterWatch(const QString& regName) {
    addWatch(regName);
}

void WatchPanel::addMemoryWatch(word_t address) {
    addWatch(QString("0x%1").arg(address, 4, 16, QLatin1Char('0')).toUpper());
}

void WatchPanel::addFlagWatch(const QString& flagName) {
    addWatch(flagName);
}

void WatchPanel::removeWatch(int index) {
    if (index >= 0 && index < static_cast<int>(watches_.size())) {
        watches_.erase(watches_.begin() + index);
        if (index < static_cast<int>(last_step_values_.size())) {
            last_step_values_.erase(last_step_values_.begin() + index);
        }
        if (index < static_cast<int>(current_step_values_.size())) {
            current_step_values_.erase(current_step_values_.begin() + index);
        }
        updateEmptyState();
        refresh();
    }
}

void WatchPanel::clearWatches() {
    watches_.clear();
    last_step_values_.clear();
    current_step_values_.clear();
    table_->setRowCount(0);
    updateEmptyState();
}

QStringList WatchPanel::watchExpressions() const {
    QStringList list;
    for (const auto& w : watches_) {
        list << w.expression;
    }
    return list;
}

void WatchPanel::loadWatchExpressions(const QStringList& expressions) {
    clearWatches();
    for (const auto& expr : expressions) {
        addWatch(expr);
    }
}

void WatchPanel::onAddClicked() {
    const QString text = expr_input_->text().trimmed();
    if (!text.isEmpty()) {
        if (addWatch(text)) {
            expr_input_->clear();
        }
    }
}

void WatchPanel::onRemoveClicked() {
    int row = table_->currentRow();
    if (row >= 0) {
        removeWatch(row);
    }
}

void WatchPanel::onClearClicked() {
    clearWatches();
}

void WatchPanel::onCustomContextMenu(const QPoint& pos) {
    int row = table_->rowAt(pos.y());
    QMenu menu(this);
    if (row >= 0) {
        auto* removeAct = menu.addAction(QString("Remove Watch: %1").arg(watches_[static_cast<std::size_t>(row)].displayName));
        connect(removeAct, &QAction::triggered, this, [this, row]() {
            removeWatch(row);
        });
        menu.addSeparator();
    }
    auto* clearAct = menu.addAction("Clear All Watches");
    connect(clearAct, &QAction::triggered, this, &WatchPanel::clearWatches);
    menu.exec(table_->viewport()->mapToGlobal(pos));
}

void WatchPanel::updateEmptyState() {
    const bool empty = watches_.empty();
    if (empty_label_ != nullptr) {
        empty_label_->setVisible(empty);
    }
    if (remove_btn_ != nullptr) {
        remove_btn_->setEnabled(!empty);
    }
    if (clear_btn_ != nullptr) {
        clear_btn_->setEnabled(!empty);
    }
}

void WatchPanel::refresh() {
    updateEmptyState();
    if (controller_ == nullptr) return;

    table_->setRowCount(static_cast<int>(watches_.size()));
    while (last_step_values_.size() < watches_.size()) {
        last_step_values_.push_back(0xFFFFFFFF);
    }
    while (current_step_values_.size() < watches_.size()) {
        current_step_values_.push_back(0xFFFFFFFF);
    }

    const auto& regs = controller_->session().registers();
    const auto& flags = controller_->session().flags();
    const auto& mem = controller_->memory();
    const std::uint64_t step_count = controller_->session().total_steps_executed();

    if (step_count != last_step_count_) {
        if (step_count < last_step_count_ || step_count == 0) {
            last_step_values_.assign(watches_.size(), 0xFFFFFFFF);
        } else {
            last_step_values_ = current_step_values_;
        }
        last_step_count_ = step_count;
    }

    for (std::size_t i = 0; i < watches_.size(); ++i) {
        const auto& item = watches_[i];
        const int row = static_cast<int>(i);

        std::uint32_t current_val = 0;
        QString type_str;
        QString hex_str;
        QString dec_str;
        QString bin_str;

        switch (item.type) {
            case WatchType::Register8: {
                type_str = "Reg (8-bit)";
                byte_t b = (item.reg_index >= 0 && item.reg_index < 4) ? regs.r[item.reg_index] : 0;
                current_val = b;
                hex_str = "0x" + QString("%1").arg(b, 2, 16, QLatin1Char('0'));
                dec_str = QString::number(b);
                bin_str = QString("0b%1").arg(b, 8, 2, QLatin1Char('0'));
                break;
            }
            case WatchType::Register16: {
                type_str = "Reg (16-bit)";
                word_t w = 0;
                if (item.displayName == "IX") w = regs.ix;
                else if (item.displayName == "PC") w = regs.pc;
                else if (item.displayName == "SP") w = regs.sp;
                current_val = w;
                hex_str = "0x" + QString("%1").arg(w, 4, 16, QLatin1Char('0'));
                dec_str = QString::number(w);
                bin_str = QString("0b%1").arg(w, 16, 2, QLatin1Char('0'));
                break;
            }
            case WatchType::Flag: {
                type_str = "Flag";
                bool fval = false;
                if (item.flag_name == "Z") fval = flags.z;
                else if (item.flag_name == "C") fval = flags.c;
                else if (item.flag_name == "N") fval = flags.n;
                else if (item.flag_name == "V") fval = flags.v;
                current_val = fval ? 1 : 0;
                hex_str = fval ? "1 (Set)" : "0 (Clear)";
                dec_str = fval ? "1" : "0";
                bin_str = fval ? "1" : "0";
                break;
            }
            case WatchType::MemoryByte: {
                type_str = "Memory Byte";
                byte_t b = mem.read_byte(item.address);
                current_val = b;
                hex_str = "0x" + QString("%1").arg(b, 2, 16, QLatin1Char('0'));
                dec_str = QString::number(b);
                bin_str = QString("0b%1").arg(b, 8, 2, QLatin1Char('0'));
                break;
            }
        }

        const bool has_mutated = (last_step_values_[i] != 0xFFFFFFFF && last_step_values_[i] != current_val);
        current_step_values_[i] = current_val;

        // Column 0: Target
        auto* target_item = new QTableWidgetItem(item.displayName);
        target_item->setTextAlignment(Qt::AlignCenter);
        target_item->setForeground(theme::kAccentTeal);
        table_->setItem(row, 0, target_item);

        // Column 1: Type
        auto* type_item = new QTableWidgetItem(type_str);
        type_item->setTextAlignment(Qt::AlignCenter);
        type_item->setForeground(theme::kTextSecondary);
        table_->setItem(row, 1, type_item);

        // Column 2: Hex Value
        auto* hex_item = new QTableWidgetItem(hex_str);
        hex_item->setTextAlignment(Qt::AlignCenter);

        // Column 3: Decimal
        auto* dec_item = new QTableWidgetItem(dec_str);
        dec_item->setTextAlignment(Qt::AlignCenter);

        // Column 4: Binary
        auto* bin_item = new QTableWidgetItem(bin_str);
        bin_item->setTextAlignment(Qt::AlignCenter);

        if (has_mutated) {
            QFont bold_font = table_->font();
            bold_font.setBold(true);

            for (auto* itm : {hex_item, dec_item, bin_item}) {
                itm->setBackground(theme::kChangedBg);
                itm->setForeground(theme::kChangedText);
                itm->setFont(bold_font);
            }
        } else {
            for (auto* itm : {hex_item, dec_item, bin_item}) {
                itm->setForeground(theme::kTextPrimary);
            }
        }

        table_->setItem(row, 2, hex_item);
        table_->setItem(row, 3, dec_item);
        table_->setItem(row, 4, bin_item);
    }
}

} // namespace bitforge::gui
