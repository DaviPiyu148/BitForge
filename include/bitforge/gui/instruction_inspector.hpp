#ifndef BITFORGE_GUI_INSTRUCTION_INSPECTOR_HPP
#define BITFORGE_GUI_INSTRUCTION_INSPECTOR_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/opcodes.hpp"

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QString>
#include <vector>
#include <optional>
#include <cstdint>

namespace bitforge::gui {

class ExecutionController;

// Decoded instruction representation
struct DecodedInstructionInfo {
    bool valid{false};
    word_t address{0};
    byte_t opcode{0};
    std::size_t length{0};
    std::size_t instruction_size{0};
    QString mnemonic;
    QString operands;
    QString full_disassembly;
    QString raw_bytes_hex;
    QString source_location;
    QString branch_evaluation;
};

// Summary of the most recently executed step
struct LastExecutedStepInfo {
    std::uint64_t step_number{0};
    word_t pc{0};
    QString mnemonic;
    QString state_after;
    QString note;
};

// Current Instruction Inspector: decodes and explains the instruction at PC,
// and summarizes the most recently executed instruction from trace history.
class InstructionInspector : public QWidget {
    Q_OBJECT

public:
    explicit InstructionInspector(QWidget* parent = nullptr);
    ~InstructionInspector() override = default;

    void setController(ExecutionController* controller);

    [[nodiscard]] static DecodedInstructionInfo decodeAt(
        const ExecutionController& controller, word_t address);

    [[nodiscard]] static DecodedInstructionInfo decodeInstruction(
        word_t address, const ExecutionController& controller) {
        return decodeAt(controller, address);
    }

    [[nodiscard]] const DecodedInstructionInfo& decodedInfo() const noexcept {
        return current_decoded_info_;
    }

    [[nodiscard]] const std::optional<LastExecutedStepInfo>& lastExecutedStep() const noexcept {
        return last_step_info_;
    }

public slots:
    void refresh();

private:
    ExecutionController* controller_{nullptr};

    DecodedInstructionInfo current_decoded_info_{};
    std::optional<LastExecutedStepInfo> last_step_info_{std::nullopt};

    // Current PC instruction widgets
    QLabel* pc_addr_label_{nullptr};
    QLabel* pc_source_label_{nullptr};
    QLabel* pc_mnemonic_label_{nullptr};
    QLabel* pc_bytes_label_{nullptr};
    QLabel* pc_branch_label_{nullptr};

    // Last executed step widgets
    QLabel* last_step_label_{nullptr};
    QLabel* last_pc_label_{nullptr};
    QLabel* last_mnem_label_{nullptr};
    QLabel* last_state_label_{nullptr};
    QLabel* last_effect_label_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_INSTRUCTION_INSPECTOR_HPP
