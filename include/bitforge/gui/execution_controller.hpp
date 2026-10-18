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

enum class ExecutionSpeed {
    Slow,    // ~2 inst/sec (500 ms delay, 1 step/tick)
    Normal,  // ~10 inst/sec (100 ms delay, 1 step/tick)
    Fast,    // ~100 inst/sec (50 ms delay, 5 steps/tick)
    Turbo    // Maximum throughput (0 ms delay, up to 10,000 steps/tick)
};

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
    void runFromEntry(std::uint64_t max_steps);
    void continueExecution(std::uint64_t max_steps);
    void pause();
    void reset();

    // Speed control
    void setSpeed(ExecutionSpeed speed);
    [[nodiscard]] ExecutionSpeed speed() const noexcept { return speed_; }

    [[nodiscard]] bool isExecuting() const noexcept { return is_running_timer_; }
    [[nodiscard]] debugger::StopReason lastStopReason() const noexcept { return last_stop_reason_; }

    // Breakpoint management
    debugger::BreakpointResult toggleBreakpoint(std::size_t line);
    debugger::BreakpointResult removeBreakpoint(word_t address);
    void clearAllBreakpoints();
    void clearTrace() noexcept;
    [[nodiscard]] bool hasLineBreakpoint(std::size_t line) const;

    // Component inspection accessors
    [[nodiscard]] const debugger::DebuggerSession& session() const noexcept { return *session_; }
    [[nodiscard]] debugger::DebuggerSession& session() noexcept { return *session_; }
    [[nodiscard]] const Cpu& cpu() const noexcept { return cpu_; }
    [[nodiscard]] const SystemBus& bus() const noexcept { return bus_; }
    [[nodiscard]] SystemBus& bus() noexcept { return bus_; }
    [[nodiscard]] const Memory& memory() const noexcept { return memory_; }
    [[nodiscard]] Memory& memory() noexcept { return memory_; }
    [[nodiscard]] const assembler::ProgramImage* programImage() const noexcept {
        return image_.has_value() ? &(*image_) : nullptr;
    }
    [[nodiscard]] const std::optional<debugger::ExecutionDelta>& lastDelta() const noexcept {
        return session_->last_delta();
    }

    // Input FIFO management (Port 0x00)
    void pushInputFifo(byte_t byte) {
        bus_.push_input_fifo(byte);
        emit stateChanged();
    }
    void pushInputFifo(const std::vector<byte_t>& bytes) {
        bus_.push_input_fifo(bytes);
        emit stateChanged();
    }
    void clearInputFifo() {
        bus_.clear_input_fifo();
        emit stateChanged();
    }

signals:
    void stateChanged();
    void executionStopped(debugger::StopReason reason);
    void assemblySucceeded();
    void assemblyFailed(const std::vector<assembler::Diagnostic>& diagnostics);
    void staleChanged(bool is_stale);
    void breakpointsChanged();
    void speedChanged(ExecutionSpeed speed);

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
    ExecutionSpeed speed_{ExecutionSpeed::Turbo};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_EXECUTION_CONTROLLER_HPP
