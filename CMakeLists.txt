cmake_minimum_required(VERSION 3.20)

# BitForge — Fictional 8-bit Computer Emulator & Workbench
# C++ language explicitly mandated; no C compiler required
project(BitForge
    VERSION 0.1.0
    DESCRIPTION "Educational 8-bit computer emulator, assembler, and debugger"
    LANGUAGES CXX
)

# Mandatory strict ISO C++17 standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warning policy
option(BITFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Determine compiler warning flags
set(BITFORGE_WARNING_FLAGS "")
if (MSVC)
    list(APPEND BITFORGE_WARNING_FLAGS /W4 /permissive-)
    if (BITFORGE_WARNINGS_AS_ERRORS)
        list(APPEND BITFORGE_WARNING_FLAGS /WX)
    endif()
else()
    list(APPEND BITFORGE_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
    )
    if (BITFORGE_WARNINGS_AS_ERRORS)
        list(APPEND BITFORGE_WARNING_FLAGS -Werror)
    endif()
endif()

# Core static library
add_library(bitforge_core STATIC
    src/memory/memory.cpp
    src/core/alu.cpp
    src/core/cpu.cpp
    src/bus/system_bus.cpp
    src/assembler/lexer.cpp
    src/assembler/assembler.cpp
)

target_include_directories(bitforge_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(bitforge_core PRIVATE ${BITFORGE_WARNING_FLAGS})

# CLI static library
add_library(bitforge_cli STATIC
    src/cli/runner.cpp
)

target_include_directories(bitforge_cli
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bitforge_cli
    PUBLIC
        bitforge_core
)

target_compile_options(bitforge_cli PRIVATE ${BITFORGE_WARNING_FLAGS})

# CLI executable
add_executable(bitforge
    src/cli/main.cpp
)

target_link_libraries(bitforge
    PRIVATE
        bitforge_cli
)

target_compile_options(bitforge PRIVATE ${BITFORGE_WARNING_FLAGS})

# Debugger static library
add_library(bitforge_debugger STATIC
    src/debugger/debugger_session.cpp
)

target_include_directories(bitforge_debugger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bitforge_debugger
    PUBLIC
        bitforge_core
)

target_compile_options(bitforge_debugger PRIVATE ${BITFORGE_WARNING_FLAGS})

# Automated testing suite
enable_testing()

add_executable(bitforge_tests
    tests/test_main.cpp
    tests/unit/test_types.cpp
    tests/unit/test_memory.cpp
    tests/unit/test_alu.cpp
    tests/unit/test_bus.cpp
    tests/unit/test_cpu_instructions.cpp
    tests/unit/test_stack_instructions.cpp
    tests/unit/test_io_instructions.cpp
    tests/unit/test_assembler.cpp
    tests/unit/test_cli.cpp
    tests/unit/test_debugger.cpp
)

target_include_directories(bitforge_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(bitforge_tests
    PRIVATE
        bitforge_core
        bitforge_cli
        bitforge_debugger
)

target_compile_options(bitforge_tests PRIVATE ${BITFORGE_WARNING_FLAGS})

add_test(
    NAME BitForgeTests
    COMMAND bitforge_tests
)

