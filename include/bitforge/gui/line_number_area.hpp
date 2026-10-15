#ifndef BITFORGE_GUI_LINE_NUMBER_AREA_HPP
#define BITFORGE_GUI_LINE_NUMBER_AREA_HPP

#include <QWidget>

namespace bitforge::gui {

class CodeEditor;

// Adjacent gutter widget displaying line numbers, breakpoint dots, and execution arrows
class LineNumberArea : public QWidget {
    Q_OBJECT

public:
    explicit LineNumberArea(CodeEditor* editor);
    ~LineNumberArea() override = default;

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    CodeEditor* editor_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_LINE_NUMBER_AREA_HPP
