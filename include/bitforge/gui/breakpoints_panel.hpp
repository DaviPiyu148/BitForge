#ifndef BITFORGE_GUI_BREAKPOINTS_PANEL_HPP
#define BITFORGE_GUI_BREAKPOINTS_PANEL_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

namespace bitforge::gui {

class ExecutionController;

// Table of configured address/source breakpoints with removal controls
class BreakpointsPanel : public QWidget {
    Q_OBJECT

public:
    explicit BreakpointsPanel(QWidget* parent = nullptr);
    ~BreakpointsPanel() override = default;

    void setController(ExecutionController* controller);
    [[nodiscard]] QTableWidget* table() const noexcept { return table_; }
    [[nodiscard]] QPushButton* removeButton() const noexcept { return remove_btn_; }
    [[nodiscard]] QPushButton* clearAllButton() const noexcept { return clear_all_btn_; }
    [[nodiscard]] QLabel* emptyLabel() const noexcept { return empty_label_; }

public slots:
    void refresh();

signals:
    void breakpointDoubleClicked(std::size_t line);

private slots:
    void onRemoveSelected();
    void onClearAll();

private:
    ExecutionController* controller_{nullptr};
    QTableWidget* table_;
    QPushButton* remove_btn_;
    QPushButton* clear_all_btn_;
    QLabel* empty_label_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_BREAKPOINTS_PANEL_HPP
