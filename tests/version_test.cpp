#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "common/version.hpp"

#include <string_view>

namespace {

bool has_x_y_z_format(std::string_view version) {
    std::size_t component_start = 0;
    std::size_t component_count = 0;
    for (std::size_t index = 0; index <= version.size(); ++index) {
        if (index != version.size() && version[index] != '.') {
            continue;
        }
        if (index == component_start) {
            return false;
        }
        for (std::size_t digit = component_start; digit < index; ++digit) {
            if (version[digit] < '0' || version[digit] > '9') {
                return false;
            }
        }
        ++component_count;
        component_start = index + 1;
    }
    return component_count == 3;
}

} // namespace

TEST_CASE("a versão do projeto segue o formato X.Y.Z") {
    CHECK(has_x_y_z_format(ccl::version()));
}
