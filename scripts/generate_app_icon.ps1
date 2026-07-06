param(
    [string]$OutputPath = "",
    [string]$PreviewPngPath = ""
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path (Join-Path $PSScriptRoot "..") "src\res\p_tool.ico"
}

function New-SIconPngBytes([int]$Size) {
    $bitmap = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $graphics.Clear([System.Drawing.Color]::FromArgb(255, 0, 132, 76))

    $fontSize = [Math]::Floor($Size * 0.68)
    $font = New-Object System.Drawing.Font "Segoe UI Semibold", $fontSize, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $format = New-Object System.Drawing.StringFormat
    $format.Alignment = [System.Drawing.StringAlignment]::Center
    $format.LineAlignment = [System.Drawing.StringAlignment]::Center
    $rect = New-Object System.Drawing.RectangleF 0, (-1 * [Math]::Floor($Size * 0.035)), $Size, $Size
    $brush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)

    $graphics.DrawString("S", $font, $brush, $rect, $format)

    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $stream.ToArray()

    $brush.Dispose()
    $format.Dispose()
    $font.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
    $stream.Dispose()

    return $bytes
}

function Write-UInt16LE([System.IO.BinaryWriter]$Writer, [int]$Value) {
    $Writer.Write([byte]($Value -band 0xFF))
    $Writer.Write([byte](($Value -shr 8) -band 0xFF))
}

function Write-UInt32LE([System.IO.BinaryWriter]$Writer, [long]$Value) {
    $Writer.Write([byte]($Value -band 0xFF))
    $Writer.Write([byte](($Value -shr 8) -band 0xFF))
    $Writer.Write([byte](($Value -shr 16) -band 0xFF))
    $Writer.Write([byte](($Value -shr 24) -band 0xFF))
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$images = @()
foreach ($size in $sizes) {
    $images += [pscustomobject]@{
        Size = $size
        Bytes = New-SIconPngBytes $size
    }
}

$outDir = Split-Path -Parent $OutputPath
if (-not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter $stream

Write-UInt16LE $writer 0
Write-UInt16LE $writer 1
Write-UInt16LE $writer $images.Count

$offset = 6 + ($images.Count * 16)
foreach ($image in $images) {
    $entrySize = if ($image.Size -eq 256) { 0 } else { $image.Size }
    $writer.Write([byte]$entrySize)
    $writer.Write([byte]$entrySize)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    Write-UInt16LE $writer 1
    Write-UInt16LE $writer 32
    Write-UInt32LE $writer $image.Bytes.Length
    Write-UInt32LE $writer $offset
    $offset += $image.Bytes.Length
}

foreach ($image in $images) {
    $writer.Write([byte[]]$image.Bytes)
}

[System.IO.File]::WriteAllBytes($OutputPath, $stream.ToArray())
$writer.Dispose()
$stream.Dispose()

if (-not [string]::IsNullOrWhiteSpace($PreviewPngPath)) {
    [System.IO.File]::WriteAllBytes($PreviewPngPath, (New-SIconPngBytes 256))
}

Write-Host "Generated icon: $OutputPath"
