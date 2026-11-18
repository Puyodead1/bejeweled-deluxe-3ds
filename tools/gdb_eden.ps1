# Runs an .nro in Eden under gdb: turns on Eden's gdbstub (and back off afterwards), attaches devkitA64 gdb with the
# symbols offset to wherever ASLR put the module, runs -Extra gdb commands (breakpoints, `commands` blocks), lets it
# run for -Seconds, interrupts and prints pc. Eden's stub can't read guest memory or list threads, so log registers
# from breakpoint `commands` (printf "%lx", $lr) and resolve them with addr2line against the ELF offset.
# Example: .\tools\gdb_eden.ps1 -Seconds 30 -Extra 'break abort'
param([int]$Seconds = 20, [string]$Nro = "$PSScriptRoot\..\build\bejeweled.nro", [string]$Elf = "$PSScriptRoot\..\build\bejeweled.elf", [string[]]$Extra = @())
$Nro = (Resolve-Path $Nro).Path
$Elf = (Resolve-Path $Elf).Path -replace '\\','/'
$edenDir = if ($env:EDEN_DIR) { $env:EDEN_DIR } else { Join-Path $env:USERPROFILE 'Downloads\eden' }
$cfg = "$env:APPDATA\eden\config\sdl2-config.ini"
$s = [IO.File]::ReadAllText($cfg)
[IO.File]::WriteAllText($cfg, $s.Replace("use_gdbstub=false", "use_gdbstub=true").Replace("use_gdbstub\default=true", "use_gdbstub\default=false"))
try {
    Get-Process eden-cli -ErrorAction SilentlyContinue | Stop-Process -Force
    $e = Start-Process (Join-Path $edenDir 'eden-cli.exe') -ArgumentList @('-g', "`"$Nro`"") -PassThru
    Start-Sleep 6

    $psi = New-Object Diagnostics.ProcessStartInfo 'C:\devkitPro\devkitA64\bin\aarch64-none-elf-gdb.exe', '-q -nx'
    $psi.UseShellExecute = $false
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $g = [Diagnostics.Process]::Start($psi)
    $out = New-Object Text.StringBuilder
    $h = { if ($EventArgs.Data -ne $null) { [void]$Event.MessageData.AppendLine($EventArgs.Data) } }
    $j1 = Register-ObjectEvent $g OutputDataReceived -Action $h -MessageData $out
    $j2 = Register-ObjectEvent $g ErrorDataReceived -Action $h -MessageData $out
    $g.BeginOutputReadLine(); $g.BeginErrorReadLine()
    $send = { param($c) $g.StandardInput.WriteLine($c); $g.StandardInput.Flush() }

    & $send ''   # .NET puts a BOM in front of the first line; let it be a junk command
    & $send 'set pagination off'
    & $send 'set confirm off'
    & $send 'set width 0'
    & $send 'target extended-remote localhost:6543'
    Start-Sleep 2
    & $send "eval `"symbol-file -o 0x%lx $Elf`", `$pc"
    Start-Sleep 3
    foreach ($c in $Extra) { & $send $c }
    & $send 'continue &'
    Start-Sleep $Seconds
    & $send 'interrupt'
    Start-Sleep 4
    & $send 'p/x $pc'
    Start-Sleep 8
    & $send 'kill'
    & $send 'quit'
    $g.WaitForExit(10000) | Out-Null
    Unregister-Event -SourceIdentifier $j1.Name; Unregister-Event -SourceIdentifier $j2.Name
    $out.ToString()
}
finally {
    Get-Process eden-cli -ErrorAction SilentlyContinue | Stop-Process -Force
    $s = [IO.File]::ReadAllText($cfg)
    [IO.File]::WriteAllText($cfg, $s.Replace("use_gdbstub=true", "use_gdbstub=false").Replace("use_gdbstub\default=false", "use_gdbstub\default=true"))
}
