# BitForge

**An 8-Bit Computer Emulator, Assembler, and Interactive Debugging Workbench**

BitForge is an educational software model of a fictional 8-bit microprocessor and workbench environment. It is designed to demonstrate computer architecture and algorithm & data structures (ADS) concepts—such as Bubble Sort, Linear Search, and recursion—through deterministic simulation and interactive inspection.

---

## Current Status: Phase 1A Complete

BitForge is developed incrementally in strict adherence to [BitForge Architecture Specification v1.7](BitForge_Architecture_Specification_v1.7.md).

Phase 1A ("Buildable Core Foundation") establishes:
- **Build Infrastructure:** Portable CMake 3.20+ build system with C++17 enforcement, compiler warning configurations, unified `CMakePresets.json`, and multi-platform CI workflow.
- **Architectural Types & Constants:** 8-bit/16-bit fixed-width types, big-endian arithmetic helpers, and all 31 canonical instruction opcodes.
- **Memory Model:** Flat 64 KiB byte-addressable memory with big-endian 16-bit read/write, safe 16-bit boundary wrapping, and segment loading bounds enforcement.
- **Pure Functional ALU:** Stateless arithmetic and logic unit computing `ADD`, `SUB`, `CMP`, `INC`, `DEC`, `AND`, `OR`, `XOR`, and `NOT`, with exact flag calculation (`Z`, `C`, `N`, `V`).
- **Machine Bus Abstraction:** Abstract `IBus` interface and initial concrete `SystemBus` with side-effect-free I/O permission queries.
- **Dependency-Free Test Suite:** Self-contained testing framework (`tests/test_harness.hpp`) and automated CTest unit tests covering types, memory, ALU, and bus.

*Note: CPU instruction execution and stepping (Phase 1B), virtual devices (Phase 2), assembler (Phase 3), debugger (Phase 4), CLI (Phase 5), and GUI workbench (Phase 6) are deferred to subsequent phases.*

---

## Architectural Highlights

- **Data & Address Width:** 8-bit data, 16-bit address space (`0x0000`–`0xFFFF`, 65,536 bytes).
- **Registers:** Four 8-bit general-purpose registers (`R0`–`R3`), 16-bit index register (`IX`), 16-bit program counter (`PC`), and 16-bit stack pointer (`SP`).
- **Flags:** Zero (`Z`), Carry/no-borrow (`C`), Negative (`N`), and Signed Overflow (`V`).
- **Instruction Set:** Exactly 31 canonical instruction mnemonics across 4 functional groups.
- **Endianness:** Strictly big-endian across 16-bit addresses and immediate words.
- **Dependencies:** Strictly standard ISO C++17. Zero third-party library dependencies for core and tests.

---

## Quickstart

### Prerequisites
A modern C++17 compliant compiler and CMake 3.20+:
- **Windows:** MSVC (Visual Studio 2019/2022) or MinGW-w64 (GCC 9.3+ / Clang 10+).
- **Linux:** GCC 9.3+ or Clang 10+.
- **macOS:** Apple Clang (Xcode 12+) on Apple Silicon or Intel.

For detailed platform setup and toolchain installation commands, see [BUILDING.md](BUILDING.md).

### Building with CMake Presets (Recommended)

```bash
# Configure debug build
cmake --preset debug

# Compile core library and unit test suite
cmake --build --preset debug

# Run automated tests via CTest
ctest --preset debug
```

### Building Manually (Single-Config / Ninja or Make)

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure
```

### Building Manually (Multi-Config / Visual Studio or Xcode)

```bash
# Configure
cmake -B build

# Build Debug
cmake --build build --config Debug

# Test Debug
ctest --test-dir build -C Debug --output-on-failure
```

---

## Specification History

Historical architectural specifications are preserved at the repository root:
- [v1.0](BitForge_Architecture_Specification_v1.0.md) — Initial architecture specification.
- [v1.1](BitForge_Architecture_Specification_v1.1.md) — Clarified opcode counts, registers, and fault contracts.
- [v1.2](BitForge_Architecture_Specification_v1.2.md) — Added cross-platform and CI engineering guidelines.
- [v1.3](BitForge_Architecture_Specification_v1.3.md) — Refined runner configurations and compiler version requirements.
- [v1.4](BitForge_Architecture_Specification_v1.4.md) — Canonical 31-instruction audit and macOS runner selection.
- [v1.5](BitForge_Architecture_Specification_v1.5.md) — Full CI workflow specification and runner lifecycles.
- [v1.6](BitForge_Architecture_Specification_v1.6.md) — Linear search memory contract and ADS benchmark budgets.
- [v1.7](BitForge_Architecture_Specification_v1.7.md) — **Active Architectural Baseline**.
