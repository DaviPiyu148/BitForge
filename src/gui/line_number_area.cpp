#include "bitforge/gui/line_number_area.hpp"
#include "bitforge/gui/code_editor.hpp"

#include <QSize>

namespace bitforge::gui {

LineNumberArea::LineNumberArea(CodeEditor* editor)
    : QWidget(editor), editor_(editor) {
    setToolTip("Click gutter or press F9 to toggle breakpoint on this line");
}

QSize LineNumberArea::sizeHint() const {
    return QSize(editor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
    editor_->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent* event) {
    editor_->lineNumberAreaMousePressEvent(event);
}

void LineNumberArea::contextMenuEvent(QContextMenuEvent* event) {
    editor_->lineNumberAreaContextMenuEvent(event);
}

} // namespace bitforge::gui
