# Generate 24x24 PNG icons for each plugin (top-bar Plugins menu).
Add-Type -AssemblyName System.Drawing

$root = $PSScriptRoot
if (-not $root) { $root = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons" }
$out = Join-Path $root "icons"
if (-not (Test-Path $out)) { New-Item -ItemType Directory -Force -Path $out | Out-Null }

$icons = @(
    @{ name="pokebot";    color="#E04A2A"; letter="P"; }   # orange-red
    @{ name="follow";     color="#22A155"; letter="F"; }   # green
    @{ name="movespam";   color="#3B82F6"; letter="M"; }   # blue
    @{ name="soundboard"; color="#9333EA"; letter="S"; }   # purple
    @{ name="voicechanger"; color="#0EA5E9"; letter="V"; } # cyan
)

foreach ($i in $icons) {
    $size = 24
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias

    # Convert hex color to Color
    $h = $i.color.TrimStart('#')
    $r = [Convert]::ToInt32($h.Substring(0, 2), 16)
    $gC = [Convert]::ToInt32($h.Substring(2, 2), 16)
    $b = [Convert]::ToInt32($h.Substring(4, 2), 16)
    $bg = [System.Drawing.Color]::FromArgb(255, $r, $gC, $b)

    # Rounded background
    $brush = New-Object System.Drawing.SolidBrush $bg
    $g.FillRectangle($brush, 0, 0, $size, $size)
    $brush.Dispose()

    # Letter
    $font = New-Object System.Drawing.Font("Segoe UI", 13, [System.Drawing.FontStyle]::Bold)
    $textBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    $rect = New-Object System.Drawing.RectangleF 0, 0, $size, $size
    $g.DrawString($i.letter, $font, $textBrush, $rect, $sf)

    $textBrush.Dispose()
    $font.Dispose()
    $sf.Dispose()
    $g.Dispose()

    $path = Join-Path $out "$($i.name).png"
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "  $path" -ForegroundColor Green
}

Write-Host ""
Write-Host "Icons in: $out" -ForegroundColor Yellow
