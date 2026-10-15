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

#include <QMainWindow>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QAction>
#include <QActionGroup>
#include <QString>

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

    ExecutionController* controller_;

    // Core central editor
    CodeEditor* editor_;

    // UI Panels
    RegistersPanel* registers_panel_;
    DevicesPanel* devices_panel_;
    MemoryViewer* memory_viewer_;
    StackViewer* stack_viewer_;
    BreakpointsPanel* breakpoints_panel_;
    TracePanel* trace_panel_;
    InstructionInspector* instruction_inspector_;

    // Actions
    QAction* new_act_;
    QAction* open_act_;
    QAction* save_act_;
    QAction* save_as_act_;
    QAction* exit_act_;

    QAction* assemble_act_;
    QAction* run_act_;
    QAction* continue_act_;
    QAction* step_act_;
    QAction* reset_act_;
    QAction* pause_act_;

    QAction* toggle_bp_act_{nullptr};
    QAction* clear_bps_act_{nullptr};
    QAction* clear_trace_act_{nullptr};
    QAction* shortcuts_act_{nullptr};

    QActionGroup* speed_action_group_{nullptr};
    QAction* speed_slow_act_{nullptr};
    QAction* speed_normal_act_{nullptr};
    QAction* speed_fast_act_{nullptr};
    QAction* speed_turbo_act_{nullptr};

    QComboBox* speed_combo_{nullptr};
    QSpinBox* budget_spinbox_;
    QLabel* state_badge_;

    // Status bar labels
    QLabel* status_file_label_;
    QLabel* status_state_label_;
    QLabel* status_pc_label_;
    QLabel* status_steps_label_;

    QString current_file_path_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_MAIN_WINDOW_HPP
