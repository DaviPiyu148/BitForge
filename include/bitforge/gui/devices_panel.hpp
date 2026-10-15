#ifndef BITFORGE_GUI_DEVICES_PANEL_HPP
#define BITFORGE_GUI_DEVICES_PANEL_HPP

#include <QWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <array>
#include <cstddef>

namespace bitforge::gui {

class ExecutionController;

// Virtual I/O device display: Text Console, Numeric Console, 8-Bit LEDs, and Diagnostics/Faults
class DevicesPanel : public QWidget {
    Q_OBJECT

public:
    explicit DevicesPanel(QWidget* parent = nullptr);
    ~DevicesPanel() override = default;

    void setController(ExecutionController* controller);

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
    QLabel* led_value_label_;

    // Consoles
    QPlainTextEdit* text_console_;
    QPlainTextEdit* numeric_console_;

    // Diagnostics & Faults
    QTableWidget* diag_table_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_DEVICES_PANEL_HPP
