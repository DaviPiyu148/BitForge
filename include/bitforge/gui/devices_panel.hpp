#ifndef BITFORGE_GUI_DEVICES_PANEL_HPP
#define BITFORGE_GUI_DEVICES_PANEL_HPP

#include "bitforge/gui/seven_segment_display.hpp"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <array>
#include <cstddef>

namespace bitforge::gui {

class ExecutionController;

// Virtual I/O device display: Text Console, Numeric Console, 8-Bit LEDs, 7-Segment Display, Input FIFO, and Diagnostics/Faults
class DevicesPanel : public QWidget {
    Q_OBJECT

public:
    explicit DevicesPanel(QWidget* parent = nullptr);
    ~DevicesPanel() override = default;

    void setController(ExecutionController* controller);
    [[nodiscard]] SevenSegmentDisplay* sevenSegmentDisplay() const noexcept { return seven_seg_display_; }
    [[nodiscard]] QLabel* fifoStatusLabel() const noexcept { return fifo_status_label_; }
    [[nodiscard]] QLabel* diagnosticsEmptyLabel() const noexcept { return diag_empty_label_; }
    [[nodiscard]] QLineEdit* fifoInputField() const noexcept { return fifo_input_edit_; }
    [[nodiscard]] QPushButton* fifoEnqueueButton() const noexcept { return fifo_enqueue_btn_; }
    [[nodiscard]] QPushButton* fifoClearButton() const noexcept { return fifo_clear_btn_; }
    [[nodiscard]] QLabel* fifoUnderflowLabel() const noexcept { return fifo_underflow_label_; }
    [[nodiscard]] QLabel* fifoContentsLabel() const noexcept { return fifo_contents_label_; }
    [[nodiscard]] QLabel* ledValueLabel() const noexcept { return led_value_label_; }
    [[nodiscard]] const std::array<QLabel*, 8>& ledDots() const noexcept { return led_dots_; }
    [[nodiscard]] QPlainTextEdit* textConsole() const noexcept { return text_console_; }
    [[nodiscard]] QPlainTextEdit* numericConsole() const noexcept { return numeric_console_; }
    [[nodiscard]] QTableWidget* diagnosticTable() const noexcept { return diag_table_; }

public slots:
    void refresh();
    void showDiagnostics();

signals:
    void diagnosticClicked(std::size_t line, std::size_t column);

private slots:
    void onDiagnosticTableClicked(int row, int column);
    void onEnqueueInput();
    void onClearFifo();

private:
    ExecutionController* controller_{nullptr};

    // Virtual LEDs
    std::array<QLabel*, 8> led_dots_{};
    QLabel* led_value_label_{nullptr};

    // 7-Segment Display (Port 0x02)
    SevenSegmentDisplay* seven_seg_display_{nullptr};

    // Input FIFO (Port 0x00)
    QLabel* fifo_status_label_{nullptr};
    QLineEdit* fifo_input_edit_{nullptr};
    QPushButton* fifo_enqueue_btn_{nullptr};
    QPushButton* fifo_clear_btn_{nullptr};
    QLabel* fifo_contents_label_{nullptr};
    QLabel* fifo_underflow_label_{nullptr};

    // Consoles
    QPlainTextEdit* text_console_{nullptr};
    QPlainTextEdit* numeric_console_{nullptr};

    // Diagnostics & Faults
    QTableWidget* diag_table_{nullptr};
    QLabel* diag_empty_label_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_DEVICES_PANEL_HPP
