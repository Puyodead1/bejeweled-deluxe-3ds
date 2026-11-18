# Build the Switch .nro through the devkitPro msys2 shell (devkitPro's CMake support refuses Windows-native CMake).
# Usage: .\build.ps1 [-Debug] [-Clean] [-Target name] [-KeepGoing]
#   -Debug      unoptimised build with debug info, into build-debug\
#   -Clean      rebuild everything
#   -Target     build one CMake target (e.g. sexyframework) instead of all
#   -KeepGoing  don't stop at the first failing file (make -k), to see every error at once
# Output: build\*.nro (or build-debug\*.nro). Run tools\pack_data.ps1 once first to fill romfs\.
param([switch]$Debug, [switch]$Clean, [string]$Target = '', [switch]$KeepGoing)
$ErrorActionPreference = 'Stop'
$dkp = if ($env:DEVKITPRO_WIN) { $env:DEVKITPRO_WIN } else { 'C:\devkitPro' }
$bash = Join-Path $dkp 'msys2\usr\bin\bash.exe'
if (-not (Test-Path $bash)) { throw "devkitPro msys2 not found at $dkp (set DEVKITPRO_WIN)" }
if (-not (Test-Path "$PSScriptRoot\romfs")) { throw 'romfs\ is missing: run tools\pack_data.ps1 first' }

# T:\a\b -> /t/a/b for msys2
$root = '/' + $PSScriptRoot.Substring(0, 1).ToLower() + ($PSScriptRoot.Substring(2) -replace '\\', '/')
$type = if ($Debug) { 'Debug' } else { 'Release' }
$dir = if ($Debug) { 'build-debug' } else { 'build' }
$cleanArg = if ($Clean) { '--clean-first' } else { '' }
$jobs = [Environment]::ProcessorCount
$targetArg = if ($Target) { "--target $Target" } else { '' }
$makeArg = if ($KeepGoing) { '-- -k' } else { '' }

$script = @"
set -e
export DEVKITPRO=/opt/devkitpro DEVKITA64=/opt/devkitpro/devkitA64
export PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:/usr/bin:`$PATH
cd '$root'
cmake -S . -B $dir -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=`$DEVKITPRO/cmake/Switch.cmake -DCMAKE_BUILD_TYPE=$type > /dev/null
cmake --build $dir $cleanArg $targetArg -j$jobs $makeArg
"@
# Native tools write warnings to stderr; under 'Stop' PowerShell 5.1 would treat those as fatal.
$ErrorActionPreference = 'Continue'
$env:MSYSTEM = 'MSYS'; $env:CHERE_INVOKING = '1'
& $bash -lc ($script -replace "`r", '')
exit $LASTEXITCODE
