#include "bitforge/gui/instruction_inspector.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFontDatabase>

namespace bitforge::gui {

namespace {

QString regName(byte_t reg_enc) {
    if (reg_enc < 4) {
        return QString("R%1").arg(reg_enc);
    }
    return QString("Invalid(0x%1)").arg(reg_enc, 2, 16, QLatin1Char('0')).toUpper();
}

} // namespace

InstructionInspector::InstructionInspector(QWidget* parent)
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

    const QFont mono_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // 1. Current PC Instruction (Next to Execute)
    auto* current_group = new QGroupBox("Current Instruction (at Program Counter)", content_widget);
    auto* current_grid = new QGridLayout(current_group);
    current_grid->setContentsMargins(8, 8, 8, 8);
    current_grid->setSpacing(6);

    current_grid->addWidget(new QLabel("PC Address:", current_group), 0, 0);
    pc_addr_label_ = new QLabel("0x0200", current_group);
    pc_addr_label_->setFont(mono_font);
    current_grid->addWidget(pc_addr_label_, 0, 1);

    current_grid->addWidget(new QLabel("Source Location:", current_group), 0, 2);
    pc_source_label_ = new QLabel("Line 1, Col 1", current_group);
    pc_source_label_->setFont(mono_font);
    current_grid->addWidget(pc_source_label_, 0, 3);

    current_grid->addWidget(new QLabel("Instruction:", current_group), 1, 0);
    pc_mnemonic_label_ = new QLabel("NOP", current_group);
    pc_mnemonic_label_->setFont(QFont("Segoe UI", 10, QFont::Bold));
    pc_mnemonic_label_->setStyleSheet("QLabel { color: #007bff; }");
    current_grid->addWidget(pc_mnemonic_label_, 1, 1);

    current_grid->addWidget(new QLabel("Raw Bytes:", current_group), 1, 2);
    pc_bytes_label_ = new QLabel("0x00", current_group);
    pc_bytes_label_->setFont(mono_font);
    current_grid->addWidget(pc_bytes_label_, 1, 3);

    current_grid->addWidget(new QLabel("Branch Target / Condition:", current_group), 2, 0);
    pc_branch_label_ = new QLabel("N/A", current_group);
    pc_branch_label_->setFont(mono_font);
    current_grid->addWidget(pc_branch_label_, 2, 1, 1, 3);

    content_layout->addWidget(current_group);

    // 2. Last Executed Step Summary
    auto* last_group = new QGroupBox("Last Executed Step (Trace Summary)", content_widget);
    auto* last_grid = new QGridLayout(last_group);
    last_grid->setContentsMargins(8, 8, 8, 8);
    last_grid->setSpacing(6);

    last_grid->addWidget(new QLabel("Step #:", last_group), 0, 0);
    last_step_label_ = new QLabel("0", last_group);
    last_step_label_->setFont(mono_font);
    last_grid->addWidget(last_step_label_, 0, 1);

    last_grid->addWidget(new QLabel("Executed Address:", last_group), 0, 2);
    last_pc_label_ = new QLabel("-", last_group);
    last_pc_label_->setFont(mono_font);
    last_grid->addWidget(last_pc_label_, 0, 3);

    last_grid->addWidget(new QLabel("Mnemonic Executed:", last_group), 1, 0);
    last_mnem_label_ = new QLabel("-", last_group);
    last_mnem_label_->setFont(mono_font);
    last_grid->addWidget(last_mnem_label_, 1, 1);

    last_grid->addWidget(new QLabel("Engine State After:", last_group), 1, 2);
    last_state_label_ = new QLabel("Ready", last_group);
    last_state_label_->setFont(mono_font);
    last_grid->addWidget(last_state_label_, 1, 3);

    last_grid->addWidget(new QLabel("Execution Note / Fault:", last_group), 2, 0);
    last_effect_label_ = new QLabel("Program ready to execute.", last_group);
    last_effect_label_->setFont(mono_font);
    last_grid->addWidget(last_effect_label_, 2, 1, 1, 3);

    content_layout->addWidget(last_group);
    content_layout->addStretch();

    scroll_area->setWidget(content_widget);
    outer_layout->addWidget(scroll_area);
}

void InstructionInspector::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &InstructionInspector::refresh);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &InstructionInspector::refresh);
    }
    refresh();
}

DecodedInstructionInfo InstructionInspector::decodeAt(const ExecutionController& controller, word_t address) {
    DecodedInstructionInfo info;
    info.address = address;

    const auto& mem = controller.memory();
    const byte_t op = mem.read_byte(address);
    info.opcode = op;

    if (!is_valid_opcode(op)) {
        info.valid = false;
        info.length = 1;
        info.instruction_size = 1;
        info.mnemonic = QString("0x%1").arg(op, 2, 16, QLatin1Char('0')).toUpper();
        info.operands = "(Invalid Opcode)";
        info.full_disassembly = QString("0x%1 (Invalid)").arg(op, 2, 16, QLatin1Char('0')).toUpper();
        info.raw_bytes_hex = QString("0x%1").arg(op, 2, 16, QLatin1Char('0')).toUpper();
        info.source_location = "-";
        info.branch_evaluation = "N/A";
        return info;
    }

    info.valid = true;
    info.length = instruction_length(op);
    info.instruction_size = info.length;
    info.mnemonic = QString::fromLatin1(opcode_to_mnemonic(op).data(),
                                        static_cast<qsizetype>(opcode_to_mnemonic(op).size()));

    // Collect raw bytes
    QStringList byte_strs;
    std::vector<byte_t> raw;
    for (std::size_t i = 0; i < info.length; ++i) {
        word_t byte_addr = static_cast<word_t>(address + i);
        byte_t b = mem.read_byte(byte_addr);
        raw.push_back(b);
        byte_strs << QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
    }
    info.raw_bytes_hex = byte_strs.join(" ");

    // Source mapping lookup
    const auto* img = controller.programImage();
    if (img != nullptr) {
        const auto* mapping = img->source_map().find_instruction_by_address(address);
        if (mapping != nullptr) {
            info.source_location = QString("Line %1, Col %2")
                                      .arg(mapping->location.line)
                                      .arg(mapping->location.column);
        } else {
            info.source_location = "-";
        }
    } else {
        info.source_location = "-";
    }

    // Decode operands based on instruction type
    const auto opcode_enum = static_cast<Opcode>(op);
    const auto& flags = controller.session().flags();
    info.branch_evaluation = "N/A";

    switch (opcode_enum) {
        case Opcode::NOP:
        case Opcode::HALT:
        case Opcode::INX:
        case Opcode::DEX:
        case Opcode::RET:
            info.operands = "";
            info.full_disassembly = info.mnemonic;
            break;

        case Opcode::INC:
        case Opcode::DEC:
        case Opcode::NOT:
        case Opcode::POP:
            info.operands = regName(raw[1]);
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::PUSH:
            info.operands = regName(raw[1]);
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::LDX:
            info.operands = QString("%1, [IX]").arg(regName(raw[1]));
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::STX:
            info.operands = QString("[IX], %1").arg(regName(raw[1]));
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::LDI:
            info.operands = QString("%1, %2 (0x%3)")
                               .arg(regName(raw[1]))
                               .arg(raw[2])
                               .arg(raw[2], 2, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2, %3").arg(info.mnemonic, regName(raw[1])).arg(raw[2]);
            break;

        case Opcode::MOV:
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::CMP:
            info.operands = QString("%1, %2").arg(regName(raw[1]), regName(raw[2]));
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::LDI16: {
            word_t val = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("IX, 0x%1").arg(val, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;
        }

        case Opcode::LD: {
            word_t target = (static_cast<word_t>(raw[2]) << 8) | raw[3];
            info.operands = QString("%1, [0x%2]").arg(regName(raw[1])).arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;
        }

        case Opcode::ST: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("[0x%1], %2").arg(target, 4, 16, QLatin1Char('0')).toUpper().arg(regName(raw[3]));
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;
        }

        case Opcode::JMP:
        case Opcode::CALL: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            info.branch_evaluation = QString("Unconditional branch to target 0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            break;
        }

        case Opcode::JZ: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            if (flags.z) {
                info.branch_evaluation = QString("Branch TAKEN: Target 0x%1 (Zero Flag Z=1)").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            } else {
                info.branch_evaluation = QString("Branch NOT taken: Fallthrough (Zero Flag Z=0)");
            }
            break;
        }

        case Opcode::JNZ: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            if (!flags.z) {
                info.branch_evaluation = QString("Branch TAKEN: Target 0x%1 (Zero Flag Z=0)").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            } else {
                info.branch_evaluation = QString("Branch NOT taken: Fallthrough (Zero Flag Z=1)");
            }
            break;
        }

        case Opcode::JC: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            if (flags.c) {
                info.branch_evaluation = QString("Branch TAKEN: Target 0x%1 (Carry Flag C=1)").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            } else {
                info.branch_evaluation = QString("Branch NOT taken: Fallthrough (Carry Flag C=0)");
            }
            break;
        }

        case Opcode::JNC: {
            word_t target = (static_cast<word_t>(raw[1]) << 8) | raw[2];
            info.operands = QString("0x%1").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            if (!flags.c) {
                info.branch_evaluation = QString("Branch TAKEN: Target 0x%1 (Carry Flag C=0)").arg(target, 4, 16, QLatin1Char('0')).toUpper();
            } else {
                info.branch_evaluation = QString("Branch NOT taken: Fallthrough (Carry Flag C=1)");
            }
            break;
        }

        case Opcode::IN:
            info.operands = QString("%1, 0x%2").arg(regName(raw[1])).arg(raw[2], 2, 16, QLatin1Char('0')).toUpper();
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;

        case Opcode::OUT:
            info.operands = QString("0x%1, %2").arg(raw[1], 2, 16, QLatin1Char('0')).toUpper().arg(regName(raw[2]));
            info.full_disassembly = QString("%1 %2").arg(info.mnemonic, info.operands);
            break;
    }

    return info;
}

void InstructionInspector::refresh() {
    if (controller_ == nullptr) return;

    const word_t pc = controller_->session().pc();
    pc_addr_label_->setText(QString("0x%1").arg(pc, 4, 16, QLatin1Char('0')).toUpper());

    auto decoded = decodeAt(*controller_, pc);
    pc_source_label_->setText(decoded.source_location);
    pc_mnemonic_label_->setText(decoded.full_disassembly);
    pc_bytes_label_->setText(decoded.raw_bytes_hex);
    pc_branch_label_->setText(decoded.branch_evaluation);
    current_decoded_info_ = decoded;

    // Colorize branch preview
    if (decoded.branch_evaluation.contains("TAKEN")) {
        pc_branch_label_->setStyleSheet("QLabel { color: #28a745; font-weight: bold; }");
    } else if (decoded.branch_evaluation.contains("NOT taken")) {
        pc_branch_label_->setStyleSheet("QLabel { color: #fd7e14; }");
    } else {
        pc_branch_label_->setStyleSheet("QLabel { color: inherit; }");
    }

    // Last step summary
    const auto& trace = controller_->session().trace();
    if (!trace.empty()) {
        const auto& last = trace.back();
        last_step_label_->setText(QString("#%1").arg(last.step_number));
        last_pc_label_->setText(QString("0x%1").arg(last.instruction_address, 4, 16, QLatin1Char('0')).toUpper());
        last_mnem_label_->setText(QString::fromStdString(last.mnemonic));

        switch (last.state_after) {
            case MachineState::Ready:
                last_state_label_->setText("Ready");
                last_state_label_->setStyleSheet("QLabel { color: #28a745; font-weight: bold; }");
                break;
            case MachineState::Running:
            case MachineState::Paused:
                last_state_label_->setText("Paused / Step Completed");
                last_state_label_->setStyleSheet("QLabel { color: #ffc107; font-weight: bold; }");
                break;
            case MachineState::Halted:
                last_state_label_->setText("Halted Normally");
                last_state_label_->setStyleSheet("QLabel { color: #6c757d; font-weight: bold; }");
                break;
            case MachineState::Faulted:
                last_state_label_->setText(last.fault.has_value()
                                               ? QString("Fault: %1").arg(last.fault->message.c_str())
                                               : "Faulted");
                last_state_label_->setStyleSheet("QLabel { color: #dc3545; font-weight: bold; }");
                break;
        }

        if (last.faulted && last.fault.has_value()) {
            last_effect_label_->setText(QString("Fault at 0x%1: %2")
                                            .arg(last.fault->instruction_address, 4, 16, QLatin1Char('0')).toUpper()
                                            .arg(last.fault->message.c_str()));
            last_effect_label_->setStyleSheet("QLabel { color: #dc3545; }");
        } else if (last.state_after == MachineState::Halted) {
            last_effect_label_->setText("HALT executed. Execution complete.");
            last_effect_label_->setStyleSheet("QLabel { color: #6c757d; }");
        } else {
            last_effect_label_->setText(QString("Flags: [Z=%1, C=%2, N=%3, V=%4]")
                                            .arg(last.flags_after.z ? 1 : 0)
                                            .arg(last.flags_after.c ? 1 : 0)
                                            .arg(last.flags_after.n ? 1 : 0)
                                            .arg(last.flags_after.v ? 1 : 0));
            last_effect_label_->setStyleSheet("QLabel { color: inherit; }");
        }

        LastExecutedStepInfo step_info;
        step_info.step_number = last.step_number;
        step_info.pc = last.instruction_address;
        step_info.mnemonic = QString::fromStdString(last.mnemonic);
        step_info.state_after = last_state_label_->text();
        step_info.note = last_effect_label_->text();
        last_step_info_ = step_info;
    } else {
        last_step_label_->setText("0");
        last_pc_label_->setText("-");
        last_mnem_label_->setText("-");
        last_state_label_->setText("Ready");
        last_state_label_->setStyleSheet("QLabel { color: #28a745; }");
        last_effect_label_->setText("Program ready to execute.");
        last_effect_label_->setStyleSheet("QLabel { color: inherit; }");
        last_step_info_ = std::nullopt;
    }
}

} // namespace bitforge::gui
