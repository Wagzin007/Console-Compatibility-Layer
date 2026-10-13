cmake_minimum_required(VERSION 3.21)

project(ccl VERSION 0.0.1 LANGUAGES CXX)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(doctest REQUIRED)

add_library(ccl_common STATIC
    src/common/version.cpp
)
target_include_directories(ccl_common PUBLIC src)
target_compile_definitions(ccl_common PUBLIC CCL_VERSION="${PROJECT_VERSION}")

add_executable(ccl-run src/cli/main.cpp)
target_link_libraries(ccl-run PRIVATE ccl_common)

include(CTest)
if(BUILD_TESTING)
    add_executable(ccl-version-test tests/version_test.cpp)
    target_link_libraries(ccl-version-test PRIVATE ccl_common doctest::doctest)
    add_test(NAME version COMMAND ccl-version-test)
endif()
