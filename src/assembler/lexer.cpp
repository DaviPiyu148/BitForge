#include "bitforge/assembler/lexer.hpp"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>

namespace bitforge::assembler {

Lexer::Lexer(std::string_view source) noexcept
    : source_(source) {}

char Lexer::peek() const noexcept {
    if (is_at_end()) return '\0';
    return source_[cursor_];
}

char Lexer::peek_next() const noexcept {
    if (cursor_ + 1 >= source_.size()) return '\0';
    return source_[cursor_ + 1];
}

char Lexer::advance() noexcept {
    char c = source_[cursor_++];
    location_.column++;
    return c;
}

bool Lexer::is_at_end() const noexcept {
    return cursor_ >= source_.size();
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t') {
            advance();
        } else if (c == ';') {
            // Comment extends until newline or end of file
            while (!is_at_end() && peek() != '\n' && peek() != '\r') {
                advance();
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize(std::vector<Diagnostic>& diagnostics) {
    std::vector<Token> tokens;
    while (true) {
        Token tok = next_token(diagnostics);
        tokens.push_back(tok);
        if (tok.kind == TokenKind::EndOfFile) {
            break;
        }
    }
    return tokens;
}

Token Lexer::next_token(std::vector<Diagnostic>& diagnostics) {
    skip_whitespace_and_comments();

    const SourceLocation loc = location_;

    if (is_at_end()) {
        return Token{TokenKind::EndOfFile, "", 0, loc};
    }

    char c = peek();

    // 1. Newline normalization (handles CRLF, LF, CR)
    if (c == '\r') {
        advance();
        if (peek() == '\n') {
            advance();
        }
        location_.line++;
        location_.column = 1;
        return Token{TokenKind::Newline, "\n", 0, loc};
    }
    if (c == '\n') {
        advance();
        location_.line++;
        location_.column = 1;
        return Token{TokenKind::Newline, "\n", 0, loc};
    }

    // 2. Single-character punctuation
    if (c == ':') {
        advance();
        return Token{TokenKind::Colon, ":", 0, loc};
    }
    if (c == ',') {
        advance();
        return Token{TokenKind::Comma, ",", 0, loc};
    }
    if (c == '[') {
        advance();
        return Token{TokenKind::LeftBracket, "[", 0, loc};
    }
    if (c == ']') {
        advance();
        return Token{TokenKind::RightBracket, "]", 0, loc};
    }
    if (c == '+') {
        advance();
        return Token{TokenKind::Plus, "+", 0, loc};
    }
    if (c == '-') {
        // Check if followed immediately by a digit (negative number)
        if (std::isdigit(static_cast<unsigned char>(peek_next()))) {
            return scan_number(diagnostics);
        }
        advance();
        return Token{TokenKind::Minus, "-", 0, loc};
    }

    // 3. String literal
    if (c == '"') {
        return scan_string(diagnostics);
    }

    // 4. Character literal
    if (c == '\'') {
        return scan_char(diagnostics);
    }

    // 5. Directives (e.g. .ORG, .ENTRY, .BYTE, .WORD, .ASCII)
    if (c == '.') {
        return scan_directive(diagnostics);
    }

    // 6. Number literals (decimal, 0x hex, 0b binary)
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number(diagnostics);
    }

    // 7. Identifiers (labels, mnemonics, registers)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scan_identifier_or_keyword();
    }

    // Unexpected character
    char bad_char = advance();
    diagnostics.push_back(Diagnostic{
        DiagnosticSeverity::Error,
        loc,
        "Unexpected character: '" + std::string(1, bad_char) + "'"
    });
    return next_token(diagnostics);
}

Token Lexer::scan_identifier_or_keyword() {
    const SourceLocation loc = location_;
    std::string text;
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        text += advance();
    }
    return Token{TokenKind::Identifier, std::move(text), 0, loc};
}

Token Lexer::scan_directive(std::vector<Diagnostic>& diagnostics) {
    const SourceLocation loc = location_;
    std::string text;
    text += advance(); // consume '.'

    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        text += advance();
    }

    if (text.size() == 1) {
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            "Invalid directive: expected directive name after '.'"
        });
    }

    return Token{TokenKind::Directive, std::move(text), 0, loc};
}

Token Lexer::scan_number(std::vector<Diagnostic>& diagnostics) {
    const SourceLocation loc = location_;
    bool negative = false;
    if (peek() == '-') {
        negative = true;
        advance();
    }

    std::string text;
    // Check for base prefixes: 0x (hex) or 0b (binary)
    if (peek() == '0' && (peek_next() == 'x' || peek_next() == 'X')) {
        advance(); // '0'
        advance(); // 'x'
        while (!is_at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
            text += advance();
        }
        if (text.empty()) {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Malformed hexadecimal literal: missing hex digits"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        if (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
            std::string bad_suffix;
            while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                bad_suffix += advance();
            }
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Invalid character in hexadecimal literal: '0x" + text + bad_suffix + "'"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        errno = 0;
        char* end = nullptr;
        unsigned long long val = std::strtoull(text.c_str(), &end, 16);
        if (errno == ERANGE || (text.size() > 16 && val == ULLONG_MAX)) {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Hexadecimal literal out of representable integer range: '0x" + text + "'"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        std::int64_t final_val = negative ? -static_cast<std::int64_t>(val) : static_cast<std::int64_t>(val);
        return Token{TokenKind::Number, (negative ? "-" : "") + text, final_val, loc};
    }

    if (peek() == '0' && (peek_next() == 'b' || peek_next() == 'B')) {
        advance(); // '0'
        advance(); // 'b'
        while (!is_at_end() && (peek() == '0' || peek() == '1')) {
            text += advance();
        }
        if (text.empty()) {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Malformed binary literal: missing binary digits"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        if (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
            std::string bad_suffix;
            while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                bad_suffix += advance();
            }
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Invalid digit in binary literal: '0b" + text + bad_suffix + "'"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        errno = 0;
        char* end = nullptr;
        unsigned long long val = std::strtoull(text.c_str(), &end, 2);
        if (errno == ERANGE || (text.size() > 64 && val == ULLONG_MAX)) {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Binary literal out of representable integer range: '0b" + text + "'"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        std::int64_t final_val = negative ? -static_cast<std::int64_t>(val) : static_cast<std::int64_t>(val);
        return Token{TokenKind::Number, (negative ? "-" : "") + text, final_val, loc};
    }

    // Standard decimal integer
    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text += advance();
    }
    if (!is_at_end() && (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_')) {
        std::string bad_suffix;
        while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
            bad_suffix += advance();
        }
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            "Invalid suffix on numeric literal: '" + text + bad_suffix + "'"
        });
        return Token{TokenKind::Number, "0", 0, loc};
    }

    errno = 0;
    char* end = nullptr;
    unsigned long long val = std::strtoull(text.c_str(), &end, 10);
    if (errno == ERANGE || (text.size() > 19 && val == ULLONG_MAX)) {
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            std::string("Decimal literal out of representable integer range: '") + (negative ? "-" : "") + text + "'"
        });
        return Token{TokenKind::Number, "0", 0, loc};
    }
    std::int64_t final_val = negative ? -static_cast<std::int64_t>(val) : static_cast<std::int64_t>(val);
    return Token{TokenKind::Number, (negative ? "-" : "") + text, final_val, loc};
}

Token Lexer::scan_string(std::vector<Diagnostic>& diagnostics) {
    const SourceLocation loc = location_;
    advance(); // Consume leading '"'

    std::string text;
    while (!is_at_end() && peek() != '"') {
        if (peek() == '\n' || peek() == '\r') {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Unterminated string literal (newline before closing '\"')"
            });
            return Token{TokenKind::String, text, 0, loc};
        }
        if (peek() == '\\') {
            advance(); // Consume '\'
            if (is_at_end()) break;
            char esc = advance();
            switch (esc) {
                case 'n':  text += '\n'; break;
                case 't':  text += '\t'; break;
                case 'r':  text += '\r'; break;
                case '\\': text += '\\'; break;
                case '"':  text += '"'; break;
                case '0':  text += '\0'; break;
                default:
                    diagnostics.push_back(Diagnostic{
                        DiagnosticSeverity::Error,
                        loc,
                        "Unknown escape sequence '\\" + std::string(1, esc) + "'"
                    });
                    text += esc;
                    break;
            }
        } else {
            text += advance();
        }
    }

    if (is_at_end()) {
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            "Unterminated string literal (end of file reached)"
        });
    } else {
        advance(); // Consume trailing '"'
    }

    return Token{TokenKind::String, std::move(text), 0, loc};
}

Token Lexer::scan_char(std::vector<Diagnostic>& diagnostics) {
    const SourceLocation loc = location_;
    advance(); // Consume leading '\''

    if (is_at_end() || peek() == '\'') {
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            "Empty character literal"
        });
        if (!is_at_end()) advance();
        return Token{TokenKind::Number, "0", 0, loc};
    }

    char c = advance();
    if (c == '\\') {
        if (is_at_end()) {
            diagnostics.push_back(Diagnostic{
                DiagnosticSeverity::Error,
                loc,
                "Unterminated character escape sequence"
            });
            return Token{TokenKind::Number, "0", 0, loc};
        }
        char esc = advance();
        switch (esc) {
            case 'n':  c = '\n'; break;
            case 't':  c = '\t'; break;
            case 'r':  c = '\r'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            case '0':  c = '\0'; break;
            default:
                diagnostics.push_back(Diagnostic{
                    DiagnosticSeverity::Error,
                    loc,
                    "Unknown character escape sequence"
                });
                break;
        }
    }

    if (is_at_end() || peek() != '\'') {
        diagnostics.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            loc,
            "Multi-character or unterminated character literal"
        });
    } else {
        advance(); // Consume trailing '\''
    }

    return Token{TokenKind::Number, std::string(1, c), static_cast<std::int64_t>(static_cast<unsigned char>(c)), loc};
}

} // namespace bitforge::assembler
