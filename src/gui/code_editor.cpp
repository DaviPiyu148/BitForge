#include "bitforge/gui/code_editor.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/theme.hpp"

#include <QPainter>
#include <QTextBlock>
#include <QFontDatabase>
#include <QToolTip>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPolygon>
#include <QPoint>
#include <QMenu>
#include <QContextMenuEvent>

namespace bitforge::gui {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent),
      line_number_area_(new LineNumberArea(this)),
      highlighter_(new AsmHighlighter(document())),
      default_font_size_(10),
      current_font_size_(10) {
    // Monospace programming font
    QFont font = theme::monoFont(current_font_size_);
    setFont(font);

    // Tab stop 4 spaces
    const int tab_stop_width = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop_width);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightLines);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::onCursorPositionChanged);

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
    const int space = 24 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 14;
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
        selection.format.setBackground(QColor(255, 255, 255, 12)); // Subtle dark theme cursor line
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extra_selections.append(selection);
    }

    // Execution line highlight (rich dark amber / gold background)
    if (current_pc_line_ > 0 && current_pc_line_ <= static_cast<std::size_t>(blockCount())) {
        QTextBlock pc_block = document()->findBlockByNumber(static_cast<int>(current_pc_line_ - 1));
        if (pc_block.isValid()) {
            QTextEdit::ExtraSelection pc_selection;
            pc_selection.format.setBackground(QColor(229, 192, 123, 40)); // Translucent amber glow
            pc_selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            pc_selection.cursor = QTextCursor(pc_block);
            pc_selection.cursor.clearSelection();
            extra_selections.append(pc_selection);
        }
    }

    setExtraSelections(extra_selections);
}

void CodeEditor::onCursorPositionChanged() {
    const QTextCursor cursor = textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.positionInBlock() + 1;
    emit cursorLineColumnChanged(line, col);
    line_number_area_->update();
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

void CodeEditor::zoomIn(int delta) {
    current_font_size_ = std::min(32, current_font_size_ + delta);
    QFont f = font();
    f.setPointSize(current_font_size_);
    setFont(f);
    const int tab_stop = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::zoomOut(int delta) {
    current_font_size_ = std::max(6, current_font_size_ - delta);
    QFont f = font();
    f.setPointSize(current_font_size_);
    setFont(f);
    const int tab_stop = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::resetZoom() {
    current_font_size_ = default_font_size_;
    QFont f = font();
    f.setPointSize(current_font_size_);
    setFont(f);
    const int tab_stop = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::setFontSize(int size) {
    current_font_size_ = std::clamp(size, 6, 48);
    QFont f = font();
    f.setPointSize(current_font_size_);
    setFont(f);
    const int tab_stop = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tab_stop);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta > 0) {
            zoomIn(1);
        } else if (delta < 0) {
            zoomOut(1);
        }
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
            zoomIn(1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Minus) {
            zoomOut(1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_0) {
            resetZoom();
            event->accept();
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}

bool CodeEditor::findText(const QString& text, bool forward, bool caseSensitive, int* matchIdx, int* totalMatches) {
    if (text.isEmpty()) {
        if (matchIdx) *matchIdx = 0;
        if (totalMatches) *totalMatches = 0;
        return false;
    }

    QTextDocument::FindFlags flags;
    if (!forward) {
        flags |= QTextDocument::FindBackward;
    }
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    // Count total matches
    int total = 0;
    int current = 0;
    QTextCursor search_cur = document()->find(text, 0, flags);
    const int active_pos = textCursor().position();

    while (!search_cur.isNull()) {
        ++total;
        if (search_cur.position() == active_pos || (current == 0 && search_cur.position() >= active_pos)) {
            current = total;
        }
        search_cur = document()->find(text, search_cur, flags);
    }

    // Perform actual find
    bool found = find(text, flags);
    if (!found) {
        // Wrap around
        QTextCursor start_cur(document());
        if (!forward) {
            start_cur.movePosition(QTextCursor::End);
        }
        setTextCursor(start_cur);
        found = find(text, flags);
    }

    if (found && current == 0 && total > 0) {
        current = 1;
    }

    if (matchIdx) *matchIdx = (found ? current : 0);
    if (totalMatches) *totalMatches = total;

    return found;
}

void CodeEditor::clearFindMatches() {
    QTextCursor cur = textCursor();
    if (cur.hasSelection()) {
        cur.clearSelection();
        setTextCursor(cur);
    }
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(line_number_area_);
    // Gutter background: Modern Dark (#21252b)
    painter.fillRect(event->rect(), QColor(0x21, 0x25, 0x2b));

    // Right subtle divider line
    painter.setPen(QColor(0x28, 0x2c, 0x34));
    painter.drawLine(line_number_area_->width() - 1, event->rect().top(),
                     line_number_area_->width() - 1, event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int block_number = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    const int font_height = fontMetrics().height();
    const int current_cursor_block = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const std::size_t line_num = static_cast<std::size_t>(block_number + 1);
            const bool has_bp = active_breakpoint_lines_.count(line_num) > 0;
            const bool is_pc = (current_pc_line_ == line_num);

            const int centerY = top + (font_height / 2);

            // 1. Draw Breakpoint Indicator (Bright crimson red circle)
            if (has_bp) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QColor(180, 40, 40));
                painter.setBrush(QColor(224, 108, 117)); // #e06c75
                painter.drawEllipse(QPoint(10, centerY), 6, 6);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            // 2. Draw Current PC Execution Indicator (Yellow/Amber Arrow)
            if (is_pc) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                if (has_bp) {
                    // Composite Marker: Inner arrow inside the breakpoint circle
                    painter.setPen(Qt::white);
                    painter.setBrush(QColor(229, 192, 123)); // #e5c07b
                    QPolygon arrow;
                    arrow << QPoint(8, centerY - 4)
                          << QPoint(13, centerY)
                          << QPoint(8, centerY + 4);
                    painter.drawPolygon(arrow);
                } else {
                    // Standalone execution pointer (Golden amber arrow)
                    painter.setPen(QColor(180, 140, 20));
                    painter.setBrush(QColor(229, 192, 123));
                    QPolygon arrow;
                    arrow << QPoint(5, centerY - 6)
                          << QPoint(15, centerY)
                          << QPoint(5, centerY + 6);
                    painter.drawPolygon(arrow);
                }
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            // 3. Draw Line Number
            const QString number = QString::number(block_number + 1);
            if (block_number == current_cursor_block) {
                painter.setPen(QColor(220, 220, 220)); // Active line number bright
                QFont bold_f = font();
                bold_f.setBold(true);
                painter.setFont(bold_f);
            } else {
                painter.setPen(QColor(92, 99, 112)); // Muted line number (#5c6370)
                painter.setFont(font());
            }

            painter.drawText(0, top, line_number_area_->width() - 6, font_height,
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++block_number;
    }
}

void CodeEditor::lineNumberAreaMousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const int font_height = fontMetrics().height();
    if (font_height <= 0) return;

    QTextBlock block = firstVisibleBlock();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid()) {
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

void CodeEditor::lineNumberAreaContextMenuEvent(QContextMenuEvent* event) {
    const int font_height = fontMetrics().height();
    if (font_height <= 0) return;

    QTextBlock block = firstVisibleBlock();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    std::size_t target_line = 0;
    while (block.isValid()) {
        if (event->pos().y() >= top && event->pos().y() <= bottom) {
            target_line = static_cast<std::size_t>(block.blockNumber() + 1);
            break;
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
    }

    if (target_line == 0) return;

    QMenu menu(this);
    const bool has_bp = active_breakpoint_lines_.count(target_line) > 0;
    const QString toggle_text = has_bp ? QString("Remove Breakpoint on Line %1").arg(target_line)
                                       : QString("Set Breakpoint on Line %1 (F9)").arg(target_line);

    QAction* toggle_act = menu.addAction(toggle_text);
    QAction* clear_all_act = nullptr;
    if (!active_breakpoint_lines_.empty()) {
        menu.addSeparator();
        clear_all_act = menu.addAction("Clear All Breakpoints");
    }

    QAction* selected = menu.exec(event->globalPos());
    if (selected == toggle_act && controller_ != nullptr) {
        auto res = controller_->toggleBreakpoint(target_line);
        if (!res.success) {
            QToolTip::showText(event->globalPos(),
                               QString::fromStdString(res.message),
                               line_number_area_);
        }
    } else if (clear_all_act != nullptr && selected == clear_all_act && controller_ != nullptr) {
        controller_->clearAllBreakpoints();
    }
}

void CodeEditor::contextMenuEvent(QContextMenuEvent* event) {
    QMenu* menu = createStandardContextMenu();
    menu->addSeparator();

    const std::size_t cursor_line = currentCursorLine();
    const bool has_bp = active_breakpoint_lines_.count(cursor_line) > 0;
    const QString bp_text = has_bp ? QString("Remove Breakpoint on Line %1").arg(cursor_line)
                                   : QString("Toggle Breakpoint on Line %1 (F9)").arg(cursor_line);
    QAction* bp_act = menu->addAction(bp_text);

    QAction* clear_all_act = nullptr;
    if (!active_breakpoint_lines_.empty()) {
        clear_all_act = menu->addAction("Clear All Breakpoints");
    }

    menu->addSeparator();
    QAction* zoom_in_act = menu->addAction("Zoom In (Ctrl++)");
    QAction* zoom_out_act = menu->addAction("Zoom Out (Ctrl+-)");
    QAction* reset_zoom_act = menu->addAction("Reset Zoom (Ctrl+0)");

    QAction* selected = menu->exec(event->globalPos());
    if (selected == bp_act) {
        toggleBreakpointAtCursor();
    } else if (clear_all_act != nullptr && selected == clear_all_act && controller_ != nullptr) {
        controller_->clearAllBreakpoints();
    } else if (selected == zoom_in_act) {
        zoomIn(1);
    } else if (selected == zoom_out_act) {
        zoomOut(1);
    } else if (selected == reset_zoom_act) {
        resetZoom();
    }
    delete menu;
}

std::size_t CodeEditor::currentCursorLine() const noexcept {
    return static_cast<std::size_t>(textCursor().blockNumber() + 1);
}

void CodeEditor::toggleBreakpointAtCursor() {
    if (controller_ == nullptr) return;
    const std::size_t line = currentCursorLine();
    auto res = controller_->toggleBreakpoint(line);
    if (!res.success) {
        QToolTip::showText(mapToGlobal(cursorRect().topRight()),
                           QString::fromStdString(res.message),
                           this);
    }
}

} // namespace bitforge::gui
