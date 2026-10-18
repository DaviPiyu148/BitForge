#ifndef BITFORGE_GUI_WELCOME_WIDGET_HPP
#define BITFORGE_GUI_WELCOME_WIDGET_HPP

#include <QWidget>
#include <QStringList>
#include <QListWidget>
#include <QPushButton>

namespace bitforge::gui {

class WelcomeWidget : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);
    ~WelcomeWidget() override = default;

    void setRecentFiles(const QStringList& files);
    [[nodiscard]] QStringList recentFiles() const;

signals:
    void newFileRequested();
    void openFileRequested();
    void openRecentRequested(const QString& filePath);
    void browseExamplesRequested();
    void startTutorialRequested();
    void openEditorRequested();

private slots:
    void onRecentItemDoubleClicked(QListWidgetItem* item);
    void onClearRecentClicked();

private:
    QListWidget* recent_list_;
    QPushButton* clear_recent_btn_;
    QStringList recent_paths_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_WELCOME_WIDGET_HPP
