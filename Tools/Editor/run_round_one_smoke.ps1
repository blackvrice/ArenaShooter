param(
    [switch]$AllRounds
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFileName = if ($AllRounds) { "CWSAllRoundsSmoke.log" } else { "CWSRoundOneSmoke.log" }
$logFile = Join-Path $projectRoot "Saved\Logs\$logFileName"
$smokeFlag = if ($AllRounds) { "-CWSAllRoundsSmokeTest" } else { "-CWSRoundOneSmokeTest" }
$successMarker = if ($AllRounds) { "CWS_ALL_ROUNDS_SMOKE_SUCCESS" } else { "CWS_ROUND_ONE_SMOKE_SUCCESS" }
$failureMarker = if ($AllRounds) { "CWS_ALL_ROUNDS_SMOKE_FAILURE" } else { "CWS_ROUND_ONE_SMOKE_FAILURE" }

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before running the Round 1 smoke test."
}

function Invoke-SmokeTest([string]$DdcGraph) {
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
        $smokeFlag `
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

Invoke-SmokeTest "Warm"
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
    Invoke-SmokeTest "Cold"
}

if ($logText.Contains($successMarker)) {
    $scope = if ($AllRounds) { "All-round" } else { "Round 1 combat-flow" }
    Write-Host "$scope smoke test passed."
    exit 0
}

$failureLine = $logText -split "`r?`n" |
    Where-Object { $_ -match $failureMarker } |
    Select-Object -Last 1
if ($failureLine) {
    throw $failureLine.Trim()
}

$scope = if ($AllRounds) { "All-round" } else { "Round 1 combat-flow" }
throw "$scope smoke test did not emit a result marker. Unreal exit code: $editorExitCode"
