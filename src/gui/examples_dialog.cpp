#include "bitforge/gui/examples_dialog.hpp"
#include "bitforge/gui/asm_highlighter.hpp"
#include "bitforge/gui/theme.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <set>

namespace bitforge::gui {

namespace {

QString resolveExamplePath(const QString& relPath) {
    if (QFile::exists(relPath)) {
        return relPath;
    }
    QString appRel = QDir(QCoreApplication::applicationDirPath()).filePath(relPath);
    if (QFile::exists(appRel)) {
        return appRel;
    }
    QString repoRel = QDir(QCoreApplication::applicationDirPath() + "/../../").filePath(relPath);
    if (QFile::exists(repoRel)) {
        return QDir::cleanPath(repoRel);
    }
    return relPath;
}

QString kindBadge(ExampleKind kind) {
    switch (kind) {
        case ExampleKind::Runnable:        return "[Runnable]";
        case ExampleKind::DebuggerDemo:    return "[Debugger Demo]";
        case ExampleKind::RuntimeFault:    return "[Runtime Fault]";
        case ExampleKind::InvalidAssembly: return "[Invalid Syntax]";
        default:                           return "[Program]";
    }
}

QColor kindColor(ExampleKind kind) {
    switch (kind) {
        case ExampleKind::Runnable:        return theme::kStateReady;
        case ExampleKind::DebuggerDemo:    return theme::kAccentTeal;
        case ExampleKind::RuntimeFault:    return theme::kStateFaulted;
        case ExampleKind::InvalidAssembly: return theme::kStateStale;
        default:                           return theme::kTextSecondary;
    }
}

} // namespace

ExamplesDialog::ExamplesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Browse Built-in Assembly Examples");
    resize(1080, 680);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(10);

    auto* header_label = new QLabel("Select an assembly program from the 51-file catalog to inspect, assemble, and execute in the workbench:", this);
    header_label->setStyleSheet("QLabel { font-weight: bold; color: #4ec9b0; font-size: 13px; }");
    main_layout->addWidget(header_label);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left pane: Categories
    auto* cat_container = new QWidget(splitter);
    auto* cat_layout = new QVBoxLayout(cat_container);
    cat_layout->setContentsMargins(0, 0, 0, 0);
    cat_layout->setSpacing(6);
    cat_layout->addWidget(new QLabel("Categories:", cat_container));

    category_list_ = new QListWidget(cat_container);
    cat_layout->addWidget(category_list_);
    splitter->addWidget(cat_container);

    // Center pane: Search and Programs
    auto* ex_container = new QWidget(splitter);
    auto* ex_layout = new QVBoxLayout(ex_container);
    ex_layout->setContentsMargins(0, 0, 0, 0);
    ex_layout->setSpacing(6);

    auto* search_row = new QHBoxLayout();
    search_row->addWidget(new QLabel("Search:", ex_container));
    search_input_ = new QLineEdit(ex_container);
    search_input_->setPlaceholderText("Filter by name, mnemonic, or keywords...");
    search_input_->setClearButtonEnabled(true);
    search_row->addWidget(search_input_);
    ex_layout->addLayout(search_row);

    example_list_ = new QListWidget(ex_container);
    ex_layout->addWidget(example_list_);
    splitter->addWidget(ex_container);

    // Right pane: Code Preview & Metadata
    auto* prev_container = new QWidget(splitter);
    auto* prev_layout = new QVBoxLayout(prev_container);
    prev_layout->setContentsMargins(0, 0, 0, 0);
    prev_layout->setSpacing(6);

    info_label_ = new QLabel("Program Preview:", prev_container);
    info_label_->setStyleSheet(QString("QLabel { font-weight: bold; font-size: 13px; color: %1; }").arg(theme::kAccentBlue.name()));
    prev_layout->addWidget(info_label_);

    details_banner_ = new QLabel("", prev_container);
    details_banner_->setTextFormat(Qt::RichText);
    details_banner_->setStyleSheet(theme::infoBannerStyle(theme::kBgDark, theme::kTextSecondary, theme::kBorderMedium));
    details_banner_->setWordWrap(true);
    prev_layout->addWidget(details_banner_);

    preview_edit_ = new QPlainTextEdit(prev_container);
    preview_edit_->setReadOnly(true);
    preview_edit_->setFont(theme::monoFont(10));
    new AsmHighlighter(preview_edit_->document());
    prev_layout->addWidget(preview_edit_);
    splitter->addWidget(prev_container);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 3);
    main_layout->addWidget(splitter, 1);

    // Button bar
    auto* btn_bar = new QHBoxLayout();
    btn_bar->addStretch();

    open_btn_ = new QPushButton("Open in Editor", this);
    open_btn_->setStyleSheet(theme::primaryButtonStyle());
    cancel_btn_ = new QPushButton("Cancel", this);

    btn_bar->addWidget(cancel_btn_);
    btn_bar->addWidget(open_btn_);
    main_layout->addLayout(btn_bar);

    connect(category_list_, &QListWidget::currentRowChanged, this, &ExamplesDialog::onCategoryChanged);
    connect(search_input_, &QLineEdit::textChanged, this, &ExamplesDialog::onFilterChanged);
    connect(example_list_, &QListWidget::currentItemChanged, this, &ExamplesDialog::onExampleItemChanged);
    connect(example_list_, &QListWidget::itemDoubleClicked, this, &ExamplesDialog::onOpenClicked);
    connect(open_btn_, &QPushButton::clicked, this, &ExamplesDialog::onOpenClicked);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);

    populateCategories();
}

void ExamplesDialog::populateCategories() {
    category_list_->clear();
    category_list_->addItem("All Categories (51)");

    std::vector<QString> distinct_cats;
    for (const auto& entry : kExampleCatalog) {
        QString cat = QString::fromUtf8(entry.category.data(), static_cast<qsizetype>(entry.category.size()));
        bool exists = false;
        for (const auto& existing : distinct_cats) {
            if (existing == cat) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            distinct_cats.push_back(cat);
        }
    }

    for (const auto& cat : distinct_cats) {
        category_list_->addItem(cat);
    }

    if (category_list_->count() > 0) {
        category_list_->setCurrentRow(0);
    }
}

void ExamplesDialog::onCategoryChanged(int /* index */) {
    applyFilter();
}

void ExamplesDialog::onFilterChanged() {
    applyFilter();
}

void ExamplesDialog::applyFilter() {
    example_list_->clear();

    const QString selected_cat = category_list_->currentItem() ? category_list_->currentItem()->text() : "All Categories (51)";
    const bool all_cats = selected_cat.startsWith("All Categories");
    const QString query = search_input_->text().trimmed().toLower();

    for (std::size_t i = 0; i < kExampleCatalog.size(); ++i) {
        const auto& entry = kExampleCatalog[i];
        const QString cat = QString::fromUtf8(entry.category.data(), static_cast<qsizetype>(entry.category.size()));
        const QString title = QString::fromUtf8(entry.title.data(), static_cast<qsizetype>(entry.title.size()));
        const QString rel_path = QString::fromUtf8(entry.relative_path.data(), static_cast<qsizetype>(entry.relative_path.size()));
        const QString desc = QString::fromUtf8(entry.description.data(), static_cast<qsizetype>(entry.description.size()));
        const QString stop = QString::fromUtf8(entry.expected_stop.data(), static_cast<qsizetype>(entry.expected_stop.size()));
        const QString behavior = QString::fromUtf8(entry.expected_behavior.data(), static_cast<qsizetype>(entry.expected_behavior.size()));

        if (!all_cats && cat != selected_cat) {
            continue;
        }

        if (!query.isEmpty()) {
            bool matches = rel_path.toLower().contains(query) ||
                           title.toLower().contains(query) ||
                           desc.toLower().contains(query) ||
                           stop.toLower().contains(query) ||
                           behavior.toLower().contains(query);
            if (!matches) {
                continue;
            }
        }

        auto* item = new QListWidgetItem(QString("%1  %2").arg(kindBadge(entry.kind)).arg(title), example_list_);
        item->setForeground(kindColor(entry.kind));
        item->setData(Qt::UserRole, static_cast<int>(i));
    }

    if (example_list_->count() > 0) {
        example_list_->setCurrentRow(0);
        open_btn_->setEnabled(true);
    } else {
        preview_edit_->clear();
        info_label_->setText("Program Preview (No matching examples):");
        details_banner_->setText("No programs match the selected category and search filter.");
        open_btn_->setEnabled(false);
    }
}

void ExamplesDialog::onExampleItemChanged(QListWidgetItem* current, QListWidgetItem* /* previous */) {
    if (!current) {
        preview_edit_->clear();
        info_label_->setText("Program Preview:");
        details_banner_->setText("");
        open_btn_->setEnabled(false);
        return;
    }
    open_btn_->setEnabled(true);

    int cat_index = current->data(Qt::UserRole).toInt();
    if (cat_index < 0 || cat_index >= static_cast<int>(kExampleCatalog.size())) {
        return;
    }

    const auto& entry = kExampleCatalog[static_cast<std::size_t>(cat_index)];
    const QString title = QString::fromUtf8(entry.title.data(), static_cast<qsizetype>(entry.title.size()));
    const QString cat = QString::fromUtf8(entry.category.data(), static_cast<qsizetype>(entry.category.size()));
    const QString kind_str = QString::fromUtf8(to_string(entry.kind).data(), static_cast<qsizetype>(to_string(entry.kind).size()));
    const QString desc = QString::fromUtf8(entry.description.data(), static_cast<qsizetype>(entry.description.size()));
    const QString stop = QString::fromUtf8(entry.expected_stop.data(), static_cast<qsizetype>(entry.expected_stop.size()));
    const QString behavior = QString::fromUtf8(entry.expected_behavior.data(), static_cast<qsizetype>(entry.expected_behavior.size()));
    const QString bps = QString::fromUtf8(entry.recommended_breakpoints.data(), static_cast<qsizetype>(entry.recommended_breakpoints.size()));
    const QString rel_path = QString::fromUtf8(entry.relative_path.data(), static_cast<qsizetype>(entry.relative_path.size()));

    info_label_->setText(QString("<b>%1</b> (%2 — %3)").arg(title, cat, kind_str));

    QString details = QString("<b>Description:</b> %1<br>"
                              "<b>Expected Termination:</b> %2 | <b>Expected State / Behavior:</b> %3")
                          .arg(desc, stop, behavior);
    if (!bps.isEmpty()) {
        details += QString("<br><b>Recommended Breakpoints:</b> %1").arg(bps);
    }
    details_banner_->setText(details);

    loadExamplePreview(rel_path);
}

void ExamplesDialog::loadExamplePreview(const QString& relPath) {
    const QString full_path = resolveExamplePath(relPath);
    QFile file(full_path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        preview_edit_->setPlainText(in.readAll());
    } else {
        preview_edit_->setPlainText("; File not found: " + full_path);
    }
}

void ExamplesDialog::onOpenClicked() {
    auto* item = example_list_->currentItem();
    if (!item) return;

    int cat_index = item->data(Qt::UserRole).toInt();
    if (cat_index < 0 || cat_index >= static_cast<int>(kExampleCatalog.size())) {
        return;
    }

    const auto& entry = kExampleCatalog[static_cast<std::size_t>(cat_index)];
    const QString rel_path = QString::fromUtf8(entry.relative_path.data(), static_cast<qsizetype>(entry.relative_path.size()));
    selected_path_ = resolveExamplePath(rel_path);
    emit exampleSelected(selected_path_);
    accept();
}

} // namespace bitforge::gui
