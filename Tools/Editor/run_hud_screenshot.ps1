$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $projectRoot "Saved\Logs\CWSHudScreenshot.log"
$screenshotFile = Join-Path $projectRoot "Saved\Screenshots\CWSRoundAnnouncement.png"
$successMarker = "CWS_HUD_SCREENSHOT_SUCCESS"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }
if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before running the HUD screenshot test."
}

function Invoke-HudScreenshotTest {
    if (Test-Path -LiteralPath $logFile) {
        Remove-Item -LiteralPath $logFile
    }
    if (Test-Path -LiteralPath $screenshotFile) {
        Remove-Item -LiteralPath $screenshotFile
    }

    & $editorCmd $projectFile `
        "/Game/Variant_Combat/Lvl_Combat" `
        -game `
        -RenderOffscreen `
        -ResX=1280 `
        -ResY=720 `
        -unattended `
        -nosound `
        -nop4 `
        -DDC=Warm `
        "-ExecCmds=DisableAllScreenMessages" `
        -CWSHUDScreenshotTest `
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

Invoke-HudScreenshotTest
$hasKnownCacheFailure = $logText -match
    "ShaderCompilingThread crashed|String index out of bounds|FLargeMemoryReader|BufferReader.h|OodleLZ_Decompress failed"
if (-not $logText.Contains($successMarker) -and $hasKnownCacheFailure) {
    Write-Warning "Shader or derived-data cache initialization failed. Retrying the HUD screenshot once."
    Invoke-HudScreenshotTest
}

if ($editorExitCode -ne 0) {
    throw "HUD screenshot test failed with Unreal exit code $editorExitCode."
}
if (-not $logText.Contains($successMarker)) {
    throw "HUD screenshot test did not emit $successMarker."
}
if (-not (Test-Path -LiteralPath $screenshotFile)) {
    throw "HUD screenshot was not written to $screenshotFile."
}

$screenshot = Get-Item -LiteralPath $screenshotFile
if ($screenshot.Length -le 0) {
    throw "HUD screenshot is empty: $screenshotFile"
}

Write-Host "Round announcement HUD screenshot passed: $screenshotFile"
