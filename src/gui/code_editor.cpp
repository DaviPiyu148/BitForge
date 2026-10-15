#include "bitforge/gui/code_editor.hpp"
#include "bitforge/gui/execution_controller.hpp"

#include <QPainter>
#include <QTextBlock>
#include <QFontDatabase>
#include <QToolTip>
#include <QMouseEvent>
#include <QPolygon>
#include <QPoint>

namespace bitforge::gui {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent),
      line_number_area_(new LineNumberArea(this)),
      highlighter_(new AsmHighlighter(document())) {
    // Monospace programming font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    setFont(font);

    // Tab stop 4 spaces
    const int tab_stop_width = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop_width);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightLines);

    updateLineNumberAreaWidth(0);
    highlightLines();
}

void CodeEditor::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::breakpointsChanged, this, &CodeEditor::updateBreakpoints);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &CodeEditor::updateBreakpoints);
        connect(controller_, &ExecutionController::stateChanged, this, [this]() {
            if (controller_->session().state() != MachineState::Ready &&
                controller_->session().state() != MachineState::Running &&
                controller_->session().state() != MachineState::Paused &&
                controller_->session().state() != MachineState::Halted &&
                controller_->session().state() != MachineState::Faulted) {
                return;
            }
            auto loc = controller_->session().resolve_source_location();
            if (loc.has_value()) {
                setCurrentPcLine(loc->line);
            } else {
                clearCurrentPcLine();
            }
        });
    }
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max_lines = std::max(1, blockCount());
    while (max_lines >= 10) {
        max_lines /= 10;
        ++digits;
    }

    // 24px gutter for breakpoint dot & execution arrow + digit text + 12px margin
    const int space = 24 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 12;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy != 0) {
        line_number_area_->scroll(0, dy);
    } else {
        line_number_area_->update(0, rect.y(), line_number_area_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    line_number_area_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightLines() {
    QList<QTextEdit::ExtraSelection> extra_selections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        const QColor line_color = palette().color(QPalette::Base).darker(105);
        selection.format.setBackground(line_color);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extra_selections.append(selection);
    }

    // Execution line highlight (warm gold/amber background)
    if (current_pc_line_ > 0 && current_pc_line_ <= static_cast<std::size_t>(blockCount())) {
        QTextBlock pc_block = document()->findBlockByNumber(static_cast<int>(current_pc_line_ - 1));
        if (pc_block.isValid()) {
            QTextEdit::ExtraSelection pc_selection;
            pc_selection.format.setBackground(QColor(255, 243, 205)); // Soft amber
            pc_selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            pc_selection.cursor = QTextCursor(pc_block);
            pc_selection.cursor.clearSelection();
            extra_selections.append(pc_selection);
        }
    }

    setExtraSelections(extra_selections);
}

void CodeEditor::setCurrentPcLine(std::size_t line) {
    current_pc_line_ = line;
    highlightLines();
    line_number_area_->update();
    if (line > 0 && line <= static_cast<std::size_t>(blockCount())) {
        gotoLine(line);
    }
}

void CodeEditor::clearCurrentPcLine() {
    current_pc_line_ = 0;
    highlightLines();
    line_number_area_->update();
}

void CodeEditor::gotoLine(std::size_t line, std::size_t column) {
    if (line == 0) return;
    int block_num = static_cast<int>(line - 1);
    if (block_num >= blockCount()) {
        block_num = blockCount() - 1;
    }
    QTextBlock block = document()->findBlockByNumber(block_num);
    if (!block.isValid()) return;

    QTextCursor cursor(block);
    int col_offset = static_cast<int>(column > 0 ? column - 1 : 0);
    if (col_offset > block.length() - 1) {
        col_offset = std::max(0, block.length() - 1);
    }
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col_offset);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void CodeEditor::updateBreakpoints() {
    active_breakpoint_lines_.clear();
    if (controller_ != nullptr && controller_->programImage() != nullptr) {
        const auto& image = *controller_->programImage();
        for (word_t addr : controller_->session().breakpoints()) {
            const auto* mapping = image.source_map().find_instruction_by_address(addr);
            if (mapping != nullptr && mapping->location.line > 0) {
                active_breakpoint_lines_.insert(mapping->location.line);
            }
        }
    }
    line_number_area_->update();
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(line_number_area_);
    painter.fillRect(event->rect(), palette().color(QPalette::Window));

    QTextBlock block = firstVisibleBlock();
    int block_number = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    const int font_height = fontMetrics().height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const std::size_t line_num = static_cast<std::size_t>(block_number + 1);
            const bool has_bp = active_breakpoint_lines_.count(line_num) > 0;
            const bool is_pc = (current_pc_line_ == line_num);

            const int centerY = top + (font_height / 2);

            // 1. Draw Breakpoint Indicator (Red filled circle)
            if (has_bp) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QColor(180, 40, 40));
                painter.setBrush(QColor(220, 53, 69));
                painter.drawEllipse(QPoint(10, centerY), 6, 6);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            // 2. Draw Current PC Execution Indicator (Yellow/Amber Arrow)
            if (is_pc) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                QPolygon arrow;
                if (has_bp) {
                    // Small golden arrow centered inside the red breakpoint circle
                    painter.setPen(Qt::white);
                    painter.setBrush(QColor(255, 215, 0));
                    arrow << QPoint(8, centerY - 4)
                          << QPoint(14, centerY)
                          << QPoint(8, centerY + 4);
                } else {
                    // Prominent yellow arrow in gutter
                    painter.setPen(QColor(180, 130, 0));
                    painter.setBrush(QColor(245, 166, 35));
                    arrow << QPoint(4, centerY - 6)
                          << QPoint(16, centerY)
                          << QPoint(4, centerY + 6);
                }
                painter.drawPolygon(arrow);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            // 3. Draw Line Number text
            const QString number_str = QString::number(block_number + 1);
            painter.setPen(is_pc ? QColor(160, 100, 0) : palette().color(QPalette::PlaceholderText));
            painter.drawText(0, top, line_number_area_->width() - 8, font_height,
                             Qt::AlignRight | Qt::AlignVCenter, number_str);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++block_number;
    }
}

void CodeEditor::lineNumberAreaMousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QTextBlock block = firstVisibleBlock();
        int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + static_cast<int>(blockBoundingRect(block).height());

        while (block.isValid() && top <= event->pos().y()) {
            if (event->pos().y() >= top && event->pos().y() <= bottom) {
                const std::size_t line = static_cast<std::size_t>(block.blockNumber() + 1);
                if (controller_ != nullptr) {
                    auto res = controller_->toggleBreakpoint(line);
                    if (!res.success) {
                        QToolTip::showText(event->globalPosition().toPoint(),
                                           QString::fromStdString(res.message),
                                           line_number_area_);
                    }
                }
                break;
            }
            block = block.next();
            top = bottom;
            bottom = top + static_cast<int>(blockBoundingRect(block).height());
        }
    }
}

} // namespace bitforge::gui
