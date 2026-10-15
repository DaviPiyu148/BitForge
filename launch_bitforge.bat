@echo off
setlocal

:: Ensure working directory is the BitForge project root
cd /d "%~dp0"

:: Add MSYS2 UCRT64 toolchain DLLs (Qt 6, MinGW runtime) to PATH
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"

set "APP_EXE=%~dp0build\gui\bitforge_gui.exe"

:: If the executable does not exist, build it automatically
if not exist "%APP_EXE%" (
    echo ================================================================
    echo [INFO] BitForge GUI executable not found.
    echo Building the GUI workbench with CMake and Ninja...
    echo ================================================================
    call cmake --preset gui
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed.
        pause
        exit /b 1
    )
    call cmake --build --preset gui
    if errorlevel 1 (
        echo [ERROR] CMake build failed.
        pause
        exit /b 1
    )
)

:: Launch BitForge GUI Workbench (passes any command-line or drag-and-drop files)
start "" "%APP_EXE%" %*
