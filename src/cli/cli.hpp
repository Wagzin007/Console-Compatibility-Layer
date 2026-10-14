#pragma once

#include <iosfwd>
#include <string_view>
#include <vector>

namespace ccl::cli {

[[nodiscard]] int run(
    const std::vector<std::string_view>& args,
    std::ostream& out,
    std::ostream& err);

} // namespace ccl::cli
