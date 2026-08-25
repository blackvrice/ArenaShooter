param(
    [switch]$InspectOnly
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$scriptFile = (Join-Path $projectRoot "Tools\Editor\configure_boss_round.py") -replace "\\", "/"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before configuring the boss round."
}

try {
    if ($InspectOnly) {
        $env:ARENA_BOSS_ROUND_MODE = "inspect"
    } else {
        Remove-Item Env:ARENA_BOSS_ROUND_MODE -ErrorAction SilentlyContinue
    }

    $pythonSucceeded = $false
    $pythonFailed = $false
    foreach ($attempt in 1..3) {
        & $editorCmd $projectFile `
            -run=pythonscript `
            "-script=$scriptFile" `
            -unattended `
            -nop4 `
            -nosplash `
            -NoLoadStartupPackages `
            -nothreading `
            -nullrhi `
            -DDC=Warm `
            -EnablePlugins=PythonScriptPlugin `
            -stdout `
            -FullStdOutLogOutput

        $commandletExitCode = $LASTEXITCODE
        $logFile = Join-Path $projectRoot "Saved\Logs\ArenaShooter.log"
        $logText = if (Test-Path -LiteralPath $logFile) { Get-Content -LiteralPath $logFile -Raw } else { "" }
        $pythonSucceeded = $logText -match "Python script executed successfully" -and $logText.Contains("BOSS_ROUND_CONFIG=")
        $pythonFailed = $logText -match "LogPython: Error|Python script executed with errors"
        if ($pythonSucceeded -and -not $pythonFailed) { break }
        if ($attempt -lt 3 -and $logText -match "FLargeMemoryReader|BufferReader.h|OodleLZ_Decompress failed") {
            Write-Warning "Warm DDC failed on attempt $attempt. Retrying while retaining completed derived data."
            continue
        }
        break
    }

    if (-not $pythonSucceeded -or $pythonFailed) {
        throw "Boss round configuration commandlet failed with exit code $commandletExitCode."
    }
    if ($commandletExitCode -ne 0) {
        Write-Warning "Unreal returned exit code $commandletExitCode because of editor warnings, but boss configuration completed successfully."
    }
} finally {
    Remove-Item Env:ARENA_BOSS_ROUND_MODE -ErrorAction SilentlyContinue
}
