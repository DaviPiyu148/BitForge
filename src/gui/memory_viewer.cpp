#include "bitforge/gui/memory_viewer.hpp"
#include "bitforge/gui/execution_controller.hpp"

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

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // Top control bar
    auto* top_bar = new QHBoxLayout();
    top_bar->addWidget(new QLabel("Address:", this));

    address_input_ = new QLineEdit("0x0200", this);
    address_input_->setMaximumWidth(90);
    address_input_->setFont(mono_font);
    top_bar->addWidget(address_input_);

    go_button_ = new QPushButton("Go", this);
    top_bar->addWidget(go_button_);

    auto* btn_prev = new QPushButton("< Prev 256B", this);
    auto* btn_next = new QPushButton("Next 256B >", this);
    top_bar->addWidget(btn_prev);
    top_bar->addWidget(btn_next);

    auto* btn_pc = new QPushButton("Goto PC", this);
    auto* btn_sp = new QPushButton("Goto SP", this);
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

    for (int r = 0; r < kRowsPerPage; ++r) {
        const word_t row_addr = static_cast<word_t>((base_address_ + r * kBytesPerRow) & 0xFFFF);
        auto* addr_item = new QTableWidgetItem(
            QString("0x%1").arg(row_addr, 4, 16, QLatin1Char('0')).toUpper()
        );
        addr_item->setTextAlignment(Qt::AlignCenter);
        table_->setItem(r, 0, addr_item);

        QString ascii_str;
        for (int c = 0; c < kBytesPerRow; ++c) {
            const byte_t b = range.data[static_cast<std::size_t>(r * kBytesPerRow + c)];
            auto* byte_item = new QTableWidgetItem(
                QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper()
            );
            byte_item->setTextAlignment(Qt::AlignCenter);
            table_->setItem(r, c + 1, byte_item);

            if (std::isprint(static_cast<unsigned char>(b))) {
                ascii_str.append(static_cast<char>(b));
            } else {
                ascii_str.append('.');
            }
        }

        auto* ascii_item = new QTableWidgetItem(ascii_str);
        table_->setItem(r, 17, ascii_item);
    }
}

} // namespace bitforge::gui
