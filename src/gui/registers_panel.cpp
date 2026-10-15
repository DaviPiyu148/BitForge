#include "bitforge/gui/registers_panel.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/bit_view_widget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QFontDatabase>

namespace bitforge::gui {

namespace {

QString formatHexWord(word_t val) {
    return QString("0x%1").arg(val, 4, 16, QLatin1Char('0')).toUpper();
}

QString formatHexByte(byte_t val) {
    return QString("0x%1").arg(val, 2, 16, QLatin1Char('0')).toUpper();
}

void applyFlagStyle(QLabel* label, bool active) {
    if (active) {
        label->setStyleSheet(
            "QLabel { background-color: #28a745; color: white; font-weight: bold; "
            "border-radius: 3px; padding: 2px 6px; }"
        );
    } else {
        label->setStyleSheet(
            "QLabel { background-color: #e9ecef; color: #adb5bd; font-weight: normal; "
            "border-radius: 3px; padding: 2px 6px; }"
        );
    }
}

void applyValueStyle(QLabel* label, bool changed) {
    if (changed) {
        label->setStyleSheet(
            "QLabel { background-color: #fff3cd; color: #856404; font-weight: bold; "
            "border: 1px solid #ffeeba; border-radius: 2px; padding: 1px 4px; }"
        );
    } else {
        label->setStyleSheet(
            "QLabel { background-color: transparent; color: inherit; font-weight: normal; "
            "border: 1px solid transparent; padding: 1px 4px; }"
        );
    }
}

} // namespace

RegistersPanel::RegistersPanel(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(4);

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    auto* tabs = new QTabWidget(this);

    // Tab 1: Hex && Dec
    auto* hex_tab = new QWidget(tabs);
    auto* hex_layout = new QVBoxLayout(hex_tab);
    hex_layout->setContentsMargins(4, 4, 4, 4);
    hex_layout->setSpacing(6);

    // 1. Group: 16-Bit Registers
    auto* group16 = new QGroupBox("16-Bit Registers", hex_tab);
    auto* layout16 = new QGridLayout(group16);
    layout16->setContentsMargins(8, 8, 8, 8);

    auto addReg16 = [&](int row, int col, const QString& name, QLabel*& val_label) {
        auto* name_label = new QLabel(name + ":", group16);
        name_label->setFont(mono_font);
        val_label = new QLabel("0x0000", group16);
        val_label->setFont(mono_font);
        layout16->addWidget(name_label, row, col * 2);
        layout16->addWidget(val_label, row, col * 2 + 1);
    };

    addReg16(0, 0, "PC", pc_label_);
    addReg16(0, 1, "SP", sp_label_);
    addReg16(1, 0, "IX", ix_label_);

    hex_layout->addWidget(group16);

    // 2. Group: 8-Bit General Purpose Registers
    auto* group8 = new QGroupBox("8-Bit Registers", hex_tab);
    auto* layout8 = new QGridLayout(group8);
    layout8->setContentsMargins(8, 8, 8, 8);

    for (int i = 0; i < 4; ++i) {
        const int row = i / 2;
        const int col = i % 2;
        auto* name_label = new QLabel(QString("R%1:").arg(i), group8);
        name_label->setFont(mono_font);
        r_labels_[static_cast<std::size_t>(i)] = new QLabel("0x00", group8);
        r_labels_[static_cast<std::size_t>(i)]->setFont(mono_font);
        layout8->addWidget(name_label, row, col * 2);
        layout8->addWidget(r_labels_[static_cast<std::size_t>(i)], row, col * 2 + 1);
    }

    hex_layout->addWidget(group8);

    // 3. Group: Flags
    auto* flags_group = new QGroupBox("ALU Flags", hex_tab);
    auto* flags_layout = new QHBoxLayout(flags_group);
    flags_layout->setContentsMargins(8, 8, 8, 8);

    flag_z_label_ = new QLabel("Z", flags_group);
    flag_c_label_ = new QLabel("C", flags_group);
    flag_n_label_ = new QLabel("N", flags_group);
    flag_v_label_ = new QLabel("V", flags_group);

    applyFlagStyle(flag_z_label_, false);
    applyFlagStyle(flag_c_label_, false);
    applyFlagStyle(flag_n_label_, false);
    applyFlagStyle(flag_v_label_, false);

    flags_layout->addWidget(flag_z_label_);
    flags_layout->addWidget(flag_c_label_);
    flags_layout->addWidget(flag_n_label_);
    flags_layout->addWidget(flag_v_label_);
    flags_layout->addStretch();

    hex_layout->addWidget(flags_group);

    // 4. Group: Execution Status
    auto* status_group = new QGroupBox("Engine Status", hex_tab);
    auto* status_layout = new QGridLayout(status_group);
    status_layout->setContentsMargins(8, 8, 8, 8);

    status_layout->addWidget(new QLabel("State:", status_group), 0, 0);
    state_label_ = new QLabel("Ready", status_group);
    state_label_->setStyleSheet("QLabel { font-weight: bold; color: #28a745; }");
    status_layout->addWidget(state_label_, 0, 1);

    status_layout->addWidget(new QLabel("Stop Reason:", status_group), 1, 0);
    stop_reason_label_ = new QLabel("None", status_group);
    status_layout->addWidget(stop_reason_label_, 1, 1);

    status_layout->addWidget(new QLabel("Steps:", status_group), 2, 0);
    steps_label_ = new QLabel("0", status_group);
    steps_label_->setFont(mono_font);
    status_layout->addWidget(steps_label_, 2, 1);

    hex_layout->addWidget(status_group);
    hex_layout->addStretch();

    tabs->addTab(hex_tab, "Hex && Dec");

    // Tab 2: Bit Visualization
    bit_view_ = new BitViewWidget(tabs);
    tabs->addTab(bit_view_, "Bit Visualization");

    main_layout->addWidget(tabs);
}

void RegistersPanel::setController(ExecutionController* controller) {
    controller_ = controller;
    if (bit_view_ != nullptr) {
        bit_view_->setController(controller);
    }
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &RegistersPanel::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &RegistersPanel::refresh);
    }
    refresh();
}

void RegistersPanel::refresh() {
    if (controller_ == nullptr) return;

    const auto& regs = controller_->session().registers();
    const auto& flg = controller_->session().flags();
    const auto st = controller_->session().state();
    const auto stop_reason = controller_->lastStopReason();
    const auto total_steps = controller_->session().total_steps_executed();

    // 16-bit register values and diffs
    pc_label_->setText(formatHexWord(regs.pc));
    sp_label_->setText(formatHexWord(regs.sp));
    ix_label_->setText(formatHexWord(regs.ix));

    if (has_prev_state_) {
        applyValueStyle(pc_label_, regs.pc != prev_registers_.pc);
        applyValueStyle(sp_label_, regs.sp != prev_registers_.sp);
        applyValueStyle(ix_label_, regs.ix != prev_registers_.ix);
    }

    // 8-bit register values and diffs
    for (std::size_t i = 0; i < 4; ++i) {
        r_labels_[i]->setText(formatHexByte(regs.r[i]) + QString(" (%1)").arg(regs.r[i], 3, 10, QLatin1Char(' ')));
        if (has_prev_state_) {
            applyValueStyle(r_labels_[i], regs.r[i] != prev_registers_.r[i]);
        }
    }

    // Flags
    applyFlagStyle(flag_z_label_, flg.z);
    applyFlagStyle(flag_c_label_, flg.c);
    applyFlagStyle(flag_n_label_, flg.n);
    applyFlagStyle(flag_v_label_, flg.v);

    // State
    switch (st) {
        case MachineState::Ready:
            state_label_->setText("READY");
            state_label_->setStyleSheet("QLabel { font-weight: bold; color: #28a745; }");
            break;
        case MachineState::Running:
            state_label_->setText("RUNNING");
            state_label_->setStyleSheet("QLabel { font-weight: bold; color: #007bff; }");
            break;
        case MachineState::Paused:
            state_label_->setText("PAUSED");
            state_label_->setStyleSheet("QLabel { font-weight: bold; color: #fd7e14; }");
            break;
        case MachineState::Halted:
            state_label_->setText("HALTED");
            state_label_->setStyleSheet("QLabel { font-weight: bold; color: #6c757d; }");
            break;
        case MachineState::Faulted:
            state_label_->setText("FAULTED");
            state_label_->setStyleSheet("QLabel { font-weight: bold; color: #dc3545; }");
            break;
    }

    // Stop Reason
    stop_reason_label_->setText(QString::fromStdString(std::string(debugger::to_string(stop_reason))));

    // Steps
    steps_label_->setText(QString::number(total_steps));

    // Update diff cache
    prev_registers_ = regs;
    prev_flags_ = flg;
    has_prev_state_ = true;
}

} // namespace bitforge::gui
