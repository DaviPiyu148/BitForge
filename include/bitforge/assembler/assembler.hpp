#ifndef BITFORGE_ASSEMBLER_ASSEMBLER_HPP
#define BITFORGE_ASSEMBLER_ASSEMBLER_HPP

#include "bitforge/assembler/program_image.hpp"
#include "bitforge/assembler/diagnostic.hpp"
#include "bitforge/assembler/symbol_table.hpp"

#include <string_view>
#include <vector>

namespace bitforge::assembler {

struct AssembleResult {
    bool success{false};
    ProgramImage image{};
    std::vector<Diagnostic> diagnostics{};
    SymbolTable symbols{};
};

class Assembler {
public:
    AssembleResult assemble(std::string_view source);
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_ASSEMBLER_HPP
