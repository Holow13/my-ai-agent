$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Test-OllamaRunning {
    try {
        $response = Invoke-WebRequest -Uri "http://127.0.0.1:11434/api/tags" -UseBasicParsing -TimeoutSec 2
        return $response.StatusCode -eq 200
    } catch {
        return $false
    }
}

if (-not (Test-OllamaRunning)) {
    Write-Host "Starting Ollama..."
    $ollamaExe = Join-Path $env:LOCALAPPDATA "Programs\Ollama\ollama.exe"
    if (Test-Path $ollamaExe) {
        Start-Process $ollamaExe -ArgumentList "serve" -WindowStyle Hidden
        Start-Sleep -Seconds 3
    } else {
        Write-Host "Ollama not found. Start it manually."
    }
}

$jarvisExe = Join-Path $PSScriptRoot "build\Release\jarvis.exe"
if (-not (Test-Path $jarvisExe)) {
    Write-Error "jarvis.exe not found. Run: powershell -ExecutionPolicy Bypass -File .\build.ps1"
}

chcp 65001 | Out-Null
& $jarvisExe
