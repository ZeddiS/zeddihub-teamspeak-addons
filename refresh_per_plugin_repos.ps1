# Strip per-plugin repos to README + LICENSE + CHANGELOG only.
# Reflects 4-plugin collection: PokeBot, Follow, MoveSpam, Soundboard.

$ErrorActionPreference = "Continue"

$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$workRoot  = Join-Path $env:TEMP "zh_repo_strip"
$ghOrg     = "ZeddiS"

if (Test-Path $workRoot) { Remove-Item -Recurse -Force $workRoot }
New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

$plugins = @(
    @{ src="pokebot";    repo="zeddihub-teamspeak-pokebot";    title="Poke Bot";    version="1.3.0"; clogKey="pokebot";    dllBase="zeddihub_pokebot" }
    @{ src="Follow";     repo="zeddihub-teamspeak-follow";     title="Follow";      version="1.3.0"; clogKey="follow";     dllBase="follow"           }
    @{ src="MoveSpam";   repo="zeddihub-teamspeak-movespam";   title="MoveSpam";    version="1.3.0"; clogKey="movespam";   dllBase="movespam"         }
    @{ src="Soundboard"; repo="zeddihub-teamspeak-soundboard"; title="SoundBoard";  version="1.1.0"; clogKey="soundboard"; dllBase="soundboard"       }
)

$desc = @{
    "pokebot" = @{ short="Right-click any TeamSpeak 3 client to launch a poke campaign.";
                   long="Preset bursts (CZ, Symbol Storm, Silent, MAX) plus a custom dialog with Burst/Schedule mode toggle, paired sliders for count and intervals. Hard-cap of 500 pokes and 50ms anti-flood floor protect against server kicks. Native TS3 theme." }
    "follow" = @{ short="Auto-follow another TeamSpeak 3 client into channels.";
                  long="A polling worker checks the target's current channel every 1 second and moves you to match. The plugin NEVER auto-stops on errors -- only manual STOP or target unreachable for 10 seconds ends the follow." }
    "movespam" = @{ short="Repeatedly move a TeamSpeak 3 client between two channels.";
                    long="Basic mode alternates the target between their current channel and the server's default channel. Custom mode opens a dialog where you specify a destination channel (by name or ID), interval, and max move count." }
    "soundboard" = @{ short="Colored tile soundboard for TeamSpeak 3. Click tiles to play .wav into your mic stream.";
                      long="Open SoundBoard from Plugins menu. Add tiles, browse for .wav files, customize each tile's name and color. Right-click a tile for options. Each tile has a hotkey (bind in TS3 Settings -> Hotkeys). When triggered, sound is mixed into your outgoing mic stream so others in the channel hear it. Supports 16/24/32-bit PCM and 32-bit float WAV. Multiple sounds mix simultaneously." }
}

$changelogs = @{
    "pokebot" = @(
        "## [1.3.0] - 2026-05-01",
        "### Changed",
        "- Presets simplified: CZ, Symbol Storm, Silent, MAX, Custom, STOP",
        "- Removed Wake-up CZ (renamed to CZ) and Halt (merged into CZ phrases)",
        "- Removed custom stylesheet -- dialogs inherit TS3 client's native theme",
        "- TS3 native install via .ts3_plugin packages (replaces .exe installer)",
        "",
        "## [1.2.x]",
        "- See git history",
        "",
        "## [1.0.0] - 2026-04-29",
        "- Initial release"
    )
    "follow" = @(
        "## [1.3.0] - 2026-05-01",
        "### Changed",
        "- TS3 native install via .ts3_plugin packages",
        "- Inherits TS3 client's native theme",
        "",
        "## [1.2.0] - 2026-04-30",
        "- Polling worker (1s) -- never auto-stops",
        "",
        "## [1.0.0] - 2026-04-30",
        "- Initial release"
    )
    "movespam" = @(
        "## [1.3.0] - 2026-05-01",
        "### Changed",
        "- TS3 native install via .ts3_plugin packages",
        "- Removed custom dialog stylesheet -- inherits TS3 native theme",
        "",
        "## [1.0.0] - 2026-04-30",
        "- Initial release"
    )
    "soundboard" = @(
        "## [1.1.0] - 2026-05-01",
        "### Changed",
        "- Plugin renamed: 'ZeddiHub Soundboard' -> 'SoundBoard'",
        "- Dialog UI redesigned as colored tile grid (click to play, right-click for options)",
        "- Each tile now has a CUSTOMIZABLE COLOR (right-click -> Change color, opens QColorDialog)",
        "- Diagnostic log entries added (TS3 Tools -> Client log shows when sounds are queued + when audio callback fires)",
        "### Fixed",
        "- WAV decoder now supports 24-bit PCM, 32-bit PCM, 32-bit float (was only 16/8-bit)",
        "- Mix function fixed for stereo capture (was advancing position 2x too fast on stereo mics)",
        "",
        "## [1.0.0] - 2026-05-01",
        "### Added",
        "- Initial release",
        "- Plugins menu -> Open Soundboard opens slot grid",
        "- Add unlimited slots (up to 32 -- one per pre-registered hotkey)",
        "- Browse and assign .wav files to slots, set per-slot volume",
        "- 32 hotkeys pre-registered (soundboard_play_1 .. soundboard_play_32)",
        "- Audio mixed into mic stream via ts3plugin_onEditCapturedVoiceDataEvent"
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

    # Try clone existing repo (Soundboard repo will fail first time and we init fresh)
    git clone "https://github.com/$ghOrg/$($p.repo).git" $work 2>&1 | Out-Null
    if (-not (Test-Path "$work\.git")) {
        Write-Host "  Repo missing, initializing fresh tree" -ForegroundColor Yellow
        New-Item -ItemType Directory -Force -Path $work | Out-Null
        Push-Location $work
        git init -b main 2>&1 | Out-Null
        Pop-Location
    }

    # Wipe everything except .git
    Get-ChildItem $work -Force | Where-Object { $_.Name -ne ".git" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    $mitLicense | Out-File "$work\LICENSE" -Encoding UTF8

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
    [void]$readme.Add("### Option A: TS3 native install (.ts3_plugin) -- recommended")
    [void]$readme.Add("")
    [void]$readme.Add("1. Download the .ts3_plugin file matching your TeamSpeak 3 version:")
    [void]$readme.Add("")
    [void]$readme.Add("| TS3 client | API | File |")
    [void]$readme.Add("|---|---|---|")
    [void]$readme.Add("| 3.5.0 | 23 | ``$($p.src)-v$($p.version)-TS3-3.5.0-api23.ts3_plugin`` |")
    [void]$readme.Add("| 3.5.1 - 3.5.5 | 24 | ``$($p.src)-v$($p.version)-TS3-3.5.1-3.5.5-api24.ts3_plugin`` |")
    [void]$readme.Add("| **3.5.6** | **25** | **``$($p.src)-v$($p.version)-TS3-3.5.6-api25.ts3_plugin``** |")
    [void]$readme.Add("| 3.6.x and newer | 26 | ``$($p.src)-v$($p.version)-TS3-3.6+-api26.ts3_plugin`` |")
    [void]$readme.Add("")
    [void]$readme.Add("2. Double-click the downloaded file. TS3 client opens an install dialog.")
    [void]$readme.Add("3. Click **Yes** to install. TS3 copies the DLL into the plugins folder.")
    [void]$readme.Add("4. Restart TS3 if requested. Enable the plugin in **Settings -> Plugins**.")
    [void]$readme.Add("")
    [void]$readme.Add("### Option B: Manual (.dll)")
    [void]$readme.Add("")
    [void]$readme.Add("Download the raw DLL matching your TS3 client version:")
    [void]$readme.Add("")
    [void]$readme.Add("- ``$($p.dllBase)_api23_win64.dll`` -- TS3 3.5.0")
    [void]$readme.Add("- ``$($p.dllBase)_api24_win64.dll`` -- TS3 3.5.1 - 3.5.5")
    [void]$readme.Add("- ``$($p.dllBase)_api25_win64.dll`` -- TS3 3.5.6")
    [void]$readme.Add("- ``$($p.dllBase)_api26_win64.dll`` -- TS3 3.6.x and newer")
    [void]$readme.Add("")
    [void]$readme.Add("Copy the DLL to ``%APPDATA%\TS3Client\plugins\``, then in TS3 go to **Settings -> Plugins -> Reload All -> tick Enabled**.")
    [void]$readme.Add("")
    [void]$readme.Add("If TS3 reports 'API version not compatible', you have the wrong file -- try a different API number.")
    [void]$readme.Add("")
    [void]$readme.Add("## Theme")
    [void]$readme.Add("")
    [void]$readme.Add("Plugin dialogs use **TS3 client's native theme**. They look exactly like the rest of the TS3 UI -- no custom Discord-like dark theme. Whatever skin/theme you have set in TS3 will be applied.")
    [void]$readme.Add("")
    [void]$readme.Add("## Source code")
    [void]$readme.Add("")
    [void]$readme.Add("Source code lives in the [collection repo](https://github.com/$ghOrg/zeddihub-teamspeak-addons), folder ``$($p.src)/``. CMake build instructions are there.")
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
    [void]$readme.Add("- **ZeddiHub web**: https://zeddihub.eu")
    [void]$readme.Add("- **ZeddiHub Tools**: https://zeddihub.eu/tools/")
    [void]$readme.Add("- **Author**: https://zeddis.xyz")
    [void]$readme.Add("- **Collection**: [zeddihub-teamspeak-addons](https://github.com/$ghOrg/zeddihub-teamspeak-addons)")
    [void]$readme.Add("")
    [void]$readme.Add("**Sister plugins:** [Poke Bot](https://github.com/$ghOrg/zeddihub-teamspeak-pokebot) | [Follow](https://github.com/$ghOrg/zeddihub-teamspeak-follow) | [MoveSpam](https://github.com/$ghOrg/zeddihub-teamspeak-movespam) | [Soundboard](https://github.com/$ghOrg/zeddihub-teamspeak-soundboard)")
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
    git -c user.email="bot@zeddihub.eu" -c user.name="ZeddiHub Bot" commit -m "$($p.version): English README, .ts3_plugin install, native TS3 theme" 2>&1 | Out-Null

    # First push for new repo
    $existing = git remote -v 2>&1
    if (-not ($existing -match "origin")) {
        # New repo - create on GH
        gh repo create "$ghOrg/$($p.repo)" --public --source=. --push --description="$($d.short)" 2>&1 | Out-Null
    } else {
        git push origin main 2>&1 | Out-Null
    }
    Pop-Location

    Write-Host "  Pushed minimal tree" -ForegroundColor Green
}

Write-Host ""
Write-Host "Done." -ForegroundColor Yellow
