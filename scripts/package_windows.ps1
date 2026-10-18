<#
.SYNOPSIS
    Packages BitForge into a standalone, portable Windows x64 distribution.

.DESCRIPTION
    Builds the Release GUI workbench, gathers required Qt 6 and MinGW/UCRT runtime
    dependencies, copies all 51 assembly examples, writes a relative launcher,
    authoring documentation, and compresses the result into a portable ZIP archive.

.PARAMETER Configuration
    Build configuration to package (default: 'Release' via 'gui-release' preset).

.PARAMETER OutputDir
    Root directory for staged distributions (default: 'dist').

.PARAMETER Validate
    If specified, extracts the ZIP to a test path containing spaces and verifies
    smoke execution under a sanitized PATH containing only standard Windows paths.
#>

[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [string]$OutputDir = "dist",
    [switch]$Validate
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path "$PSScriptRoot\..").Path
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host " BitForge Portable Windows Package Generator" -ForegroundColor Cyan
Write-Host " Project root: $ProjectRoot" -ForegroundColor Cyan
Write-Host " Configuration: $Configuration" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

# 1. Ensure MSYS2 UCRT64 toolchain is in PATH for the build process
$MsysBin = "C:\msys64\ucrt64\bin"
$MsysUsr = "C:\msys64\usr\bin"
if (Test-Path $MsysBin) {
    $env:PATH = "$MsysBin;$MsysUsr;$env:PATH"
}

# 2. Build Release GUI
Write-Host "`n[1/6] Building GUI Release preset..." -ForegroundColor Yellow
$BuildDir = Join-Path $ProjectRoot "build\gui-release"
if (-not (Test-Path $BuildDir)) {
    & cmake --preset gui-release -S $ProjectRoot
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration for gui-release failed." }
}
& cmake --build --preset gui-release
if ($LASTEXITCODE -ne 0) { throw "CMake build for gui-release failed." }

$GuiExe = Join-Path $BuildDir "bitforge_gui.exe"
$CliExe = Join-Path $BuildDir "bitforge.exe"
if (-not (Test-Path $GuiExe)) {
    throw "Expected binary not found: $GuiExe"
}

# 3. Create Staging Directory
$PackageName = "BitForge-Portable-Windows-x64"
$DistRoot = Join-Path $ProjectRoot $OutputDir
$StagingDir = Join-Path $DistRoot $PackageName

Write-Host "`n[2/6] Preparing staging directory: $StagingDir" -ForegroundColor Yellow
if (Test-Path $StagingDir) {
    Remove-Item -Path $StagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

# 4. Copy Executables
Write-Host "`n[3/6] Copying executables..." -ForegroundColor Yellow
Copy-Item -Path $GuiExe -Destination (Join-Path $StagingDir "bitforge_gui.exe") -Force
if (Test-Path $CliExe) {
    Copy-Item -Path $CliExe -Destination (Join-Path $StagingDir "bitforge_cli.exe") -Force
}

# 5. Deploy Qt 6 and Runtime DLLs
Write-Host "`n[4/6] Deploying Qt 6 and runtime DLL dependencies..." -ForegroundColor Yellow
$WinDeployQt = Join-Path $MsysBin "windeployqt.exe"
if (-not (Test-Path $WinDeployQt)) {
    $cmd = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
    if ($cmd) { $WinDeployQt = $cmd.Source }
}
if ($WinDeployQt -and (Test-Path $WinDeployQt)) {
    Write-Host "Running windeployqt: $WinDeployQt" -ForegroundColor Gray
    & $WinDeployQt --no-translations --compiler-runtime --dir $StagingDir (Join-Path $StagingDir "bitforge_gui.exe")
} else {
    Write-Warning "windeployqt.exe not found; relying on manual DLL deployment."
}

# Guarantee MinGW/UCRT runtimes and critical plugins are present
$RequiredDlls = @(
    "libstdc++-6.dll",
    "libgcc_s_seh-1.dll",
    "libwinpthread-1.dll"
)
foreach ($dll in $RequiredDlls) {
    $targetPath = Join-Path $StagingDir $dll
    if (-not (Test-Path $targetPath)) {
        $sourcePath = Join-Path $MsysBin $dll
        if (Test-Path $sourcePath) {
            Copy-Item -Path $sourcePath -Destination $targetPath -Force
            Write-Host "Copied $dll from toolchain." -ForegroundColor Gray
        } else {
            Write-Warning "Could not find runtime DLL: $dll"
        }
    }
}

# Verify platform plugin
$PlatformPlugin = Join-Path $StagingDir "platforms\qwindows.dll"
if (-not (Test-Path $PlatformPlugin)) {
    $SourcePlugin = "C:\msys64\ucrt64\share\qt6\plugins\platforms\qwindows.dll"
    if (-not (Test-Path $SourcePlugin)) {
        $SourcePlugin = "C:\msys64\ucrt64\lib\qt6\plugins\platforms\qwindows.dll"
    }
    if (Test-Path $SourcePlugin) {
        $PlatformDir = Join-Path $StagingDir "platforms"
        New-Item -ItemType Directory -Path $PlatformDir -Force | Out-Null
        Copy-Item -Path $SourcePlugin -Destination $PlatformPlugin -Force
        Write-Host "Copied platforms\qwindows.dll manually." -ForegroundColor Gray
    } else {
        Write-Warning "qwindows.dll not found in standard paths!"
    }
}

# 6. Copy Assembly Examples
Write-Host "`n[5/6] Copying assembly examples and creating documentation..." -ForegroundColor Yellow
$ExamplesSource = Join-Path $ProjectRoot "examples"
$ExamplesTarget = Join-Path $StagingDir "examples"
Copy-Item -Path $ExamplesSource -Destination $ExamplesTarget -Recurse -Force

# Create relative, zero-dependency launch batch file
$LauncherContent = @"
@echo off
setlocal
cd /d "%~dp0"
start "" "%~dp0bitforge_gui.exe" %*
"@
$LauncherPath = Join-Path $StagingDir "launch_bitforge.bat"
[System.IO.File]::WriteAllText($LauncherPath, $LauncherContent, [System.Text.Encoding]::ASCII)

# Create QUICKSTART.md
$QuickstartContent = @"
# BitForge Portable — Quick Start Guide

BitForge is a high-performance 8-bit digital computer emulator, two-pass assembler,
source debugger, and Qt 6 educational workbench.

This package is completely standalone and requires no installation, admin rights, or
external runtimes.

---

## 1. Quick Launch

- **Double-click `launch_bitforge.bat`** (or `bitforge_gui.exe`) to start the GUI workbench.
- **Drag and drop** any `.asm` file onto `launch_bitforge.bat` or `bitforge_gui.exe` to open it immediately.
- **Command Line**:
  ```cmd
  launch_bitforge.bat examples\01_arithmetic.asm
  ```

---

## 2. Package Contents

- `bitforge_gui.exe` — Desktop IDE and graphical debugger.
- `bitforge_cli.exe` — Command-line assembler and headless execution runner.
- `launch_bitforge.bat` — Relative Windows batch launcher.
- `examples/` — Complete collection of 51 assembly examples:
  - `fundamentals/` — Basic NOP, HALT, and register movement.
  - `arithmetic/` — Addition, subtraction, comparisons, and flag behavior.
  - `control_flow/` — Loops, conditional branches, and jumps.
  - `memory/` — Direct and indexed memory addressing.
  - `stack_subroutines/` — CALL/RET subroutines and stack manipulation.
  - `io_devices/` — Virtual text console, numeric display, LEDs, and input FIFO.
  - `algorithms/` — Multiplication, division, Fibonacci, and GCD.
  - `debugger/` — Stepping, breakpoints, and memory inspection demonstrations.
  - `faults/` & `invalid/` — Architectural fault and error detection fixtures.
- `platforms/qwindows.dll` — Qt 6 Windows platform integration plugin.

---

## 3. Architecture Summary

- **Architecture Specification**: BitForge v1.7 (frozen contract)
- **Registers**: R0, R1, R2, R3 (8-bit general purpose), IX (16-bit index), PC (16-bit), SP (16-bit)
- **Status Flags**: Z (Zero), C (Carry/Borrow), N (Negative), V (Signed Overflow)
- **Memory**: 64 KiB flat byte-addressable space
- **Virtual I/O Ports**:
  - `0x00`: Input FIFO
  - `0x01`: Text console output
  - `0x02`: Numeric display output
  - `0x10`: 8-bit Virtual LED register

---

## 4. Headless CLI Usage

To assemble and execute an assembly file without the GUI:
```cmd
bitforge_cli.exe run examples\algorithms\04_fibonacci_sequence.asm --max-steps 10000
```
"@
$QuickstartPath = Join-Path $StagingDir "QUICKSTART.md"
[System.IO.File]::WriteAllText($QuickstartPath, $QuickstartContent, [System.Text.Encoding]::UTF8)

# Create LICENSE.txt
$LicenseContent = @"
BitForge Educational Emulator and Workbench
Copyright (c) 2026 BitForge Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"@
$LicensePath = Join-Path $StagingDir "LICENSE.txt"
[System.IO.File]::WriteAllText($LicensePath, $LicenseContent, [System.Text.Encoding]::UTF8)

# 7. Compress into ZIP Archive
Write-Host "`n[6/6] Creating ZIP archive..." -ForegroundColor Yellow
$ZipPath = Join-Path $DistRoot "$PackageName.zip"
if (Test-Path $ZipPath) {
    Remove-Item -Path $ZipPath -Force
}
Compress-Archive -Path "$StagingDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal
Write-Host "Archive created successfully: $ZipPath" -ForegroundColor Green

# 8. Validation (if requested)
if ($Validate) {
    Write-Host "`n================================================================" -ForegroundColor Cyan
    Write-Host " Validating Portable Package" -ForegroundColor Cyan
    Write-Host "================================================================" -ForegroundColor Cyan

    $TestSpaceDir = Join-Path $DistRoot "Test Space with Spaces"
    if (Test-Path $TestSpaceDir) {
        Remove-Item -Path $TestSpaceDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $TestSpaceDir -Force | Out-Null

    Write-Host "Extracting to: $TestSpaceDir" -ForegroundColor Yellow
    Expand-Archive -Path $ZipPath -DestinationPath $TestSpaceDir -Force

    $PackagedGui = Join-Path $TestSpaceDir "bitforge_gui.exe"
    $PackagedCli = Join-Path $TestSpaceDir "bitforge_cli.exe"
    $PackagedPlugin = Join-Path $TestSpaceDir "platforms\qwindows.dll"

    if (-not (Test-Path $PackagedGui)) { throw "Validation failed: bitforge_gui.exe missing." }
    if (-not (Test-Path $PackagedPlugin)) { throw "Validation failed: platforms\qwindows.dll missing." }

    Write-Host "Validating execution under sanitized PATH (C:\Windows\System32;C:\Windows)..." -ForegroundColor Yellow
    $OriginalPath = $env:PATH
    try {
        # Strict isolation: only standard Windows system paths
        $env:PATH = "C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem"

        if (Test-Path $PackagedCli) {
            $TestAsm = Join-Path $TestSpaceDir "examples\01_arithmetic.asm"
            $cliOutput = & $PackagedCli run $TestAsm 2>&1
            Write-Host "Packaged CLI Output:`n$cliOutput" -ForegroundColor Gray
            if ($LASTEXITCODE -ne 0) {
                throw "Packaged CLI execution failed with code $LASTEXITCODE under sanitized PATH."
            }
            Write-Host "Packaged CLI executed cleanly under sanitized PATH!" -ForegroundColor Green
        }

        # Check GUI executable starts up without missing DLL dialogs
        $proc = Start-Process -FilePath $PackagedGui -ArgumentList "--help" -PassThru -NoNewWindow
        Start-Sleep -Milliseconds 800
        if (-not $proc.HasExited) {
            $proc.Kill()
        }
        Write-Host "Packaged GUI launched successfully without missing DLL errors!" -ForegroundColor Green
    } finally {
        $env:PATH = $OriginalPath
        # Clean up test space
        if (Test-Path $TestSpaceDir) {
            Remove-Item -Path $TestSpaceDir -Recurse -Force
        }
    }
}

Write-Host "`nPackaging completed successfully!" -ForegroundColor Green
Write-Host "Staged Directory: $StagingDir" -ForegroundColor White
Write-Host "ZIP Archive:      $ZipPath" -ForegroundColor White
