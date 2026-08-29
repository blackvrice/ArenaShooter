param()

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSBalanceCombat.log"
$successMarker = "CWS_BALANCE_COMBAT_SUCCESS"
$failureMarker = "CWS_BALANCE_COMBAT_FAILURE"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before running the balance combat test."
}

function Invoke-BalanceCombatTest([string]$DdcGraph) {
    if (Test-Path -LiteralPath $logFile) {
        Remove-Item -LiteralPath $logFile
    }

    & $editorCmd $projectFile `
        "/Game/Variant_Combat/Lvl_Combat" `
        -game `
        -nullrhi `
        -unattended `
        -ReduceThreadUsage `
        -nosound `
        -nop4 `
        "-DDC=$DdcGraph" `
        -CWSBalanceCombatTest `
        "-abslog=$logFile" `
        -stdout `
        -FullStdOutLogOutput

    $script:editorExitCode = $LASTEXITCODE
    $script:logText = if (Test-Path -LiteralPath $logFile) {
        Get-Content -LiteralPath $logFile -Raw
    } else {
        ""
    }
}

Invoke-BalanceCombatTest "Warm"
$hasResultMarker = $logText.Contains($successMarker) -or $logText.Contains($failureMarker)
$hasDdcSerializationFailure =
    $logText -match "FLargeMemoryReader|BufferReader.h|Copied 0 bytes when .* bytes were expected"
if (-not $hasResultMarker) {
    $retryReason = if ($hasDdcSerializationFailure) {
        "Warm DDC failed during asset deserialization."
    } else {
        "The Warm DDC run exited without a result marker."
    }
    Write-Warning "$retryReason Retrying once with an isolated Cold DDC."
    Invoke-BalanceCombatTest "Cold"
}

if ($logText.Contains($successMarker)) {
    Write-Host "Round 1-5 actual-hit balance combat test passed."
    exit 0
}

$failureLine = $logText -split "`r?`n" |
    Where-Object { $_ -match $failureMarker } |
    Select-Object -Last 1
if ($failureLine) {
    throw $failureLine.Trim()
}

throw "Balance combat test did not emit a result marker. Unreal exit code: $editorExitCode"
