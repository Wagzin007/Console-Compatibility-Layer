#pragma once

#include "common/error.hpp"
#include "common/result.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace ccl {

[[nodiscard]] Result<std::vector<std::uint8_t>, Error> read_file(
    const std::filesystem::path& path,
    std::size_t max_bytes);

} // namespace ccl
