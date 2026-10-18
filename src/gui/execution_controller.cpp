#include "bitforge/gui/execution_controller.hpp"

#include <QElapsedTimer>
#include <algorithm>

namespace bitforge::gui {

namespace {

int timerIntervalForSpeed(ExecutionSpeed speed) {
    switch (speed) {
        case ExecutionSpeed::Slow:   return 500; // 500 ms (~2 inst/sec)
        case ExecutionSpeed::Normal: return 100; // 100 ms (~10 inst/sec)
        case ExecutionSpeed::Fast:   return 50;  // 50 ms (~100 inst/sec with 5 steps/tick)
        case ExecutionSpeed::Turbo:  return 0;   // 0 ms (max throughput)
    }
    return 0;
}

std::uint64_t sliceBatchForSpeed(ExecutionSpeed speed) {
    switch (speed) {
        case ExecutionSpeed::Slow:   return 1;
        case ExecutionSpeed::Normal: return 1;
        case ExecutionSpeed::Fast:   return 5;
        case ExecutionSpeed::Turbo:  return 10000;
    }
    return 10000;
}

} // namespace

ExecutionController::ExecutionController(QObject* parent)
    : QObject(parent),
      session_(std::make_unique<debugger::DebuggerSession>(cpu_, nullptr)) {
    session_->reset();

    slice_timer_.setSingleShot(false);
    slice_timer_.setInterval(0); // Default Turbo
    connect(&slice_timer_, &QTimer::timeout, this, &ExecutionController::onSliceTimeout);
}

void ExecutionController::setSpeed(ExecutionSpeed speed) {
    if (speed_ != speed) {
        speed_ = speed;
        slice_timer_.setInterval(timerIntervalForSpeed(speed_));
        emit speedChanged(speed_);
    }
}

void ExecutionController::runFromEntry(std::uint64_t max_steps) {
    reset();
    run(max_steps);
}

void ExecutionController::run(std::uint64_t max_steps) {
    if (is_running_timer_) {
        pause();
    }
    if ((!is_assembled_ && !image_.has_value()) || is_stale_) {
        return;
    }

    if (session_->state() == MachineState::Halted || session_->state() == MachineState::Faulted) {
        reset();
    }

    remaining_budget_ = max_steps;
    is_continue_mode_ = false;
    last_stop_reason_ = debugger::StopReason::None;
    is_running_timer_ = true;
    slice_timer_.setInterval(timerIntervalForSpeed(speed_));
    slice_timer_.start();
    emit stateChanged();
}

void ExecutionController::continueExecution(std::uint64_t max_steps) {
    if (is_running_timer_) {
        pause();
    }
    if ((!is_assembled_ && !image_.has_value()) || is_stale_) {
        return;
    }

    remaining_budget_ = max_steps;
    is_continue_mode_ = true;
    last_stop_reason_ = debugger::StopReason::None;
    is_running_timer_ = true;
    slice_timer_.setInterval(timerIntervalForSpeed(speed_));
    slice_timer_.start();
    emit stateChanged();
}

bool ExecutionController::assemble(const QString& sourceCode) {
    if (is_running_timer_) {
        pause();
    }

    assembler::Assembler as;
    auto res = as.assemble(sourceCode.toStdString());
    last_diagnostics_ = res.diagnostics;

    if (!res.success) {
        is_assembled_ = false;
        emit assemblyFailed(last_diagnostics_);
        emit stateChanged();
        return false;
    }

    // Retain program image and reload emulator state
    image_ = std::move(res.image);
    memory_.reset();
    bus_.reset_devices();
    image_->load_into(memory_);

    // Preserve existing breakpoints if compatible or rebind to new image
    std::vector<word_t> old_bps = session_->breakpoints();
    session_ = std::make_unique<debugger::DebuggerSession>(cpu_, &(*image_));
    for (word_t bp : old_bps) {
        session_->add_breakpoint(bp);
    }
    session_->reset(image_->entry_point());

    is_assembled_ = true;
    is_stale_ = false;
    last_stop_reason_ = debugger::StopReason::None;

    emit assemblySucceeded();
    emit staleChanged(false);
    emit stateChanged();
    return true;
}

void ExecutionController::unloadProgram() {
    if (is_running_timer_) {
        pause();
    }
    image_.reset();
    is_assembled_ = false;
    is_stale_ = false;
    last_diagnostics_.clear();
    last_stop_reason_ = debugger::StopReason::None;
    bus_.reset_devices();
    memory_.reset();
    session_ = std::make_unique<debugger::DebuggerSession>(cpu_, nullptr);
    session_->reset();
    emit breakpointsChanged();
    emit staleChanged(false);
    emit stateChanged();
}

void ExecutionController::setStale(bool stale) noexcept {
    if (is_stale_ != stale) {
        is_stale_ = stale;
        emit staleChanged(is_stale_);
    }
}

void ExecutionController::step() {
    if (is_running_timer_) {
        pause();
    }
    if ((!is_assembled_ && !image_.has_value()) || is_stale_) {
        return;
    }

    auto res = session_->step();
    last_stop_reason_ = res.stop_reason;
    emit stateChanged();
    if (res.stop_reason != debugger::StopReason::StepCompleted) {
        emit executionStopped(res.stop_reason);
    }
}

void ExecutionController::pause() {
    if (is_running_timer_) {
        slice_timer_.stop();
        is_running_timer_ = false;
        is_continue_mode_ = false;
        last_stop_reason_ = debugger::StopReason::None;
        emit executionStopped(debugger::StopReason::None);
        emit stateChanged();
    }
}

void ExecutionController::reset() {
    if (is_running_timer_) {
        slice_timer_.stop();
        is_running_timer_ = false;
        is_continue_mode_ = false;
    }

    bus_.reset_devices();
    last_stop_reason_ = debugger::StopReason::None;

    if (image_.has_value()) {
        memory_.reset();
        image_->load_into(memory_);
        session_->reset(image_->entry_point());
    } else {
        session_->reset();
    }

    emit stateChanged();
}

debugger::BreakpointResult ExecutionController::toggleBreakpoint(std::size_t line) {
    if (!image_.has_value()) {
        return debugger::BreakpointResult{
            false, 0, false, false, "Cannot set line breakpoints before assembling a program."
        };
    }

    if (hasLineBreakpoint(line)) {
        auto res = session_->remove_line_breakpoint(line);
        emit breakpointsChanged();
        return res;
    }

    auto res = session_->add_line_breakpoint(line);
    emit breakpointsChanged();
    return res;
}

debugger::BreakpointResult ExecutionController::removeBreakpoint(word_t address) {
    auto res = session_->remove_breakpoint(address);
    emit breakpointsChanged();
    return res;
}

void ExecutionController::clearAllBreakpoints() {
    session_->clear_breakpoints();
    emit breakpointsChanged();
}

void ExecutionController::clearTrace() noexcept {
    if (session_ != nullptr) {
        session_->clear_trace();
        emit stateChanged();
    }
}

bool ExecutionController::hasLineBreakpoint(std::size_t line) const {
    if (!image_.has_value()) return false;
    auto resolution = image_->source_map().resolve_line_breakpoint(line);
    if (!resolution.success) return false;
    return session_->has_breakpoint(resolution.address);
}

void ExecutionController::onSliceTimeout() {
    if (!is_running_timer_) {
        return;
    }

    if (remaining_budget_ == 0) {
        slice_timer_.stop();
        is_running_timer_ = false;
        is_continue_mode_ = false;
        last_stop_reason_ = debugger::StopReason::BudgetExhausted;
        emit executionStopped(debugger::StopReason::BudgetExhausted);
        emit stateChanged();
        return;
    }

    if (speed_ == ExecutionSpeed::Turbo) {
        // Time-sliced Turbo execution: use QElapsedTimer targeting ~10-15 ms slices
        // so UI stays responsive and event loop processes paint/events smoothly
        // without dropping instructions or freezing.
        QElapsedTimer slice_elapsed;
        slice_elapsed.start();
        constexpr qint64 kMaxSliceMs = 12; // 12 ms slice preserves ~60 FPS responsiveness
        constexpr std::uint64_t kTurboBatchChunk = 2000;

        while (is_running_timer_ && remaining_budget_ > 0) {
            const std::uint64_t chunk = std::min(remaining_budget_, kTurboBatchChunk);
            debugger::ExecutionResult res;
            if (is_continue_mode_) {
                res = session_->continue_execution(chunk);
                is_continue_mode_ = false;
            } else {
                res = session_->run(chunk);
            }

            if (res.steps_executed >= remaining_budget_) {
                remaining_budget_ = 0;
            } else {
                remaining_budget_ -= res.steps_executed;
            }

            if (res.stop_reason == debugger::StopReason::BreakpointHit ||
                res.stop_reason == debugger::StopReason::Halted ||
                res.stop_reason == debugger::StopReason::Faulted ||
                res.stop_reason == debugger::StopReason::AlreadyHalted ||
                res.stop_reason == debugger::StopReason::AlreadyFaulted) {
                slice_timer_.stop();
                is_running_timer_ = false;
                last_stop_reason_ = res.stop_reason;
                emit executionStopped(res.stop_reason);
                emit stateChanged();
                return;
            }

            if (remaining_budget_ == 0) {
                slice_timer_.stop();
                is_running_timer_ = false;
                last_stop_reason_ = debugger::StopReason::BudgetExhausted;
                emit executionStopped(debugger::StopReason::BudgetExhausted);
                emit stateChanged();
                return;
            }

            if (slice_elapsed.elapsed() >= kMaxSliceMs) {
                break;
            }
        }

        // Periodic UI update after time slice
        emit stateChanged();
        return;
    }

    const std::uint64_t max_batch = sliceBatchForSpeed(speed_);
    const std::uint64_t slice_steps = std::min(remaining_budget_, max_batch);
    debugger::ExecutionResult res;

    if (is_continue_mode_) {
        res = session_->continue_execution(slice_steps);
        is_continue_mode_ = false; // Continue bypass only applies to the first step
    } else {
        res = session_->run(slice_steps);
    }

    if (res.steps_executed >= remaining_budget_) {
        remaining_budget_ = 0;
    } else {
        remaining_budget_ -= res.steps_executed;
    }

    // Stop on termination, breakpoint hit, or fault
    if (res.stop_reason == debugger::StopReason::BreakpointHit ||
        res.stop_reason == debugger::StopReason::Halted ||
        res.stop_reason == debugger::StopReason::Faulted ||
        res.stop_reason == debugger::StopReason::AlreadyHalted ||
        res.stop_reason == debugger::StopReason::AlreadyFaulted) {
        slice_timer_.stop();
        is_running_timer_ = false;
        last_stop_reason_ = res.stop_reason;
        emit executionStopped(res.stop_reason);
        emit stateChanged();
        return;
    }

    if (remaining_budget_ == 0) {
        slice_timer_.stop();
        is_running_timer_ = false;
        last_stop_reason_ = debugger::StopReason::BudgetExhausted;
        emit executionStopped(debugger::StopReason::BudgetExhausted);
        emit stateChanged();
        return;
    }

    // Still running within total budget: periodic UI update
    emit stateChanged();
}

} // namespace bitforge::gui
