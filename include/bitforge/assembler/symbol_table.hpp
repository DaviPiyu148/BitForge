#ifndef BITFORGE_ASSEMBLER_SYMBOL_TABLE_HPP
#define BITFORGE_ASSEMBLER_SYMBOL_TABLE_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/assembler/source_location.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace bitforge::assembler {

struct Symbol {
    std::string name;
    word_t address{0};
    SourceLocation defined_at{};
};

class SymbolTable {
public:
    static std::string normalize(std::string_view name) {
        std::string s(name);
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        return s;
    }

    bool define(std::string name, word_t address, SourceLocation loc) {
        std::string key = normalize(name);
        if (symbols_.find(key) != symbols_.end()) {
            return false; // Duplicate symbol
        }
        symbols_[key] = Symbol{std::move(name), address, loc};
        return true;
    }

    [[nodiscard]] const Symbol* lookup(std::string_view name) const {
        std::string key = normalize(name);
        auto it = symbols_.find(key);
        if (it != symbols_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] bool contains(std::string_view name) const {
        return lookup(name) != nullptr;
    }

    [[nodiscard]] const std::unordered_map<std::string, Symbol>& symbols() const noexcept {
        return symbols_;
    }

    void clear() noexcept {
        symbols_.clear();
    }

private:
    std::unordered_map<std::string, Symbol> symbols_{};
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_SYMBOL_TABLE_HPP
