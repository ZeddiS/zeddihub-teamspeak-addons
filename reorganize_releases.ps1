# Recreate releases for 4 plugins with new asset layout:
#   - 4x .ts3_plugin (TS3 native install)
#   - 4x raw .dll (manual install)
# (Drops Inno Setup .exe — ts3_plugin is the new standard.)

$ErrorActionPreference = "Continue"
$root = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist = Join-Path $root "dist"
$ts3pkgs = Join-Path $root "ts3_plugins"

$plugins = @(
    @{ src="pokebot";    repo="zeddihub-teamspeak-pokebot";    version="1.3.0"; dllBase="zeddihub_pokebot" }
    @{ src="Follow";     repo="zeddihub-teamspeak-follow";     version="1.3.0"; dllBase="follow"           }
    @{ src="MoveSpam";   repo="zeddihub-teamspeak-movespam";   version="1.3.0"; dllBase="movespam"         }
    @{ src="Soundboard"; repo="zeddihub-teamspeak-soundboard"; version="1.2.4"; dllBase="soundboard"       }
    @{ src="VoiceChanger"; repo="zeddihub-teamspeak-voicechanger"; version="1.2.5"; dllBase="voicechanger" }
)

function Ts3Label($api) {
    switch ($api) {
        23 { return "TS3-3.5.0" }
        24 { return "TS3-3.5.1-3.5.5" }
        25 { return "TS3-3.5.6" }
        26 { return "TS3-3.6+" }
    }
}

foreach ($p in $plugins) {
    $tag = "v$($p.version)"
    Write-Host ""
    Write-Host "=== $($p.repo) $tag ===" -ForegroundColor Cyan

    # Wipe existing release if any
    gh release delete $tag --repo "ZeddiS/$($p.repo)" --yes --cleanup-tag 2>&1 | Out-Null

    # Collect assets: 4 ts3_plugin + 4 raw DLLs
    $assets = @()
    foreach ($api in 23, 24, 25, 26) {
        $label = Ts3Label $api
        $pkg = Join-Path $ts3pkgs "$($p.src)-v$($p.version)-${label}-api${api}.ts3_plugin"
        if (Test-Path $pkg) { $assets += $pkg }
        $dll = Join-Path $dist "$($p.src)\api${api}\$($p.dllBase)_api${api}_win64.dll"
        if (Test-Path $dll) { $assets += $dll }
    }

    if ($assets.Count -eq 0) {
        Write-Host "  No assets found, skipping" -ForegroundColor Yellow
        continue
    }

    $notes = @(
        "## Downloads",
        "",
        "**Recommended -- TS3 native install:** double-click the .ts3_plugin file matching your TS3 version.",
        "**Manual:** drop the raw .dll into ``%APPDATA%\TS3Client\plugins\``.",
        "",
        "| TS3 client | API | TS3 install | Manual DLL |",
        "|---|---|---|---|",
        "| 3.5.0 | 23 | $($p.src)-v$($p.version)-TS3-3.5.0-api23.ts3_plugin | $($p.dllBase)_api23_win64.dll |",
        "| 3.5.1 - 3.5.5 | 24 | $($p.src)-v$($p.version)-TS3-3.5.1-3.5.5-api24.ts3_plugin | $($p.dllBase)_api24_win64.dll |",
        "| 3.5.6 | 25 | $($p.src)-v$($p.version)-TS3-3.5.6-api25.ts3_plugin | $($p.dllBase)_api25_win64.dll |",
        "| 3.6+ | 26 | $($p.src)-v$($p.version)-TS3-3.6+-api26.ts3_plugin | $($p.dllBase)_api26_win64.dll |",
        "",
        "Built against Qt 5.12.12 for TS3 ABI compatibility.",
        "Dialogs inherit TS3 client's native theme.",
        "",
        "(C) 2026 ZeddiHub.eu | zeddis.xyz | https://zeddihub.eu"
    ) -join "`r`n"
    $notesFile = Join-Path $env:TEMP "zh_release_$($p.repo)_$tag.txt"
    $notes | Out-File $notesFile -Encoding UTF8

    gh release create $tag --repo "ZeddiS/$($p.repo)" --title "$($p.repo) $tag" --notes-file $notesFile @assets 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Released with $($assets.Count) assets" -ForegroundColor Green
    } else {
        Write-Host "  Release create failed" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Done." -ForegroundColor Yellow
