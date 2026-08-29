function Invoke-CWSStableUnrealProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,

        [Parameter(Mandatory = $true)]
        [string[]]$ArgumentList,

        [Int64]$AffinityMask = 0x0FFF0000,

        [switch]$Visible
    )

    $startParameters = @{
        FilePath = $Executable
        ArgumentList = $ArgumentList
        PassThru = $true
    }
    if (-not $Visible) {
        $startParameters.WindowStyle = "Hidden"
    }

    $process = Start-Process @startParameters
    $affinityWarningShown = $false
    while (-not $process.HasExited) {
        try {
            # Unreal may reset its affinity during startup, so keep applying the
            # stable E-core mask until the process exits.
            $process.ProcessorAffinity = [IntPtr]$AffinityMask
        } catch {
            if (-not $affinityWarningShown) {
                Write-Warning "Could not apply Unreal CPU affinity mask 0x$($AffinityMask.ToString('X'))."
                $affinityWarningShown = $true
            }
        }
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    }

    return $process.ExitCode
}
