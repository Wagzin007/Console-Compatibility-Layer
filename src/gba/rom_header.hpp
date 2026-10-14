#pragma once

#include "common/error.hpp"
#include "common/result.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ccl::gba {

inline constexpr std::size_t rom_header_size = 192;
inline constexpr std::size_t max_rom_size = 0x2000000;

struct RomHeader {
    std::uint32_t entry_instruction{};
    std::string title;
    std::string game_code;
    std::string maker_code;
    std::uint8_t fixed_value{};
    std::uint8_t unit_code{};
    std::uint8_t software_version{};
    std::uint8_t checksum_stored{};
    std::uint8_t checksum_calculated{};
    std::vector<std::string> warnings;
};

[[nodiscard]] std::optional<Error> validate_rom_size(std::size_t size);
[[nodiscard]] Result<RomHeader, Error> parse_header(std::span<const std::uint8_t> bytes);

} // namespace ccl::gba
