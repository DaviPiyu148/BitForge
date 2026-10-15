#ifndef BITFORGE_ASSEMBLER_PROGRAM_IMAGE_HPP
#define BITFORGE_ASSEMBLER_PROGRAM_IMAGE_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/memory/memory.hpp"
#include "bitforge/assembler/source_map.hpp"

#include <vector>
#include <cstddef>

namespace bitforge::assembler {

// Contiguous block of assembled bytes loaded at a base address
struct Segment {
    word_t start_address{0};
    std::vector<byte_t> data;
};

// Result of successful assembly: entry point and assembled memory segments
class ProgramImage {
public:
    ProgramImage() = default;
    explicit ProgramImage(word_t entry_point) noexcept
        : entry_point_(entry_point) {}

    [[nodiscard]] word_t entry_point() const noexcept { return entry_point_; }
    void set_entry_point(word_t ep) noexcept { entry_point_ = ep; }

    [[nodiscard]] const std::vector<Segment>& segments() const noexcept { return segments_; }
    void add_segment(Segment seg) { segments_.push_back(std::move(seg)); }

    [[nodiscard]] const SourceMap& source_map() const noexcept { return source_map_; }
    void set_source_map(SourceMap sm) { source_map_ = std::move(sm); }

    // Total count of assembled bytes across all segments
    [[nodiscard]] std::size_t total_bytes() const noexcept {
        std::size_t count = 0;
        for (const auto& seg : segments_) {
            count += seg.data.size();
        }
        return count;
    }

    // Direct memory loading into the BitForge memory model
    bool load_into(Memory& memory) const {
        for (const auto& seg : segments_) {
            if (!seg.data.empty()) {
                if (!memory.load_segment(seg.start_address, seg.data.data(), seg.data.size())) {
                    return false;
                }
            }
        }
        return true;
    }

private:
    word_t entry_point_{0x0000};
    std::vector<Segment> segments_{};
    SourceMap source_map_{};
};

} // namespace bitforge::assembler

#endif // BITFORGE_ASSEMBLER_PROGRAM_IMAGE_HPP
