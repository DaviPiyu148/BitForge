#ifndef BITFORGE_GUI_TUTORIAL_GUIDE_HPP
#define BITFORGE_GUI_TUTORIAL_GUIDE_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

namespace bitforge::gui {

class ExecutionController;

class TutorialGuide : public QDialog {
    Q_OBJECT

public:
    explicit TutorialGuide(QWidget* parent = nullptr);
    ~TutorialGuide() override = default;

    void setController(ExecutionController* controller);
    void setStage(int stage);
    [[nodiscard]] int currentStage() const noexcept { return current_stage_; }

signals:
    void loadTutorialExampleRequested();

private slots:
    void onNextClicked();
    void onBackClicked();
    void onRestartClicked();
    void checkEmulatorState();

private:
    void updateStageUi();

    ExecutionController* controller_{nullptr};
    int current_stage_{0};

    QLabel* stage_title_label_;
    QLabel* stage_body_label_;
    QLabel* status_feedback_label_;
    QProgressBar* progress_bar_;

    QPushButton* action_btn_;
    QPushButton* back_btn_;
    QPushButton* next_btn_;
    QPushButton* restart_btn_;
    QPushButton* close_btn_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_TUTORIAL_GUIDE_HPP
