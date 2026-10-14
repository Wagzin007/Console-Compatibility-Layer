cmake_minimum_required(VERSION 3.21)

project(ccl VERSION 0.0.2 LANGUAGES CXX)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(doctest REQUIRED)

add_library(ccl_common STATIC
    src/common/file.cpp
    src/common/version.cpp
)
target_include_directories(ccl_common PUBLIC src)
target_compile_definitions(ccl_common PUBLIC CCL_VERSION="${PROJECT_VERSION}")

add_library(ccl_gba STATIC
    src/gba/rom_header.cpp
)
target_include_directories(ccl_gba PUBLIC src)
target_link_libraries(ccl_gba PUBLIC ccl_common)

add_executable(ccl-run
    src/cli/cli.cpp
    src/cli/main.cpp
)
target_link_libraries(ccl-run PRIVATE ccl_common ccl_gba)

include(CTest)
if(BUILD_TESTING)
    add_executable(ccl-tests
        src/cli/cli.cpp
        tests/m1_test.cpp
        tests/version_test.cpp
    )
    target_link_libraries(ccl-tests PRIVATE ccl_common ccl_gba doctest::doctest)
    add_test(NAME ccl COMMAND ccl-tests)
endif()
