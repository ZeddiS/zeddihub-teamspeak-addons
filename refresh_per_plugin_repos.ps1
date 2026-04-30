# Strip each per-plugin repo to MINIMUM content:
#   - README.md (English)
#   - LICENSE (MIT)
#   - CHANGELOG.md (Keep a Changelog)
# No source code, no CMakeLists, no build files. Source is in collection repo only.

$ErrorActionPreference = "Continue"

$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$workRoot  = Join-Path $env:TEMP "zh_repo_strip"
$ghOrg     = "ZeddiS"

if (Test-Path $workRoot) { Remove-Item -Recurse -Force $workRoot }
New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

$plugins = @(
    @{ src="pokebot";       repo="zeddihub-teamspeak-pokebot";       title="Poke Bot";       version="1.2.1"; clogKey="pokebot";       dllBase="zeddihub_pokebot" }
    @{ src="Follow";        repo="zeddihub-teamspeak-follow";        title="Follow";         version="1.2.1"; clogKey="follow";        dllBase="follow"           }
    @{ src="MoveSpam";      repo="zeddihub-teamspeak-movespam";      title="MoveSpam";       version="1.2.0"; clogKey="movespam";      dllBase="movespam"         }
    @{ src="VoiceChanger";  repo="zeddihub-teamspeak-voicechanger";  title="Voice Changer";  version="1.2.4"; clogKey="voicechanger";  dllBase="voicechanger"     }
    @{ src="AutoReconnect"; repo="zeddihub-teamspeak-autoreconnect"; title="AutoReconnect";  version="1.0.1"; clogKey="autoreconnect"; dllBase="autoreconnect"    }
    @{ src="GreetingBot";   repo="zeddihub-teamspeak-greetingbot";   title="Greeting Bot";   version="1.0.1"; clogKey="greetingbot";   dllBase="greetingbot"      }
)

# Description per plugin (English)
$desc = @{
    "pokebot" = @{ short = "Right-click any TeamSpeak 3 client to launch a poke campaign.";
                   long = "Preset bursts (Wake-up, Halt, Symbol Storm, Silent, MAX SPAM) plus a custom Qt dialog with Burst/Schedule mode toggle, paired sliders for count and intervals, and a TS3 dark theme. Hard-cap of 500 pokes and 50ms anti-flood floor protect against server kicks."; }
    "follow" = @{ short = "Auto-follow another TeamSpeak 3 client into channels.";
                  long = "A polling worker thread checks the target's current channel every 1 second and moves you to match. The plugin NEVER auto-stops on errors -- only manual STOP or target unreachable for 10 seconds ends the follow. Robust against missed events and 'User is already in this channel' server responses."; }
    "movespam" = @{ short = "Repeatedly move a TeamSpeak 3 client between two channels.";
                    long = "Basic mode alternates the target between their current channel and the server's default channel. Custom mode opens a Qt dialog where you specify a destination channel (by name or ID), interval, and max move count."; }
    "voicechanger" = @{ short = "Real-time DSP effects on outgoing microphone audio.";
                        long = "Helium / Chipmunk / Demon / Deep / Custom pitch (phase-vocoder), Robot (AM modulation), Echo (delay), Distortion (raspy/guttural), Whisper (breathy), Telephone (300-3400 Hz bandpass), Underwater (muffled + reverb), Megaphone (PA-system distortion + bandpass), Volume Boost (sanity test). Each preset has a one-click menu item under Plugins -> Voice Changer."; }
    "autoreconnect" = @{ short = "Auto-reconnect to TeamSpeak 3 server after unexpected disconnects.";
                         long = "Captures connection parameters (host, port, nickname, identity) when you connect. On unexpected disconnect (network drop, server kick aside), automatically reconnects with exponential backoff (2s -> 4s -> 8s -> ... up to 64x base)."; }
    "greetingbot" = @{ short = "Auto-poke users entering your TeamSpeak 3 channel with a custom greeting.";
                       long = "Configurable greeting message (use {name} placeholder for personalization). Triggers when any client moves into your current channel. Toggle via Plugins -> Greeting Bot menu, or chat command /greet on|off|set <text>."; }
}

$changelogs = @{
    "pokebot" = @(
        "## [1.2.1] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- About URL points to dedicated zeddihub-teamspeak-pokebot repo",
        "",
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
        "- Initial release with 4 presets + Custom dialog"
    )
    "follow" = @(
        "## [1.2.1] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- About URL points to dedicated zeddihub-teamspeak-follow repo",
        "",
        "## [1.2.0] - 2026-04-30",
        "### Changed",
        "- Rewrite to polling worker (1s interval). Never auto-stops on errors -- only manual STOP or target gone for 10s+",
        "",
        "## [1.1.0] - 2026-04-30",
        "### Added",
        "- Plugins top-bar menu (Stop / Status / About)",
        "- Author: zeddis.xyz, Copyright (C) 2026 ZeddiHub.eu",
        "- Multi-API support: 23 / 24 / 25 / 26",
        "### Fixed",
        "- ERROR_channel_already_in is now treated as success",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: single-target follow with auto-stop on permission errors"
    )
    "movespam" = @(
        "## [1.2.0] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- About URL points to dedicated zeddihub-teamspeak-movespam repo",
        "",
        "## [1.1.0] - 2026-04-30",
        "### Added",
        "- Plugins top-bar menu (Stop / Status / About)",
        "- Author: zeddis.xyz, Copyright (C) 2026 ZeddiHub.eu",
        "- Multi-API support: 23 / 24 / 25 / 26",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: Basic and Custom Qt dialog modes"
    )
    "voicechanger" = @(
        "## [1.2.4] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- About URL points to dedicated zeddihub-teamspeak-voicechanger repo",
        "",
        "## [1.2.3] - 2026-04-30",
        "### Added",
        "- Per-preset menu items in Plugins top bar (one click = pick preset + enable)",
        "- Compat Mode toggle for diagnosing VAD blocking",
        "",
        "## [1.2.2] - 2026-04-30",
        "### Fixed",
        "- Pre-allocate scratch buffers in constructor (avoid heap allocation on audio thread)",
        "",
        "## [1.2.1] - 2026-04-30",
        "### Added",
        "- VolumeBoost sanity-test preset",
        "### Fixed",
        "- NaN/Inf protection",
        "- Robot uses AM instead of full ring mod",
        "- Pitch shifter wraps gSumPhase to [-PI, PI]",
        "",
        "## [1.2.0] - 2026-04-30",
        "### Added",
        "- 5 new effects: Distortion, Whisper, Telephone, Underwater, Megaphone",
        "### Fixed",
        "- Off preset now guaranteed to never set edited=1",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: Helium / Chipmunk / Demon / Deep / Robot / Echo / Custom + phase-vocoder pitch shifter"
    )
    "autoreconnect" = @(
        "## [1.0.1] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- About URL points to dedicated zeddihub-teamspeak-autoreconnect repo",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: auto-reconnect with exponential backoff",
        "- Captures connection params on connect, replays on reconnect",
        "- Plugins top-bar menu: Toggle / Status / About"
    )
    "greetingbot" = @(
        "## [1.0.1] - 2026-04-30",
        "### Changed",
        "- All UI text translated to English",
        "- Default greeting changed to 'Welcome {name}! :)'",
        "- About URL points to dedicated zeddihub-teamspeak-greetingbot repo",
        "",
        "## [1.0.0] - 2026-04-30",
        "### Added",
        "- Initial release: auto-poke on channel join",
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
    Write-Host "=== Stripping $($p.repo) ===" -ForegroundColor Cyan
    $work = Join-Path $workRoot $p.repo

    git clone "https://github.com/$ghOrg/$($p.repo).git" $work 2>&1 | Out-Null
    if (-not (Test-Path "$work\.git")) {
        Write-Host "  Clone failed, skipping" -ForegroundColor Red
        continue
    }

    # Wipe everything except .git
    Get-ChildItem $work -Force | Where-Object { $_.Name -ne ".git" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    # Write LICENSE
    $mitLicense | Out-File "$work\LICENSE" -Encoding UTF8

    # Write README (English, comprehensive)
    $d = $desc[$p.clogKey]
    $readme = New-Object System.Collections.ArrayList
    [void]$readme.Add("# $($p.title)")
    [void]$readme.Add("")
    [void]$readme.Add("[![Latest Release](https://img.shields.io/github/v/release/$ghOrg/$($p.repo))](https://github.com/$ghOrg/$($p.repo)/releases)")
    [void]$readme.Add("[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)")
    [void]$readme.Add("")
    [void]$readme.Add($d.short)
    [void]$readme.Add("")
    [void]$readme.Add($d.long)
    [void]$readme.Add("")
    [void]$readme.Add("Part of the [**ZeddiHub TeamSpeak Addons**](https://github.com/$ghOrg/zeddihub-teamspeak-addons) collection.")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("## Installation")
    [void]$readme.Add("")
    [void]$readme.Add("All download files are in **[Releases](https://github.com/$ghOrg/$($p.repo)/releases/latest)**.")
    [void]$readme.Add("")
    [void]$readme.Add("### Option A: Installer (.exe) -- recommended")
    [void]$readme.Add("")
    [void]$readme.Add("Download and run **``$($p.src)-Setup-v$($p.version).exe``**")
    [void]$readme.Add("")
    [void]$readme.Add("The wizard:")
    [void]$readme.Add("1. Detects your TeamSpeak 3 version and selects the correct API DLL (23 / 24 / 25 / 26)")
    [void]$readme.Add("2. Detects running TS3 and offers to close it (DLL would otherwise be locked)")
    [void]$readme.Add("3. Installs the DLL to ``%APPDATA%\TS3Client\plugins\`` (per-user, no admin needed)")
    [void]$readme.Add("4. Registers an uninstaller in Add/Remove Programs")
    [void]$readme.Add("")
    [void]$readme.Add("### Option B: Manual (.dll)")
    [void]$readme.Add("")
    [void]$readme.Add("Download the raw DLL matching your TS3 client version:")
    [void]$readme.Add("")
    [void]$readme.Add("| TS3 client | Plugin API | File |")
    [void]$readme.Add("|---|---|---|")
    [void]$readme.Add("| 3.5.0 | 23 | ``$($p.dllBase)_api23_win64.dll`` |")
    [void]$readme.Add("| 3.5.1 - 3.5.5 | 24 | ``$($p.dllBase)_api24_win64.dll`` |")
    [void]$readme.Add("| **3.5.6** | **25** | **``$($p.dllBase)_api25_win64.dll``** |")
    [void]$readme.Add("| 3.6.x and newer | 26 | ``$($p.dllBase)_api26_win64.dll`` |")
    [void]$readme.Add("")
    [void]$readme.Add("Copy the DLL to ``%APPDATA%\TS3Client\plugins\``, then in TS3 go to **Settings -> Plugins -> Reload All -> tick Enabled**.")
    [void]$readme.Add("")
    [void]$readme.Add("If TS3 reports 'API version not compatible', you have the wrong file -- try a different API number.")
    [void]$readme.Add("")
    [void]$readme.Add("## Source code")
    [void]$readme.Add("")
    [void]$readme.Add("Source code lives in the [**collection repo**](https://github.com/$ghOrg/zeddihub-teamspeak-addons), folder ``$($p.src)/``. Build instructions and CMake configuration are there.")
    [void]$readme.Add("")
    [void]$readme.Add("## Changelog")
    [void]$readme.Add("")
    [void]$readme.Add("See [CHANGELOG.md](CHANGELOG.md).")
    [void]$readme.Add("")
    [void]$readme.Add("## License")
    [void]$readme.Add("")
    [void]$readme.Add("MIT -- see [LICENSE](LICENSE).")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("## Links")
    [void]$readme.Add("")
    [void]$readme.Add("- :house: **ZeddiHub web**: https://zeddihub.eu")
    [void]$readme.Add("- :wrench: **ZeddiHub Tools**: https://zeddihub.eu/tools/")
    [void]$readme.Add("- :busts_in_silhouette: **Author**: https://zeddis.xyz")
    [void]$readme.Add("- :file_folder: **Collection**: [zeddihub-teamspeak-addons](https://github.com/$ghOrg/zeddihub-teamspeak-addons)")
    [void]$readme.Add("")
    [void]$readme.Add("**Sister plugins:** [Poke Bot](https://github.com/$ghOrg/zeddihub-teamspeak-pokebot) | [Follow](https://github.com/$ghOrg/zeddihub-teamspeak-follow) | [MoveSpam](https://github.com/$ghOrg/zeddihub-teamspeak-movespam) | [Voice Changer](https://github.com/$ghOrg/zeddihub-teamspeak-voicechanger) | [AutoReconnect](https://github.com/$ghOrg/zeddihub-teamspeak-autoreconnect) | [Greeting Bot](https://github.com/$ghOrg/zeddihub-teamspeak-greetingbot)")
    [void]$readme.Add("")
    [void]$readme.Add("---")
    [void]$readme.Add("")
    [void]$readme.Add("(C) 2026 [ZeddiHub.eu](https://zeddihub.eu) -- zeddis.xyz")
    ($readme -join "`r`n") | Out-File "$work\README.md" -Encoding UTF8

    # CHANGELOG
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

    # Commit + push
    Push-Location $work
    git add -A 2>&1 | Out-Null
    git -c user.email="bot@zeddihub.eu" -c user.name="ZeddiHub Bot" commit -m "Strip to README + LICENSE + CHANGELOG only (English)" 2>&1 | Out-Null
    git push origin main 2>&1 | Out-Null
    Pop-Location

    Write-Host "  Pushed minimal tree" -ForegroundColor Green
}

Write-Host ""
Write-Host "Done." -ForegroundColor Yellow
