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
    [[nodiscard]] int totalStages() const noexcept { return 9; }

    [[nodiscard]] QLabel* stageTitleLabel() const noexcept { return stage_title_label_; }
    [[nodiscard]] QLabel* stageBodyLabel() const noexcept { return stage_body_label_; }
    [[nodiscard]] QLabel* stepBadgeLabel() const noexcept { return step_badge_label_; }
    [[nodiscard]] QLabel* statusFeedbackLabel() const noexcept { return status_feedback_label_; }
    [[nodiscard]] QProgressBar* progressBar() const noexcept { return progress_bar_; }

    [[nodiscard]] QPushButton* nextButton() const noexcept { return next_btn_; }
    [[nodiscard]] QPushButton* backButton() const noexcept { return back_btn_; }
    [[nodiscard]] QPushButton* restartButton() const noexcept { return restart_btn_; }
    [[nodiscard]] QPushButton* closeButton() const noexcept { return close_btn_; }
    [[nodiscard]] QPushButton* actionButton() const noexcept { return action_btn_; }

    void nextStage();
    void previousStage();
    void restartTour();

signals:
    void loadTutorialExampleRequested();
    void stageChanged(int stage);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onNextClicked();
    void onBackClicked();
    void onRestartClicked();
    void checkEmulatorState();

private:
    void updateStageUi();

    ExecutionController* controller_{nullptr};
    int current_stage_{0};

    QLabel* step_badge_label_{nullptr};
    QLabel* stage_title_label_{nullptr};
    QLabel* stage_body_label_{nullptr};
    QLabel* status_feedback_label_{nullptr};
    QProgressBar* progress_bar_{nullptr};

    QPushButton* action_btn_{nullptr};
    QPushButton* back_btn_{nullptr};
    QPushButton* next_btn_{nullptr};
    QPushButton* restart_btn_{nullptr};
    QPushButton* close_btn_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_TUTORIAL_GUIDE_HPP
