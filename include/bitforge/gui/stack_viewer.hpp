#ifndef BITFORGE_GUI_STACK_VIEWER_HPP
#define BITFORGE_GUI_STACK_VIEWER_HPP

#include <QWidget>
#include <QLabel>
#include <QTableWidget>

namespace bitforge::gui {

class ExecutionController;

// Displays the downward-growing stack according to ADR-001 architectural conventions
class StackViewer : public QWidget {
    Q_OBJECT

public:
    explicit StackViewer(QWidget* parent = nullptr);
    ~StackViewer() override = default;

    void setController(ExecutionController* controller);

public slots:
    void refresh();

private:
    ExecutionController* controller_{nullptr};

    QLabel* header_label_;
    QLabel* status_label_;
    QTableWidget* table_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_STACK_VIEWER_HPP
