#ifndef BITFORGE_ASSEMBLER_LEXER_HPP
#define BITFORGE_ASSEMBLER_LEXER_HPP

#include "bitforge/assembler/token.hpp"
#include "bitforge/assembler/diagnostic.hpp"

#include <string_view>
#include <vector>

namespace bitforge::assembler {

class Lexer {
public:
    explicit Lexer(std::string_view source) noexcept;

    // Tokenizes entire source into a vector of tokens, collecting any lexical errors
    std::vector<Token> tokenize(std::vector<Diagnostic>& diagnostics);

    // Get the next single token
    Token next_token(std::vector<Diagnostic>& diagnostics);

private:
    std::string_view source_;
    std::size_t cursor_{0};
    SourceLocation location_{1, 1};

    char peek() const noexcept;
    char peek_next() const noexcept;
    char advance() noexcept;
    bool is_at_end() const noexcept;

    void skip_whitespace_and_comments();
    Token scan_identifier_or_keyword();
    Token scan_directive(std::vector<Diagnostic>& diagnostics);
    Token scan_number(std::vector<Diagnostic>& diagnostics);
    Token scan_string(std::vector<Diagnostic>& diagnostics);
    Token scan_char(std::vector<Diagnostic>& diagnostics);
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_LEXER_HPP
