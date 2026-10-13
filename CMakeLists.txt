cmake_minimum_required(VERSION 3.20)

project(ccl VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(doctest REQUIRED)

add_library(ccl_common STATIC
    src/common/version.cpp
)
target_include_directories(ccl_common PUBLIC src)
target_compile_options(ccl_common PRIVATE -Wall -Wextra -Wpedantic)

add_executable(ccl-run src/cli/main.cpp)
target_link_libraries(ccl-run PRIVATE ccl_common)
target_compile_options(ccl-run PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
    add_executable(ccl-version-test tests/version_test.cpp)
    target_link_libraries(ccl-version-test PRIVATE ccl_common doctest::doctest)
    target_compile_options(ccl-version-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME version COMMAND ccl-version-test)
endif()

if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=address")
    target_sources(ccl-run PRIVATE src/common/asan_options.cpp)
    if(BUILD_TESTING)
        target_sources(ccl-version-test PRIVATE src/common/asan_options.cpp)
    endif()
endif()
