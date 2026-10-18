#ifndef BITFORGE_GUI_FIND_REPLACE_BAR_HPP
#define BITFORGE_GUI_FIND_REPLACE_BAR_HPP

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

namespace bitforge::gui {

class FindReplaceBar : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceBar(QWidget* parent = nullptr);
    ~FindReplaceBar() override = default;

    void showAndFocus();
    void setMatchStatus(int current, int total);
    void clearMatchStatus();
    [[nodiscard]] QString query() const;
    [[nodiscard]] bool isCaseSensitive() const;

    [[nodiscard]] QLineEdit* searchInput() const noexcept { return search_input_; }
    [[nodiscard]] QPushButton* nextButton() const noexcept { return next_btn_; }
    [[nodiscard]] QPushButton* prevButton() const noexcept { return prev_btn_; }
    [[nodiscard]] QPushButton* closeButton() const noexcept { return close_btn_; }
    [[nodiscard]] QCheckBox* caseCheckbox() const noexcept { return case_checkbox_; }
    [[nodiscard]] QLabel* matchLabel() const noexcept { return match_label_; }

    void triggerFindNext();
    void triggerFindPrevious();

signals:
    void findNext(const QString& text, bool caseSensitive);
    void findPrevious(const QString& text, bool caseSensitive);
    void closed();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLineEdit* search_input_;
    QCheckBox* case_checkbox_;
    QLabel* match_label_;
    QPushButton* prev_btn_;
    QPushButton* next_btn_;
    QPushButton* close_btn_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_FIND_REPLACE_BAR_HPP
