# Build all 6 plugins x 4 API versions = 24 DLLs.
# Built against Qt 5.12.12 (matches TS3 3.5.6 ABI; forward-compat with 5.15+).

$ErrorActionPreference = "Stop"

$cmake     = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$qtPrefix  = "C:\Qt\5.12.12\msvc2017_64"
$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }

$plugins = @(
    @{ dir = "pokebot";       var = "POKEBOT_API_VERSION";       base = "zeddihub_pokebot" }
    @{ dir = "Follow";        var = "FOLLOW_API_VERSION";        base = "follow"           }
    @{ dir = "MoveSpam";      var = "MOVESPAM_API_VERSION";      base = "movespam"         }
    @{ dir = "VoiceChanger";  var = "VC_API_VERSION";            base = "voicechanger"     }
    @{ dir = "AutoReconnect"; var = "AUTORECONNECT_API_VERSION"; base = "autoreconnect"    }
    @{ dir = "GreetingBot";   var = "GREETINGBOT_API_VERSION";   base = "greetingbot"      }
)
$apis = @(23, 24, 25, 26)

$dist = Join-Path $root "dist"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$results = @()

foreach ($p in $plugins) {
    $pluginDir = Join-Path $root $p.dir
    foreach ($api in $apis) {
        $buildDir = Join-Path $pluginDir "build_api$api"
        $tag = "$($p.dir) api$api"
        Write-Host "`n=== Building $tag ===" -ForegroundColor Cyan

        if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }

        & $cmake -S $pluginDir -B $buildDir -G "Visual Studio 17 2022" -A x64 `
                 "-DCMAKE_PREFIX_PATH=$qtPrefix" `
                 "-D$($p.var)=$api" 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  CONFIGURE FAILED: $tag" -ForegroundColor Red
            $results += @{ tag=$tag; ok=$false; phase="configure" }
            continue
        }

        & $cmake --build $buildDir --config Release 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  BUILD FAILED: $tag" -ForegroundColor Red
            $results += @{ tag=$tag; ok=$false; phase="build" }
            continue
        }

        $dll = Join-Path $buildDir "Release\$($p.base)_api${api}_win64.dll"
        if (-not (Test-Path $dll)) {
            Write-Host "  DLL MISSING: expected $dll" -ForegroundColor Red
            $results += @{ tag=$tag; ok=$false; phase="missing-dll" }
            continue
        }

        $outDir = Join-Path $dist "$($p.dir)\api$api"
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
        Copy-Item $dll $outDir -Force

        $sz = (Get-Item $dll).Length
        Write-Host "  OK ($sz B) -> $outDir" -ForegroundColor Green
        $results += @{ tag=$tag; ok=$true; phase="ok"; size=$sz }
    }
}

Write-Host "`n=== Summary ===" -ForegroundColor Yellow
$ok = 0; $fail = 0
foreach ($r in $results) {
    $color = if ($r.ok) { "Green" } else { "Red" }
    $status = if ($r.ok) { "OK" } else { "FAIL ($($r.phase))" }
    Write-Host ("  {0,-30} {1}" -f $r.tag, $status) -ForegroundColor $color
    if ($r.ok) { $ok++ } else { $fail++ }
}
Write-Host "`n$ok / $($results.Count) builds succeeded." -ForegroundColor $(if ($fail -eq 0) {"Green"} else {"Yellow"})
