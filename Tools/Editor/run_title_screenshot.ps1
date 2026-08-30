$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSTitleScreenshot.log"
$screenshotFile = Join-Path $projectRoot "Saved\Screenshots\CWSTitleScreen.png"
$successMarker = "CWS_TITLE_SCREENSHOT_SUCCESS"
. (Join-Path $PSScriptRoot "CWSStableUnreal.ps1")

$openEditor = Get-CimInstance Win32_Process |
    Where-Object { $_.Name -eq "UnrealEditor.exe" -and $_.CommandLine -like "*ArenaShooter.uproject*" }
if ($openEditor) { throw "ArenaShooter is open in Unreal Editor. Close it before running title screen QA." }

function Invoke-TitleScreenshotTest([string]$DdcGraph) {
    if (Test-Path -LiteralPath $logFile) { Remove-Item -LiteralPath $logFile }
    if (Test-Path -LiteralPath $screenshotFile) { Remove-Item -LiteralPath $screenshotFile }
    $arguments = @(
        "`"$projectFile`"",
        "/Game/Variant_Combat/Lvl_Combat",
        "-game",
        "-RenderOffscreen",
        "-ResX=1280",
        "-ResY=720",
        "-unattended",
        "-NoLoadStartupPackages",
        "-ReduceThreadUsage",
        "-d3d11",
        "-nosound",
        "-nop4",
        "-DDC=$DdcGraph",
        "-ExecCmds=DisableAllScreenMessages",
        "-CWSTitleScreenshotTest",
        "-abslog=`"$logFile`""
    )
    $script:editorExitCode = Invoke-CWSStableUnrealProcess `
        -Executable $editorCmd `
        -ArgumentList $arguments
    $script:logText = if (Test-Path -LiteralPath $logFile) { Get-Content -LiteralPath $logFile -Raw } else { "" }
}

foreach ($warmAttempt in 1..3) {
    Invoke-TitleScreenshotTest "Warm"
    if ($logText.Contains($successMarker)) { break }
    $hasKnownCacheFailure = $logText -match
        "ShaderCompilingThread crashed|String index out of bounds|FLargeMemoryReader|BufferReader.h|OodleLZ_Decompress failed"
    if (-not $hasKnownCacheFailure -or $warmAttempt -eq 3) { break }
    Write-Warning "Warm DDC failed on title screenshot attempt $warmAttempt. Retrying while retaining completed derived data."
}
if (-not $logText.Contains($successMarker) -and $hasKnownCacheFailure) {
    Write-Warning "Shader or derived-data cache initialization failed. Retrying once with an isolated Cold DDC."
    Invoke-TitleScreenshotTest "Cold"
}

if ($editorExitCode -ne 0) { throw "Title screenshot test failed with Unreal exit code $editorExitCode." }
if (-not $logText.Contains($successMarker)) { throw "Title screenshot test did not emit $successMarker." }
if (-not (Test-Path -LiteralPath $screenshotFile)) { throw "Title screenshot was not written." }
if ((Get-Item -LiteralPath $screenshotFile).Length -le 0) { throw "Title screenshot is empty." }
Write-Host "Title screen screenshot passed: $screenshotFile"
