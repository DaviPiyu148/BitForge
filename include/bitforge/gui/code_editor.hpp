#ifndef BITFORGE_GUI_CODE_EDITOR_HPP
#define BITFORGE_GUI_CODE_EDITOR_HPP

#include "bitforge/gui/line_number_area.hpp"
#include "bitforge/gui/asm_highlighter.hpp"

#include <QPlainTextEdit>
#include <set>
#include <cstddef>

namespace bitforge::gui {

class ExecutionController;

// Source code editor with line numbers, breakpoint gutter, execution indicators, syntax highlighting, and zoom
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor() override = default;

    void setController(ExecutionController* controller);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    void lineNumberAreaMousePressEvent(QMouseEvent* event);
    void lineNumberAreaContextMenuEvent(QContextMenuEvent* event);
    [[nodiscard]] int lineNumberAreaWidth() const;

    void setCurrentPcLine(std::size_t line);
    void clearCurrentPcLine();
    [[nodiscard]] std::size_t currentPcLine() const noexcept { return current_pc_line_; }

    void gotoLine(std::size_t line, std::size_t column = 1);
    void toggleBreakpointAtCursor();
    [[nodiscard]] std::size_t currentCursorLine() const noexcept;

    // Zooming
    void zoomIn(int delta = 1);
    void zoomOut(int delta = 1);
    void resetZoom();
    void setFontSize(int size);
    [[nodiscard]] int currentFontSize() const noexcept { return current_font_size_; }

    // Search
    bool findText(const QString& text, bool forward, bool caseSensitive, int* matchIdx = nullptr, int* totalMatches = nullptr);
    void clearFindMatches();

signals:
    void cursorLineColumnChanged(int line, int column);

public slots:
    void updateBreakpoints();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightLines();
    void onCursorPositionChanged();

private:
    LineNumberArea* line_number_area_;
    AsmHighlighter* highlighter_;
    ExecutionController* controller_{nullptr};

    std::size_t current_pc_line_{0};
    std::set<std::size_t> active_breakpoint_lines_;
    int default_font_size_{10};
    int current_font_size_{10};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_CODE_EDITOR_HPP
