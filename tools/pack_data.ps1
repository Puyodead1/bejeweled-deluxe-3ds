# Copy the game's data files from an installed Bejeweled Deluxe 1.87 into romfs/, which the build packs into the .3dsx.
# Usage: .\tools\pack_data.ps1 [-GameDir <path>]
#
# RomFS is case-sensitive and the game's paths aren't consistent ("music\1980-autonomus.s3m" vs the
# file 1980-Autonomus.s3m, dialogbox2.PNG), so every path is stored in lowercase; the file layer lowercases paths too.
# Only what the game reads is copied: the PopCap DRM wrapper (drm\, drm.*, Flash.ocx), bass.dll and the exes stay out.
param([string]$GameDir = 'J:\SteamLibrary\steamapps\common\Bejeweled Deluxe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'romfs'
# Not userdata\: the game doesn't ship one (the default high scores are built in); a userdata folder in an install
# is someone's saved game and scores.
$folders = 'data', 'images', 'music', 'properties', 'sounds'

if (-not (Test-Path (Join-Path $GameDir 'images'))) { throw "No game data in '$GameDir' (expected an images folder)" }
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory $out | Out-Null

$count = 0; $bytes = 0
foreach ($f in $folders) {
    $dir = Join-Path $GameDir $f
    if (-not (Test-Path $dir)) { Write-Warning "missing $f\"; continue }
    Get-ChildItem $dir -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($GameDir.TrimEnd('\').Length + 1).ToLowerInvariant()
        $dest = Join-Path $out $rel
        New-Item -ItemType Directory -Force (Split-Path $dest -Parent) | Out-Null
        Copy-Item $_.FullName $dest
        $count++; $bytes += $_.Length
    }
}
'{0} files, {1:0.0} MB -> {2}' -f $count, ($bytes / 1MB), $out
