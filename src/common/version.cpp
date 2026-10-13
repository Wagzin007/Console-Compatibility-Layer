#include "common/version.hpp"

namespace ccl {

std::string_view version() noexcept {
    return CCL_VERSION;
}

} // namespace ccl
