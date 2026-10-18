#include "bitforge/gui/memory_viewer.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFontDatabase>
#include <cctype>

namespace bitforge::gui {

MemoryViewer::MemoryViewer(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = theme::monoFont(10);

    // Top control bar
    auto* top_bar = new QHBoxLayout();
    auto* addr_lbl = new QLabel("Address:", this);
    addr_lbl->setStyleSheet("QLabel { color: #abb2bf; font-weight: 500; }");
    top_bar->addWidget(addr_lbl);

    address_input_ = new QLineEdit("0x0200", this);
    address_input_->setMaximumWidth(90);
    address_input_->setFont(mono_font);
    top_bar->addWidget(address_input_);

    go_button_ = new QPushButton("Go", this);
    top_bar->addWidget(go_button_);

    auto* btn_prev = new QPushButton("◀ Prev 256B", this);
    auto* btn_next = new QPushButton("Next 256B ▶", this);
    top_bar->addWidget(btn_prev);
    top_bar->addWidget(btn_next);

    auto* btn_pc = new QPushButton("Jump to PC", this);
    auto* btn_sp = new QPushButton("Jump to SP", this);
    btn_pc->setStyleSheet("QPushButton { color: #61afef; font-weight: bold; }");
    btn_sp->setStyleSheet("QPushButton { color: #4ec9b0; font-weight: bold; }");
    top_bar->addWidget(btn_pc);
    top_bar->addWidget(btn_sp);

    top_bar->addStretch();
    main_layout->addLayout(top_bar);

    // Memory Table
    table_ = new QTableWidget(kRowsPerPage, 18, this);
    table_->setFont(mono_font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Address";
    for (int i = 0; i < 16; ++i) {
        headers << QString("%1").arg(i, 1, 16).toUpper();
    }
    headers << "ASCII";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);

    main_layout->addWidget(table_);

    connect(go_button_, &QPushButton::clicked, this, &MemoryViewer::onGoClicked);
    connect(address_input_, &QLineEdit::returnPressed, this, &MemoryViewer::onGoClicked);
    connect(btn_prev, &QPushButton::clicked, this, &MemoryViewer::onPrevPage);
    connect(btn_next, &QPushButton::clicked, this, &MemoryViewer::onNextPage);

    connect(btn_pc, &QPushButton::clicked, this, [this]() {
        if (controller_ != nullptr) {
            scrollToAddress(controller_->session().pc());
        }
    });

    connect(btn_sp, &QPushButton::clicked, this, [this]() {
        if (controller_ != nullptr) {
            scrollToAddress(controller_->session().sp());
        }
    });
}

void MemoryViewer::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &MemoryViewer::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &MemoryViewer::refresh);
    }
    refresh();
}

void MemoryViewer::scrollToAddress(word_t address) {
    base_address_ = static_cast<word_t>(address & 0xFFF0); // Align to 16 bytes
    address_input_->setText(QString("0x%1").arg(base_address_, 4, 16, QLatin1Char('0')).toUpper());
    updateTable();
}

void MemoryViewer::refresh() {
    updateTable();
}

void MemoryViewer::onGoClicked() {
    QString text = address_input_->text().trimmed();
    bool ok = false;
    uint val = text.toUInt(&ok, 0); // Handles 0x prefix automatically
    if (ok) {
        scrollToAddress(static_cast<word_t>(val & 0xFFFF));
    }
}

void MemoryViewer::onPrevPage() {
    base_address_ = static_cast<word_t>((base_address_ - (kRowsPerPage * kBytesPerRow)) & 0xFFFF);
    address_input_->setText(QString("0x%1").arg(base_address_, 4, 16, QLatin1Char('0')).toUpper());
    updateTable();
}

void MemoryViewer::onNextPage() {
    base_address_ = static_cast<word_t>((base_address_ + (kRowsPerPage * kBytesPerRow)) & 0xFFFF);
    address_input_->setText(QString("0x%1").arg(base_address_, 4, 16, QLatin1Char('0')).toUpper());
    updateTable();
}

void MemoryViewer::updateTable() {
    if (controller_ == nullptr) return;

    const std::size_t total_bytes = static_cast<std::size_t>(kRowsPerPage * kBytesPerRow);
    auto range = controller_->session().read_memory_range(base_address_, total_bytes);
    if (!range.success || range.data.size() < total_bytes) {
        return;
    }

    const bool has_diff = (prev_base_address_ == base_address_ && prev_data_.size() == range.data.size());

    for (int r = 0; r < kRowsPerPage; ++r) {
        const word_t row_addr = static_cast<word_t>((base_address_ + r * kBytesPerRow) & 0xFFFF);
        auto* addr_item = new QTableWidgetItem(
            QString("0x%1").arg(row_addr, 4, 16, QLatin1Char('0')).toUpper()
        );
        addr_item->setTextAlignment(Qt::AlignCenter);
        addr_item->setForeground(QColor(0x61, 0xaf, 0xef)); // Soft blue address
        table_->setItem(r, 0, addr_item);

        QString ascii_str;
        for (int c = 0; c < kBytesPerRow; ++c) {
            const std::size_t idx = static_cast<std::size_t>(r * kBytesPerRow + c);
            const byte_t b = range.data[idx];
            auto* byte_item = new QTableWidgetItem(
                QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper()
            );
            byte_item->setTextAlignment(Qt::AlignCenter);

            // Highlight recently changed memory bytes with warm amber
            if (has_diff && b != prev_data_[idx]) {
                byte_item->setBackground(theme::kChangedBg);
                byte_item->setForeground(theme::kChangedText);
                QFont f = table_->font();
                f.setBold(true);
                byte_item->setFont(f);
            } else if (b == 0x00) {
                byte_item->setForeground(QColor(0x5c, 0x63, 0x70)); // Dim zero bytes
            } else {
                byte_item->setForeground(QColor(0xdc, 0xdc, 0xdc));
            }

            table_->setItem(r, c + 1, byte_item);

            if (std::isprint(static_cast<unsigned char>(b))) {
                ascii_str.append(static_cast<char>(b));
            } else {
                ascii_str.append('.');
            }
        }

        auto* ascii_item = new QTableWidgetItem(ascii_str);
        ascii_item->setForeground(QColor(0x98, 0xc3, 0x79)); // Green ASCII representation
        table_->setItem(r, 17, ascii_item);
    }

    prev_data_ = range.data;
    prev_base_address_ = base_address_;
}

} // namespace bitforge::gui
