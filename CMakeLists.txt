cmake_minimum_required(VERSION 3.21)

project(ccl VERSION 0.0.3 LANGUAGES CXX)

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
    src/gba/bus.cpp
    src/gba/rom_header.cpp
    src/gba/scheduler.cpp
)
target_include_directories(ccl_gba PUBLIC src)
target_link_libraries(ccl_gba PUBLIC ccl_common)

add_library(ccl_cli STATIC
    src/cli/cli.cpp
)
target_include_directories(ccl_cli PUBLIC src)
target_link_libraries(ccl_cli PUBLIC ccl_common ccl_gba)

add_executable(ccl-run src/cli/main.cpp)
target_link_libraries(ccl-run PRIVATE ccl_cli)

include(CTest)
if(BUILD_TESTING)
    add_executable(ccl-tests
        tests/m1_test.cpp
        tests/m2_test.cpp
        tests/version_test.cpp
    )
    target_link_libraries(ccl-tests PRIVATE ccl_cli doctest::doctest)
    include(doctest OPTIONAL)
    if(COMMAND doctest_discover_tests)
        doctest_discover_tests(ccl-tests)
    else()
        add_test(NAME ccl COMMAND ccl-tests)
    endif()
endif()
