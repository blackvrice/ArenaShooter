$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSAttackFeedbackScreenshot.log"
$screenshotFile = Join-Path $projectRoot "Saved\Screenshots\CWSEnemyAttackFeedback.png"
$successMarker = "CWS_ATTACK_FEEDBACK_SCREENSHOT_SUCCESS"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object { $_.Name -eq "UnrealEditor.exe" -and $_.CommandLine -like "*ArenaShooter.uproject*" }
if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before running the attack feedback screenshot test."
}
function Invoke-AttackFeedbackScreenshotTest([string]$DdcGraph) {
    if (Test-Path -LiteralPath $logFile) { Remove-Item -LiteralPath $logFile }
    if (Test-Path -LiteralPath $screenshotFile) { Remove-Item -LiteralPath $screenshotFile }

    & $editorCmd $projectFile `
        "/Game/Variant_Combat/Lvl_Combat" `
        -game `
        -RenderOffscreen `
        -ResX=1280 `
        -ResY=720 `
        -unattended `
        -nop4 `
        "-DDC=$DdcGraph" `
        "-ExecCmds=DisableAllScreenMessages" `
        -CWSAttackFeedbackScreenshotTest `
        "-abslog=$logFile" `
        -stdout `
        -FullStdOutLogOutput

    $script:editorExitCode = $LASTEXITCODE
    $script:logText = if (Test-Path -LiteralPath $logFile) { Get-Content -LiteralPath $logFile -Raw } else { "" }
}

Invoke-AttackFeedbackScreenshotTest "Warm"
$hasKnownCacheFailure = $logText -match
    "ShaderCompilingThread crashed|String index out of bounds|FLargeMemoryReader|BufferReader.h|OodleLZ_Decompress failed"
if (-not $logText.Contains($successMarker) -and $hasKnownCacheFailure) {
    Write-Warning "Shader or derived-data cache initialization failed. Retrying once with an isolated Cold DDC."
    Invoke-AttackFeedbackScreenshotTest "Cold"
}

if ($editorExitCode -ne 0) { throw "Attack feedback screenshot test failed with Unreal exit code $editorExitCode." }
if (-not $logText.Contains($successMarker)) { throw "Attack feedback screenshot test did not emit $successMarker." }
if (-not (Test-Path -LiteralPath $screenshotFile)) { throw "Attack feedback screenshot was not written to $screenshotFile." }
if ((Get-Item -LiteralPath $screenshotFile).Length -le 0) { throw "Attack feedback screenshot is empty: $screenshotFile" }

Write-Host "Attack feedback screenshot passed: $screenshotFile"
