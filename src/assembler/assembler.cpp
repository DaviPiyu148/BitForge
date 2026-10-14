#include "bitforge/assembler/assembler.hpp"
#include "bitforge/assembler/lexer.hpp"
#include "bitforge/core/opcodes.hpp"
#include "bitforge/core/constants.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <optional>
#include <unordered_set>

namespace bitforge::assembler {

namespace {

std::string to_upper(std::string_view s) {
    std::string res(s);
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return res;
}

std::string to_hex_word(word_t w) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << w;
    return oss.str();
}

std::optional<byte_t> parse_register(std::string_view s) {
    std::string u = to_upper(s);
    if (u == "R0") return 0;
    if (u == "R1") return 1;
    if (u == "R2") return 2;
    if (u == "R3") return 3;
    return std::nullopt;
}

bool is_ix(std::string_view s) {
    return to_upper(s) == "IX";
}

} // namespace

AssembleResult Assembler::assemble(std::string_view source) {
    AssembleResult result;
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize(result.diagnostics);

    // If lexical errors occurred, abort before parsing
    for (const auto& diag : result.diagnostics) {
        if (diag.severity == DiagnosticSeverity::Error) {
            result.success = false;
            return result;
        }
    }

    // Helper parser state
    class Parser {
    public:
        Parser(const std::vector<Token>& tokens, AssembleResult& res, bool pass2)
            : tokens_(tokens), res_(res), pass2_(pass2) {}

        void run() {
            while (!is_at_end()) {
                skip_newlines();
                if (is_at_end()) break;
                parse_statement();
                skip_newlines();
            }
        }

        word_t entry_point() const noexcept { return entry_point_; }
        bool entry_point_explicitly_set() const noexcept { return entry_point_set_; }
        SourceLocation entry_point_location() const noexcept { return entry_point_loc_; }
        const std::vector<Segment>& segments() const noexcept { return segments_; }
        std::size_t instruction_count() const noexcept { return instruction_count_; }
        std::size_t total_bytes() const noexcept {
            std::size_t total = 0;
            for (const auto& seg : segments_) {
                total += seg.data.size();
            }
            return total;
        }

    private:
        const std::vector<Token>& tokens_;
        AssembleResult& res_;
        bool pass2_{false};

        std::size_t cursor_{0};
        word_t location_counter_{0x0000};
        word_t entry_point_{0x0000};
        bool entry_point_set_{false};
        SourceLocation entry_point_loc_{1, 1};
        std::size_t instruction_count_{0};

        std::vector<Segment> segments_{};
        std::unordered_set<word_t> written_addresses_{};

        const Token& peek() const {
            if (cursor_ >= tokens_.size()) return tokens_.back();
            return tokens_[cursor_];
        }

        const Token& peek_next() const {
            if (cursor_ + 1 >= tokens_.size()) return tokens_.back();
            return tokens_[cursor_ + 1];
        }

        const Token& advance() {
            const Token& t = peek();
            if (cursor_ < tokens_.size()) cursor_++;
            return t;
        }

        bool match(TokenKind kind) {
            if (peek().kind == kind) {
                advance();
                return true;
            }
            return false;
        }

        bool is_at_end() const {
            return peek().kind == TokenKind::EndOfFile;
        }

        void skip_newlines() {
            while (peek().kind == TokenKind::Newline) {
                advance();
            }
        }

        void add_error(SourceLocation loc, std::string msg) {
            res_.diagnostics.push_back(Diagnostic{DiagnosticSeverity::Error, loc, std::move(msg)});
        }

        void emit_byte(byte_t b, SourceLocation loc) {
            if (location_counter_ >= kStackReservedStart) {
                add_error(loc, "Cannot emit program data into reserved stack region (address " +
                               to_hex_word(location_counter_) + " >= 0xF000)");
                return;
            }

            if (pass2_) {
                if (written_addresses_.find(location_counter_) != written_addresses_.end()) {
                    add_error(loc, "Overlapping segment emission at address " + to_hex_word(location_counter_));
                } else {
                    written_addresses_.insert(location_counter_);
                }

                if (segments_.empty() || segments_.back().start_address + segments_.back().data.size() != location_counter_) {
                    segments_.push_back(Segment{location_counter_, {b}});
                } else {
                    segments_.back().data.push_back(b);
                }
            }

            location_counter_ = static_cast<word_t>((location_counter_ + 1) & 0xFFFF);
        }

        void emit_word(word_t w, SourceLocation loc) {
            emit_byte(high_byte(w), loc);
            emit_byte(low_byte(w), loc);
        }

        void expect_end_of_statement() {
            if (peek().kind != TokenKind::Newline && peek().kind != TokenKind::EndOfFile) {
                Token extra = advance();
                add_error(extra.location, "Unexpected trailing token: '" + extra.lexeme + "'");
                while (!is_at_end() && peek().kind != TokenKind::Newline) {
                    advance();
                }
            }
        }

        // Statement = [ Label ':' ] [ Directive | Instruction ]
        void parse_statement() {
            // Check for label definition
            if (peek().kind == TokenKind::Identifier && peek_next().kind == TokenKind::Colon) {
                Token label_tok = advance();
                advance(); // consume ':'

                if (!pass2_) {
                    if (!res_.symbols.define(label_tok.lexeme, location_counter_, label_tok.location)) {
                        add_error(label_tok.location, "Duplicate label definition: '" + label_tok.lexeme + "'");
                    }
                }

                // If end of statement, return
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    return;
                }
            }

            // Directives
            if (peek().kind == TokenKind::Directive) {
                parse_directive();
                expect_end_of_statement();
                return;
            }

            // Instructions
            if (peek().kind == TokenKind::Identifier) {
                parse_instruction();
                expect_end_of_statement();
                return;
            }

            // Unexpected token on line
            Token bad = advance();
            add_error(bad.location, "Unexpected token in statement: '" + bad.lexeme + "'");
            while (!is_at_end() && peek().kind != TokenKind::Newline) {
                advance();
            }
        }

        void parse_directive() {
            Token dir_tok = advance();
            std::string dir = to_upper(dir_tok.lexeme);

            if (dir == ".ORG") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(dir_tok.location, "Missing address operand for .ORG");
                    return;
                }
                auto val = parse_expression();
                if (!val) {
                    add_error(dir_tok.location, "Expected origin address after .ORG");
                    return;
                }
                if (*val < 0 || *val > 0xFFFF) {
                    add_error(dir_tok.location, ".ORG address out of range [0x0000, 0xFFFF]: " + std::to_string(*val));
                    return;
                }
                if (*val >= kStackReservedStart) {
                    add_error(dir_tok.location, "Cannot set .ORG into reserved stack region (address " +
                                                   to_hex_word(static_cast<word_t>(*val)) + " >= 0xF000)");
                    return;
                }
                location_counter_ = static_cast<word_t>(*val);
                return;
            }

            if (dir == ".ENTRY") {
                if (entry_point_set_) {
                    add_error(dir_tok.location, "Duplicate .ENTRY directive: multiple entry points not permitted");
                    return;
                }
                entry_point_loc_ = dir_tok.location;
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(dir_tok.location, "Missing label or address operand for .ENTRY");
                    return;
                }
                Token target_tok = peek();
                if (target_tok.kind == TokenKind::Identifier) {
                    advance();
                    if (pass2_) {
                        const Symbol* sym = res_.symbols.lookup(target_tok.lexeme);
                        if (!sym) {
                            add_error(target_tok.location, "Unresolved entry label: '" + target_tok.lexeme + "'");
                        } else {
                            if (sym->address >= kStackReservedStart) {
                                add_error(target_tok.location, "Entry address " + to_hex_word(sym->address) +
                                                               " cannot be in reserved stack region (>= 0xF000)");
                            } else {
                                entry_point_ = sym->address;
                                entry_point_set_ = true;
                            }
                        }
                    } else {
                        entry_point_set_ = true;
                    }
                    return;
                }
                auto val = parse_expression();
                if (!val) {
                    add_error(dir_tok.location, "Expected entry address or label after .ENTRY");
                    return;
                }
                if (*val < 0 || *val > 0xFFFF) {
                    add_error(dir_tok.location, "Entry address out of range [0x0000, 0xFFFF]: " + std::to_string(*val));
                    return;
                }
                if (*val >= kStackReservedStart) {
                    add_error(dir_tok.location, "Entry address " + to_hex_word(static_cast<word_t>(*val)) +
                                                   " cannot be in reserved stack region (>= 0xF000)");
                    return;
                }
                entry_point_ = static_cast<word_t>(*val);
                entry_point_set_ = true;
                return;
            }

            if (dir == ".BYTE") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(dir_tok.location, "Missing byte value operand for .BYTE");
                    return;
                }
                do {
                    SourceLocation arg_loc = peek().location;
                    auto val = parse_expression();
                    if (!val) {
                        add_error(arg_loc, "Expected byte value expression after .BYTE");
                        break;
                    }
                    if (*val < -128 || *val > 255) {
                        add_error(arg_loc, ".BYTE value out of range [-128, 255]: " + std::to_string(*val));
                    }
                    emit_byte(static_cast<byte_t>(*val & 0xFF), arg_loc);
                } while (match(TokenKind::Comma));
                return;
            }

            if (dir == ".WORD") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(dir_tok.location, "Missing word value operand for .WORD");
                    return;
                }
                do {
                    SourceLocation arg_loc = peek().location;
                    auto val = parse_expression();
                    if (!val) {
                        add_error(arg_loc, "Expected word value expression after .WORD");
                        break;
                    }
                    if (*val < -32768 || *val > 65535) {
                        add_error(arg_loc, ".WORD value out of range [-32768, 65535]: " + std::to_string(*val));
                    }
                    emit_word(static_cast<word_t>(*val & 0xFFFF), arg_loc);
                } while (match(TokenKind::Comma));
                return;
            }

            if (dir == ".ASCII") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(dir_tok.location, "Missing string literal operand for .ASCII");
                    return;
                }
                if (peek().kind != TokenKind::String) {
                    add_error(dir_tok.location, "Expected string literal after .ASCII");
                    return;
                }
                Token str_tok = advance();
                for (char c : str_tok.lexeme) {
                    emit_byte(static_cast<byte_t>(c), str_tok.location);
                }
                return;
            }

            add_error(dir_tok.location, "Unknown directive: '" + dir_tok.lexeme + "'");
        }

        std::optional<std::int64_t> parse_expression() {
            if (peek().kind == TokenKind::Minus) {
                advance();
                auto inner = parse_primary();
                if (!inner) return std::nullopt;
                return -(*inner);
            }
            if (peek().kind == TokenKind::Plus) {
                advance();
            }
            auto left = parse_primary();
            if (!left) return std::nullopt;

            while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
                Token op = advance();
                auto right = parse_primary();
                if (!right) return std::nullopt;
                if (op.kind == TokenKind::Plus) {
                    left = *left + *right;
                } else {
                    left = *left - *right;
                }
            }
            return left;
        }

        std::optional<std::int64_t> parse_primary() {
            if (peek().kind == TokenKind::Number) {
                Token tok = advance();
                return tok.number_value;
            }
            if (peek().kind == TokenKind::Identifier) {
                Token tok = advance();
                if (!pass2_) {
                    // In Pass 1, symbols may not yet be defined; return 0 for sizing
                    return 0;
                }
                const Symbol* sym = res_.symbols.lookup(tok.lexeme);
                if (!sym) {
                    add_error(tok.location, "Unresolved label: '" + tok.lexeme + "'");
                    return 0;
                }
                return static_cast<std::int64_t>(sym->address);
            }
            return std::nullopt;
        }

        void parse_instruction() {
            instruction_count_++;
            Token mnem_tok = advance();
            std::string mnem = to_upper(mnem_tok.lexeme);

            // Group 1: 0 operands (NOP, HALT, INX, DEX, RET)
            if (mnem == "NOP")  { emit_byte(static_cast<byte_t>(Opcode::NOP), mnem_tok.location); return; }
            if (mnem == "HALT") { emit_byte(static_cast<byte_t>(Opcode::HALT), mnem_tok.location); return; }
            if (mnem == "INX")  { emit_byte(static_cast<byte_t>(Opcode::INX), mnem_tok.location); return; }
            if (mnem == "DEX")  { emit_byte(static_cast<byte_t>(Opcode::DEX), mnem_tok.location); return; }
            if (mnem == "RET")  { emit_byte(static_cast<byte_t>(Opcode::RET), mnem_tok.location); return; }

            // Group 2: 1 register operand (INC, DEC, NOT, PUSH, POP)
            if (mnem == "INC" || mnem == "DEC" || mnem == "NOT" || mnem == "PUSH" || mnem == "POP") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing register operand for " + mnem);
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid register '" + reg_tok.lexeme + "' for " + mnem);
                    return;
                }
                byte_t op = 0;
                if (mnem == "INC")  op = static_cast<byte_t>(Opcode::INC);
                if (mnem == "DEC")  op = static_cast<byte_t>(Opcode::DEC);
                if (mnem == "NOT")  op = static_cast<byte_t>(Opcode::NOT);
                if (mnem == "PUSH") op = static_cast<byte_t>(Opcode::PUSH);
                if (mnem == "POP")  op = static_cast<byte_t>(Opcode::POP);
                emit_byte(op, mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                return;
            }

            // Group 3: 2 register operands (MOV, ADD, SUB, AND, OR, XOR, CMP)
            if (mnem == "MOV" || mnem == "ADD" || mnem == "SUB" || mnem == "AND" ||
                mnem == "OR"  || mnem == "XOR" || mnem == "CMP") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing destination register for " + mnem);
                    return;
                }
                Token r1_tok = advance();
                auto r1 = parse_register(r1_tok.lexeme);
                if (!r1) {
                    add_error(r1_tok.location, "Invalid register '" + r1_tok.lexeme + "' in first operand of " + mnem);
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' between operands in " + mnem);
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing source register for " + mnem);
                    return;
                }
                Token r2_tok = advance();
                auto r2 = parse_register(r2_tok.lexeme);
                if (!r2) {
                    add_error(r2_tok.location, "Invalid register '" + r2_tok.lexeme + "' in second operand of " + mnem);
                    return;
                }
                byte_t op = 0;
                if (mnem == "MOV") op = static_cast<byte_t>(Opcode::MOV);
                if (mnem == "ADD") op = static_cast<byte_t>(Opcode::ADD);
                if (mnem == "SUB") op = static_cast<byte_t>(Opcode::SUB);
                if (mnem == "AND") op = static_cast<byte_t>(Opcode::AND);
                if (mnem == "OR")  op = static_cast<byte_t>(Opcode::OR);
                if (mnem == "XOR") op = static_cast<byte_t>(Opcode::XOR);
                if (mnem == "CMP") op = static_cast<byte_t>(Opcode::CMP);
                emit_byte(op, mnem_tok.location);
                emit_byte(*r1, r1_tok.location);
                emit_byte(*r2, r2_tok.location);
                return;
            }

            // Group 4: LDI Rd, imm8
            if (mnem == "LDI") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing destination register for LDI");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid destination register '" + reg_tok.lexeme + "' for LDI");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after register in LDI");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing immediate value operand for LDI");
                    return;
                }
                SourceLocation imm_loc = peek().location;
                auto imm = parse_expression();
                if (!imm) {
                    add_error(imm_loc, "Expected 8-bit immediate expression for LDI");
                    return;
                }
                if (*imm < -128 || *imm > 255) {
                    add_error(imm_loc, "Immediate value out of range [-128, 255] for LDI: " + std::to_string(*imm));
                }
                emit_byte(static_cast<byte_t>(Opcode::LDI), mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                emit_byte(static_cast<byte_t>(*imm & 0xFF), imm_loc);
                return;
            }

            // Group 5: LDI16 IX, addr16
            if (mnem == "LDI16") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing IX register operand for LDI16");
                    return;
                }
                Token ix_tok = advance();
                if (!is_ix(ix_tok.lexeme)) {
                    add_error(ix_tok.location, "First operand of LDI16 must be IX");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after IX in LDI16");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing address operand for LDI16");
                    return;
                }
                SourceLocation addr_loc = peek().location;
                auto addr = parse_expression();
                if (!addr) {
                    add_error(addr_loc, "Expected 16-bit address or label for LDI16");
                    return;
                }
                if (*addr < -32768 || *addr > 65535) {
                    add_error(addr_loc, "Address value out of range [-32768, 65535] for LDI16: " + std::to_string(*addr));
                }
                emit_byte(static_cast<byte_t>(Opcode::LDI16), mnem_tok.location);
                emit_word(static_cast<word_t>(*addr & 0xFFFF), addr_loc);
                return;
            }

            // Group 6: LD Rd, [addr16]
            if (mnem == "LD") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing destination register for LD");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid destination register '" + reg_tok.lexeme + "' for LD");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after register in LD");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing memory operand for LD");
                    return;
                }
                if (!match(TokenKind::LeftBracket)) {
                    add_error(peek().location, "Expected '[' for memory operand in LD");
                    return;
                }
                SourceLocation addr_loc = peek().location;
                auto addr = parse_expression();
                if (!addr) {
                    add_error(addr_loc, "Expected 16-bit address or label inside brackets for LD");
                    return;
                }
                if (!match(TokenKind::RightBracket)) {
                    add_error(peek().location, "Expected ']' closing memory operand in LD");
                    return;
                }
                if (*addr < -32768 || *addr > 65535) {
                    add_error(addr_loc, "Address value out of range [-32768, 65535] for LD: " + std::to_string(*addr));
                }
                emit_byte(static_cast<byte_t>(Opcode::LD), mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                emit_word(static_cast<word_t>(*addr & 0xFFFF), addr_loc);
                return;
            }

            // Group 7: ST [addr16], Rs
            if (mnem == "ST") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing memory destination operand for ST");
                    return;
                }
                if (!match(TokenKind::LeftBracket)) {
                    add_error(peek().location, "Expected '[' for memory destination in ST");
                    return;
                }
                SourceLocation addr_loc = peek().location;
                auto addr = parse_expression();
                if (!addr) {
                    add_error(addr_loc, "Expected 16-bit address or label inside brackets for ST");
                    return;
                }
                if (!match(TokenKind::RightBracket)) {
                    add_error(peek().location, "Expected ']' closing memory operand in ST");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after memory operand in ST");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing source register for ST");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid source register '" + reg_tok.lexeme + "' for ST");
                    return;
                }
                if (*addr < -32768 || *addr > 65535) {
                    add_error(addr_loc, "Address value out of range [-32768, 65535] for ST: " + std::to_string(*addr));
                }
                emit_byte(static_cast<byte_t>(Opcode::ST), mnem_tok.location);
                emit_word(static_cast<word_t>(*addr & 0xFFFF), addr_loc);
                emit_byte(*reg, reg_tok.location);
                return;
            }

            // Group 8: LDX Rd, [IX]
            if (mnem == "LDX") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing destination register for LDX");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid destination register '" + reg_tok.lexeme + "' for LDX");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after register in LDX");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing [IX] operand for LDX");
                    return;
                }
                if (!match(TokenKind::LeftBracket)) {
                    add_error(peek().location, "Expected '[IX]' operand for LDX");
                    return;
                }
                Token ix_tok = advance();
                if (!is_ix(ix_tok.lexeme) || !match(TokenKind::RightBracket)) {
                    add_error(ix_tok.location, "Expected '[IX]' as second operand for LDX");
                    return;
                }
                emit_byte(static_cast<byte_t>(Opcode::LDX), mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                return;
            }

            // Group 9: STX [IX], Rs
            if (mnem == "STX") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing [IX] destination operand for STX");
                    return;
                }
                if (!match(TokenKind::LeftBracket)) {
                    add_error(peek().location, "Expected '[IX]' as first operand for STX");
                    return;
                }
                Token ix_tok = advance();
                if (!is_ix(ix_tok.lexeme) || !match(TokenKind::RightBracket)) {
                    add_error(ix_tok.location, "Expected '[IX]' as first operand for STX");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after [IX] in STX");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing source register for STX");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid source register '" + reg_tok.lexeme + "' for STX");
                    return;
                }
                emit_byte(static_cast<byte_t>(Opcode::STX), mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                return;
            }

            // Group 10: Branch and Call (JMP, JZ, JNZ, JC, JNC, CALL)
            if (mnem == "JMP" || mnem == "JZ" || mnem == "JNZ" ||
                mnem == "JC"  || mnem == "JNC" || mnem == "CALL") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing target address or label for " + mnem);
                    return;
                }
                SourceLocation target_loc = peek().location;
                auto target = parse_expression();
                if (!target) {
                    add_error(target_loc, "Expected target address or label for " + mnem);
                    return;
                }
                if (*target < -32768 || *target > 65535) {
                    add_error(target_loc, "Target address out of range [-32768, 65535] for " + mnem + ": " + std::to_string(*target));
                }
                byte_t op = 0;
                if (mnem == "JMP")  op = static_cast<byte_t>(Opcode::JMP);
                if (mnem == "JZ")   op = static_cast<byte_t>(Opcode::JZ);
                if (mnem == "JNZ")  op = static_cast<byte_t>(Opcode::JNZ);
                if (mnem == "JC")   op = static_cast<byte_t>(Opcode::JC);
                if (mnem == "JNC")  op = static_cast<byte_t>(Opcode::JNC);
                if (mnem == "CALL") op = static_cast<byte_t>(Opcode::CALL);
                emit_byte(op, mnem_tok.location);
                emit_word(static_cast<word_t>(*target & 0xFFFF), target_loc);
                return;
            }

            // Group 11: IN Rd, port8
            if (mnem == "IN") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing destination register for IN");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid destination register '" + reg_tok.lexeme + "' for IN");
                    return;
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after register in IN");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing port operand for IN");
                    return;
                }
                SourceLocation port_loc = peek().location;
                auto port = parse_expression();
                if (!port) {
                    add_error(port_loc, "Expected 8-bit port expression for IN");
                    return;
                }
                if (*port < 0 || *port > 255) {
                    add_error(port_loc, "Port out of range [0, 255] for IN: " + std::to_string(*port));
                }
                emit_byte(static_cast<byte_t>(Opcode::IN), mnem_tok.location);
                emit_byte(*reg, reg_tok.location);
                emit_byte(static_cast<byte_t>(*port & 0xFF), port_loc);
                return;
            }

            // Group 12: OUT port8, Rs
            if (mnem == "OUT") {
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(mnem_tok.location, "Missing port operand for OUT");
                    return;
                }
                SourceLocation port_loc = peek().location;
                auto port = parse_expression();
                if (!port) {
                    add_error(port_loc, "Expected 8-bit port expression for OUT");
                    return;
                }
                if (*port < 0 || *port > 255) {
                    add_error(port_loc, "Port out of range [0, 255] for OUT: " + std::to_string(*port));
                }
                if (!match(TokenKind::Comma)) {
                    add_error(peek().location, "Expected ',' after port in OUT");
                    return;
                }
                if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::EndOfFile) {
                    add_error(peek().location, "Missing source register for OUT");
                    return;
                }
                Token reg_tok = advance();
                auto reg = parse_register(reg_tok.lexeme);
                if (!reg) {
                    add_error(reg_tok.location, "Invalid source register '" + reg_tok.lexeme + "' for OUT");
                    return;
                }
                emit_byte(static_cast<byte_t>(Opcode::OUT), mnem_tok.location);
                emit_byte(static_cast<byte_t>(*port & 0xFF), port_loc);
                emit_byte(*reg, reg_tok.location);
                return;
            }

            add_error(mnem_tok.location, "Unknown instruction mnemonic: '" + mnem_tok.lexeme + "'");
        }
    };

    // -------------------------------------------------------------
    // PASS 1: Location Counter Tracking & Symbol Table Population
    // -------------------------------------------------------------
    Parser pass1(tokens, result, false);
    pass1.run();

    // If Pass 1 encountered errors, stop before Pass 2
    for (const auto& diag : result.diagnostics) {
        if (diag.severity == DiagnosticSeverity::Error) {
            result.success = false;
            return result;
        }
    }

    // -------------------------------------------------------------
    // PASS 2: Symbol Resolution & Machine Code Encoding
    // -------------------------------------------------------------
    Parser pass2(tokens, result, true);
    pass2.run();

    // Check diagnostics after Pass 2
    for (const auto& diag : result.diagnostics) {
        if (diag.severity == DiagnosticSeverity::Error) {
            result.success = false;
            return result;
        }
    }

    // 1. Empty program check: must have emitted at least one byte
    if (pass2.segments().empty() || pass2.total_bytes() == 0) {
        result.diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            SourceLocation{1, 1},
            "Empty program: no instructions or data emitted"
        });
        result.success = false;
        return result;
    }

    // 2. Explicit entry point verification: must point to an emitted byte outside uninitialized gaps
    if (pass2.entry_point_explicitly_set()) {
        word_t ep = pass2.entry_point();
        bool found_emitted = false;
        for (const auto& seg : pass2.segments()) {
            if (ep >= seg.start_address &&
                static_cast<std::size_t>(ep - seg.start_address) < seg.data.size()) {
                found_emitted = true;
                break;
            }
        }
        if (!found_emitted) {
            result.diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                pass2.entry_point_location(),
                "Explicit entry point " + to_hex_word(ep) + " does not point to an emitted byte (uninitialized memory gap)"
            });
            result.success = false;
            return result;
        }
    }

    // 3. Data-only program diagnostic: if no executable instructions were emitted
    if (pass2.instruction_count() == 0) {
        result.diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Warning,
            SourceLocation{1, 1},
            "Program contains only data declarations (.BYTE, .WORD, .ASCII) and no executable instructions"
        });
    }

    // Sort segments by start address
    auto sorted_segments = pass2.segments();
    std::sort(sorted_segments.begin(), sorted_segments.end(), [](const Segment& a, const Segment& b) {
        return a.start_address < b.start_address;
    });

    // Build ProgramImage
    ProgramImage image(pass2.entry_point());
    for (auto& seg : sorted_segments) {
        image.add_segment(std::move(seg));
    }

    result.image = std::move(image);
    result.success = true;
    return result;
}

} // namespace bitforge::assembler
