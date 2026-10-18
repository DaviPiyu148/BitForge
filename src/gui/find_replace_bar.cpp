#include "bitforge/gui/find_replace_bar.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>

namespace bitforge::gui {

FindReplaceBar::FindReplaceBar(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(6);

    setStyleSheet(
        "FindReplaceBar { background-color: #21252b; border-bottom: 1px solid #3b4048; }"
    );

    auto* search_label = new QLabel("Find:", this);
    search_label->setStyleSheet("QLabel { color: #4ec9b0; font-weight: bold; }");
    layout->addWidget(search_label);

    search_input_ = new QLineEdit(this);
    search_input_->setPlaceholderText("Search in file (Enter for next, Shift+Enter for prev)");
    search_input_->setClearButtonEnabled(true);
    search_input_->setMinimumWidth(240);
    layout->addWidget(search_input_);

    case_checkbox_ = new QCheckBox("Match Case", this);
    layout->addWidget(case_checkbox_);

    prev_btn_ = new QPushButton("▲ Prev", this);
    prev_btn_->setToolTip("Find Previous (Shift+F3)");
    layout->addWidget(prev_btn_);

    next_btn_ = new QPushButton("▼ Next", this);
    next_btn_->setToolTip("Find Next (F3 / Enter)");
    layout->addWidget(next_btn_);

    match_label_ = new QLabel(this);
    match_label_->setStyleSheet("QLabel { color: #abb2bf; font-size: 11px; margin-left: 4px; }");
    layout->addWidget(match_label_);

    layout->addStretch();

    close_btn_ = new QPushButton("✕", this);
    close_btn_->setToolTip("Close Find Bar (Escape)");
    close_btn_->setFixedSize(22, 22);
    close_btn_->setStyleSheet(
        "QPushButton { background: transparent; border: none; font-weight: bold; color: #abb2bf; }"
        "QPushButton:hover { color: #e06c75; background: #282c34; border-radius: 3px; }"
    );
    layout->addWidget(close_btn_);

    // Wire events
    connect(next_btn_, &QPushButton::clicked, this, [this]() {
        if (!search_input_->text().isEmpty()) {
            emit findNext(search_input_->text(), case_checkbox_->isChecked());
        }
    });

    connect(prev_btn_, &QPushButton::clicked, this, [this]() {
        if (!search_input_->text().isEmpty()) {
            emit findPrevious(search_input_->text(), case_checkbox_->isChecked());
        }
    });

    connect(search_input_, &QLineEdit::returnPressed, this, [this]() {
        if (!search_input_->text().isEmpty()) {
            emit findNext(search_input_->text(), case_checkbox_->isChecked());
        }
    });

    connect(search_input_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!text.isEmpty()) {
            emit findNext(text, case_checkbox_->isChecked());
        } else {
            clearMatchStatus();
        }
    });

    connect(case_checkbox_, &QCheckBox::toggled, this, [this]() {
        if (!search_input_->text().isEmpty()) {
            emit findNext(search_input_->text(), case_checkbox_->isChecked());
        }
    });

    connect(close_btn_, &QPushButton::clicked, this, [this]() {
        hide();
        emit closed();
    });

    hide();
}

void FindReplaceBar::showAndFocus() {
    show();
    search_input_->setFocus();
    search_input_->selectAll();
}

void FindReplaceBar::triggerFindNext() {
    if (!search_input_->text().isEmpty()) {
        emit findNext(search_input_->text(), case_checkbox_->isChecked());
    }
}

void FindReplaceBar::triggerFindPrevious() {
    if (!search_input_->text().isEmpty()) {
        emit findPrevious(search_input_->text(), case_checkbox_->isChecked());
    }
}

void FindReplaceBar::setMatchStatus(int current, int total) {
    if (total == 0) {
        match_label_->setText("No matches");
        match_label_->setStyleSheet("QLabel { color: #e06c75; font-size: 11px; font-weight: bold; }");
    } else {
        match_label_->setText(QString("%1 of %2").arg(current).arg(total));
        match_label_->setStyleSheet("QLabel { color: #4ec9b0; font-size: 11px; }");
    }
}

void FindReplaceBar::clearMatchStatus() {
    match_label_->clear();
}

QString FindReplaceBar::query() const {
    return search_input_->text();
}

bool FindReplaceBar::isCaseSensitive() const {
    return case_checkbox_->isChecked();
}

void FindReplaceBar::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit closed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace bitforge::gui
