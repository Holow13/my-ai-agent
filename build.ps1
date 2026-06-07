param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Remove-BuildCache {
    if (Test-Path "build") {
        Write-Host "Removing old build cache..."
        Remove-Item -Recurse -Force "build"
    }
}

function Test-Generator {
    param([string]$Name)
    & cmake -G $Name --help 2>&1 | Out-Null
    return $LASTEXITCODE -eq 0
}

$generators = @(
    @{ Name = "Visual Studio 17 2022"; Args = @("-A", "x64") },
    @{ Name = "Visual Studio 16 2019"; Args = @("-A", "x64") }
)

$configured = $false
foreach ($generator in $generators) {
    if (-not (Test-Generator $generator.Name)) {
        continue
    }

    Write-Host "Configuring with $($generator.Name)..."
    $cmakeArgs = @("-G", $generator.Name) + $generator.Args + @("-S", ".", "-B", "build")
    & cmake @cmakeArgs
    if ($LASTEXITCODE -eq 0) {
        $configured = $true
        break
    }

    Remove-BuildCache
}

if (-not $configured) {
    Write-Error @"
CMake could not find Visual Studio with C++.

STEP 1 — install Build Tools (PowerShell as Administrator):
  winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

STEP 2 — if C++ workload is still missing, open "Visual Studio Installer":
  Start -> Visual Studio Installer -> Modify -> check "Desktop development with C++" -> Install

STEP 3 — reboot PC, reopen PowerShell, then run:
  powershell -ExecutionPolicy Bypass -File .\build.ps1
"@
}

Write-Host "Building $Configuration..."
& cmake --build build --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "Build failed"
}

$binary = Join-Path $PSScriptRoot "build\Release\jarvis.exe"
if ($Configuration -eq "Debug") {
    $binary = Join-Path $PSScriptRoot "build\Debug\jarvis.exe"
}

Write-Host ""
Write-Host "Build complete: $binary"
