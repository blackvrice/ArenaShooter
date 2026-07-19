$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSRoundOneSmoke.log"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before running the Round 1 smoke test."
}

if (Test-Path -LiteralPath $logFile) {
    Remove-Item -LiteralPath $logFile
}

& $editorCmd $projectFile `
    "/Game/Variant_Combat/Lvl_Combat" `
    -game `
    -nullrhi `
    -unattended `
    -nosound `
    -nop4 `
    -CWSRoundOneSmokeTest `
    "-abslog=$logFile" `
    -stdout `
    -FullStdOutLogOutput

$editorExitCode = $LASTEXITCODE
$logText = if (Test-Path -LiteralPath $logFile) {
    Get-Content -LiteralPath $logFile -Raw
} else {
    ""
}

if ($logText.Contains("CWS_ROUND_ONE_SMOKE_SUCCESS")) {
    Write-Host "Round 1 smoke test passed: spawn, NavMesh movement, death, and round clear were observed."
    exit 0
}

$failureLine = $logText -split "`r?`n" |
    Where-Object { $_ -match "CWS_ROUND_ONE_SMOKE_FAILURE" } |
    Select-Object -Last 1
if ($failureLine) {
    throw $failureLine.Trim()
}

throw "Round 1 smoke test did not emit a result marker. Unreal exit code: $editorExitCode"
