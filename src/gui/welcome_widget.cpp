#include "bitforge/gui/welcome_widget.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>

namespace bitforge::gui {

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent) {
    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(32, 24, 32, 24);
    root_layout->setSpacing(16);

    // 1. Header Banner
    auto* header_container = new QWidget(this);
    auto* header_layout = new QVBoxLayout(header_container);
    header_layout->setContentsMargins(0, 0, 0, 0);
    header_layout->setSpacing(4);

    auto* title_label = new QLabel("⚡ BitForge Workbench", header_container);
    title_label->setStyleSheet("QLabel { font-size: 26px; font-weight: bold; color: #4ec9b0; }");
    header_layout->addWidget(title_label);

    auto* subtitle_label = new QLabel(
        "8-Bit Educational Computer Emulator, Two-Pass Assembler & Interactive Debugger",
        header_container
    );
    subtitle_label->setStyleSheet("QLabel { font-size: 13px; color: #abb2bf; }");
    header_layout->addWidget(subtitle_label);

    root_layout->addWidget(header_container);

    // 2. Main Content Splitter (Left: Action Cards, Right: Recent Files & Quick Reference)
    auto* content_layout = new QHBoxLayout();
    content_layout->setSpacing(20);

    // Left Column: Primary Actions
    auto* actions_box = new QGroupBox("Start Here", this);
    auto* actions_layout = new QVBoxLayout(actions_box);
    actions_layout->setContentsMargins(16, 16, 16, 16);
    actions_layout->setSpacing(10);

    auto makeActionBtn = [this](const QString& title, const QString& subtitle, const QString& shortcut, const QString& color = "#282c34") {
        auto* btn = new QPushButton(this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setMinimumHeight(54);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid #3b4048; border-radius: 6px; text-align: left; padding: 8px 14px; }"
            "QPushButton:hover { background-color: #3e4451; border-color: #4ec9b0; }"
            "QPushButton:pressed { background-color: #181a1f; }"
        ).arg(color));

        auto* btn_layout = new QVBoxLayout(btn);
        btn_layout->setContentsMargins(0, 0, 0, 0);
        btn_layout->setSpacing(2);

        auto* t_lbl = new QLabel(QString("<b>%1</b> <span style='color: #61afef; font-size: 11px;'>%2</span>").arg(title, shortcut), btn);
        t_lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        btn_layout->addWidget(t_lbl);

        auto* s_lbl = new QLabel(subtitle, btn);
        s_lbl->setStyleSheet("QLabel { color: #8a93a0; font-size: 11px; }");
        s_lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        btn_layout->addWidget(s_lbl);

        return btn;
    };

    auto* btn_new = makeActionBtn("New Assembly File", "Create a blank assembly program with .ENTRY template", "[Ctrl+N]");
    auto* btn_open = makeActionBtn("Open Assembly File...", "Load an existing .asm source file from disk", "[Ctrl+O]");
    auto* btn_examples = makeActionBtn("Browse Example Library", "Explore 51 categorized programs with live previews", "");
    auto* btn_tutorial = makeActionBtn("Interactive Debugging Tutorial", "Take a step-by-step 9-stage guided walkthrough", "", "#243336");
    auto* btn_editor = makeActionBtn("Switch to Code Editor", "Jump directly into the editing and debugging workbench", "[Esc]");

    actions_layout->addWidget(btn_new);
    actions_layout->addWidget(btn_open);
    actions_layout->addWidget(btn_examples);
    actions_layout->addWidget(btn_tutorial);
    actions_layout->addWidget(btn_editor);
    actions_layout->addStretch();

    content_layout->addWidget(actions_box, 3);

    // Right Column: Recent Files & Quick Shortcuts
    auto* right_col = new QVBoxLayout();
    right_col->setSpacing(12);

    // Recent Files
    auto* recent_box = new QGroupBox("Recent Files", this);
    auto* recent_layout = new QVBoxLayout(recent_box);
    recent_layout->setContentsMargins(12, 12, 12, 12);
    recent_layout->setSpacing(8);

    recent_list_ = new QListWidget(recent_box);
    recent_list_->setAlternatingRowColors(true);
    recent_layout->addWidget(recent_list_, 1);

    auto* recent_btn_bar = new QHBoxLayout();
    recent_btn_bar->addStretch();
    clear_recent_btn_ = new QPushButton("Clear Recent", recent_box);
    clear_recent_btn_->setStyleSheet("QPushButton { font-size: 11px; padding: 3px 8px; }");
    recent_btn_bar->addWidget(clear_recent_btn_);
    recent_layout->addLayout(recent_btn_bar);

    right_col->addWidget(recent_box, 3);

    // Quick Reference
    auto* ref_box = new QGroupBox("Quick Reference", this);
    auto* ref_layout = new QGridLayout(ref_box);
    ref_layout->setContentsMargins(12, 10, 12, 10);
    ref_layout->setSpacing(6);

    auto addRef = [&](int row, const QString& key, const QString& desc) {
        auto* k = new QLabel(QString("<b>%1</b>").arg(key), ref_box);
        k->setStyleSheet("QLabel { color: #4ec9b0; }");
        auto* d = new QLabel(desc, ref_box);
        d->setStyleSheet("QLabel { color: #abb2bf; font-size: 11px; }");
        ref_layout->addWidget(k, row, 0);
        ref_layout->addWidget(d, row, 1);
    };

    addRef(0, "F5", "Run from Entry (Fresh execution)");
    addRef(1, "F8", "Continue (Resume without reset)");
    addRef(2, "F10", "Step (Execute single instruction)");
    addRef(3, "F7", "Assemble (Compile active buffer)");
    addRef(4, "F9", "Toggle Breakpoint at cursor");
    addRef(5, "F6", "Reset CPU, stack, and peripherals");
    addRef(6, "Ctrl+F", "Find in file");

    right_col->addWidget(ref_box, 2);
    content_layout->addLayout(right_col, 4);

    root_layout->addLayout(content_layout, 1);

    // Connect button signals
    connect(btn_new, &QPushButton::clicked, this, &WelcomeWidget::newFileRequested);
    connect(btn_open, &QPushButton::clicked, this, &WelcomeWidget::openFileRequested);
    connect(btn_examples, &QPushButton::clicked, this, &WelcomeWidget::browseExamplesRequested);
    connect(btn_tutorial, &QPushButton::clicked, this, &WelcomeWidget::startTutorialRequested);
    connect(btn_editor, &QPushButton::clicked, this, &WelcomeWidget::openEditorRequested);

    connect(recent_list_, &QListWidget::itemDoubleClicked, this, &WelcomeWidget::onRecentItemDoubleClicked);
    connect(clear_recent_btn_, &QPushButton::clicked, this, &WelcomeWidget::onClearRecentClicked);
}

void WelcomeWidget::setRecentFiles(const QStringList& files) {
    recent_paths_.clear();
    recent_list_->clear();

    for (const auto& file : files) {
        if (file.trimmed().isEmpty()) continue;
        if (!QFile::exists(file)) continue;
        if (!recent_paths_.contains(file)) {
            recent_paths_.append(file);
        }
    }

    if (recent_paths_.empty()) {
        auto* item = new QListWidgetItem("No recent files yet. Open or create an assembly file to begin.", recent_list_);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor(108, 117, 125));
        clear_recent_btn_->setEnabled(false);
    } else {
        clear_recent_btn_->setEnabled(true);
        for (const auto& path : recent_paths_) {
            QFileInfo fi(path);
            auto* item = new QListWidgetItem(recent_list_);
            item->setText(fi.fileName());
            item->setToolTip(path);
            item->setData(Qt::UserRole, path);
        }
    }
}

QStringList WelcomeWidget::recentFiles() const {
    return recent_paths_;
}

void WelcomeWidget::onRecentItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    if (!QFile::exists(path)) {
        QMessageBox::warning(
            this, "File Not Found",
            QString("The file was not found at the specified path:\n%1\n\nIt will be removed from recent files.").arg(path)
        );
        recent_paths_.removeAll(path);
        setRecentFiles(recent_paths_);
        return;
    }

    emit openRecentRequested(path);
}

void WelcomeWidget::onClearRecentClicked() {
    recent_paths_.clear();
    setRecentFiles(recent_paths_);
}

} // namespace bitforge::gui
