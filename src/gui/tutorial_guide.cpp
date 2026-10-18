#include "bitforge/gui/tutorial_guide.hpp"
#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

namespace bitforge::gui {

namespace {

struct StageInfo {
    QString title;
    QString body;
    QString actionButtonText;
};

const StageInfo kStages[9] = {
    {
        "1. Load Assembly Example",
        "Welcome to the BitForge Interactive Tutorial!<br><br>"
        "In this 9-step walkthrough, you will assemble, set breakpoints, step through code, "
        "and observe register and memory mutations in real time.<br><br>"
        "Click the button below to load <b>examples/01_arithmetic.asm</b> into the editor.",
        "Load 01_arithmetic.asm"
    },
    {
        "2. Assemble into Machine Code",
        "The assembly program is now in your editor.<br><br>"
        "Before execution can begin, the two-pass assembler must validate the syntax, "
        "resolve symbols, and load bytecode segments into memory.<br><br>"
        "Click <b>Assemble (F7)</b> in the top toolbar to assemble the program.",
        ""
    },
    {
        "3. Set a Breakpoint",
        "Breakpoints pause execution before a specific instruction executes.<br><br>"
        "In the code editor, click the gutter next to <b>Line 10 (ADD R0, R1)</b>, "
        "or place your cursor on line 10 and press <b>F9</b>.<br><br>"
        "A red circular marker will appear in the gutter.",
        ""
    },
    {
        "4. Run from Entry Point",
        "Now begin fresh execution.<br><br>"
        "Click <b>Run from Entry (F5)</b>. The CPU resets registers and executes from "
        "the <code>.ENTRY</code> address (0x0200). Execution will automatically stop when the breakpoint on line 10 is encountered.",
        ""
    },
    {
        "5. Inspect Registers & Flags",
        "Execution stopped at the breakpoint before executing <code>ADD R0, R1</code>.<br><br>"
        "Inspect the <b>CPU Registers & Flags</b> panel on the right:<br>"
        "• <b>R0 = 10</b> (0x0A) and <b>R1 = 20</b> (0x14)<br>"
        "• <b>PC = 0x0206</b> (points to ADD instruction)<br><br>"
        "Switch to the <b>Bit Visualization</b> tab to view the live binary LED rows!",
        ""
    },
    {
        "6. Step One Instruction",
        "Single-stepping allows you to advance execution exactly one instruction at a time.<br><br>"
        "Click <b>Step (F10)</b> in the toolbar to execute <code>ADD R0, R1</code>.",
        ""
    },
    {
        "7. Observe Register Mutation",
        "Notice what just happened in the register inspector:<br><br>"
        "• <b>R0 changed to 30 (0x1E)</b> (10 + 20 = 30)<br>"
        "• R0 is highlighted with an <b>amber background</b> indicating mutation in the last step!<br>"
        "• The Instruction Inspector at the bottom shows the next instruction: <code>ST [0x8000], R0</code>.",
        ""
    },
    {
        "8. Continue Execution to Halt",
        "The remaining instructions store the sum into memory at address 0x8000 and halt the CPU.<br><br>"
        "Click <b>Continue (F8)</b> to resume running until the program halts.",
        ""
    },
    {
        "9. Review Trace and Memory",
        "Congratulations! You completed your first BitForge debugging session.<br><br>"
        "• Inspect the <b>Memory (64 KiB)</b> tab at address 0x8000 to verify 0x1E is stored.<br>"
        "• Inspect the <b>Execution Trace</b> tab to see the chronological history of every executed instruction.<br><br>"
        "You can restart this tutorial at any time from <b>Help → Interactive Tutorial</b>.",
        ""
    }
};

} // namespace

TutorialGuide::TutorialGuide(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("BitForge Interactive Tutorial");
    setMinimumSize(480, 360);
    resize(520, 400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    // Header: Stage Title & Step Badge
    auto* header_layout = new QHBoxLayout();
    stage_title_label_ = new QLabel(this);
    stage_title_label_->setStyleSheet(QString("QLabel { font-size: 16px; font-weight: bold; color: %1; }").arg(theme::kAccentTeal.name()));
    header_layout->addWidget(stage_title_label_, 1);

    step_badge_label_ = new QLabel("Step 1 of 9", this);
    step_badge_label_->setStyleSheet(
        QString("QLabel { font-size: 11px; font-weight: bold; color: %1; background: %2; border: 1px solid %3; border-radius: %4px; padding: 3px 10px; }")
            .arg(theme::kAccentBlue.name(), theme::kBgMedium.name(), theme::kBorderMedium.name())
            .arg(theme::kRadiusPill)
    );
    header_layout->addWidget(step_badge_label_);
    layout->addLayout(header_layout);

    // Progress Bar
    progress_bar_ = new QProgressBar(this);
    progress_bar_->setRange(0, 8);
    progress_bar_->setTextVisible(false);
    progress_bar_->setFixedHeight(6);
    progress_bar_->setStyleSheet(
        QString("QProgressBar { background: %1; border: none; border-radius: %2px; }"
                "QProgressBar::chunk { background: %3; border-radius: %2px; }")
            .arg(theme::kBgMedium.name())
            .arg(theme::kRadiusSmall)
            .arg(theme::kAccentTeal.name())
    );
    layout->addWidget(progress_bar_);

    // Body text
    stage_body_label_ = new QLabel(this);
    stage_body_label_->setWordWrap(true);
    stage_body_label_->setStyleSheet(QString("QLabel { font-size: 12px; line-height: 1.4; color: %1; }").arg(theme::kTextPrimary.name()));
    layout->addWidget(stage_body_label_, 1);

    // Contextual Action Button (e.g. "Load Example")
    action_btn_ = new QPushButton(this);
    action_btn_->setStyleSheet(theme::primaryButtonStyle());
    connect(action_btn_, &QPushButton::clicked, this, [this]() {
        if (current_stage_ == 0) {
            emit loadTutorialExampleRequested();
            setStage(1);
        }
    });
    layout->addWidget(action_btn_);

    // Real-time status feedback label
    status_feedback_label_ = new QLabel(this);
    status_feedback_label_->setStyleSheet(
        QString("QLabel { color: %1; font-size: 11px; padding: 4px 8px; border-radius: %2px; background: %3; border: 1px solid %4; }")
            .arg(theme::kTextSecondary.name())
            .arg(theme::kRadiusControl)
            .arg(theme::kBgDark.name(), theme::kBorderMedium.name())
    );
    layout->addWidget(status_feedback_label_);

    // Navigation buttons
    auto* btn_layout = new QHBoxLayout();
    restart_btn_ = new QPushButton("Restart", this);
    restart_btn_->setStyleSheet(theme::dangerButtonStyle());
    btn_layout->addWidget(restart_btn_);

    btn_layout->addStretch();

    back_btn_ = new QPushButton("◀ Back", this);
    next_btn_ = new QPushButton("Next ▶", this);
    close_btn_ = new QPushButton("Close", this);

    btn_layout->addWidget(back_btn_);
    btn_layout->addWidget(next_btn_);
    btn_layout->addWidget(close_btn_);

    layout->addLayout(btn_layout);

    connect(back_btn_, &QPushButton::clicked, this, &TutorialGuide::onBackClicked);
    connect(next_btn_, &QPushButton::clicked, this, &TutorialGuide::onNextClicked);
    connect(restart_btn_, &QPushButton::clicked, this, &TutorialGuide::onRestartClicked);
    connect(close_btn_, &QPushButton::clicked, this, &QDialog::accept);

    setStage(0);
}

void TutorialGuide::setController(ExecutionController* controller) {
    controller_ = controller;
    if (controller_ != nullptr) {
        connect(controller_, &ExecutionController::stateChanged, this, &TutorialGuide::checkEmulatorState);
        connect(controller_, &ExecutionController::assemblySucceeded, this, &TutorialGuide::checkEmulatorState);
        connect(controller_, &ExecutionController::breakpointsChanged, this, &TutorialGuide::checkEmulatorState);
        connect(controller_, &ExecutionController::executionStopped, this, &TutorialGuide::checkEmulatorState);
    }
    checkEmulatorState();
}

void TutorialGuide::setStage(int stage) {
    current_stage_ = std::clamp(stage, 0, 8);
    updateStageUi();
    checkEmulatorState();
}

void TutorialGuide::updateStageUi() {
    const auto& s = kStages[current_stage_];
    stage_title_label_->setText(s.title);
    stage_body_label_->setText(s.body);
    progress_bar_->setValue(current_stage_);

    back_btn_->setEnabled(current_stage_ > 0);
    next_btn_->setEnabled(current_stage_ < 8);

    if (step_badge_label_ != nullptr) {
        step_badge_label_->setText(QString("Step %1 of %2").arg(current_stage_ + 1).arg(9));
    }

    if (!s.actionButtonText.isEmpty()) {
        action_btn_->setText(s.actionButtonText);
        action_btn_->show();
    } else {
        action_btn_->hide();
    }

    emit stageChanged(current_stage_);
}

void TutorialGuide::checkEmulatorState() {
    if (!controller_) return;

    auto makeFeedbackStyle = [](const QColor& textCol, bool bold) {
        return QString("QLabel { color: %1; %2 padding: 4px 8px; border-radius: %3px; background: %4; border: 1px solid %5; }")
            .arg(textCol.name())
            .arg(bold ? "font-weight: bold;" : "")
            .arg(theme::kRadiusControl)
            .arg(theme::kBgDark.name(), theme::kBorderMedium.name());
    };

    switch (current_stage_) {
        case 1: { // Assemble
            if (controller_->isAssembled()) {
                status_feedback_label_->setText("✔ Program assembled successfully! Click Next to proceed.");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStateReady, true));
            } else {
                status_feedback_label_->setText("Waiting for assembly (Press F7)...");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStatePaused, false));
            }
            break;
        }
        case 2: { // Set breakpoint
            if (!controller_->session().breakpoints().empty()) {
                status_feedback_label_->setText("✔ Breakpoint detected! Click Next to proceed.");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStateReady, true));
            } else {
                status_feedback_label_->setText("Waiting for breakpoint on line 10 (Press F9)...");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStatePaused, false));
            }
            break;
        }
        case 3: { // Run to breakpoint
            if (controller_->lastStopReason() == debugger::StopReason::BreakpointHit) {
                status_feedback_label_->setText("✔ Breakpoint hit! Execution paused at Line 6. Click Next.");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStateReady, true));
            } else {
                status_feedback_label_->setText("Waiting to run to breakpoint (Press F5)...");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStatePaused, false));
            }
            break;
        }
        case 5: { // Step
            if (controller_->session().total_steps_executed() >= 3) {
                status_feedback_label_->setText("✔ Instruction stepped! R0 is now 30 (0x1E). Click Next.");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStateReady, true));
            } else {
                status_feedback_label_->setText("Waiting for single step (Press F10)...");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStatePaused, false));
            }
            break;
        }
        case 7: { // Continue to halt
            if (controller_->session().state() == MachineState::Halted) {
                status_feedback_label_->setText("✔ CPU Halted! Click Next to review trace and memory.");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStateReady, true));
            } else {
                status_feedback_label_->setText("Waiting for program completion (Press F8)...");
                status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kStatePaused, false));
            }
            break;
        }
        default: {
            status_feedback_label_->setText(QString("Current CPU State: %1 | PC: 0x%2 | Steps: %3")
                .arg(controller_->session().state() == MachineState::Halted ? "Halted" :
                     controller_->session().state() == MachineState::Ready ? "Ready" :
                     controller_->session().state() == MachineState::Running ? "Running" : "Paused")
                .arg(controller_->session().pc(), 4, 16, QLatin1Char('0')).toUpper()
                .arg(controller_->session().total_steps_executed()));
            status_feedback_label_->setStyleSheet(makeFeedbackStyle(theme::kTextSecondary, false));
            break;
        }
    }
}

void TutorialGuide::onNextClicked() {
    if (current_stage_ < 8) {
        setStage(current_stage_ + 1);
    }
}

void TutorialGuide::onBackClicked() {
    if (current_stage_ > 0) {
        setStage(current_stage_ - 1);
    }
}

void TutorialGuide::onRestartClicked() {
    setStage(0);
}

void TutorialGuide::nextStage() {
    onNextClicked();
}

void TutorialGuide::previousStage() {
    onBackClicked();
}

void TutorialGuide::restartTour() {
    onRestartClicked();
}

void TutorialGuide::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_N) {
        onNextClicked();
        event->accept();
    } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_B) {
        onBackClicked();
        event->accept();
    } else if (event->key() == Qt::Key_R) {
        onRestartClicked();
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        accept();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

} // namespace bitforge::gui
