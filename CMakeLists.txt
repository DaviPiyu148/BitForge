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

# -----------------------------------------------------------------
# GUI desktop application and automated GUI tests (opt-in)
# -----------------------------------------------------------------
option(BITFORGE_BUILD_GUI "Build the BitForge Qt 6 GUI application and GUI tests" OFF)

if (BITFORGE_BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Test)

    set(BITFORGE_GUI_HEADERS
        include/bitforge/gui/execution_controller.hpp
        include/bitforge/gui/code_editor.hpp
        include/bitforge/gui/line_number_area.hpp
        include/bitforge/gui/asm_highlighter.hpp
        include/bitforge/gui/registers_panel.hpp
        include/bitforge/gui/memory_viewer.hpp
        include/bitforge/gui/stack_viewer.hpp
        include/bitforge/gui/devices_panel.hpp
        include/bitforge/gui/breakpoints_panel.hpp
        include/bitforge/gui/trace_panel.hpp
        include/bitforge/gui/main_window.hpp
    )

    set(BITFORGE_GUI_SOURCES
        src/gui/main_window.cpp
        src/gui/code_editor.cpp
        src/gui/line_number_area.cpp
        src/gui/asm_highlighter.cpp
        src/gui/registers_panel.cpp
        src/gui/memory_viewer.cpp
        src/gui/stack_viewer.cpp
        src/gui/devices_panel.cpp
        src/gui/breakpoints_panel.cpp
        src/gui/trace_panel.cpp
        src/gui/execution_controller.cpp
        ${BITFORGE_GUI_HEADERS}
    )

    add_executable(bitforge_gui
        src/gui/main.cpp
        ${BITFORGE_GUI_SOURCES}
    )

    target_include_directories(bitforge_gui
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(bitforge_gui
        PRIVATE
            bitforge_debugger
            bitforge_core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Core
    )

    set(BITFORGE_GUI_WARNING_FLAGS "")
    if (MSVC)
        list(APPEND BITFORGE_GUI_WARNING_FLAGS /W4 /permissive-)
    else()
        list(APPEND BITFORGE_GUI_WARNING_FLAGS -Wall -Wextra)
    endif()
    target_compile_options(bitforge_gui PRIVATE ${BITFORGE_GUI_WARNING_FLAGS})

    # Automated GUI testing suite
    add_executable(bitforge_gui_tests
        tests/gui/test_gui.cpp
        ${BITFORGE_GUI_SOURCES}
    )

    target_include_directories(bitforge_gui_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(bitforge_gui_tests
        PRIVATE
            bitforge_debugger
            bitforge_core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Core
            Qt6::Test
    )

    target_compile_options(bitforge_gui_tests PRIVATE ${BITFORGE_GUI_WARNING_FLAGS})

    add_test(
        NAME BitForgeGuiTests
        COMMAND bitforge_gui_tests -platform offscreen
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()


