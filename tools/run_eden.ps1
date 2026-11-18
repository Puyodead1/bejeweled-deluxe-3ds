# Run an .nro in the Eden emulator, then collect its log and a screenshot.
# Usage: .\tools\run_eden.ps1 [-Nro build\hello.nro] [-Seconds 30] [-Args '--quit-after 20'] [-Log hello.log] [-Shot out.png]
#                             [-AutoInput script.txt] [-Shots dir]
#   -Seconds  how long to wait before taking the screenshot and closing Eden (if the app hasn't quit by itself)
#   -Args     passed to the homebrew's argv (eden-cli -p). Eden 0.2.1 doesn't deliver these (argc stays 0), so rely
#             on -Seconds to end the run.
#   -Log      file name under sdmc:/switch/Bejeweled/ to print afterwards
#   -AutoInput  scripted input for the game (format: SexyAppBase::LoadAutoInput), installed for this run only
#   -Shots    copy the game's own screenshots (the script's 'shot' command) here afterwards
#   -Fresh    delete the saved game first, so the run starts without the resume dialog
# Eden is looked up in $env:EDEN_DIR, then ~\Downloads\eden. Its SD card is %APPDATA%\eden\sdmc.
param(
    [string]$Nro = "$PSScriptRoot\..\build\hello.nro",
    [int]$Seconds = 30,
    [string]$Args = '',
    [string]$Log = 'hello.log',
    [string]$Shot = '',
    [string]$AutoInput = '',
    [string]$Shots = '',
    [switch]$Fresh
)
$ErrorActionPreference = 'Stop'
$edenDir = if ($env:EDEN_DIR) { $env:EDEN_DIR } else { Join-Path $env:USERPROFILE 'Downloads\eden' }
$eden = Join-Path $edenDir 'eden-cli.exe'
if (-not (Test-Path $eden)) { throw "eden-cli.exe not found in $edenDir (set EDEN_DIR)" }
$Nro = (Resolve-Path $Nro).Path
$logPath = Join-Path $env:APPDATA "eden\sdmc\switch\Bejeweled\$Log"
Remove-Item $logPath -ErrorAction SilentlyContinue
$dataDir = Join-Path $env:APPDATA 'eden\sdmc\switch\Bejeweled'
$autoPath = Join-Path $dataDir 'autoinput.txt'
$shotDir = Join-Path $dataDir '_screenshots'
Remove-Item $shotDir -Recurse -ErrorAction SilentlyContinue
if ($Fresh) { Remove-Item (Join-Path $dataDir 'userdata\savegame.dat') -ErrorAction SilentlyContinue }
if ($AutoInput) { New-Item -ItemType Directory -Force $dataDir | Out-Null; Copy-Item $AutoInput $autoPath -Force }
Get-Process eden-cli -ErrorAction SilentlyContinue | Stop-Process -Force

$argList = @('-g', "`"$Nro`"")
if ($Args) { $argList += @('-p', "`"$Args`"") }
$p = Start-Process $eden -ArgumentList $argList -PassThru
$deadline = (Get-Date).AddSeconds($Seconds)
while (-not $p.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }

if (-not $p.HasExited) {
    $p.Refresh()
    "eden: $($p.MainWindowTitle)"
    if ($Shot) { & "$PSScriptRoot\screenshot_window.ps1" -ProcessId $p.Id -Out $Shot }
    Stop-Process -Id $p.Id -Force
} else {
    "eden exited by itself (code $($p.ExitCode))"
}
Start-Sleep -Milliseconds 500
Remove-Item $autoPath -ErrorAction SilentlyContinue
if ($Shots -and (Test-Path $shotDir)) {
    New-Item -ItemType Directory -Force $Shots | Out-Null
    Get-ChildItem $shotDir -Filter *.png | Sort-Object { [int](($_.BaseName) -replace '\D', '') } | ForEach-Object { Copy-Item $_.FullName $Shots -Force; "shot: $(Join-Path $Shots $_.Name)" }
}
"--- sdmc:/switch/Bejeweled/$Log"
if (Test-Path $logPath) { Get-Content $logPath } else { '(no log written)' }
