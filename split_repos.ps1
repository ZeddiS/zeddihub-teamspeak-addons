$ErrorActionPreference = "Continue"
$env:GIT_REDIRECT_STDERR = "2>&1"

$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$release   = Join-Path $root "release"
$workRoot  = Join-Path $env:TEMP "zh_repo_split"
$ghOrg     = "ZeddiS"
$version   = "v1.1.0"

if (Test-Path $workRoot) { Remove-Item -Recurse -Force $workRoot }
New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

$plugins = @(
    @{ src="pokebot";       repo="ts3-pokebot";       title="Poke Bot";       blurb="TS3 plugin: preset/custom poke campaigns with Burst+Schedule modes, Qt dark theme, MAX SPAM hard-cap." }
    @{ src="Follow";        repo="ts3-follow";        title="Follow";         blurb="TS3 plugin: single-target auto-follow that moves you with another client. Auto-stops on permission errors." }
    @{ src="MoveSpam";      repo="ts3-movespam";      title="MoveSpam";       blurb="TS3 plugin: alternate target between two channels in a loop. Basic + Custom Qt dialog." }
    @{ src="VoiceChanger";  repo="ts3-voicechanger";  title="Voice Changer";  blurb="TS3 plugin: real-time DSP for outgoing mic. Helium, Demon, Robot, Echo, Custom pitch slider." }
    @{ src="AutoReconnect"; repo="ts3-autoreconnect"; title="AutoReconnect";  blurb="TS3 plugin: auto-reconnect to server after unexpected disconnects with exponential backoff." }
    @{ src="GreetingBot";   repo="ts3-greetingbot";   title="Greeting Bot";   blurb="TS3 plugin: auto-pokes users when they enter your channel with a configurable greeting." }
)

$brandHeader = Get-Content (Join-Path $root "common\zh_brand.h") -Raw

foreach ($p in $plugins) {
    $work = Join-Path $workRoot $p.repo
    Write-Host ""
    Write-Host "=== Preparing $($p.repo) ===" -ForegroundColor Cyan

    $srcPath = Join-Path $root $p.src
    Copy-Item -Recurse $srcPath $work
    Get-ChildItem $work -Directory -Filter "build*" -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    Get-ChildItem $work -Directory -Filter "debug*" -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    $brandLocal = Join-Path $work "src\zh_brand.h"
    $brandHeader | Out-File -FilePath $brandLocal -Encoding UTF8 -NoNewline
    $cpp = Join-Path $work "src\plugin.cpp"
    if (Test-Path $cpp) {
        (Get-Content $cpp -Raw) -replace '#include "\.\./\.\./common/zh_brand\.h"', '#include "zh_brand.h"' |
            Set-Content $cpp -NoNewline -Encoding UTF8
    }

    $gitignore = "build/" + "`r`n" + "build_api*/" + "`r`n" + "debug_build/" + "`r`n" + "out/" + "`r`n" +
                 "*.dll" + "`r`n" + "*.pdb" + "`r`n" + "*.ilk" + "`r`n" + "*.exp" + "`r`n" + "*.lib" + "`r`n" +
                 "*.obj" + "`r`n" + "CMakeCache.txt" + "`r`n" + "CMakeFiles/" + "`r`n" + "cmake_install.cmake" + "`r`n" +
                 ".vs/" + "`r`n" + "*.vcxproj.user" + "`r`n" + "*.vcxproj.filters" + "`r`n" +
                 "moc_*.cpp" + "`r`n" + "*_automoc.cpp" + "`r`n" + "ts3sdk/include/" + "`r`n" + "dist/" + "`r`n" + "release/" + "`r`n"
    $gitignore | Out-File -FilePath (Join-Path $work ".gitignore") -Encoding UTF8

    # Per-repo README built line-by-line (no @"..."@ here-string - encoding issues)
    $readmeLines = @(
        "# $($p.title)",
        "",
        "$($p.blurb)",
        "",
        "Part of the [ZeddiHub TeamSpeak Addons](https://github.com/$ghOrg/zeddihub-teamspeak-addons) collection.",
        "Built with C++17 + Qt 5.12.12. Multi-API support (23 / 24 / 25 / 26).",
        "",
        "## Stazeni",
        "",
        "Vyber zip podle sve TS3 verze v [Releases](../../releases):",
        "",
        "| TS3 client | API | Stahni |",
        "|---|---|---|",
        "| 3.5.0 | 23 | ``$($p.src)-$version-TS3-3.5.0-api23.zip`` |",
        "| 3.5.1 - 3.5.5 | 24 | ``$($p.src)-$version-TS3-3.5.1-3.5.5-api24.zip`` |",
        "| **3.5.6** | 25 | ``$($p.src)-$version-TS3-3.5.6-api25.zip`` |",
        "| 3.6.x+ | 26 | ``$($p.src)-$version-TS3-3.6+-api26.zip`` |",
        "",
        "## Instalace",
        "",
        "1. Rozbal zip",
        "2. Zkopiruj DLL do ``%APPDATA%\TS3Client\plugins\``",
        "3. V TS3 -> Settings -> Plugins -> Reload All -> zaskrtni Enabled",
        "",
        "## Build ze zdrojaku",
        "",
        '```powershell',
        "# Setup (once)",
        "py -m pip install aqtinstall",
        "py -m aqt install-qt windows desktop 5.12.12 win64_msvc2017_64 -O C:\Qt --archives qtbase",
        "git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone",
        "Move-Item ts3sdk_clone/include ts3sdk/include",
        "",
        "# Build",
        "cmake -S . -B build_api25 -G `"Visual Studio 17 2022`" -A x64 ``",
        "      -DCMAKE_PREFIX_PATH=`"C:\Qt\5.12.12\msvc2017_64`"",
        "cmake --build build_api25 --config Release",
        '```',
        "",
        "## License",
        "",
        "MIT.",
        "",
        "---",
        "",
        "zeddis.xyz | (C) 2026 ZeddiHub.eu | https://zeddihub.eu"
    )
    ($readmeLines -join "`r`n") | Out-File -FilePath (Join-Path $work "README.md") -Encoding UTF8

    Write-Host "  Prepared $work" -ForegroundColor Green
}

# Push + release
foreach ($p in $plugins) {
    $work = Join-Path $workRoot $p.repo
    Push-Location $work
    try {
        Write-Host ""
        Write-Host "=== Pushing $($p.repo) ===" -ForegroundColor Cyan
        git init -b main 2>&1 | Out-Null
        git add . 2>&1 | Out-Null
        git -c user.email="bot@zeddihub.eu" -c user.name="ZeddiHub Bot" commit -m "Initial release: $($p.title) $version" 2>&1 | Out-Null

        gh repo create "$ghOrg/$($p.repo)" --public --source=. --push --description=$p.blurb 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            git remote remove origin 2>&1 | Out-Null
            git remote add origin "https://github.com/$ghOrg/$($p.repo).git" 2>&1 | Out-Null
            git push -u origin main 2>&1 | Out-Null
        }

        $pluginZips = Get-ChildItem $release -Filter "$($p.src)-*.zip"
        if ($pluginZips.Count -gt 0) {
            $notesLines = @(
                "First release of $($p.title).",
                "",
                "$($p.blurb)",
                "",
                "## Stazeni podle TS3 verze",
                "",
                "| TS3 client | API | File |",
                "|---|---|---|",
                "| 3.5.0 | 23 | $($p.src)-$version-TS3-3.5.0-api23.zip |",
                "| 3.5.1 - 3.5.5 | 24 | $($p.src)-$version-TS3-3.5.1-3.5.5-api24.zip |",
                "| 3.5.6 | 25 | $($p.src)-$version-TS3-3.5.6-api25.zip |",
                "| 3.6.x+ | 26 | $($p.src)-$version-TS3-3.6+-api26.zip |",
                "",
                "Plugin built against Qt 5.12.12 (matches TS3 3.5.6 ABI for proper loading).",
                "",
                "zeddis.xyz | (C) 2026 ZeddiHub.eu"
            )
            $notes = $notesLines -join "`r`n"
            $tmpNotes = Join-Path $env:TEMP "zh_notes_$($p.repo).txt"
            $notes | Out-File -FilePath $tmpNotes -Encoding UTF8
            $zipPaths = $pluginZips | ForEach-Object { $_.FullName }
            gh release create $version --repo "$ghOrg/$($p.repo)" --title "$($p.title) $version" --notes-file $tmpNotes @zipPaths 2>&1 | Out-Null
            Write-Host "  Released $version with $($pluginZips.Count) zips" -ForegroundColor Green
        } else {
            Write-Host "  No zips found for $($p.src)" -ForegroundColor Yellow
        }
    } finally {
        Pop-Location
    }
}

Write-Host ""
Write-Host "All repos split & released." -ForegroundColor Yellow
