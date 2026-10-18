#include "bitforge/gui/theme.hpp"

namespace bitforge::gui::theme {

QFont monoFont(int pointSize, bool bold) {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (pointSize > 0) {
        font.setPointSize(pointSize);
    }
    font.setBold(bold);
    return font;
}

QFont uiFont(int pointSize, bool bold) {
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    if (pointSize > 0) {
        font.setPointSize(pointSize);
    }
    font.setBold(bold);
    return font;
}

QString applicationStyleSheet() {
    return QString(R"(
        /* Global App Surface */
        QMainWindow, QWidget#CentralWidget {
            background-color: #181a1f;
            color: #dcdcdc;
        }

        QWidget {
            background-color: #21252b;
            color: #dcdcdc;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Helvetica Neue", sans-serif;
            font-size: 12px;
        }

        /* Menu Bar & Menus */
        QMenuBar {
            background-color: #181a1f;
            color: #abb2bf;
            border-bottom: 1px solid #282c34;
            padding: 2px 6px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #282c34;
            color: #ffffff;
        }
        QMenu {
            background-color: #21252b;
            color: #dcdcdc;
            border: 1px solid #3b4048;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #2c313a;
            color: #4ec9b0;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3b4048;
            margin: 4px 8px;
        }

        /* Primary Toolbar */
        QToolBar#MainToolBar {
            background-color: #21252b;
            border-bottom: 1px solid #282c34;
            spacing: 4px;
            padding: 4px 8px;
        }
        QToolButton {
            background-color: #282c34;
            color: #dcdcdc;
            border: 1px solid #3b4048;
            border-radius: 4px;
            padding: 5px 10px;
            font-weight: 500;
        }
        QToolButton:hover {
            background-color: #3e4451;
            border-color: #4ec9b0;
            color: #ffffff;
        }
        QToolButton:focus {
            border: 1px solid #4ec9b0;
        }
        QToolButton:pressed {
            background-color: #181a1f;
        }
        QToolButton:disabled {
            background-color: #21252b;
            color: #5c6370;
            border-color: #2c313a;
        }

        /* Dock Widgets */
        QDockWidget {
            color: #dcdcdc;
            font-weight: bold;
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
        }
        QDockWidget::title {
            background-color: #1b1d23;
            text-align: left;
            padding: 6px 10px;
            border-bottom: 1px solid #282c34;
            font-weight: 600;
            color: #abb2bf;
        }

        /* Tabs (Modern Flat Dark) */
        QTabWidget::pane {
            border: 1px solid #282c34;
            background-color: #21252b;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #181a1f;
            color: #abb2bf;
            border: 1px solid #282c34;
            border-bottom: none;
            padding: 5px 8px 5px 12px;
            margin-right: 3px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            font-size: 11px;
            font-weight: 500;
            min-height: 22px;
        }
        QTabBar::tab:selected {
            background-color: #21252b;
            color: #ffffff;
            border-top: 2px solid #4ec9b0;
            font-weight: 600;
        }
        QTabBar::tab:hover:!selected {
            background-color: #282c34;
            color: #dcdcdc;
        }
        QTabBar::close-button {
            image: url(:/icons/close-tab.png);
            subcontrol-position: right;
            subcontrol-origin: padding;
            width: 18px;
            height: 18px;
            margin-top: 1px;
            margin-bottom: 1px;
            margin-right: 4px;
            margin-left: 6px;
            border-radius: 3px;
        }
        QTabBar::close-button:hover {
            image: url(:/icons/close-tab-hover.png);
            background-color: #3e4451;
        }
        QTabBar::close-button:pressed {
            image: url(:/icons/close-tab-pressed.png);
            background-color: #e06c75;
        }

        /* Text Editors & Console Views */
        QPlainTextEdit, QTextEdit {
            background-color: #1e1e1e;
            color: #dcdcdc;
            border: 1px solid #282c34;
            selection-background-color: #264f78;
            selection-color: #ffffff;
        }
        QPlainTextEdit:focus, QTextEdit:focus {
            border: 1px solid #4ec9b0;
        }

        /* Inputs: QLineEdit, QComboBox */
        QLineEdit, QComboBox {
            background-color: #1e1e1e;
            color: #dcdcdc;
            border: 1px solid #3b4048;
            border-radius: 4px;
            padding: 4px 8px;
            min-height: 20px;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #4ec9b0;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid #3b4048;
        }
        QComboBox QAbstractItemView {
            background-color: #21252b;
            border: 1px solid #3b4048;
            selection-background-color: #2c313a;
            selection-color: #4ec9b0;
            padding: 4px;
        }

        /* Inputs: QSpinBox */
        QSpinBox {
            background-color: #1e1e1e;
            color: #dcdcdc;
            border: 1px solid #3b4048;
            border-radius: 4px;
            padding-top: 3px;
            padding-bottom: 3px;
            padding-left: 6px;
            padding-right: 22px;
            min-height: 20px;
        }
        QSpinBox:focus {
            border: 1px solid #4ec9b0;
        }
        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 18px;
            border-left: 1px solid #3b4048;
            border-bottom: 1px solid #3b4048;
            border-top-right-radius: 3px;
            background-color: #282c34;
        }
        QSpinBox::up-button:hover {
            background-color: #3e4451;
        }
        QSpinBox::up-button:pressed {
            background-color: #1b1d23;
        }
        QSpinBox::up-arrow {
            image: url(:/icons/spin-up.png);
            width: 8px;
            height: 6px;
        }
        QSpinBox::up-arrow:hover {
            image: url(:/icons/spin-up-hover.png);
        }
        QSpinBox::up-arrow:disabled, QSpinBox::up-arrow:off {
            image: none;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 18px;
            border-left: 1px solid #3b4048;
            border-bottom-right-radius: 3px;
            background-color: #282c34;
        }
        QSpinBox::down-button:hover {
            background-color: #3e4451;
        }
        QSpinBox::down-button:pressed {
            background-color: #1b1d23;
        }
        QSpinBox::down-arrow {
            image: url(:/icons/spin-down.png);
            width: 8px;
            height: 6px;
        }
        QSpinBox::down-arrow:hover {
            image: url(:/icons/spin-down-hover.png);
        }
        QSpinBox::down-arrow:disabled, QSpinBox::down-arrow:off {
            image: none;
        }

        /* Buttons */
        QPushButton {
            background-color: #282c34;
            color: #dcdcdc;
            border: 1px solid #3b4048;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #3e4451;
            border-color: #4ec9b0;
            color: #ffffff;
        }
        QPushButton:focus {
            border: 1px solid #4ec9b0;
        }
        QPushButton:pressed {
            background-color: #1b1d23;
        }
        QPushButton:disabled {
            background-color: #21252b;
            color: #5c6370;
            border-color: #282c34;
        }

        /* Tables */
        QTableWidget, QTableView {
            background-color: #1e1e1e;
            color: #dcdcdc;
            gridline-color: #282c34;
            border: 1px solid #282c34;
            selection-background-color: #264f78;
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #181a1f;
            color: #abb2bf;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid #3b4048;
            border-right: 1px solid #282c34;
            font-weight: 600;
            font-size: 11px;
        }

        /* Lists & Trees */
        QListWidget, QListView, QTreeWidget, QTreeView {
            background-color: #1e1e1e;
            color: #dcdcdc;
            border: 1px solid #282c34;
            selection-background-color: #264f78;
            selection-color: #ffffff;
            outline: none;
        }
        QListWidget::item, QListView::item, QTreeWidget::item, QTreeView::item {
            padding: 4px 8px;
            border-radius: 3px;
        }
        QListWidget::item:hover, QListView::item:hover, QTreeWidget::item:hover, QTreeView::item:hover {
            background-color: #282c34;
            color: #ffffff;
        }
        QListWidget::item:selected, QListView::item:selected, QTreeWidget::item:selected, QTreeView::item:selected {
            background-color: #264f78;
            color: #ffffff;
        }

        /* Group Boxes */
        QGroupBox {
            border: 1px solid #3b4048;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: 600;
            color: #abb2bf;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 4px;
            background-color: #21252b;
            color: #4ec9b0;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background-color: #181a1f;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background-color: #3e4451;
            min-height: 20px;
            border-radius: 5px;
            margin: 1px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4ec9b0;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #181a1f;
            height: 10px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background-color: #3e4451;
            min-width: 20px;
            border-radius: 5px;
            margin: 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4ec9b0;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* Status Bar */
        QStatusBar {
            background-color: #181a1f;
            color: #abb2bf;
            border-top: 1px solid #282c34;
            font-size: 11px;
        }
        QStatusBar QLabel {
            background-color: transparent;
            padding: 2px 6px;
        }

        /* Tooltips */
        QToolTip {
            background-color: #181a1f;
            color: #ffffff;
            border: 1px solid #4ec9b0;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 11px;
        }

        /* Splitter */
        QSplitter::handle {
            background-color: #181a1f;
        }
    )");
}

QString stateBadgeStyle(bitforge::MachineState state, bool is_executing, bool is_stale) {
    if (is_executing) {
        return "QLabel { background-color: #007acc; color: #ffffff; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
    }
    if (is_stale) {
        return "QLabel { background-color: #d19a66; color: #181a1f; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
    }
    switch (state) {
        case MachineState::Ready:
            return "QLabel { background-color: #28a745; color: #ffffff; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
        case MachineState::Running:
        case MachineState::Paused:
            return "QLabel { background-color: #e5c07b; color: #181a1f; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
        case MachineState::Halted:
            return "QLabel { background-color: #495057; color: #dcdcdc; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
        case MachineState::Faulted:
            return "QLabel { background-color: #dc3545; color: #ffffff; font-weight: bold; border-radius: 4px; padding: 3px 10px; font-size: 11px; }";
    }
    return "QLabel { background-color: #495057; color: #dcdcdc; border-radius: 4px; padding: 3px 10px; }";
}

QString flagBadgeStyle(bool active) {
    if (active) {
        return "QLabel { background-color: #28a745; color: #ffffff; font-weight: bold; border-radius: 3px; padding: 2px 7px; min-width: 14px; }";
    }
    return "QLabel { background-color: #2c313a; color: #5c6370; font-weight: normal; border: 1px solid #3b4048; border-radius: 3px; padding: 2px 7px; min-width: 14px; }";
}

QString changedRegisterStyle(bool changed) {
    if (changed) {
        return "QLabel { background-color: #3e3520; color: #ffd54f; font-weight: bold; border: 1px solid #b28900; border-radius: 3px; padding: 1px 5px; }";
    }
    return "QLabel { background-color: transparent; color: #dcdcdc; font-weight: normal; border: 1px solid transparent; padding: 1px 5px; }";
}

QString changedMemoryCellStyle() {
    return "background-color: #3e3520; color: #ffd54f; font-weight: bold;";
}

QString emptyStateStyle() {
    return "QLabel { color: #7f848e; font-style: italic; font-size: 12px; padding: 16px; background-color: transparent; }";
}

} // namespace bitforge::gui::theme
