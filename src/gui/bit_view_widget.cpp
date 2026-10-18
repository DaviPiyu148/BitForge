#include "bitforge/gui/bit_view_widget.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFontDatabase>

namespace bitforge::gui {

namespace {

QString formatHexWord(word_t val) {
    return QString("0x%1").arg(QString::number(val, 16).toUpper().rightJustified(4, '0'));
}

QString formatHexByte(byte_t val) {
    return QString("0x%1").arg(QString::number(val, 16).toUpper().rightJustified(2, '0'));
}

void applyBitStyle(QLabel* dot, bool is_set, bool is_16bit) {
    if (is_set) {
        const QString bg = is_16bit ? "#007bff" : "#28a745";
        const QString border = is_16bit ? "#0056b3" : "#1e7e34";
        dot->setStyleSheet(QString(
            "QLabel { background-color: %1; color: white; font-weight: bold; font-size: 9px; "
            "border-radius: 3px; border: 1px solid %2; min-width: 13px; max-width: 13px; "
            "min-height: 14px; max-height: 14px; text-align: center; }"
        ).arg(bg, border));
        dot->setText("1");
    } else {
        dot->setStyleSheet(
            "QLabel { background-color: #343a40; color: #6c757d; font-weight: normal; font-size: 9px; "
            "border-radius: 3px; border: 1px solid #495057; min-width: 13px; max-width: 13px; "
            "min-height: 14px; max-height: 14px; text-align: center; }"
        );
        dot->setText("0");
    }
}

} // namespace

BitViewWidget::BitViewWidget(QWidget* parent)
    : QWidget(parent) {
    auto* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(0, 0, 0, 0);

    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);

    auto* content_widget = new QWidget(scroll_area);
    auto* content_layout = new QVBoxLayout(content_widget);
    content_layout->setContentsMargins(6, 6, 6, 6);
    content_layout->setSpacing(8);

    // 1. Group: 16-Bit Registers (PC, SP, IX)
    auto* group16 = new QGroupBox("16-Bit Registers (Bits 15..0)", content_widget);
    auto* layout16 = new QVBoxLayout(group16);
    layout16->setContentsMargins(6, 8, 6, 8);
    layout16->setSpacing(6);

    pc_row_ = createBitRow16("PC", group16);
    sp_row_ = createBitRow16("SP", group16);
    ix_row_ = createBitRow16("IX", group16);

    layout16->addWidget(pc_row_.container);
    layout16->addWidget(sp_row_.container);
    layout16->addWidget(ix_row_.container);
    content_layout->addWidget(group16);

    // 2. Group: 8-Bit Registers (R0..R3)
    auto* group8 = new QGroupBox("8-Bit General Purpose Registers (Bits 7..0)", content_widget);
    auto* layout8 = new QVBoxLayout(group8);
    layout8->setContentsMargins(6, 8, 6, 8);
    layout8->setSpacing(6);

    for (int i = 0; i < 4; ++i) {
        r_rows_[static_cast<std::size_t>(i)] = createBitRow8(QString("R%1").arg(i), group8);
        layout8->addWidget(r_rows_[static_cast<std::size_t>(i)].container);
    }
    content_layout->addWidget(group8);

    // 3. Group: ALU Flags
    auto* flags_group = new QGroupBox("ALU Status Flags", content_widget);
    auto* flags_layout = new QGridLayout(flags_group);
    flags_layout->setContentsMargins(8, 8, 8, 8);
    flags_layout->setSpacing(8);

    auto addFlagItem = [&](int col, const QString& name, const QString& tip,
                           QLabel*& dot, QLabel*& val_label) {
        auto* label = new QLabel(name, flags_group);
        label->setFont(QFont("Segoe UI", 9, QFont::Bold));
        label->setToolTip(tip);

        dot = new QLabel(flags_group);
        dot->setAlignment(Qt::AlignCenter);
        dot->setToolTip(tip);
        applyBitStyle(dot, false, false);

        val_label = new QLabel("0", flags_group);
        val_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        val_label->setToolTip(tip);

        flags_layout->addWidget(label, 0, col);
        flags_layout->addWidget(dot, 1, col);
        flags_layout->addWidget(val_label, 2, col);
    };

    addFlagItem(0, "Z", "Zero Flag: 1 when ALU result is 0", flag_z_dot_, flag_z_val_);
    addFlagItem(1, "C", "Carry / No-Borrow Flag: 1 on unsigned carry-out or (a >= b)", flag_c_dot_, flag_c_val_);
    addFlagItem(2, "N", "Negative Flag: 1 when bit 7 (MSB) of result is 1", flag_n_dot_, flag_n_val_);
    addFlagItem(3, "V", "Signed Overflow Flag: 1 on two's-complement overflow", flag_v_dot_, flag_v_val_);

    content_layout->addWidget(flags_group);
    content_layout->addStretch();

    scroll_area->setWidget(content_widget);
    outer_layout->addWidget(scroll_area);
}

BitViewWidget::BitRow16 BitViewWidget::createBitRow16(const QString& name, QWidget* parent) {
    BitRow16 row;
    row.container = new QWidget(parent);
    auto* layout = new QVBoxLayout(row.container);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(2);

    auto* top_line = new QHBoxLayout();
    row.title_label = new QLabel(QString("%1: 0x0000 (0)").arg(name), row.container);
    row.title_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    top_line->addWidget(row.title_label);
    top_line->addStretch();
    layout->addLayout(top_line);

    auto* bits_line = new QHBoxLayout();
    bits_line->setSpacing(2);

    for (int i = 15; i >= 0; --i) {
        auto* dot = new QLabel(row.container);
        dot->setAlignment(Qt::AlignCenter);
        dot->setToolTip(QString("%1 Bit %2").arg(name).arg(i));
        applyBitStyle(dot, false, true);
        row.bit_dots[static_cast<std::size_t>(i)] = dot;
        bits_line->addWidget(dot);

        // Add small visual gap between high byte and low byte
        if (i == 8) {
            bits_line->addSpacing(6);
        }
    }
    bits_line->addStretch();
    layout->addLayout(bits_line);

    return row;
}

BitViewWidget::BitRow8 BitViewWidget::createBitRow8(const QString& name, QWidget* parent) {
    BitRow8 row;
    row.container = new QWidget(parent);
    auto* layout = new QVBoxLayout(row.container);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(2);

    auto* top_line = new QHBoxLayout();
    row.title_label = new QLabel(QString("%1: 0x00 (0)").arg(name), row.container);
    row.title_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    top_line->addWidget(row.title_label);
    top_line->addStretch();
    layout->addLayout(top_line);

    auto* bits_line = new QHBoxLayout();
    bits_line->setSpacing(3);

    for (int i = 7; i >= 0; --i) {
        auto* dot = new QLabel(row.container);
        dot->setAlignment(Qt::AlignCenter);
        dot->setToolTip(QString("%1 Bit %2").arg(name).arg(i));
        applyBitStyle(dot, false, false);
        row.bit_dots[static_cast<std::size_t>(i)] = dot;
        bits_line->addWidget(dot);
    }
    bits_line->addStretch();
    layout->addLayout(bits_line);

    return row;
}

void BitViewWidget::updateBitRow16(BitRow16& row, word_t val, bool changed) {
    for (int i = 0; i < 16; ++i) {
        const bool bit_set = (val & (1 << i)) != 0;
        applyBitStyle(row.bit_dots[static_cast<std::size_t>(i)], bit_set, true);
    }
    if (changed) {
        row.container->setStyleSheet(
            "QWidget { background-color: #3b3218; border-radius: 4px; border: 1px solid #d4a017; }"
        );
    } else {
        row.container->setStyleSheet("QWidget { background-color: transparent; border: 1px solid transparent; }");
    }
}

void BitViewWidget::updateBitRow8(BitRow8& row, byte_t val, bool changed) {
    for (int i = 0; i < 8; ++i) {
        const bool bit_set = (val & (1 << i)) != 0;
        applyBitStyle(row.bit_dots[static_cast<std::size_t>(i)], bit_set, false);
    }
    if (changed) {
        row.container->setStyleSheet(
            "QWidget { background-color: #3b3218; border-radius: 4px; border: 1px solid #d4a017; }"
        );
    } else {
        row.container->setStyleSheet("QWidget { background-color: transparent; border: 1px solid transparent; }");
    }
}

void BitViewWidget::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &BitViewWidget::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &BitViewWidget::refresh);
    }
    refresh();
}

void BitViewWidget::refresh() {
    if (controller_ == nullptr) return;

    const auto& regs = controller_->session().registers();
    const auto& flags = controller_->session().flags();

    const bool pc_changed = has_prev_state_ && (regs.pc != prev_registers_.pc);
    const bool sp_changed = has_prev_state_ && (regs.sp != prev_registers_.sp);
    const bool ix_changed = has_prev_state_ && (regs.ix != prev_registers_.ix);

    pc_row_.title_label->setText(QString("PC: %1 (%2)")
                                     .arg(formatHexWord(regs.pc))
                                     .arg(regs.pc));
    updateBitRow16(pc_row_, regs.pc, pc_changed);

    sp_row_.title_label->setText(QString("SP: %1 (%2)")
                                     .arg(formatHexWord(regs.sp))
                                     .arg(regs.sp));
    updateBitRow16(sp_row_, regs.sp, sp_changed);

    ix_row_.title_label->setText(QString("IX: %1 (%2)")
                                     .arg(formatHexWord(regs.ix))
                                     .arg(regs.ix));
    updateBitRow16(ix_row_, regs.ix, ix_changed);

    for (int i = 0; i < 4; ++i) {
        const byte_t r_val = regs.r[i];
        const bool r_changed = has_prev_state_ && (r_val != prev_registers_.r[i]);
        r_rows_[static_cast<std::size_t>(i)].title_label->setText(
            QString("R%1: %2 (%3)")
                .arg(i)
                .arg(formatHexByte(r_val))
                .arg(r_val)
        );
        updateBitRow8(r_rows_[static_cast<std::size_t>(i)], r_val, r_changed);
    }

    applyBitStyle(flag_z_dot_, flags.z, false);
    applyBitStyle(flag_c_dot_, flags.c, false);
    applyBitStyle(flag_n_dot_, flags.n, false);
    applyBitStyle(flag_v_dot_, flags.v, false);

    flag_z_val_->setText(flags.z ? "1" : "0");
    flag_c_val_->setText(flags.c ? "1" : "0");
    flag_n_val_->setText(flags.n ? "1" : "0");
    flag_v_val_->setText(flags.v ? "1" : "0");

    prev_registers_ = regs;
    prev_flags_ = flags;
    has_prev_state_ = true;
}

} // namespace bitforge::gui
