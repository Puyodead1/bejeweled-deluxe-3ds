# Run a .3dsx in the Azahar emulator, then collect its log and a screenshot.
# Usage: .\tools\run_azahar.ps1 [-App build\smoke.3dsx] [-Seconds 20] [-Log smoke.log] [-Shot out.png] [-Azahar <dir or exe>]
#   -Seconds  how long to wait before taking the screenshot and closing Azahar (if the app hasn't quit by itself)
#   -Log      file name under sdmc:/3ds/Bejeweled/ to print afterwards
#   -Shot     save a PNG of the Azahar window before closing it
#   -Azahar   the Azahar folder or azahar.exe; defaults to $env:AZAHAR_DIR
# Azahar's SD card is %APPDATA%\Azahar\sdmc. It emulates the DSP, so no dspfirm.cdc is needed there.
param(
    [string]$App = "$PSScriptRoot\..\build\smoke.3dsx",
    [int]$Seconds = 20,
    [string]$Log = 'smoke.log',
    [string]$Shot = '',
    [string]$Azahar = $env:AZAHAR_DIR
)
$ErrorActionPreference = 'Stop'
if (-not $Azahar) { throw 'Pass -Azahar <folder or azahar.exe>, or set AZAHAR_DIR' }
$exe = if (Test-Path $Azahar -PathType Leaf) { $Azahar } else { Join-Path $Azahar 'azahar.exe' }
if (-not (Test-Path $exe)) { throw "azahar.exe not found at $exe" }
$App = (Resolve-Path $App).Path

$dataDir = Join-Path $env:APPDATA 'Azahar\sdmc\3ds\Bejeweled'
$logPath = Join-Path $dataDir $Log
Remove-Item $logPath -ErrorAction SilentlyContinue
Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force

$p = Start-Process $exe -ArgumentList "`"$App`"" -PassThru
$deadline = (Get-Date).AddSeconds($Seconds)
while (-not $p.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }

if (-not $p.HasExited) {
    $p.Refresh()
    "azahar: $($p.MainWindowTitle)"
    if ($Shot) { & "$PSScriptRoot\screenshot_window.ps1" -ProcessId $p.Id -Out $Shot }
    Stop-Process -Id $p.Id -Force
} else {
    "azahar exited by itself (code $($p.ExitCode))"
}
Start-Sleep -Milliseconds 500

"--- sdmc:/3ds/Bejeweled/$Log"
if (Test-Path $logPath) { Get-Content $logPath } else { '(no log written)' }
