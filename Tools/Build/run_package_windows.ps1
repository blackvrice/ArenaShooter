[CmdletBinding()]
param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.6",
    [string]$WorkspaceParent = "C:\ArenaShooterPackageWork",
    [string]$PackageOutputParent = "C:\ArenaShooterPackages",
    [string]$ExistingPackageDirectory = "",
    [ValidateRange(10, 600)]
    [int]$SmokeTimeoutSeconds = 60,
    [switch]$KeepWorkspace
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$runUat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$workspacePath = $null
$worktreeCreated = $false

if (-not (Test-Path -LiteralPath $projectFile)) {
    throw "ArenaShooter.uproject was not found at $projectFile"
}
if (-not (Test-Path -LiteralPath $runUat)) {
    throw "RunUAT.bat was not found at $runUat"
}

function Assert-AsciiPath([string]$PathValue, [string]$PathLabel) {
    if ($PathValue.ToCharArray() | Where-Object { [int]$_ -gt 127 }) {
        throw "$PathLabel must use an ASCII-only path because UnrealBuildTool cannot compile this project from a Korean path: $PathValue"
    }
}

function Invoke-PackagedSmoke([string]$ArchiveDirectory) {
    $windowsRoot = Join-Path $ArchiveDirectory "Windows"
    $shippingExe = Join-Path $windowsRoot "ArenaShooter\Binaries\Win64\ArenaShooter-Win64-Shipping.exe"
    $bootstrapExe = Join-Path $windowsRoot "ArenaShooter.exe"

    if (-not (Test-Path -LiteralPath $bootstrapExe)) {
        throw "Packaged bootstrap executable was not found at $bootstrapExe"
    }
    if (-not (Test-Path -LiteralPath $shippingExe)) {
        throw "Packaged Shipping executable was not found at $shippingExe"
    }

    $packagedRun = Start-Process `
        -FilePath $shippingExe `
        -ArgumentList @("-nullrhi", "-unattended", "-nosound", "-nop4", "-CWSAllRoundsSmokeTest") `
        -WorkingDirectory $windowsRoot `
        -WindowStyle Hidden `
        -PassThru

    if (-not $packagedRun.WaitForExit($SmokeTimeoutSeconds * 1000)) {
        Stop-Process -Id $packagedRun.Id -Force -ErrorAction SilentlyContinue
        throw "Packaged all-round smoke did not exit within $SmokeTimeoutSeconds seconds."
    }
    if ($packagedRun.ExitCode -ne 0) {
        throw "Packaged all-round smoke failed with exit code $($packagedRun.ExitCode)."
    }

    return $shippingExe
}

if ($ExistingPackageDirectory) {
    $existingArchive = (Resolve-Path -LiteralPath $ExistingPackageDirectory).Path
    $verifiedExe = Invoke-PackagedSmoke $existingArchive
    Write-Host "CWS_PACKAGE_VERIFICATION_SUCCESS: existing Shipping package passed the all-round smoke test."
    Write-Host "Package executable: $verifiedExe"
    exit 0
}

$commit = (& git -C $projectRoot rev-parse --verify HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $commit) {
    throw "Could not resolve the current Git commit."
}
$shortCommit = $commit.Substring(0, 7)
$runId = "{0}-{1}" -f (Get-Date -Format "yyyyMMdd-HHmmss"), $shortCommit

Assert-AsciiPath $WorkspaceParent "WorkspaceParent"
Assert-AsciiPath $PackageOutputParent "PackageOutputParent"

New-Item -ItemType Directory -Path $WorkspaceParent -Force | Out-Null
New-Item -ItemType Directory -Path $PackageOutputParent -Force | Out-Null
$workspacePath = Join-Path $WorkspaceParent "ArenaShooter-$runId"
$archiveDirectory = Join-Path $PackageOutputParent "ArenaShooter-$runId"

if (Test-Path -LiteralPath $workspacePath) {
    throw "Generated workspace path already exists: $workspacePath"
}
if (Test-Path -LiteralPath $archiveDirectory) {
    throw "Generated package output path already exists: $archiveDirectory"
}

$dirtyFiles = & git -C $projectRoot status --porcelain
if ($dirtyFiles) {
    Write-Warning "The source checkout is dirty. Packaging uses committed HEAD $shortCommit only."
}

try {
    & git -C $projectRoot worktree add --detach $workspacePath $commit
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create the ASCII packaging worktree."
    }
    $worktreeCreated = $true

    $workspaceProject = Join-Path $workspacePath "ArenaShooter.uproject"
    $env:DOTNET_PROCESSOR_COUNT = "1"
    $uatArguments = @(
        "BuildCookRun",
        "-project=$workspaceProject",
        "-noP4",
        "-platform=Win64",
        "-clientconfig=Shipping",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        "-archivedirectory=$archiveDirectory",
        "-unattended",
        "-utf8output",
        "-NoXGE"
    )

    & $runUat @uatArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Windows Shipping BuildCookRun failed with exit code $LASTEXITCODE."
    }

    $verifiedExe = Invoke-PackagedSmoke $archiveDirectory
    Write-Host "CWS_PACKAGE_VERIFICATION_SUCCESS: commit $shortCommit was packaged and passed the all-round Shipping smoke test."
    Write-Host "Package executable: $verifiedExe"
}
finally {
    if ($worktreeCreated -and -not $KeepWorkspace) {
        & git -C $projectRoot worktree remove --force $workspacePath
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "The temporary packaging worktree could not be removed: $workspacePath"
        }
    }
}
