#ifndef BITFORGE_GUI_EXAMPLES_DIALOG_HPP
#define BITFORGE_GUI_EXAMPLES_DIALOG_HPP

#include "bitforge/core/example_catalog.hpp"

#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

namespace bitforge::gui {

class ExamplesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExamplesDialog(QWidget* parent = nullptr);
    ~ExamplesDialog() override = default;

    [[nodiscard]] QString selectedExamplePath() const { return selected_path_; }

    [[nodiscard]] QListWidget* exampleList() const noexcept { return example_list_; }
    [[nodiscard]] QListWidget* categoryList() const noexcept { return category_list_; }
    [[nodiscard]] QLineEdit* searchInput() const noexcept { return search_input_; }
    [[nodiscard]] QLabel* infoLabel() const noexcept { return info_label_; }
    [[nodiscard]] QLabel* detailsBanner() const noexcept { return details_banner_; }
    [[nodiscard]] QPushButton* openButton() const noexcept { return open_btn_; }

signals:
    void exampleSelected(const QString& filePath);

private slots:
    void onCategoryChanged(int index);
    void onFilterChanged();
    void onExampleItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onOpenClicked();

private:
    void populateCategories();
    void applyFilter();
    void loadExamplePreview(const QString& relPath);

    QListWidget* category_list_{nullptr};
    QLineEdit* search_input_{nullptr};
    QListWidget* example_list_{nullptr};
    QPlainTextEdit* preview_edit_{nullptr};
    QLabel* info_label_{nullptr};
    QLabel* details_banner_{nullptr};
    QPushButton* open_btn_{nullptr};
    QPushButton* cancel_btn_{nullptr};

    QString selected_path_{};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_EXAMPLES_DIALOG_HPP
