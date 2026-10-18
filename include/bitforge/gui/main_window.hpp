#ifndef BITFORGE_GUI_MAIN_WINDOW_HPP
#define BITFORGE_GUI_MAIN_WINDOW_HPP

#include "bitforge/gui/execution_controller.hpp"
#include "bitforge/gui/code_editor.hpp"
#include "bitforge/gui/registers_panel.hpp"
#include "bitforge/gui/memory_viewer.hpp"
#include "bitforge/gui/stack_viewer.hpp"
#include "bitforge/gui/devices_panel.hpp"
#include "bitforge/gui/breakpoints_panel.hpp"
#include "bitforge/gui/trace_panel.hpp"
#include "bitforge/gui/instruction_inspector.hpp"
#include "bitforge/gui/find_replace_bar.hpp"
#include "bitforge/gui/welcome_widget.hpp"
#include "bitforge/gui/examples_dialog.hpp"
#include "bitforge/gui/tutorial_guide.hpp"
#include "bitforge/gui/theme.hpp"

#include <QMainWindow>
#include <QStackedWidget>
#include <QDockWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QAction>
#include <QActionGroup>
#include <QString>
#include <QStringList>

namespace bitforge::gui {

// Professional desktop IDE main window for BitForge emulator and assembly workbench
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    [[nodiscard]] ExecutionController* controller() const noexcept { return controller_; }
    [[nodiscard]] CodeEditor* editor() const noexcept { return editor_; }
    [[nodiscard]] RegistersPanel* registersPanel() const noexcept { return registers_panel_; }
    [[nodiscard]] DevicesPanel* devicesPanel() const noexcept { return devices_panel_; }
    [[nodiscard]] InstructionInspector* instructionInspector() const noexcept { return instruction_inspector_; }
    [[nodiscard]] BreakpointsPanel* breakpointsPanel() const noexcept { return breakpoints_panel_; }
    [[nodiscard]] TracePanel* tracePanel() const noexcept { return trace_panel_; }
    [[nodiscard]] MemoryViewer* memoryViewer() const noexcept { return memory_viewer_; }
    [[nodiscard]] StackViewer* stackViewer() const noexcept { return stack_viewer_; }

    [[nodiscard]] QStackedWidget* centralStack() const noexcept { return central_stack_; }
    [[nodiscard]] WelcomeWidget* welcomeWidget() const noexcept { return welcome_widget_; }
    [[nodiscard]] FindReplaceBar* findBar() const noexcept { return find_bar_; }
    [[nodiscard]] QDockWidget* rightDock() const noexcept { return right_dock_; }
    [[nodiscard]] QDockWidget* bottomDock() const noexcept { return bottom_dock_; }

    [[nodiscard]] QComboBox* speedComboBox() const noexcept { return speed_combo_; }
    [[nodiscard]] QSpinBox* budgetSpinBox() const noexcept { return budget_spinbox_; }
    [[nodiscard]] QAction* runAction() const noexcept { return run_act_; }
    [[nodiscard]] QAction* continueAction() const noexcept { return continue_act_; }
    [[nodiscard]] QAction* stepAction() const noexcept { return step_act_; }
    [[nodiscard]] QAction* toggleBreakpointAction() const noexcept { return toggle_bp_act_; }
    [[nodiscard]] QAction* clearBreakpointsAction() const noexcept { return clear_bps_act_; }
    [[nodiscard]] QAction* clearTraceAction() const noexcept { return clear_trace_act_; }
    [[nodiscard]] QAction* speedSlowAction() const noexcept { return speed_slow_act_; }
    [[nodiscard]] QAction* speedNormalAction() const noexcept { return speed_normal_act_; }
    [[nodiscard]] QAction* speedFastAction() const noexcept { return speed_fast_act_; }
    [[nodiscard]] QAction* speedTurboAction() const noexcept { return speed_turbo_act_; }

    // Phase 9 UI Action Getters
    [[nodiscard]] QAction* welcomeScreenAction() const noexcept { return welcome_screen_act_; }
    [[nodiscard]] QAction* showEditorAction() const noexcept { return show_editor_act_; }
    [[nodiscard]] QAction* restoreDefaultLayoutAction() const noexcept { return restore_default_layout_act_; }
    [[nodiscard]] QAction* browseExamplesAction() const noexcept { return browse_examples_act_; }
    [[nodiscard]] QAction* interactiveTutorialAction() const noexcept { return tutorial_act_; }
    [[nodiscard]] QAction* findAction() const noexcept { return find_act_; }
    [[nodiscard]] QAction* findNextAction() const noexcept { return find_next_act_; }
    [[nodiscard]] QAction* findPreviousAction() const noexcept { return find_prev_act_; }
    [[nodiscard]] QAction* gotoLineAction() const noexcept { return goto_line_act_; }
    [[nodiscard]] QAction* zoomInAction() const noexcept { return zoom_in_act_; }
    [[nodiscard]] QAction* zoomOutAction() const noexcept { return zoom_out_act_; }
    [[nodiscard]] QAction* resetZoomAction() const noexcept { return reset_zoom_act_; }

    bool loadFile(const QString& filePath);
    bool loadStartupFile(const QStringList& args);
    bool saveFile(const QString& filePath);
    bool maybeSave();
    [[nodiscard]] const QString& currentFilePath() const noexcept { return current_file_path_; }

public slots:
    void newFile();
    void openFile();
    bool save();
    bool saveAs();

    void showWelcomeScreen();
    void showEditor();
    void onBrowseExamples();
    void onStartTutorial();
    void onFind();
    void onFindNext();
    void onFindPrevious();
    void onGoToLine();
    void onRestoreDefaultLayout();

    void onAssemble();
    void onRun();
    void onContinue();
    void onStep();
    void onReset();
    void onPause();

    void onSpeedComboChanged(int index);
    void onToggleBreakpoint();
    void onClearBreakpoints();
    void onClearTrace();
    void showKeyboardShortcuts();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateUiState();
    void updateWindowTitle();

private:
    void setupUi();
    void setupMenusAndToolbars();
    void setupStatusBar();
    void setCurrentFile(const QString& filePath);
    void updateRecentFilesList(const QString& filePath);
    void loadSettings();
    void saveSettings();

    ExecutionController* controller_;

    // Central Stack & Components
    QStackedWidget* central_stack_{nullptr};
    WelcomeWidget* welcome_widget_{nullptr};
    QWidget* editor_container_{nullptr};
    FindReplaceBar* find_bar_{nullptr};
    CodeEditor* editor_{nullptr};

    // Docks
    QDockWidget* right_dock_{nullptr};
    QDockWidget* bottom_dock_{nullptr};

    // UI Panels
    RegistersPanel* registers_panel_{nullptr};
    DevicesPanel* devices_panel_{nullptr};
    MemoryViewer* memory_viewer_{nullptr};
    StackViewer* stack_viewer_{nullptr};
    BreakpointsPanel* breakpoints_panel_{nullptr};
    TracePanel* trace_panel_{nullptr};
    InstructionInspector* instruction_inspector_{nullptr};

    // Actions: File
    QAction* new_act_{nullptr};
    QAction* open_act_{nullptr};
    QAction* save_act_{nullptr};
    QAction* save_as_act_{nullptr};
    QAction* exit_act_{nullptr};

    // Actions: Edit
    QAction* undo_act_{nullptr};
    QAction* redo_act_{nullptr};
    QAction* cut_act_{nullptr};
    QAction* copy_act_{nullptr};
    QAction* paste_act_{nullptr};
    QAction* select_all_act_{nullptr};
    QAction* find_act_{nullptr};
    QAction* find_next_act_{nullptr};
    QAction* find_prev_act_{nullptr};
    QAction* goto_line_act_{nullptr};
    QAction* zoom_in_act_{nullptr};
    QAction* zoom_out_act_{nullptr};
    QAction* reset_zoom_act_{nullptr};

    // Actions: View
    QAction* welcome_screen_act_{nullptr};
    QAction* show_editor_act_{nullptr};
    QAction* restore_default_layout_act_{nullptr};

    // Actions: Execution & Debug
    QAction* assemble_act_{nullptr};
    QAction* run_act_{nullptr};
    QAction* continue_act_{nullptr};
    QAction* step_act_{nullptr};
    QAction* reset_act_{nullptr};
    QAction* pause_act_{nullptr};

    QAction* toggle_bp_act_{nullptr};
    QAction* clear_bps_act_{nullptr};
    QAction* clear_trace_act_{nullptr};
    QAction* shortcuts_act_{nullptr};

    // Actions: Help
    QAction* tutorial_act_{nullptr};
    QAction* browse_examples_act_{nullptr};
    QAction* about_act_{nullptr};

    // Speed actions
    QActionGroup* speed_action_group_{nullptr};
    QAction* speed_slow_act_{nullptr};
    QAction* speed_normal_act_{nullptr};
    QAction* speed_fast_act_{nullptr};
    QAction* speed_turbo_act_{nullptr};

    QComboBox* speed_combo_{nullptr};
    QSpinBox* budget_spinbox_{nullptr};
    QLabel* state_badge_{nullptr};

    // Status bar labels
    QLabel* status_file_label_{nullptr};
    QLabel* status_cursor_label_{nullptr};
    QLabel* status_state_label_{nullptr};
    QLabel* status_pc_label_{nullptr};
    QLabel* status_steps_label_{nullptr};

    QString current_file_path_;
    TutorialGuide* active_tutorial_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_MAIN_WINDOW_HPP
