# Building BitForge

This guide provides instructions for configuring, compiling, and testing BitForge across Windows, Linux, and macOS platforms.

---

## 1. Toolchain Requirements

BitForge requires a C++17-compliant compiler and CMake 3.20+:

| Tool | Minimum Version | Recommended Version | Notes |
|---|---|---|---|
| **CMake** | 3.20 | 3.25+ | Required for build configuration and presets |
| **Ninja** | 1.10+ | 1.11+ | Recommended fast cross-platform build tool |
| **MSVC** (Windows) | VS 2019 (v16.10 / 19.28) | VS 2022 (v17.0+ / 19.30+) | Microsoft STL (`/std:c++17`, `/permissive-`) |
| **GCC** (Linux/Windows) | 9.3+ | 11.0+ / 14.0+ | `libstdc++` with complete C++17 `<filesystem>` |
| **Clang** (Linux/macOS) | 10.0+ | 14.0+ / 18.0+ | `libc++` or `libstdc++` |
| **Apple Clang** (macOS) | Xcode 12.0+ | Xcode 15.0+ | Apple Silicon and Intel x86_64 supported |

> [!WARNING]
> **Legacy MinGW Notice (Windows):** Legacy 32-bit MinGW.org GCC 6.3.0 (commonly located at `C:\MinGW`) was released in 2016 and does not support standard ISO C++17 library headers. If present on your PATH, ensure CMake selects modern MSVC, MinGW-w64 (GCC 9+), or LLVM Clang.

---

## 2. Platform Setup & Installation

### Windows 10/11

#### Option A: Visual Studio 2022 Build Tools (Recommended)
Install using `winget` in an administrative terminal:
```powershell
winget install -e --id Kitware.CMake
winget install -e --id Ninja-build.Ninja
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --config vs_buildtools.json --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```
Then launch the **x64 Native Tools Command Prompt for VS 2022** or PowerShell with MSVC initialized.

#### Option B: MinGW-w64 (via MSYS2)
```powershell
winget install -e --id MSYS2.MSYS2
```
In the MSYS2 UCRT64 or MINGW64 shell:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```
Add `C:\msys64\ucrt64\bin` to PATH.

#### Option C: LLVM Clang
```powershell
winget install -e --id Kitware.CMake
winget install -e --id Ninja-build.Ninja
winget install -e --id LLVM.LLVM
```

---

### Linux (Ubuntu / Debian / Fedora)

#### Ubuntu / Debian:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build gcc g++ clang
```

#### Fedora / RHEL:
```bash
sudo dnf install -y gcc-c++ clang cmake ninja-build
```

---

### macOS (Apple Silicon & Intel)

1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Install CMake and Ninja via Homebrew:
   ```bash
   brew install cmake ninja
   ```

---

## 3. Build Workflows

### 3.1 Unified CMake Presets Workflow (Recommended)

BitForge provides standardized presets in `CMakePresets.json`:

```bash
# 1. Configure
cmake --preset debug        # For debug build with symbols
# or: cmake --preset release # For optimized release build
# or: cmake --preset ci      # For strict CI build with -Werror

# 2. Build
cmake --build --preset debug

# 3. Test
ctest --preset debug
```

---

### 3.2 Single-Configuration Generators (Ninja / Unix Makefiles)

Single-configuration generators select the configuration during the initial CMake setup:

```bash
# Configure with Ninja in Debug mode
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Optionally enable warnings-as-errors
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBITFORGE_WARNINGS_AS_ERRORS=ON

# Compile
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

---

### 3.3 Multi-Configuration Generators (Visual Studio / Xcode)

Multi-configuration generators store configuration settings inside the project files and select the configuration at build time:

```bash
# Configure (default generator on Windows is Visual Studio)
cmake -B build

# Build Debug configuration
cmake --build build --config Debug

# Run CTest with the Debug configuration
ctest --test-dir build -C Debug --output-on-failure
```

---

## 4. Running Tests Directly

In addition to running tests through `ctest`, the standalone test binary can be executed directly for detailed verbose assertion logs:

```bash
# Windows
.\build\debug\bitforge_tests.exe
# or for multi-config:
.\build\Debug\bitforge_tests.exe

# Linux / macOS
./build/debug/bitforge_tests
```

---

## 5. Build Options Reference

| CMake Option | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Debug` | Build profile (`Debug`, `Release`, `RelWithDebInfo`) |
| `BITFORGE_WARNINGS_AS_ERRORS` | `OFF` | Treats compiler warnings as errors (`/WX` on MSVC, `-Werror` on GCC/Clang). Enabled by default in CI. |
| `CMAKE_CXX_COMPILER` | System default | Explicit path or binary name for C++ compiler (e.g. `cl`, `g++`, `clang++`) |
