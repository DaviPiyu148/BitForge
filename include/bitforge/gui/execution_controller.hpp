#ifndef BITFORGE_GUI_EXECUTION_CONTROLLER_HPP
#define BITFORGE_GUI_EXECUTION_CONTROLLER_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/cpu.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/bus/system_bus.hpp"
#include "bitforge/assembler/assembler.hpp"
#include "bitforge/assembler/program_image.hpp"
#include "bitforge/debugger/debugger_session.hpp"

#include <QObject>
#include <QTimer>
#include <QString>
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace bitforge::gui {

// Manages emulator hardware, debugger session, assembly lifecycle, and timed execution slices
class ExecutionController : public QObject {
    Q_OBJECT

public:
    explicit ExecutionController(QObject* parent = nullptr);
    ~ExecutionController() override = default;

    // Assembly lifecycle
    bool assemble(const QString& sourceCode);
    void unloadProgram();
    [[nodiscard]] bool isAssembled() const noexcept { return is_assembled_; }
    [[nodiscard]] bool isStale() const noexcept { return is_stale_; }
    void setStale(bool stale) noexcept;
    [[nodiscard]] const std::vector<assembler::Diagnostic>& diagnostics() const noexcept { return last_diagnostics_; }

    // Execution control
    void step();
    void run(std::uint64_t max_steps);
    void continueExecution(std::uint64_t max_steps);
    void pause();
    void reset();

    [[nodiscard]] bool isExecuting() const noexcept { return is_running_timer_; }
    [[nodiscard]] debugger::StopReason lastStopReason() const noexcept { return last_stop_reason_; }

    // Breakpoint management
    debugger::BreakpointResult toggleBreakpoint(std::size_t line);
    debugger::BreakpointResult removeBreakpoint(word_t address);
    void clearAllBreakpoints();
    [[nodiscard]] bool hasLineBreakpoint(std::size_t line) const;

    // Component inspection accessors
    [[nodiscard]] const debugger::DebuggerSession& session() const noexcept { return *session_; }
    [[nodiscard]] debugger::DebuggerSession& session() noexcept { return *session_; }
    [[nodiscard]] const Cpu& cpu() const noexcept { return cpu_; }
    [[nodiscard]] const SystemBus& bus() const noexcept { return bus_; }
    [[nodiscard]] const Memory& memory() const noexcept { return memory_; }
    [[nodiscard]] const assembler::ProgramImage* programImage() const noexcept {
        return image_.has_value() ? &(*image_) : nullptr;
    }

signals:
    void stateChanged();
    void executionStopped(debugger::StopReason reason);
    void assemblySucceeded();
    void assemblyFailed(const std::vector<assembler::Diagnostic>& diagnostics);
    void staleChanged(bool is_stale);
    void breakpointsChanged();

private slots:
    void onSliceTimeout();

private:
    static constexpr std::uint64_t kSliceStepSize = 10000;

    Memory memory_{};
    SystemBus bus_{memory_};
    Cpu cpu_{bus_};
    std::unique_ptr<debugger::DebuggerSession> session_;
    std::optional<assembler::ProgramImage> image_;
    std::vector<assembler::Diagnostic> last_diagnostics_;

    bool is_assembled_{false};
    bool is_stale_{false};
    bool is_running_timer_{false};
    bool is_continue_mode_{false};
    std::uint64_t remaining_budget_{0};
    debugger::StopReason last_stop_reason_{debugger::StopReason::None};

    QTimer slice_timer_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_EXECUTION_CONTROLLER_HPP
