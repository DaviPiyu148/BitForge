#ifndef BITFORGE_GUI_DEVICES_PANEL_HPP
#define BITFORGE_GUI_DEVICES_PANEL_HPP

#include "bitforge/gui/seven_segment_display.hpp"

#include <QWidget>
#include <QLabel>
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

public slots:
    void refresh();
    void showDiagnostics();

signals:
    void diagnosticClicked(std::size_t line, std::size_t column);

private slots:
    void onDiagnosticTableClicked(int row, int column);

private:
    ExecutionController* controller_{nullptr};

    // Virtual LEDs
    std::array<QLabel*, 8> led_dots_{};
    QLabel* led_value_label_{nullptr};

    // 7-Segment Display (Port 0x02)
    SevenSegmentDisplay* seven_seg_display_{nullptr};

    // Input FIFO (Port 0x00)
    QLabel* fifo_status_label_{nullptr};

    // Consoles
    QPlainTextEdit* text_console_{nullptr};
    QPlainTextEdit* numeric_console_{nullptr};

    // Diagnostics & Faults
    QTableWidget* diag_table_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_DEVICES_PANEL_HPP
