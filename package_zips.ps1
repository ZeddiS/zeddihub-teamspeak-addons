# Package built DLLs into per-plugin x per-API .zip files for GitHub release.
# Run AFTER build_all.ps1 succeeds.

$ErrorActionPreference = "Stop"

$root    = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist    = Join-Path $root "dist"
$release = Join-Path $root "release"
$version = "1.0.0"

if (-not (Test-Path $dist)) {
    Write-Host "ERROR: dist/ not found. Run build_all.ps1 first." -ForegroundColor Red
    exit 1
}

if (Test-Path $release) { Remove-Item -Recurse -Force $release }
New-Item -ItemType Directory -Force -Path $release | Out-Null

$plugins = @("pokebot", "Follow", "MoveSpam", "VoiceChanger")
$apis    = @(23, 24, 25, 26)

function ClientFor($api) {
    switch ($api) {
        23 { return "~3.5.0" }
        24 { return "3.5.1 - 3.5.5" }
        25 { return "3.5.6" }
        26 { return "3.6.x+" }
        default { return "unknown" }
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
        if (-not $dll) {
            Write-Host "  SKIP: $plugin api$api (no .dll in $srcDir)" -ForegroundColor Yellow
            continue
        }

        $stage = Join-Path $env:TEMP "zhrelease_$($plugin)_api$api"
        if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
        New-Item -ItemType Directory -Force -Path $stage | Out-Null

        Copy-Item $dll.FullName $stage -Force

        $clientVer = ClientFor $api
        $readmeLines = @(
            "$plugin - TeamSpeak 3 plugin (API $api)",
            "",
            "Verze: v$version",
            "Pro TS3 client: $clientVer",
            "",
            "INSTALACE:",
            "1. Zkopiruj $($dll.Name) do:",
            "   %APPDATA%\TS3Client\plugins\",
            "   (typicky C:\Users\<USER>\AppData\Roaming\TS3Client\plugins\)",
            "",
            "2. V TS3 -> Settings -> Plugins -> Reload All -> zaskrtni Enabled",
            "",
            "3. Pouziti viz README repozitare.",
            "",
            "POZNAMKA: stahuj jen JEDNU API verzi pluginu - jinak menu duplicitni.",
            "Pokud TS3 hlasi API not compatible, mas spatnou variantu."
        )
        $readmeLines -join "`r`n" | Out-File -FilePath (Join-Path $stage "README.txt") -Encoding UTF8

        $zipPath = Join-Path $release "${plugin}_api${api}_v${version}.zip"
        Compress-Archive -Path "$stage\*" -DestinationPath $zipPath -Force
        Write-Host "  ${plugin}_api${api}: $($dll.Length) B -> $zipPath" -ForegroundColor Green

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
        $bundleLines = @(
            "ZeddiHub TeamSpeak Addons - bundle pro API $api",
            "",
            "Obsahuje vsechny pluginy: Poke Bot, Follow, MoveSpam, Voice Changer.",
            "",
            "INSTALACE: zkopiruj vsechny .dll do %APPDATA%\TS3Client\plugins\",
            "a v TS3 zaskrtni Enabled. Pokud nechces vsechny, smaz ty co nepotrebujes."
        )
        $bundleLines -join "`r`n" | Out-File -FilePath (Join-Path $stage "README.txt") -Encoding UTF8
        $zipPath = Join-Path $release "all_plugins_api${api}_v${version}.zip"
        Compress-Archive -Path "$stage\*" -DestinationPath $zipPath -Force
        Write-Host "  bundle api${api} -> $zipPath" -ForegroundColor Cyan
    }
    Remove-Item -Recurse -Force $stage
}

Write-Host "`nReleases packaged in: $release" -ForegroundColor Yellow
Get-ChildItem $release | Select-Object Name, Length
