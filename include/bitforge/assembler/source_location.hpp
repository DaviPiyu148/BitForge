#ifndef BITFORGE_ASSEMBLER_SOURCE_LOCATION_HPP
#define BITFORGE_ASSEMBLER_SOURCE_LOCATION_HPP

#include <cstddef>
#include <string>

namespace bitforge::assembler {

struct SourceLocation {
    std::size_t line{1};
    std::size_t column{1};

    std::string to_string() const {
        return std::to_string(line) + ":" + std::to_string(column);
    }
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_SOURCE_LOCATION_HPP
