# Package built DLLs into per-plugin x per-API .zip files.
# Filename includes TS3 client version range so user can identify at a glance.

$ErrorActionPreference = "Stop"

$root    = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist    = Join-Path $root "dist"
$release = Join-Path $root "release"
$version = "1.1.0"

if (-not (Test-Path $dist)) {
    Write-Host "ERROR: dist/ not found. Run build_all.ps1 first." -ForegroundColor Red
    exit 1
}

if (Test-Path $release) { Remove-Item -Recurse -Force $release }
New-Item -ItemType Directory -Force -Path $release | Out-Null

$plugins = @("pokebot", "Follow", "MoveSpam", "VoiceChanger", "AutoReconnect", "GreetingBot")
$apis    = @(23, 24, 25, 26)

# API -> human TS3 client version label used in zip filename
function Ts3LabelFor($api) {
    switch ($api) {
        23 { return "TS3-3.5.0" }
        24 { return "TS3-3.5.1-3.5.5" }
        25 { return "TS3-3.5.6" }
        26 { return "TS3-3.6+" }
        default { return "TS3-unknown" }
    }
}

foreach ($plugin in $plugins) {
    foreach ($api in $apis) {
        $srcDir = Join-Path $dist "$plugin\api$api"
        if (-not (Test-Path $srcDir)) {
            Write-Host "  SKIP: $plugin api$api (no DLL)" -ForegroundColor Yellow
            continue
        }

        $dll = Get-ChildItem $srcDir -Filter "*.dll" | Select-Object -First 1
        if (-not $dll) { continue }

        $stage = Join-Path $env:TEMP "zhrelease_$($plugin)_api$api"
        if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
        New-Item -ItemType Directory -Force -Path $stage | Out-Null

        Copy-Item $dll.FullName $stage -Force

        $ts3label = Ts3LabelFor $api
        $readmeLines = @(
            "$plugin v$version",
            "Plugin for: $ts3label (TS3 plugin API $api)",
            "",
            "INSTALACE:",
            "1. Zkopiruj $($dll.Name) do:",
            "   %APPDATA%\TS3Client\plugins\",
            "   (typicky C:\Users\<USER>\AppData\Roaming\TS3Client\plugins\)",
            "",
            "2. V TS3 -> Settings -> Plugins -> Reload All -> zaskrtni Enabled",
            "",
            "POZNAMKA:",
            "Stahuj jen JEDNU API verzi pluginu - jinak by TS3 ukazal duplicity menu.",
            "Pokud TS3 hlasi 'API not compatible', mas spatnou variantu - zkus jinou.",
            "",
            "ZeddiHub TeamSpeak Addons | zeddis.xyz | (C) 2026 ZeddiHub.eu",
            "https://github.com/ZeddiS/zeddihub-teamspeak-addons"
        )
        $readmeLines -join "`r`n" | Out-File -FilePath (Join-Path $stage "README.txt") -Encoding UTF8

        $zipPath = Join-Path $release "${plugin}-v${version}-${ts3label}-api${api}.zip"
        Compress-Archive -Path "$stage\*" -DestinationPath $zipPath -Force
        Write-Host "  $($dll.Name): $($dll.Length) B -> $(Split-Path $zipPath -Leaf)" -ForegroundColor Green

        Remove-Item -Recurse -Force $stage
    }
}

# Master "all plugins" bundle per API
foreach ($api in $apis) {
    $stage = Join-Path $env:TEMP "zhrelease_all_api$api"
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    New-Item -ItemType Directory -Force -Path $stage | Out-Null

    $hasAny = $false
    foreach ($plugin in $plugins) {
        $srcDir = Join-Path $dist "$plugin\api$api"
        if (Test-Path $srcDir) {
            Get-ChildItem $srcDir -Filter "*.dll" | ForEach-Object {
                Copy-Item $_.FullName $stage -Force
                $hasAny = $true
            }
        }
    }

    if ($hasAny) {
        $ts3label = Ts3LabelFor $api
        $bundleLines = @(
            "ZeddiHub TeamSpeak Addons - kompletni bundle v$version",
            "Plugin pro: $ts3label (TS3 plugin API $api)",
            "",
            "Obsah: PokeBot, Follow, MoveSpam, VoiceChanger, AutoReconnect, GreetingBot",
            "",
            "INSTALACE: zkopiruj vsechny .dll do %APPDATA%\TS3Client\plugins\",
            "a v TS3 zaskrtni Enabled. Pokud nechces vsechny, smaz ty co nepotrebujes.",
            "",
            "ZeddiHub TeamSpeak Addons | zeddis.xyz | (C) 2026 ZeddiHub.eu",
            "https://github.com/ZeddiS/zeddihub-teamspeak-addons"
        )
        $bundleLines -join "`r`n" | Out-File -FilePath (Join-Path $stage "README.txt") -Encoding UTF8
        $zipPath = Join-Path $release "all-plugins-v${version}-${ts3label}-api${api}.zip"
        Compress-Archive -Path "$stage\*" -DestinationPath $zipPath -Force
        Write-Host "  bundle $ts3label -> $(Split-Path $zipPath -Leaf)" -ForegroundColor Cyan
    }
    Remove-Item -Recurse -Force $stage
}

Write-Host "`nReleases packaged in: $release" -ForegroundColor Yellow
Get-ChildItem $release | Sort-Object Name | Select-Object Name, Length
