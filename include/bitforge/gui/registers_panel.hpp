#ifndef BITFORGE_GUI_REGISTERS_PANEL_HPP
#define BITFORGE_GUI_REGISTERS_PANEL_HPP

#include "bitforge/core/registers.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/core/cpu_state.hpp"
#include "bitforge/gui/bit_view_widget.hpp"

#include <QWidget>
#include <QLabel>
#include <array>

namespace bitforge::gui {

class ExecutionController;

// Displays 16-bit and 8-bit CPU registers, flags, machine state, and instructions executed
class RegistersPanel : public QWidget {
    Q_OBJECT

public:
    explicit RegistersPanel(QWidget* parent = nullptr);
    ~RegistersPanel() override = default;

    void setController(ExecutionController* controller);
    [[nodiscard]] BitViewWidget* bitView() const noexcept { return bit_view_; }
    [[nodiscard]] QLabel* stateLabel() const noexcept { return state_label_; }
    [[nodiscard]] QLabel* rLabel(std::size_t index) const noexcept {
        return index < r_labels_.size() ? r_labels_[index] : nullptr;
    }

public slots:
    void refresh();

signals:
    void requestAddWatch(const QString& symbol);

private slots:
    void onCustomContextMenu(const QPoint& pos);

private:
    ExecutionController* controller_{nullptr};

    // 16-bit registers
    QLabel* pc_label_;
    QLabel* sp_label_;
    QLabel* ix_label_;

    // 8-bit registers (R0..R3)
    std::array<QLabel*, 4> r_labels_{};

    // Flags
    QLabel* flag_z_label_;
    QLabel* flag_c_label_;
    QLabel* flag_n_label_;
    QLabel* flag_v_label_;

    // Status
    QLabel* state_label_;
    QLabel* stop_reason_label_;
    QLabel* steps_label_;

    // Change detection cache
    Registers prev_registers_{};
    Flags prev_flags_{};
    bool has_prev_state_{false};

    BitViewWidget* bit_view_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_REGISTERS_PANEL_HPP
