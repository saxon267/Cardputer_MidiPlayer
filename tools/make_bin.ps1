# Build the firmware and produce a single flashable image (offset 0x0).
#   pwsh tools/make_bin.ps1
#   pwsh tools/make_bin.ps1 -Env cardputer_midi_usb
param(
    [string]$Python = "python",
    [string]$Env = "cardputer_midi"
)

$ErrorActionPreference = "Stop"
$root  = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$proj  = Join-Path $root "firmware"
$out   = Join-Path $root "dist"
$build = Join-Path $proj ".pio\build\$Env"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    $pioCmd = Join-Path (Split-Path -Parent (& $Python -c "import sys;print(sys.executable)")) "Scripts\platformio.exe"
} else {
    $pioCmd = "pio"
}

Write-Host "== building $Env ==" -ForegroundColor Cyan
& $pioCmd run -d $proj -e $Env
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$esp = Join-Path $env:USERPROFILE ".platformio\packages\tool-esptoolpy\esptool.py"
if (-not (Test-Path $esp)) { throw "esptool not found at $esp" }

New-Item -ItemType Directory -Force -Path $out | Out-Null
if ($Env -eq "cardputer_midi_usb") {
    $target = Join-Path $out "CardputerMidiPlayer-USBHost-8MB.bin"
} else {
    $target = Join-Path $out "CardputerMidiPlayer-8MB.bin"
}

Write-Host "== merging ==" -ForegroundColor Cyan
& $Python $esp --chip esp32s3 merge_bin `
    -o $target `
    --flash_mode qio --flash_freq 80m --flash_size 8MB `
    0x0    (Join-Path $build "bootloader.bin") `
    0x8000 (Join-Path $build "partitions.bin") `
    0x10000 (Join-Path $build "firmware.bin")
if ($LASTEXITCODE -ne 0) { throw "merge failed" }

$kb = [math]::Round((Get-Item $target).Length / 1KB, 1)
Write-Host "`nOK -> $target ($kb KB, flash at 0x0)" -ForegroundColor Green
