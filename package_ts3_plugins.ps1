# Package each plugin x API into .ts3_plugin file (TS3 native install).
# Format: ZIP renamed to .ts3_plugin, containing:
#   package.ini  (plugin metadata)
#   plugins/<name>_win64.dll
# When user double-clicks, TS3 client opens install dialog.

$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist = Join-Path $root "dist"
$out  = Join-Path $root "ts3_plugins"

if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force -Path $out | Out-Null

$plugins = @(
    @{ src="pokebot";    title="Poke Bot";    version="1.3.0"; dllBase="zeddihub_pokebot"; iconBase="pokebot";
       desc="Right-click any TeamSpeak 3 client to launch a poke campaign." }
    @{ src="Follow";     title="Follow";      version="1.3.0"; dllBase="follow"; iconBase="follow";
       desc="Auto-follow another TeamSpeak 3 client into channels." }
    @{ src="MoveSpam";   title="MoveSpam";    version="1.3.0"; dllBase="movespam"; iconBase="movespam";
       desc="Repeatedly move a TeamSpeak 3 client between two channels." }
    @{ src="Soundboard"; title="Soundboard";  version="1.0.0"; dllBase="soundboard"; iconBase="soundboard";
       desc="Play sound files into your microphone stream with hotkeys." }
    @{ src="VoiceChanger"; title="Voice Changer"; version="1.2.5"; dllBase="voicechanger"; iconBase="voicechanger";
       desc="Real-time DSP voice effects for outgoing microphone." }
)
$iconsDir = Join-Path $root "icons"

function Ts3Label($api) {
    switch ($api) {
        23 { return "TS3-3.5.0" }
        24 { return "TS3-3.5.1-3.5.5" }
        25 { return "TS3-3.5.6" }
        26 { return "TS3-3.6+" }
    }
}

foreach ($p in $plugins) {
    foreach ($api in 23, 24, 25, 26) {
        $dll = Join-Path $dist "$($p.src)\api$api\$($p.dllBase)_api${api}_win64.dll"
        if (-not (Test-Path $dll)) {
            Write-Host "  SKIP: $($p.src) api$api (DLL missing)" -ForegroundColor Yellow
            continue
        }

        # Build staging tree
        $stage = Join-Path $env:TEMP "zh_ts3pkg_$($p.src)_api$api"
        if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
        New-Item -ItemType Directory -Force -Path "$stage\plugins" | Out-Null

        # package.ini
        $iniLines = @(
            "Name = $($p.title) (API $api)",
            "Type = Plugin",
            "Author = zeddis.xyz",
            "Version = $($p.version)",
            "Platforms = win64",
            "Description = ""$($p.desc) | Built for TS3 plugin API $api. Copyright (C) 2026 ZeddiHub.eu"""
        )
        ($iniLines -join "`r`n") | Out-File "$stage\package.ini" -Encoding UTF8

        # Copy DLL into plugins/
        Copy-Item $dll "$stage\plugins\" -Force

        # Bundle icon into plugins/<dll_name>/icon.png so TS3 can find it
        # via menuIcon = "icon.png" (relative to plugin's icon directory).
        $dllBaseNoExt = "$($p.dllBase)_api${api}_win64"
        $iconDestDir = "$stage\plugins\$dllBaseNoExt"
        New-Item -ItemType Directory -Force -Path $iconDestDir | Out-Null
        $iconSrc = Join-Path $iconsDir "$($p.iconBase).png"
        if (Test-Path $iconSrc) {
            Copy-Item $iconSrc "$iconDestDir\icon.png" -Force
        }

        # Zip and rename to .ts3_plugin
        $label = Ts3Label $api
        $zipPath = Join-Path $out "$($p.src)-v$($p.version)-${label}-api${api}.ts3_plugin"
        $tmpZip  = "$zipPath.zip"
        if (Test-Path $tmpZip) { Remove-Item $tmpZip -Force }
        Compress-Archive -Path "$stage\*" -DestinationPath $tmpZip -Force
        if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
        Move-Item $tmpZip $zipPath -Force

        Remove-Item -Recurse -Force $stage
        Write-Host "  $(Split-Path $zipPath -Leaf) ($((Get-Item $zipPath).Length) B)" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "ts3_plugin packages in: $out" -ForegroundColor Yellow
Get-ChildItem $out | Format-Table Name, Length -AutoSize
