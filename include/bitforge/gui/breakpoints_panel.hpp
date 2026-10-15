#ifndef BITFORGE_GUI_BREAKPOINTS_PANEL_HPP
#define BITFORGE_GUI_BREAKPOINTS_PANEL_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

namespace bitforge::gui {

class ExecutionController;

// Table of configured address/source breakpoints with removal controls
class BreakpointsPanel : public QWidget {
    Q_OBJECT

public:
    explicit BreakpointsPanel(QWidget* parent = nullptr);
    ~BreakpointsPanel() override = default;

    void setController(ExecutionController* controller);

public slots:
    void refresh();

private slots:
    void onRemoveSelected();
    void onClearAll();

private:
    ExecutionController* controller_{nullptr};
    QTableWidget* table_;
    QPushButton* remove_btn_;
    QPushButton* clear_all_btn_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_BREAKPOINTS_PANEL_HPP
