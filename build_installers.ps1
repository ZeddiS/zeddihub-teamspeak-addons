# Build per-plugin Windows installer (.exe) using Inno Setup.
# Output: installer/output/<slug>-Setup-v<version>.exe
# Run AFTER build_all.ps1 — needs dist/<plugin>/api*/*.dll to exist.

$ErrorActionPreference = "Stop"

$root      = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$dist      = Join-Path $root "dist"
$installer = Join-Path $root "installer"
$output    = Join-Path $installer "output"
$template  = Join-Path $installer "template.iss"
$iscc      = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"

if (-not (Test-Path $iscc)) {
    Write-Host "ERROR: Inno Setup not found at $iscc" -ForegroundColor Red
    Write-Host "Install via: winget install JRSoftware.InnoSetup" -ForegroundColor Yellow
    exit 1
}
if (-not (Test-Path $template)) {
    Write-Host "ERROR: Template missing: $template" -ForegroundColor Red
    exit 1
}

if (Test-Path $output) { Remove-Item -Recurse -Force $output }
New-Item -ItemType Directory -Force -Path $output | Out-Null

# plugin folder -> (title, slug, version, dll-base, repo-url)
$plugins = @(
    @{ src="pokebot";       slug="pokebot";       title="Poke Bot";       version="1.2.1"; dllBase="zeddihub_pokebot"; repo="https://github.com/ZeddiS/zeddihub-teamspeak-pokebot" }
    @{ src="Follow";        slug="follow";        title="Follow";         version="1.2.1"; dllBase="follow";           repo="https://github.com/ZeddiS/zeddihub-teamspeak-follow" }
    @{ src="MoveSpam";      slug="movespam";      title="MoveSpam";       version="1.2.0"; dllBase="movespam";         repo="https://github.com/ZeddiS/zeddihub-teamspeak-movespam" }
    @{ src="VoiceChanger";  slug="voicechanger";  title="Voice Changer";  version="1.2.4"; dllBase="voicechanger";     repo="https://github.com/ZeddiS/zeddihub-teamspeak-voicechanger" }
    @{ src="AutoReconnect"; slug="autoreconnect"; title="AutoReconnect";  version="1.0.1"; dllBase="autoreconnect";    repo="https://github.com/ZeddiS/zeddihub-teamspeak-autoreconnect" }
    @{ src="GreetingBot";   slug="greetingbot";   title="Greeting Bot";   version="1.0.1"; dllBase="greetingbot";      repo="https://github.com/ZeddiS/zeddihub-teamspeak-greetingbot" }
)

$tmpl = Get-Content $template -Raw

foreach ($p in $plugins) {
    Write-Host "`n=== Building installer: $($p.title) v$($p.version) ===" -ForegroundColor Cyan

    $issDir = Join-Path $installer "build_$($p.slug)"
    if (Test-Path $issDir) { Remove-Item -Recurse -Force $issDir }
    New-Item -ItemType Directory -Force -Path $issDir | Out-Null

    # Substitute tokens — use absolute paths to dist subdirs
    $body = $tmpl `
        -replace '\{\{PLUGIN_TITLE\}\}',    $p.title `
        -replace '\{\{PLUGIN_SLUG\}\}',     $p.slug `
        -replace '\{\{PLUGIN_VERSION\}\}',  $p.version `
        -replace '\{\{DLL_BASE\}\}',        $p.dllBase `
        -replace '\{\{DLL_DIR_API23\}\}',   ([regex]::Escape((Join-Path $dist "$($p.src)\api23"))) `
        -replace '\{\{DLL_DIR_API24\}\}',   ([regex]::Escape((Join-Path $dist "$($p.src)\api24"))) `
        -replace '\{\{DLL_DIR_API25\}\}',   ([regex]::Escape((Join-Path $dist "$($p.src)\api25"))) `
        -replace '\{\{DLL_DIR_API26\}\}',   ([regex]::Escape((Join-Path $dist "$($p.src)\api26"))) `
        -replace '\{\{REPO_URL\}\}',        $p.repo

    # Restore single backslashes (regex::Escape doubled them)
    $body = $body -replace '\\\\', '\'

    $issPath = Join-Path $issDir "$($p.slug).iss"
    $body | Out-File $issPath -Encoding UTF8

    # Compile with output dir
    & $iscc /Q "/O$output" $issPath 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  COMPILE FAILED for $($p.slug)" -ForegroundColor Red
        continue
    }
    $exeName = "$($p.slug)-Setup-v$($p.version).exe"
    $exePath = Join-Path $output $exeName
    if (Test-Path $exePath) {
        Write-Host "  OK: $exeName ($((Get-Item $exePath).Length) B)" -ForegroundColor Green
    } else {
        Write-Host "  Missing expected output: $exeName" -ForegroundColor Yellow
    }
}

Write-Host "`nAll installers in: $output" -ForegroundColor Yellow
Get-ChildItem $output -Filter "*.exe" | Select-Object Name, Length | Format-Table -AutoSize
