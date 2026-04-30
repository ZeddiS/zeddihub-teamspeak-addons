# Reorganize releases: delete all .zip assets, upload raw DLLs + installer.
# Run AFTER build_all.ps1 + build_installers.ps1.

$ErrorActionPreference = "Continue"
$root = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist = Join-Path $root "dist"
$installers = Join-Path $root "installer\output"

$plugins = @(
    @{ src="pokebot";       repo="zeddihub-teamspeak-pokebot";       version="1.2.1"; dllBase="zeddihub_pokebot"; installer="pokebot-Setup-v1.2.1.exe" }
    @{ src="Follow";        repo="zeddihub-teamspeak-follow";        version="1.2.1"; dllBase="follow";           installer="follow-Setup-v1.2.1.exe" }
    @{ src="MoveSpam";      repo="zeddihub-teamspeak-movespam";      version="1.2.0"; dllBase="movespam";         installer="movespam-Setup-v1.2.0.exe" }
    @{ src="VoiceChanger";  repo="zeddihub-teamspeak-voicechanger";  version="1.2.4"; dllBase="voicechanger";     installer="voicechanger-Setup-v1.2.4.exe" }
    @{ src="AutoReconnect"; repo="zeddihub-teamspeak-autoreconnect"; version="1.0.1"; dllBase="autoreconnect";    installer="autoreconnect-Setup-v1.0.1.exe" }
    @{ src="GreetingBot";   repo="zeddihub-teamspeak-greetingbot";   version="1.0.1"; dllBase="greetingbot";      installer="greetingbot-Setup-v1.0.1.exe" }
)

foreach ($p in $plugins) {
    Write-Host ""
    Write-Host "=== Reorganizing $($p.repo) v$($p.version) release ===" -ForegroundColor Cyan
    $tag = "v$($p.version)"

    # Delete existing release if any (so we get a clean slate)
    gh release delete $tag --repo "ZeddiS/$($p.repo)" --yes --cleanup-tag 2>&1 | Out-Null

    # Collect assets to upload: 4 DLLs + 1 installer
    $assets = @()
    foreach ($api in 23, 24, 25, 26) {
        $dll = Join-Path $dist "$($p.src)\api$api\$($p.dllBase)_api${api}_win64.dll"
        if (Test-Path $dll) { $assets += $dll }
    }
    $exe = Join-Path $installers $p.installer
    if (Test-Path $exe) { $assets += $exe }

    if ($assets.Count -eq 0) {
        Write-Host "  No assets found, skipping" -ForegroundColor Yellow
        continue
    }

    $notes = @(
        "## $($p.repo) $tag",
        "",
        "### Downloads",
        "",
        "**Recommended:** Run the installer ``$($p.installer)``.",
        "",
        "**Manual:** Pick the DLL matching your TeamSpeak 3 version:",
        "",
        "| TS3 client | API | File |",
        "|---|---|---|",
        "| 3.5.0 | 23 | ``$($p.dllBase)_api23_win64.dll`` |",
        "| 3.5.1 - 3.5.5 | 24 | ``$($p.dllBase)_api24_win64.dll`` |",
        "| 3.5.6 | 25 | ``$($p.dllBase)_api25_win64.dll`` |",
        "| 3.6+ | 26 | ``$($p.dllBase)_api26_win64.dll`` |",
        "",
        "Copy the DLL to ``%APPDATA%\TS3Client\plugins\`` and enable the plugin in TS3 Settings -> Plugins.",
        "",
        "Built against Qt 5.12.12 for TS3 ABI compatibility.",
        "",
        "(C) 2026 ZeddiHub.eu | zeddis.xyz | https://zeddihub.eu"
    ) -join "`r`n"
    $notesFile = Join-Path $env:TEMP "zh_release_$($p.repo)_$tag.txt"
    $notes | Out-File $notesFile -Encoding UTF8

    gh release create $tag --repo "ZeddiS/$($p.repo)" --title "$($p.repo) $tag" --notes-file $notesFile @assets 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Released $tag with $($assets.Count) assets" -ForegroundColor Green
    } else {
        Write-Host "  Release create FAILED for $($p.repo)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Done." -ForegroundColor Yellow
