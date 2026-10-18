#ifndef BITFORGE_GUI_TRACE_PANEL_HPP
#define BITFORGE_GUI_TRACE_PANEL_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

namespace bitforge::gui {

class ExecutionController;

// Table of bounded chronological execution trace entries with HALT/fault highlighting
class TracePanel : public QWidget {
    Q_OBJECT

public:
    explicit TracePanel(QWidget* parent = nullptr);
    ~TracePanel() override = default;

    void setController(ExecutionController* controller);
    [[nodiscard]] QLabel* emptyLabel() const noexcept { return empty_label_; }

public slots:
    void refresh();

private slots:
    void onClearTrace();

private:
    ExecutionController* controller_{nullptr};
    QTableWidget* table_;
    QPushButton* clear_btn_;
    QLabel* empty_label_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_TRACE_PANEL_HPP
