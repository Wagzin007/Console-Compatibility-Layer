#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "common/version.hpp"

TEST_CASE("a versão do projeto é 0.0.1") {
    CHECK(ccl::version() == "0.0.1");
}
