#pragma once

#include <cstdint>

namespace ccl::gba {

// GBATEK §1.4.18, LCD Dimensions and Timings: 1232 cycles per line, 228 lines per frame.
inline constexpr std::uint64_t cycles_per_scanline = 1232;
inline constexpr std::uint64_t scanlines_per_frame = 228;
inline constexpr std::uint64_t cycles_per_frame = cycles_per_scanline * scanlines_per_frame;

static_assert(cycles_per_frame == 280896);

} // namespace ccl::gba
