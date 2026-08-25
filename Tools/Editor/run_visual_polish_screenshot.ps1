$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSVisualPolishScreenshot.log"
$screenshotFile = Join-Path $projectRoot "Saved\Screenshots\CWSArenaVisualPolish.png"
$successMarker = "CWS_VISUAL_POLISH_SCREENSHOT_SUCCESS"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object { $_.Name -eq "UnrealEditor.exe" -and $_.CommandLine -like "*ArenaShooter.uproject*" }
if ($openEditor) { throw "ArenaShooter is open in Unreal Editor. Close it before running visual QA." }

function Invoke-VisualPolishScreenshot([string]$DdcGraph) {
    if (Test-Path -LiteralPath $logFile) { Remove-Item -LiteralPath $logFile }
    if (Test-Path -LiteralPath $screenshotFile) { Remove-Item -LiteralPath $screenshotFile }
    & $editorCmd $projectFile `
        "/Game/Variant_Combat/Lvl_Combat" `
        -game `
        -RenderOffscreen `
        -ResX=1280 `
        -ResY=720 `
        -unattended `
        -NoLoadStartupPackages `
        -nosound `
        -nop4 `
        "-DDC=$DdcGraph" `
        "-ExecCmds=DisableAllScreenMessages" `
        -CWSVisualPolishScreenshotTest `
        "-abslog=$logFile" `
        -stdout `
        -FullStdOutLogOutput
    $script:editorExitCode = $LASTEXITCODE
    $script:logText = if (Test-Path -LiteralPath $logFile) { Get-Content -LiteralPath $logFile -Raw } else { "" }
}

foreach ($warmAttempt in 1..3) {
    Invoke-VisualPolishScreenshot "Warm"
    if ($logText.Contains($successMarker)) { break }
    $hasKnownCacheFailure = $logText -match
        "ShaderCompilingThread crashed|String index out of bounds|FLargeMemoryReader|BufferReader.h|OodleLZ_Decompress failed"
    if (-not $hasKnownCacheFailure -or $warmAttempt -eq 3) { break }
    Write-Warning "Warm DDC failed on screenshot attempt $warmAttempt. Retrying while retaining completed derived data."
}
if (-not $logText.Contains($successMarker) -and $hasKnownCacheFailure) {
    Write-Warning "Shader or derived-data cache initialization failed. Retrying once with an isolated Cold DDC."
    Invoke-VisualPolishScreenshot "Cold"
}

if ($editorExitCode -ne 0) { throw "Visual polish screenshot failed with Unreal exit code $editorExitCode." }
if (-not $logText.Contains($successMarker)) { throw "Visual polish screenshot did not emit $successMarker." }
if (-not (Test-Path -LiteralPath $screenshotFile)) { throw "Visual polish screenshot was not written." }
if ((Get-Item -LiteralPath $screenshotFile).Length -le 0) { throw "Visual polish screenshot is empty." }
Write-Host "Visual polish screenshot passed: $screenshotFile"
