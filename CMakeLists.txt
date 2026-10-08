cmake_minimum_required(VERSION 3.15)

project(nand2tetris VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_HDL ON)
option(BUILD_TESTS OFF)

add_compile_definitions(TOML_EXCEPTIONS=0)

set(SOURCE_FILES
    src/args.hpp
    src/args.cpp
    src/main.cpp
    src/hdl/hdl_parser.hpp
    src/hdl/hdl_parser.cpp
    src/util/string.hpp
    src/util/string.cpp
)

set(ARGPARSE_BUILD_TESTS OFF)

include(cmake/raylib.cmake)
include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)

add_subdirectory(external/spdlog)
add_subdirectory(external/argparse)
add_subdirectory(external/magic_enum)
add_subdirectory(external/nativefiledialog-extended)
add_subdirectory(external/tomlplusplus)
add_subdirectory(external/catch2)

add_executable(main ${SOURCE_FILES} src/main.cpp)

target_link_libraries(main PRIVATE
    argparse::argparse
    spdlog::spdlog
    magic_enum
    tomlplusplus::tomlplusplus
    rlimgui
    nfd
)

target_include_directories(main PUBLIC external/rlimgui)

set_target_properties(main PROPERTIES OUTPUT_NAME nand2tetris)

if (BUILD_HDL)

    add_executable(hdl
        src/args.hpp
        src/args.cpp
        src/hdl_main.cpp
        src/hdl/hdl_parser.hpp
        src/hdl/hdl_parser.cpp
        src/hdl/test_parser.hpp
        src/hdl/test_parser.cpp
        src/util/file.hpp
        src/util/file.cpp
        src/util/string.hpp
        src/util/string.cpp
    )

    target_link_libraries(hdl PRIVATE
        argparse::argparse
        spdlog::spdlog
        magic_enum
        tomlplusplus::tomlplusplus
        rlimgui
        nfd
    )

    if (BUILD_TESTS)
        enable_testing()

        add_executable(hdl-tests
            tests/hdl_test.cpp
            src/hdl/hdl_parser.hpp
            src/hdl/hdl_parser.cpp
            src/hdl/test_parser.hpp
            src/hdl/test_parser.cpp
            src/util/string.hpp
            src/util/string.cpp
        )

        target_link_libraries(hdl-tests
            spdlog::spdlog
            magic_enum
            Catch2::Catch2WithMain
        )
    endif()

endif()
