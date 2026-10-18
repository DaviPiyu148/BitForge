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

        /* Progress Bar */
        QProgressBar {
            background-color: #282c34;
            border: 1px solid #3b4048;
            border-radius: 3px;
            text-align: center;
            color: #dcdcdc;
            font-size: 10px;
        }
        QProgressBar::chunk {
            background-color: #4ec9b0;
            border-radius: 2px;
        }

        /* Splitter */
        QSplitter::handle {
            background-color: #181a1f;
        }
    )");
}

QString primaryButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #4ec9b0;"
        "  color: #181a1f;"
        "  font-weight: bold;"
        "  border: 1px solid #4ec9b0;"
        "  border-radius: %1px;"
        "  padding: 6px 14px;"
        "  min-height: %2px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #5cd8be;"
        "  border-color: #5cd8be;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #3ba28c;"
        "  border-color: #3ba28c;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #21252b;"
        "  color: #7f848e;"
        "  border-color: #2c313a;"
        "}"
    ).arg(kRadiusControl).arg(kControlMinHeight);
}

QString secondaryButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #282c34;"
        "  color: #dcdcdc;"
        "  border: 1px solid #3b4048;"
        "  border-radius: %1px;"
        "  padding: 5px 12px;"
        "  min-height: %2px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3e4451;"
        "  border-color: #4ec9b0;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b1d23;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #21252b;"
        "  color: #7f848e;"
        "  border-color: #2c313a;"
        "}"
    ).arg(kRadiusControl).arg(kControlMinHeight);
}

QString accentButtonStyle(const QColor& accentColor) {
    return QString(
        "QPushButton {"
        "  background-color: #282c34;"
        "  color: %1;"
        "  border: 1px solid #3b4048;"
        "  border-radius: %2px;"
        "  padding: 5px 12px;"
        "  min-height: %3px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3e4451;"
        "  border-color: %1;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b1d23;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #21252b;"
        "  color: #7f848e;"
        "  border-color: #2c313a;"
        "}"
    ).arg(accentColor.name()).arg(kRadiusControl).arg(kControlMinHeight);
}

QString dangerButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #282c34;"
        "  color: #e06c75;"
        "  border: 1px solid #e06c75;"
        "  border-radius: %1px;"
        "  padding: 5px 12px;"
        "  min-height: %2px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3e2023;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #181a1f;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #21252b;"
        "  color: #7f848e;"
        "  border-color: #2c313a;"
        "}"
    ).arg(kRadiusControl).arg(kControlMinHeight);
}

QString compactButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #282c34;"
        "  color: #dcdcdc;"
        "  border: 1px solid #3b4048;"
        "  border-radius: %1px;"
        "  padding: 3px 8px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3e4451;"
        "  border-color: #4ec9b0;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b1d23;"
        "}"
    ).arg(kRadiusSmall);
}

QString inputStyle(bool has_error) {
    if (has_error) {
        return QString(
            "QLineEdit {"
            "  background-color: #2e2024;"
            "  color: #ffffff;"
            "  border: 1px solid #e06c75;"
            "  border-radius: %1px;"
            "  padding: 4px 8px;"
            "  min-height: %2px;"
            "}"
        ).arg(kRadiusControl).arg(kControlMinHeight);
    }
    return QString(
        "QLineEdit {"
        "  background-color: #1e1e1e;"
        "  color: #dcdcdc;"
        "  border: 1px solid #3b4048;"
        "  border-radius: %1px;"
        "  padding: 4px 8px;"
        "  min-height: %2px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #4ec9b0;"
        "}"
    ).arg(kRadiusControl).arg(kControlMinHeight);
}

QString infoBannerStyle(const QColor& bg, const QColor& text, const QColor& border) {
    return QString(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: 6px 10px;"
        "  font-size: 11px;"
        "}"
    ).arg(bg.name(), text.name(), border.name()).arg(kRadiusPanel);
}

QString statusBannerStyle(const QColor& bg, const QColor& text, const QColor& border, bool bold) {
    return QString(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: 6px 10px;"
        "  font-size: 11px;"
        "  %5"
        "}"
    ).arg(bg.name(), text.name(), border.name())
     .arg(kRadiusControl)
     .arg(bold ? "font-weight: bold;" : "");
}

QString welcomeActionCardStyle(const QString& customBg) {
    const QString bg = customBg.isEmpty() ? "#282c34" : customBg;
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid #3b4048;"
        "  border-radius: %2px;"
        "  text-align: left;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3e4451;"
        "  border-color: #4ec9b0;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #181a1f;"
        "}"
    ).arg(bg).arg(kRadiusLarge);
}

QString stateBadgeStyle(bitforge::MachineState state, bool is_executing, bool is_stale) {
    QString bg;
    QString fg = "#ffffff";
    if (is_executing) {
        bg = kStateRunning.name();
        fg = "#181a1f";
    } else if (is_stale) {
        bg = kStateStale.name();
        fg = "#181a1f";
    } else {
        switch (state) {
            case MachineState::Ready:
                bg = kStateReady.name();
                fg = "#181a1f";
                break;
            case MachineState::Running:
                bg = kStateRunning.name();
                fg = "#181a1f";
                break;
            case MachineState::Paused:
                bg = kStatePaused.name();
                fg = "#181a1f";
                break;
            case MachineState::Halted:
                bg = kStateHalted.name();
                fg = "#dcdcdc";
                break;
            case MachineState::Faulted:
                bg = kStateFaulted.name();
                fg = "#ffffff";
                break;
        }
    }
    return QString(
        "QLabel { background-color: %1; color: %2; font-weight: bold; border-radius: %3px; padding: 3px 10px; font-size: 11px; }"
    ).arg(bg, fg).arg(kRadiusPill);
}

QString flagBadgeStyle(bool active) {
    if (active) {
        return QString("QLabel { background-color: %1; color: #181a1f; font-weight: bold; border-radius: %2px; padding: 2px 7px; min-width: 14px; }")
            .arg(kStateReady.name()).arg(kRadiusSmall);
    }
    return QString("QLabel { background-color: #2c313a; color: %1; font-weight: normal; border: 1px solid #3b4048; border-radius: %2px; padding: 2px 7px; min-width: 14px; }")
        .arg(kTextMuted.name()).arg(kRadiusSmall);
}

QString changedRegisterStyle(bool changed) {
    if (changed) {
        return QString("QLabel { background-color: %1; color: %2; font-weight: bold; border: 1px solid %3; border-radius: %4px; padding: 1px 5px; }")
            .arg(kChangedBg.name(), kChangedText.name(), kChangedBorder.name()).arg(kRadiusSmall);
    }
    return "QLabel { background-color: transparent; color: #dcdcdc; font-weight: normal; border: 1px solid transparent; padding: 1px 5px; }";
}

QString changedMemoryCellStyle() {
    return QString("background-color: %1; color: %2; font-weight: bold;")
        .arg(kChangedBg.name(), kChangedText.name());
}

QString emptyStateStyle(bool compact) {
    const int padding = compact ? 4 : 16;
    return QString("QLabel { color: %1; font-style: italic; font-size: 11px; padding: %2px; background-color: transparent; }")
        .arg(kTextMuted.name()).arg(padding);
}

} // namespace bitforge::gui::theme
