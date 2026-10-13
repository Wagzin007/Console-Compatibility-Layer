#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "common/version.hpp"

TEST_CASE("a versão do projeto corresponde à configuração do build") {
    CHECK(ccl::version() == CCL_VERSION);
}
