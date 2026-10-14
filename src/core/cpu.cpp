#include "bitforge/core/cpu.hpp"
#include "bitforge/core/alu.hpp"

#include <sstream>
#include <iomanip>

namespace bitforge {

namespace {

constexpr bool is_valid_reg(byte_t r) noexcept {
    return r <= kMaxRegisterId;
}

std::string to_hex_byte(byte_t b) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

std::string to_hex_word(word_t w) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << w;
    return oss.str();
}

} // namespace

Cpu::Cpu(IBus& bus) noexcept
    : bus_(bus) {
    reset();
}

void Cpu::reset(word_t entry_point) noexcept {
    registers_.reset(entry_point);
    flags_.reset();
    state_ = MachineState::Ready;
    last_fault_.clear();
}

StepResult Cpu::record_fault(FaultType type, word_t pc, std::string msg) {
    last_fault_.type = type;
    last_fault_.instruction_address = pc;
    last_fault_.message = std::move(msg);
    state_ = MachineState::Faulted;

    StepResult res;
    res.executed = false;
    res.faulted = true;
    res.instruction_address = pc;
    return res;
}

StepResult Cpu::step() {
    // Stepping from Halted or Faulted is an explicit no-op
    if (state_ == MachineState::Halted) {
        StepResult res;
        res.executed = false;
        res.halted = true;
        res.instruction_address = registers_.pc;
        return res;
    }
    if (state_ == MachineState::Faulted) {
        StepResult res;
        res.executed = false;
        res.faulted = true;
        res.instruction_address = registers_.pc;
        return res;
    }

    // 1. Capture starting instruction PC and initialize transient fetch cursor
    const word_t start_pc = registers_.pc;
    word_t cursor = start_pc;

    // 2. Fetch opcode byte
    const byte_t opcode_raw = bus_.read_byte(cursor);
    cursor = static_cast<word_t>((cursor + 1) & 0xFFFF);

    // 3. Pre-validate opcode
    if (!is_valid_opcode(opcode_raw)) {
        std::ostringstream oss;
        oss << "Undefined opcode 0x" << to_hex_byte(opcode_raw)
            << " at address 0x" << to_hex_word(start_pc);
        auto res = record_fault(FaultType::InvalidOpcode, start_pc, oss.str());
        res.opcode_raw = opcode_raw;
        return res;
    }

    // 4. Fetch operand bytes using transient cursor with modulo 65,536 wrapping
    const std::size_t len = instruction_length(opcode_raw);
    byte_t operands[3] = {0, 0, 0};
    for (std::size_t i = 1; i < len; ++i) {
        operands[i - 1] = bus_.read_byte(cursor);
        cursor = static_cast<word_t>((cursor + 1) & 0xFFFF);
    }

    // Lambda to update machine state upon successful non-faulting instruction completion
    auto update_state = [this](bool is_halt) {
        if (is_halt) {
            state_ = MachineState::Halted;
        } else if (state_ == MachineState::Ready) {
            state_ = MachineState::Running;
        }
        // If already Running or Paused, maintain respective state
    };

    const auto op = static_cast<Opcode>(opcode_raw);
    StepResult res;
    res.executed = true;
    res.opcode_raw = opcode_raw;
    res.opcode = op;
    res.instruction_address = start_pc;

    // 5. Decode, validate operands, and atomically commit instruction effects
    switch (op) {
        // -------------------------------------------------------------
        // Group 1: Basic Control & Data Movement (11 mnemonics)
        // -------------------------------------------------------------
        case Opcode::NOP: {
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::HALT: {
            registers_.pc = cursor;
            update_state(true);
            res.halted = true;
            return res;
        }

        case Opcode::LDI: {
            const byte_t reg = operands[0];
            const byte_t imm8 = operands[1];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for LDI: " + std::to_string(reg));
            }
            registers_.r[reg] = imm8;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::MOV: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for MOV: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            registers_.r[dst] = registers_.r[src];
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::LD: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for LD: " + std::to_string(reg));
            }
            const word_t addr = make_word(operands[1], operands[2]);
            const byte_t val = bus_.read_byte(addr);
            registers_.r[reg] = val;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::ST: {
            const word_t addr = make_word(operands[0], operands[1]);
            const byte_t reg = operands[2];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for ST: " + std::to_string(reg));
            }
            bus_.write_byte(addr, registers_.r[reg]);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::LDI16: {
            const word_t addr = make_word(operands[0], operands[1]);
            registers_.ix = addr;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::LDX: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for LDX: " + std::to_string(reg));
            }
            const byte_t val = bus_.read_byte(registers_.ix);
            registers_.r[reg] = val;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::STX: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for STX: " + std::to_string(reg));
            }
            bus_.write_byte(registers_.ix, registers_.r[reg]);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::INX: {
            registers_.ix = static_cast<word_t>((registers_.ix + 1) & 0xFFFF);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::DEX: {
            registers_.ix = static_cast<word_t>((registers_.ix - 1) & 0xFFFF);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        // -------------------------------------------------------------
        // Group 2: Arithmetic & Logic (9 mnemonics)
        // -------------------------------------------------------------
        case Opcode::ADD: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for ADD: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            const AluResult alu_res = Alu::add(registers_.r[dst], registers_.r[src]);
            registers_.r[dst] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::SUB: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for SUB: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            const AluResult alu_res = Alu::sub(registers_.r[dst], registers_.r[src]);
            registers_.r[dst] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::INC: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for INC: " + std::to_string(reg));
            }
            const AluResult alu_res = Alu::inc(registers_.r[reg], flags_);
            registers_.r[reg] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::DEC: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for DEC: " + std::to_string(reg));
            }
            const AluResult alu_res = Alu::dec(registers_.r[reg], flags_);
            registers_.r[reg] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::AND: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for AND: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            const AluResult alu_res = Alu::and_op(registers_.r[dst], registers_.r[src]);
            registers_.r[dst] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::OR: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for OR: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            const AluResult alu_res = Alu::or_op(registers_.r[dst], registers_.r[src]);
            registers_.r[dst] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::XOR: {
            const byte_t dst = operands[0];
            const byte_t src = operands[1];
            if (!is_valid_reg(dst) || !is_valid_reg(src)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for XOR: dst=" + std::to_string(dst) + " src=" + std::to_string(src));
            }
            const AluResult alu_res = Alu::xor_op(registers_.r[dst], registers_.r[src]);
            registers_.r[dst] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::NOT: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for NOT: " + std::to_string(reg));
            }
            const AluResult alu_res = Alu::not_op(registers_.r[reg]);
            registers_.r[reg] = alu_res.result;
            flags_ = alu_res.flags;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::CMP: {
            const byte_t left = operands[0];
            const byte_t right = operands[1];
            if (!is_valid_reg(left) || !is_valid_reg(right)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for CMP: left=" + std::to_string(left) + " right=" + std::to_string(right));
            }
            flags_ = Alu::cmp(registers_.r[left], registers_.r[right]);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        // -------------------------------------------------------------
        // Group 3: Branch, Stack & Subroutine (9 mnemonics)
        // -------------------------------------------------------------
        case Opcode::JMP: {
            const word_t target = make_word(operands[0], operands[1]);
            registers_.pc = target;
            update_state(false);
            return res;
        }

        case Opcode::JZ: {
            const word_t target = make_word(operands[0], operands[1]);
            registers_.pc = flags_.z ? target : cursor;
            update_state(false);
            return res;
        }

        case Opcode::JNZ: {
            const word_t target = make_word(operands[0], operands[1]);
            registers_.pc = (!flags_.z) ? target : cursor;
            update_state(false);
            return res;
        }

        case Opcode::JC: {
            const word_t target = make_word(operands[0], operands[1]);
            registers_.pc = flags_.c ? target : cursor;
            update_state(false);
            return res;
        }

        case Opcode::JNC: {
            const word_t target = make_word(operands[0], operands[1]);
            registers_.pc = (!flags_.c) ? target : cursor;
            update_state(false);
            return res;
        }

        case Opcode::PUSH: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for PUSH: " + std::to_string(reg));
            }
            // Requires SP > 0xF000. If SP <= 0xF000, StackOverflow
            if (registers_.sp <= kStackReservedStart) {
                return record_fault(FaultType::StackOverflow, start_pc,
                    "Stack overflow in PUSH: SP=0x" + to_hex_word(registers_.sp));
            }
            registers_.sp = static_cast<word_t>((registers_.sp - 1) & 0xFFFF);
            bus_.write_byte(registers_.sp, registers_.r[reg]);
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::POP: {
            const byte_t reg = operands[0];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for POP: " + std::to_string(reg));
            }
            // Requires SP < 0xFFFF. If SP >= 0xFFFF, StackUnderflow
            if (registers_.sp >= kStackResetValue) {
                return record_fault(FaultType::StackUnderflow, start_pc,
                    "Stack underflow in POP: SP=0x" + to_hex_word(registers_.sp));
            }
            const byte_t val = bus_.read_byte(registers_.sp);
            registers_.sp = static_cast<word_t>((registers_.sp + 1) & 0xFFFF);
            registers_.r[reg] = val;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::CALL: {
            // Requires SP >= 0xF002. If SP < 0xF002, StackOverflow
            if (registers_.sp < static_cast<word_t>(kStackReservedStart + 2)) {
                return record_fault(FaultType::StackOverflow, start_pc,
                    "Stack overflow in CALL: SP=0x" + to_hex_word(registers_.sp));
            }
            const word_t target = make_word(operands[0], operands[1]);
            const word_t ret_addr = cursor; // Return address is address following CALL instruction
            const byte_t ret_hi = high_byte(ret_addr);
            const byte_t ret_lo = low_byte(ret_addr);

            const word_t sp_new = static_cast<word_t>((registers_.sp - 2) & 0xFFFF);
            bus_.write_byte(sp_new, ret_hi);
            bus_.write_byte(static_cast<word_t>((sp_new + 1) & 0xFFFF), ret_lo);
            registers_.sp = sp_new;
            registers_.pc = target;
            update_state(false);
            return res;
        }

        case Opcode::RET: {
            // Requires SP <= 0xFFFD. If SP > 0xFFFD, StackUnderflow
            if (registers_.sp > static_cast<word_t>(kStackResetValue - 2)) {
                return record_fault(FaultType::StackUnderflow, start_pc,
                    "Stack underflow in RET: SP=0x" + to_hex_word(registers_.sp));
            }
            const byte_t ret_hi = bus_.read_byte(registers_.sp);
            const byte_t ret_lo = bus_.read_byte(static_cast<word_t>((registers_.sp + 1) & 0xFFFF));
            registers_.sp = static_cast<word_t>((registers_.sp + 2) & 0xFFFF);
            registers_.pc = make_word(ret_hi, ret_lo);
            update_state(false);
            return res;
        }

        // -------------------------------------------------------------
        // Group 4: Virtual Input/Output (2 mnemonics)
        // -------------------------------------------------------------
        case Opcode::IN: {
            const byte_t reg = operands[0];
            const byte_t port = operands[1];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for IN: " + std::to_string(reg));
            }
            if (!bus_.can_read_io(port)) {
                return record_fault(FaultType::InvalidPort, start_pc,
                    "Invalid port or direction mismatch for IN: 0x" + to_hex_byte(port));
            }
            const auto opt_val = bus_.read_io(port);
            if (!opt_val.has_value()) {
                return record_fault(FaultType::InvalidPort, start_pc,
                    "Read failed on port 0x" + to_hex_byte(port));
            }
            registers_.r[reg] = *opt_val;
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        case Opcode::OUT: {
            const byte_t port = operands[0];
            const byte_t reg = operands[1];
            if (!is_valid_reg(reg)) {
                return record_fault(FaultType::InvalidRegisterEncoding, start_pc,
                    "Invalid register ID for OUT: " + std::to_string(reg));
            }
            if (!bus_.can_write_io(port)) {
                return record_fault(FaultType::InvalidPort, start_pc,
                    "Invalid port or direction mismatch for OUT: 0x" + to_hex_byte(port));
            }
            const bool ok = bus_.write_io(port, registers_.r[reg]);
            if (!ok) {
                return record_fault(FaultType::InvalidPort, start_pc,
                    "Write failed on port 0x" + to_hex_byte(port));
            }
            registers_.pc = cursor;
            update_state(false);
            return res;
        }

        default: {
            return record_fault(FaultType::InvalidOpcode, start_pc, "Unhandled instruction opcode");
        }
    }
}

} // namespace bitforge
