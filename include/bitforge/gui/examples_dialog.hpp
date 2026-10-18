#ifndef BITFORGE_GUI_EXAMPLES_DIALOG_HPP
#define BITFORGE_GUI_EXAMPLES_DIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <map>
#include <vector>

namespace bitforge::gui {

struct ExampleItem {
    QString title;
    QString relativePath;
    QString description;
};

class ExamplesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExamplesDialog(QWidget* parent = nullptr);
    ~ExamplesDialog() override = default;

    [[nodiscard]] QString selectedExamplePath() const { return selected_path_; }

signals:
    void exampleSelected(const QString& filePath);

private slots:
    void onCategoryChanged(int index);
    void onExampleItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onOpenClicked();

private:
    void populateCategories();
    void loadExamplePreview(const QString& relPath);

    QListWidget* category_list_;
    QListWidget* example_list_;
    QPlainTextEdit* preview_edit_;
    QLabel* info_label_;
    QPushButton* open_btn_;
    QPushButton* cancel_btn_;

    std::map<QString, std::vector<ExampleItem>> catalog_;
    QString selected_path_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_EXAMPLES_DIALOG_HPP
