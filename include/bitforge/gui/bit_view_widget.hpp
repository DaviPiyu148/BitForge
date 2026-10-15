#ifndef BITFORGE_GUI_BIT_VIEW_WIDGET_HPP
#define BITFORGE_GUI_BIT_VIEW_WIDGET_HPP

#include "bitforge/core/registers.hpp"
#include "bitforge/core/flags.hpp"
#include "bitforge/core/types.hpp"

#include <QWidget>
#include <QLabel>
#include <array>
#include <vector>

namespace bitforge::gui {

class ExecutionController;

// Educational bit-level visualization of 8-bit registers (R0..R3),
// 16-bit registers (PC, SP, IX), and ALU flags (Z, C, N, V).
class BitViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit BitViewWidget(QWidget* parent = nullptr);
    ~BitViewWidget() override = default;

    void setController(ExecutionController* controller);

public slots:
    void refresh();

private:
    struct BitRow8 {
        QWidget* container{nullptr};
        QLabel* title_label{nullptr};
        std::array<QLabel*, 8> bit_dots{};
    };

    struct BitRow16 {
        QWidget* container{nullptr};
        QLabel* title_label{nullptr};
        std::array<QLabel*, 16> bit_dots{};
    };

    BitRow16 createBitRow16(const QString& name, QWidget* parent);
    BitRow8 createBitRow8(const QString& name, QWidget* parent);
    void updateBitRow16(BitRow16& row, word_t val, bool changed);
    void updateBitRow8(BitRow8& row, byte_t val, bool changed);

    ExecutionController* controller_{nullptr};

    // 16-bit register bit rows
    BitRow16 pc_row_;
    BitRow16 sp_row_;
    BitRow16 ix_row_;

    // 8-bit register bit rows
    std::array<BitRow8, 4> r_rows_{};

    // Flag indicators
    QLabel* flag_z_dot_{nullptr};
    QLabel* flag_c_dot_{nullptr};
    QLabel* flag_n_dot_{nullptr};
    QLabel* flag_v_dot_{nullptr};
    QLabel* flag_z_val_{nullptr};
    QLabel* flag_c_val_{nullptr};
    QLabel* flag_n_val_{nullptr};
    QLabel* flag_v_val_{nullptr};

    // Change detection cache
    Registers prev_registers_{};
    Flags prev_flags_{};
    bool has_prev_state_{false};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_BIT_VIEW_WIDGET_HPP
