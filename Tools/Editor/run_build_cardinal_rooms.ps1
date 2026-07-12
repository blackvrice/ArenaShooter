param(
    [switch]$InspectOnly
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$scriptFile = Join-Path $projectRoot "Tools\Editor\build_cardinal_rooms.py"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before rebuilding the cardinal rooms."
}

try {
    if ($InspectOnly) {
        $env:ARENA_CARDINAL_ROOMS_MODE = "inspect"
    } else {
        Remove-Item Env:ARENA_CARDINAL_ROOMS_MODE -ErrorAction SilentlyContinue
    }

    & $editorCmd $projectFile `
        -run=pythonscript `
        "-script=$scriptFile" `
        -unattended `
        -nop4 `
        -nosplash `
        -EnablePlugins=PythonScriptPlugin `
        -stdout `
        -FullStdOutLogOutput

    $commandletExitCode = $LASTEXITCODE
    if ($commandletExitCode -ne 0) {
        $logFile = Join-Path $projectRoot "Saved\Logs\ArenaShooter.log"
        $logText = if (Test-Path -LiteralPath $logFile) {
            Get-Content -LiteralPath $logFile -Raw
        } else {
            ""
        }
        $expectedMarker = if ($InspectOnly) { "CARDINAL_ROOMS_INSPECT=" } else { "Cardinal rooms rebuilt" }
        $pythonSucceeded = $logText -match "Python script executed successfully" -and $logText.Contains($expectedMarker)
        $pythonFailed = $logText -match "LogPython: Error|Python script executed with errors"
        if (-not $pythonSucceeded -or $pythonFailed) {
            throw "Cardinal room commandlet failed with exit code $commandletExitCode."
        }
        Write-Warning "Unreal returned exit code $commandletExitCode because of editor/source-control warnings, but the cardinal room script completed successfully."
    }
} finally {
    Remove-Item Env:ARENA_CARDINAL_ROOMS_MODE -ErrorAction SilentlyContinue
}
