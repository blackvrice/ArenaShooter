$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "ArenaShooter.uproject"
$scriptFile = Join-Path $projectRoot "Tools\Editor\build_cws_blockout.py"
$editorCmd = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$openEditor = Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -like "*ArenaShooter.uproject*"
    }

if ($openEditor) {
    throw "ArenaShooter is open in Unreal Editor. Close the editor before rebuilding the generated blockout."
}

& $editorCmd $projectFile `
    -run=pythonscript `
    "-script=$scriptFile" `
    -unattended `
    -nop4 `
    -nosplash `
    -EnablePlugins=PythonScriptPlugin
