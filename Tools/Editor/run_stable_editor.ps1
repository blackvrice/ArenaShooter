param(
    [switch]$DirectX12,
    [switch]$ColdDdc,
    [Int64]$AffinityMask = 0x0FFF0000
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$editor = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
. (Join-Path $PSScriptRoot "CWSStableUnreal.ps1")

$arguments = @(
    "`"$projectFile`"",
    "-NoLoadStartupPackages",
    "-ReduceThreadUsage",
    "-DDC=$(if ($ColdDdc) { 'Cold' } else { 'Warm' })"
)
if (-not $DirectX12) {
    $arguments += "-d3d11"
}

Write-Host "Starting ArenaShooter with stable CPU affinity 0x$($AffinityMask.ToString('X'))."
$exitCode = Invoke-CWSStableUnrealProcess `
    -Executable $editor `
    -ArgumentList $arguments `
    -AffinityMask $AffinityMask `
    -Visible
exit $exitCode
