#ifndef BITFORGE_GUI_WELCOME_WIDGET_HPP
#define BITFORGE_GUI_WELCOME_WIDGET_HPP

#include <QWidget>
#include <QStringList>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>
#include <QTextBrowser>
#include <QLabel>
#include <QFrame>

namespace bitforge::gui {

class WelcomeWidget : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);
    ~WelcomeWidget() override = default;

    void setRecentFiles(const QStringList& files);
    [[nodiscard]] QStringList recentFiles() const;

    void setShowOnStartup(bool show);
    [[nodiscard]] bool showOnStartup() const;

    void selectTab(int index);
    void selectLesson(int lessonIndex);
    [[nodiscard]] int currentLessonIndex() const;

    [[nodiscard]] QTabWidget* welcomeTabs() const noexcept { return welcome_tabs_; }
    [[nodiscard]] QListWidget* recentList() const noexcept { return recent_list_; }
    [[nodiscard]] QListWidget* lessonList() const noexcept { return lesson_list_; }
    [[nodiscard]] QTextBrowser* lessonContent() const noexcept { return lesson_content_; }
    [[nodiscard]] QPushButton* firstProgramBtn() const noexcept { return first_program_btn_; }
    [[nodiscard]] QCheckBox* showOnStartupCheckBox() const noexcept { return show_on_startup_cb_; }
    [[nodiscard]] QPushButton* openLessonExampleBtn() const noexcept { return open_lesson_example_btn_; }
    [[nodiscard]] QFrame* lessonActionBar() const noexcept { return lesson_action_bar_; }

signals:
    void newFileRequested();
    void openFileRequested();
    void openRecentRequested(const QString& filePath);
    void browseExamplesRequested();
    void startTutorialRequested();
    void openEditorRequested();
    void openFirstProgramTutorialRequested();
    void openExampleRequested(const QString& relativePath);
    void showOnStartupChanged(bool show);

private slots:
    void onRecentItemDoubleClicked(QListWidgetItem* item);
    void onClearRecentClicked();
    void onLessonSelected(int row);

private:
    void setupQuickStartTab();
    void setupCurriculumTab();
    void setupFirstProgramTab();
    void loadLessonData();

    QTabWidget* welcome_tabs_{nullptr};
    QCheckBox* show_on_startup_cb_{nullptr};

    // Quick Start Tab Components
    QListWidget* recent_list_{nullptr};
    QPushButton* clear_recent_btn_{nullptr};
    QStringList recent_paths_;

    // Curriculum Tab Components
    QListWidget* lesson_list_{nullptr};
    QTextBrowser* lesson_content_{nullptr};
    QFrame* lesson_action_bar_{nullptr};
    QPushButton* open_lesson_example_btn_{nullptr};
    int current_lesson_idx_{0};

    // First Program Tab Components
    QPushButton* first_program_btn_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_WELCOME_WIDGET_HPP
