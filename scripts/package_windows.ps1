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
    Copy-Item -Path $CliExe -Destination (Join-Path $StagingDir "bitforge.exe") -Force
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

# Guarantee MinGW/UCRT runtimes, Qt6 shared dependencies, and critical plugins are present
$RequiredDlls = @(
    "libstdc++-6.dll",
    "libgcc_s_seh-1.dll",
    "libwinpthread-1.dll",
    "libb2-1.dll",
    "libdouble-conversion.dll",
    "libicuin78.dll",
    "libicuuc78.dll",
    "libicudt78.dll",
    "libpcre2-16-0.dll",
    "zlib1.dll",
    "libzstd.dll",
    "libfreetype-6.dll",
    "libharfbuzz-0.dll",
    "libmd4c.dll",
    "libpng16-16.dll",
    "libbrotlidec.dll",
    "libjpeg-8.dll",
    "libgio-2.0-0.dll",
    "libgobject-2.0-0.dll",
    "libbz2-1.dll",
    "libglib-2.0-0.dll",
    "libgraphite2.dll",
    "libbrotlicommon.dll",
    "libintl-8.dll",
    "libgmodule-2.0-0.dll",
    "libffi-8.dll",
    "libpcre2-8-0.dll",
    "libiconv-2.dll"
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

# Verify and deploy Qt platform plugins
$PlatformDir = Join-Path $StagingDir "platforms"
New-Item -ItemType Directory -Path $PlatformDir -Force | Out-Null

$PlatformPluginsToDeploy = @("qwindows.dll", "qoffscreen.dll", "qminimal.dll")
foreach ($pName in $PlatformPluginsToDeploy) {
    $pTarget = Join-Path $PlatformDir $pName
    if (-not (Test-Path $pTarget)) {
        $SourcePlugin = "C:\msys64\ucrt64\share\qt6\plugins\platforms\$pName"
        if (-not (Test-Path $SourcePlugin)) {
            $SourcePlugin = "C:\msys64\ucrt64\lib\qt6\plugins\platforms\$pName"
        }
        if (Test-Path $SourcePlugin) {
            Copy-Item -Path $SourcePlugin -Destination $pTarget -Force
            Write-Host "Copied platforms\$pName." -ForegroundColor Gray
        } else {
            if ($pName -eq "qwindows.dll") {
                Write-Warning "$pName not found in standard paths!"
            }
        }
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

# Copy application icon
$IconSrc = Join-Path $ProjectRoot "resources\icons\bitforge.ico"
if (Test-Path $IconSrc) {
    Copy-Item -Path $IconSrc -Destination (Join-Path $StagingDir "bitforge.ico") -Force
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
    $PackagedCliAlt = Join-Path $TestSpaceDir "bitforge.exe"
    $PackagedPlugin = Join-Path $TestSpaceDir "platforms\qwindows.dll"

    if (-not (Test-Path $PackagedGui)) { throw "Validation failed: bitforge_gui.exe missing." }
    if (-not (Test-Path $PackagedCli)) { throw "Validation failed: bitforge_cli.exe missing." }
    if (-not (Test-Path $PackagedCliAlt)) { throw "Validation failed: bitforge.exe missing." }
    if (-not (Test-Path $PackagedPlugin)) { throw "Validation failed: platforms\qwindows.dll missing." }

    Write-Host "Validating execution under sanitized PATH (C:\Windows\System32;C:\Windows)..." -ForegroundColor Yellow
    $OriginalPath = $env:PATH
    try {
        # Strict isolation: only standard Windows system paths
        $env:PATH = "C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem"

        # 1. Validate CLI version command
        $verOutput = (& $PackagedCli --version 2>&1) -join "`n"
        Write-Host "CLI --version output: $verOutput" -ForegroundColor Gray
        if ($LASTEXITCODE -ne 0 -or ($verOutput -notmatch "BitForge 0\.1\.0")) {
            throw "Packaged CLI --version failed (code: $LASTEXITCODE, output: $verOutput)"
        }
        Write-Host "Packaged CLI --version passed!" -ForegroundColor Green

        # 2. Validate CLI help command
        $helpOutput = (& $PackagedCli --help 2>&1) -join "`n"
        if ($LASTEXITCODE -ne 0 -or ($helpOutput -notmatch "Usage:")) {
            throw "Packaged CLI --help failed (code: $LASTEXITCODE)"
        }
        Write-Host "Packaged CLI --help passed!" -ForegroundColor Green

        # 3. Validate CLI example execution
        $TestAsm = Join-Path $TestSpaceDir "examples\01_arithmetic.asm"
        $cliOutput = (& $PackagedCli run $TestAsm --max-steps 1000 --dump-regs 2>&1) -join "`n"
        Write-Host "Packaged CLI Run Output:`n$cliOutput" -ForegroundColor Gray
        if ($LASTEXITCODE -ne 0) {
            throw "Packaged CLI execution failed with code $LASTEXITCODE under sanitized PATH."
        }
        Write-Host "Packaged CLI executed cleanly under sanitized PATH!" -ForegroundColor Green

        # 4. Reliable GUI startup and panel initialization verification in offscreen mode
        Write-Host "Running packaged GUI smoke test with -platform offscreen --smoke-test..." -ForegroundColor Yellow
        $pinfo = New-Object System.Diagnostics.ProcessStartInfo
        $pinfo.FileName = $PackagedGui
        $pinfo.Arguments = "-platform offscreen --smoke-test"
        $pinfo.RedirectStandardOutput = $true
        $pinfo.RedirectStandardError = $true
        $pinfo.UseShellExecute = $false
        $pinfo.CreateNoWindow = $true

        $guiProc = [System.Diagnostics.Process]::Start($pinfo)
        $stdoutTask = $guiProc.StandardOutput.ReadToEndAsync()
        $stderrTask = $guiProc.StandardError.ReadToEndAsync()

        $finished = $guiProc.WaitForExit(10000)
        if (-not $finished) {
            $guiProc.Kill()
            throw "Packaged GUI smoke test timed out after 10000ms."
        }
        $stdout = $stdoutTask.Result
        $stderr = $stderrTask.Result

        Write-Host "Packaged GUI exit code: $($guiProc.ExitCode)" -ForegroundColor Gray
        if ($stdout) { Write-Host "Packaged GUI stdout:`n$stdout" -ForegroundColor Gray }
        if ($stderr) { Write-Host "Packaged GUI stderr:`n$stderr" -ForegroundColor Yellow }

        if ($guiProc.ExitCode -ne 0) {
            throw "Packaged GUI smoke test failed with exit code $($guiProc.ExitCode). Error: $stderr"
        }
        if ($stdout -notmatch "\[BitForge Smoke Test\] SUCCESS") {
            throw "Packaged GUI smoke test did not output expected success message. Output: $stdout"
        }
        Write-Host "Packaged GUI smoke test passed cleanly under sanitized PATH!" -ForegroundColor Green
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
