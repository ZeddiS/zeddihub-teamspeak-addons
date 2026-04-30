# ASCII-only source. Czech text only inside string literals.
$ErrorActionPreference = "Continue"

$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$workRoot  = Join-Path $env:TEMP "zh_repo_refresh"
$ghOrg     = "ZeddiS"
$brandHdr  = Get-Content (Join-Path $root "common\zh_brand.h") -Raw

if (Test-Path $workRoot) { Remove-Item -Recurse -Force $workRoot }
New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

$plugins = @(
    @{ src="pokebot";       repo="zeddihub-teamspeak-pokebot";       title="Poke Bot";       version="1.2.0"; clogKey="pokebot";       dllBase="zeddihub_pokebot";
       blurb="Right-click a TeamSpeak 3 client to launch a poke campaign. Preset bursts, custom poke dialog, MAX SPAM hard-cap, Qt dark theme." }
    @{ src="Follow";        repo="zeddihub-teamspeak-follow";        title="Follow";         version="1.2.0"; clogKey="follow";        dllBase="follow";
       blurb="Auto-follow another TeamSpeak 3 client into channels. Polling worker (1s) - never auto-stops on errors, only manual STOP or target disconnect." }
    @{ src="MoveSpam";      repo="zeddihub-teamspeak-movespam";      title="MoveSpam";       version="1.1.0"; clogKey="movespam";      dllBase="movespam";
       blurb="Repeatedly move a TeamSpeak 3 client between two channels. Basic mode (current vs default) or Custom Qt dialog with channel name/ID and interval slider." }
    @{ src="VoiceChanger";  repo="zeddihub-teamspeak-voicechanger";  title="Voice Changer";  version="1.2.3"; clogKey="voicechanger";  dllBase="voicechanger";
       blurb="Real-time DSP effects on outgoing microphone audio. Helium / Demon / Robot / Echo / Telephone / Underwater / Megaphone / Distortion / Whisper / Custom pitch slider. Per-preset menu in Plugins top bar." }
    @{ src="AutoReconnect"; repo="zeddihub-teamspeak-autoreconnect"; title="AutoReconnect";  version="1.0.0"; clogKey="autoreconnect"; dllBase="autoreconnect";
       blurb="Detects unexpected TeamSpeak 3 disconnects (network drop, kicks aside) and automatically reconnects with exponential backoff. Plugins top-bar menu." }
    @{ src="GreetingBot";   repo="zeddihub-teamspeak-greetingbot";   title="Greeting Bot";   version="1.0.0"; clogKey="greetingbot";   dllBase="greetingbot";
       blurb="Automatically pokes any user that joins your TeamSpeak 3 channel with a configurable greeting. Use {name} placeholder for personalization." }
)

$changelogs = @{
    "pokebot" = @(
        "## [1.2.0] - 2026-04-30",
        "### Added",
        "- Qt 5.12 dark-theme custom dialog with Burst/Schedule mode toggle and synced sliders",
        "- MAX SPAM preset (hard-cap 500 pokes, 50ms anti-flood floor)",
        "- Plugins top-bar menu (Stop campaign / Status / About)",
        "- Author: zeddis.xyz, Copyright (C) 2026 ZeddiHub.eu",
        "- Multi-API support: 23 / 24 / 25 / 26",
        "",
        "## [1.0.1] - 2026-04-29",
        "### Fixed",
        "- Added ts3plugin_freeMemory + requestAutoload exports (previously menu items did not appear)",
        "- Plugin renamed to 'Poke Bot'",
        "",
        "## [1.0.0] - 2026-04-29",
        "### Added",
        "- Initial release with 4 presets (Wake-up CZ / Halt / Symbol Storm / Silent) + Custom dialog"
    )
    "follow" = @(
        "## [1.2.0] - 2026-04-30",
        "### Changed",
        "- Rewrite to polling worker (1s interval). Never auto-stops on errors, only manual STOP or target gone for 10s+",
        "- Robust against missed events and 'User is already in this channel' responses",
        "",
        "## [1.1.0] - 2026-04-30",
        "### Added",
        "- Plugins top-bar menu (Stop / Status / About)",
        "- Author: zeddis.xyz, Copyright (C) 2026 ZeddiHub.eu",
        "- Multi-API support: 23 / 24 / 25 / 26",
        "### Fixed",
        "- ERROR_channel_already_in is now treated as success (do not stop following)",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: single-target follow with auto-stop on permission errors"
    )
    "movespam" = @(
        "## [1.1.0] - 2026-04-30",
        "### Added",
        "- Plugins top-bar menu (Stop / Status / About)",
        "- Author: zeddis.xyz, Copyright (C) 2026 ZeddiHub.eu",
        "- Multi-API support: 23 / 24 / 25 / 26",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: Basic (current vs default) and Custom Qt dialog modes"
    )
    "voicechanger" = @(
        "## [1.2.3] - 2026-04-30",
        "### Added",
        "- Per-preset menu items in Plugins top bar (one click = pick preset + enable)",
        "- Compat Mode toggle - when ON, plugin does not set edited=1 (mic stays working but voice changes do not transmit; useful for diagnosing VAD blocking)",
        "",
        "## [1.2.2] - 2026-04-30",
        "### Fixed",
        "- Pre-allocate scratch buffers in constructor (avoid heap allocation on audio thread)",
        "",
        "## [1.2.1] - 2026-04-30",
        "### Added",
        "- VolumeBoost sanity-test preset (1.5x gain, no other DSP)",
        "### Fixed",
        "- NaN/Inf protection in floatToInt16",
        "- Robot uses AM (amplitude modulation) instead of full ring mod (no zero-cross silence)",
        "- Distortion: removed post-clip 0.7x attenuation",
        "- Whisper: less aggressive attenuation (0.4x instead of 0.15x)",
        "- Pitch shifter wraps gSumPhase to [-PI, PI] (precision drift fix)",
        "",
        "## [1.2.0] - 2026-04-30",
        "### Added",
        "- 5 new effects: Distortion, Whisper, Telephone, Underwater, Megaphone",
        "- Settings dialog reorganized with Pitch / Effects section dividers",
        "### Fixed",
        "- Off preset now guaranteed to never set edited=1",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: Helium / Chipmunk / Demon / Deep / Robot / Echo / Custom + phase-vocoder pitch shifter"
    )
    "autoreconnect" = @(
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: auto-reconnect with exponential backoff on unexpected disconnects",
        "- Captures connection params (host, port, nickname, identity) on connect, replays on reconnect",
        "- Plugins top-bar menu: Toggle / Status / About"
    )
    "greetingbot" = @(
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: auto-pokes users entering your channel",
        "- Configurable greeting with {name} placeholder",
        "- Plugins top-bar menu: Toggle / Status / About"
    )
}

$mitLicense = @(
    "MIT License",
    "",
    "Copyright (c) 2026 ZeddiHub.eu (zeddis.xyz)",
    "",
    "Permission is hereby granted, free of charge, to any person obtaining a copy",
    "of this software and associated documentation files (the 'Software'), to deal",
    "in the Software without restriction, including without limitation the rights",
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell",
    "copies of the Software, and to permit persons to whom the Software is",
    "furnished to do so, subject to the following conditions:",
    "",
    "The above copyright notice and this permission notice shall be included in all",
    "copies or substantial portions of the Software.",
    "",
    "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR",
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,",
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE",
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER",
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,",
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE",
    "SOFTWARE."
) -join "`r`n"

foreach ($p in $plugins) {
    Write-Host ""
    Write-Host "=== Refreshing $($p.repo) ===" -ForegroundColor Cyan
    $work = Join-Path $workRoot $p.repo

    git clone "https://github.com/$ghOrg/$($p.repo).git" $work 2>&1 | Out-Null
    if (-not (Test-Path "$work\.git")) {
        Write-Host "  Clone failed, skipping" -ForegroundColor Red
        continue
    }

    Get-ChildItem $work -Force | Where-Object { $_.Name -ne ".git" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    $srcPath = Join-Path $root $p.src
    foreach ($item in @("CMakeLists.txt", "BUILD.md")) {
        $f = Join-Path $srcPath $item
        if (Test-Path $f) { Copy-Item $f $work -Force }
    }
    Copy-Item -Recurse (Join-Path $srcPath "src") $work -Force
    if (Test-Path (Join-Path $srcPath "vendor")) {
        Copy-Item -Recurse (Join-Path $srcPath "vendor") $work -Force
    }

    $brandHdr | Out-File "$work\src\zh_brand.h" -Encoding UTF8 -NoNewline

    $cpp = "$work\src\plugin.cpp"
    if (Test-Path $cpp) {
        $cppContent = Get-Content $cpp -Raw
        $cppContent = $cppContent -replace '#include "\.\./\.\./common/zh_brand\.h"', '#include "zh_brand.h"'
        Set-Content $cpp -Value $cppContent -NoNewline -Encoding UTF8
    }

    @(
        "build/", "build_api*/", "debug_build/", "out/",
        "*.dll", "*.pdb", "*.ilk", "*.exp", "*.lib", "*.obj", "*.so", "*.dylib",
        "CMakeCache.txt", "CMakeFiles/", "cmake_install.cmake",
        ".vs/", "*.vcxproj.user", "*.vcxproj.filters", "*.suo",
        "moc_*.cpp", "*_automoc.cpp",
        "ts3sdk/", "dist/", "release/", "*.exe", ""
    ) -join "`r`n" | Out-File "$work\.gitignore" -Encoding UTF8

    $mitLicense | Out-File "$work\LICENSE" -Encoding UTF8

    # README - constructed line by line, ASCII source
    $readme = New-Object System.Collections.ArrayList
    [void]$readme.Add("# $($p.title)")
    [void]$readme.Add("")
    [void]$readme.Add("[![Latest Release](https://img.shields.io/github/v/release/$ghOrg/$($p.repo))](https://github.com/$ghOrg/$($p.repo)/releases)")
    [void]$readme.Add("[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)")
    [void]$readme.Add("")
    [void]$readme.Add($p.blurb)
    [void]$readme.Add("")
    [void]$readme.Add("Part of the [**ZeddiHub TeamSpeak Addons**](https://github.com/$ghOrg/zeddihub-teamspeak-addons) collection.")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("## Instalace")
    [void]$readme.Add("")
    [void]$readme.Add("### A) Installer (.exe) - doporuceny zpusob")
    [void]$readme.Add("")
    [void]$readme.Add("Stahni a spust z [Releases](https://github.com/$ghOrg/$($p.repo)/releases/latest):")
    [void]$readme.Add("")
    [void]$readme.Add("**``$($p.src)-Setup-v$($p.version).exe``**")
    [void]$readme.Add("")
    [void]$readme.Add("Wizard:")
    [void]$readme.Add("1. Detekuje tvou TS3 verzi a vybere spravnou API DLL (23 / 24 / 25 / 26)")
    [void]$readme.Add("2. Detekuje bezici TS3 a nabidne uzavreni")
    [void]$readme.Add("3. Nainstaluje DLL do ``%APPDATA%\TS3Client\plugins\`` (per-user, bez admin prav)")
    [void]$readme.Add("4. Zaregistruje uninstaller v Add/Remove Programs")
    [void]$readme.Add("")
    [void]$readme.Add("### B) Manualni instalace (.zip)")
    [void]$readme.Add("")
    [void]$readme.Add("Vyber zip podle sve TS3 verze v [Releases](https://github.com/$ghOrg/$($p.repo)/releases/latest):")
    [void]$readme.Add("")
    [void]$readme.Add("| TS3 client | API | Stahni |")
    [void]$readme.Add("|---|---|---|")
    [void]$readme.Add("| 3.5.0 | 23 | ``$($p.src)-v$($p.version)-TS3-3.5.0-api23.zip`` |")
    [void]$readme.Add("| 3.5.1 - 3.5.5 | 24 | ``$($p.src)-v$($p.version)-TS3-3.5.1-3.5.5-api24.zip`` |")
    [void]$readme.Add("| **3.5.6** | **25** | **``$($p.src)-v$($p.version)-TS3-3.5.6-api25.zip``** |")
    [void]$readme.Add("| 3.6.x+ | 26 | ``$($p.src)-v$($p.version)-TS3-3.6+-api26.zip`` |")
    [void]$readme.Add("")
    [void]$readme.Add("Rozbal zip, zkopiruj DLL do ``%APPDATA%\TS3Client\plugins\``, a v TS3 -> Settings -> Plugins -> Reload All -> zaskrtni Enabled.")
    [void]$readme.Add("")
    [void]$readme.Add("## Build ze zdrojaku")
    [void]$readme.Add("")
    [void]$readme.Add('```powershell')
    [void]$readme.Add("# 1. Pozadavky")
    [void]$readme.Add("py -m pip install aqtinstall")
    [void]$readme.Add("py -m aqt install-qt windows desktop 5.12.12 win64_msvc2017_64 -O C:\Qt --archives qtbase")
    [void]$readme.Add("git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone")
    [void]$readme.Add("Move-Item ts3sdk_clone\include ts3sdk\include")
    [void]$readme.Add("")
    [void]$readme.Add("# 2. Build (napr. pro TS3 3.5.6 = API 25)")
    [void]$readme.Add("cmake -S . -B build_api25 -G `"Visual Studio 17 2022`" -A x64 ``")
    [void]$readme.Add("      -DCMAKE_PREFIX_PATH=`"C:\Qt\5.12.12\msvc2017_64`"")
    [void]$readme.Add("cmake --build build_api25 --config Release")
    [void]$readme.Add('```')
    [void]$readme.Add("")
    [void]$readme.Add("Output: ``build_api25\Release\$($p.dllBase)_api25_win64.dll``")
    [void]$readme.Add("")
    [void]$readme.Add("## Changelog")
    [void]$readme.Add("")
    [void]$readme.Add("Viz [CHANGELOG.md](CHANGELOG.md).")
    [void]$readme.Add("")
    [void]$readme.Add("## License")
    [void]$readme.Add("")
    [void]$readme.Add("MIT - viz [LICENSE](LICENSE).")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("## Links")
    [void]$readme.Add("")
    [void]$readme.Add("- **Web**: [zeddihub.eu](https://zeddihub.eu)")
    [void]$readme.Add("- **ZeddiHub Tools**: [zeddihub.eu/tools](https://zeddihub.eu/tools/)")
    [void]$readme.Add("- **Author**: [zeddis.xyz](https://zeddis.xyz)")
    [void]$readme.Add("- **Collection**: [zeddihub-teamspeak-addons](https://github.com/$ghOrg/zeddihub-teamspeak-addons)")
    [void]$readme.Add("- **Sister plugins**: [pokebot](https://github.com/$ghOrg/zeddihub-teamspeak-pokebot) | [follow](https://github.com/$ghOrg/zeddihub-teamspeak-follow) | [movespam](https://github.com/$ghOrg/zeddihub-teamspeak-movespam) | [voicechanger](https://github.com/$ghOrg/zeddihub-teamspeak-voicechanger) | [autoreconnect](https://github.com/$ghOrg/zeddihub-teamspeak-autoreconnect) | [greetingbot](https://github.com/$ghOrg/zeddihub-teamspeak-greetingbot)")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("(C) 2026 [ZeddiHub.eu](https://zeddihub.eu) - zeddis.xyz")
    ($readme -join "`r`n") | Out-File "$work\README.md" -Encoding UTF8

    $clog = New-Object System.Collections.ArrayList
    [void]$clog.Add("# Changelog")
    [void]$clog.Add("")
    [void]$clog.Add("All notable changes to **$($p.title)** are documented here.")
    [void]$clog.Add("Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).")
    [void]$clog.Add("")
    if ($changelogs.ContainsKey($p.clogKey)) {
        foreach ($line in $changelogs[$p.clogKey]) { [void]$clog.Add($line) }
    }
    ($clog -join "`r`n") | Out-File "$work\CHANGELOG.md" -Encoding UTF8

    Push-Location $work
    git add -A 2>&1 | Out-Null
    git -c user.email="bot@zeddihub.eu" -c user.name="ZeddiHub Bot" commit -m "Refresh repo: clean tree, professional README, CHANGELOG, LICENSE" 2>&1 | Out-Null
    git push origin main 2>&1 | Out-Null
    Pop-Location

    Write-Host "  Pushed clean tree" -ForegroundColor Green
}

Write-Host ""
Write-Host "Done. All 6 repos refreshed." -ForegroundColor Yellow
