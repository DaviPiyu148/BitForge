#ifndef BITFORGE_ASSEMBLER_TOKEN_HPP
#define BITFORGE_ASSEMBLER_TOKEN_HPP

#include "bitforge/assembler/source_location.hpp"
#include <string>
#include <cstdint>
#include <string_view>

namespace bitforge::assembler {

enum class TokenKind {
    EndOfFile,
    Newline,
    Identifier,
    Directive,
    Number,
    String,
    Colon,
    Comma,
    LeftBracket,
    RightBracket,
    Plus,
    Minus
};

inline std::string_view to_string(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::EndOfFile:    return "end of file";
        case TokenKind::Newline:      return "newline";
        case TokenKind::Identifier:   return "identifier";
        case TokenKind::Directive:    return "directive";
        case TokenKind::Number:       return "number";
        case TokenKind::String:       return "string";
        case TokenKind::Colon:        return "':'";
        case TokenKind::Comma:        return "','";
        case TokenKind::LeftBracket:  return "'['";
        case TokenKind::RightBracket: return "']'";
        case TokenKind::Plus:         return "'+'";
        case TokenKind::Minus:        return "'-'";
        default:                      return "unknown";
    }
}

struct Token {
    TokenKind kind{TokenKind::EndOfFile};
    std::string lexeme{};
    std::int64_t number_value{0};
    SourceLocation location{};
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_TOKEN_HPP
