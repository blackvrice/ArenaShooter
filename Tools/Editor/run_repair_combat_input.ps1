param(
    [switch]$InspectOnly
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$scriptFile = (Join-Path $projectRoot "Tools\Editor\repair_combat_input.py") -replace "\\", "/"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before repairing combat input mappings."
}

try {
    if ($InspectOnly) {
        $env:ARENA_COMBAT_INPUT_MODE = "inspect"
    } else {
        Remove-Item Env:ARENA_COMBAT_INPUT_MODE -ErrorAction SilentlyContinue
    }

    & $editorCmd $projectFile `
        -run=pythonscript `
        "-script=$scriptFile" `
        -unattended `
        -nop4 `
        -nosplash `
        -nullrhi `
        -DDC=Warm `
        -EnablePlugins=PythonScriptPlugin `
        -stdout `
        -FullStdOutLogOutput

    $commandletExitCode = $LASTEXITCODE
    $logFile = Join-Path $projectRoot "Saved\Logs\ArenaShooter.log"
    $logText = if (Test-Path -LiteralPath $logFile) {
        Get-Content -LiteralPath $logFile -Raw
    } else {
        ""
    }
    $pythonSucceeded = $logText -match "Python script executed successfully" -and $logText.Contains("COMBAT_INPUT_REPAIR=")
    $pythonFailed = $logText -match "LogPython: Error|Python script executed with errors"

    if (-not $pythonSucceeded -or $pythonFailed) {
        throw "Combat input repair commandlet failed with exit code $commandletExitCode."
    }
    if ($commandletExitCode -ne 0) {
        Write-Warning "Unreal returned exit code $commandletExitCode because of editor warnings, but the combat input script completed successfully."
    }
} finally {
    Remove-Item Env:ARENA_COMBAT_INPUT_MODE -ErrorAction SilentlyContinue
}
