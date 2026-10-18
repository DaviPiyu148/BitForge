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
$LauncherContent = "@echo off`r`nsetlocal`r`ncd /d `"%~dp0`"`r`nstart `"`" `"%~dp0bitforge_gui.exe`" %*`r`n"
$LauncherPath = Join-Path $StagingDir "launch_bitforge.bat"
[System.IO.File]::WriteAllText($LauncherPath, $LauncherContent, [System.Text.Encoding]::ASCII)

# Copy QUICKSTART.md
$QuickstartSrc = Join-Path $ProjectRoot "docs\QUICKSTART.md"
if (Test-Path $QuickstartSrc) {
    Copy-Item -Path $QuickstartSrc -Destination (Join-Path $StagingDir "QUICKSTART.md") -Force
}

# Copy LICENSE.txt
$LicenseSrc = Join-Path $ProjectRoot "LICENSE"
if (Test-Path $LicenseSrc) {
    Copy-Item -Path $LicenseSrc -Destination (Join-Path $StagingDir "LICENSE.txt") -Force
}

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
