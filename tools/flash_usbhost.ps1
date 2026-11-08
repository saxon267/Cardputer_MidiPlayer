# Flash the USB-Host MIDI build and keep the ESP32 in the ROM loader
# (--after no_reset) so the Cardputer does not become a USB host while the
# PC is still supplying VBUS.
#   1. Disconnect FM-1.
#   2. Connect Cardputer to the PC, put it in download mode if needed.
#   3. Run: pwsh tools/flash_usbhost.ps1 -Port COM3
#   4. Unplug the USB-C cable, power-cycle the Cardputer, then connect FM-1.
param(
    [string]$Port = "COM3",
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$esp  = Join-Path $env:USERPROFILE ".platformio\packages\tool-esptoolpy\esptool.py"
$bin  = Join-Path $root "dist\CardputerMidiPlayer-USBHost-8MB.bin"

if (-not (Test-Path $bin)) { throw "missing $bin; run tools/make_bin.ps1 -Env cardputer_midi_usb first" }
if (-not (Test-Path $esp)) { throw "esptool not found at $esp" }

& $Python $esp --chip esp32s3 -p $Port -b 460800 `
    --before default_reset --after no_reset `
    write_flash 0x0 $bin
if ($LASTEXITCODE -ne 0) { throw "flash failed" }

Write-Host ""
Write-Host "Flashed. The device is left in the ROM loader on purpose." -ForegroundColor Yellow
Write-Host "1) UNPLUG the USB-C cable from the PC." -ForegroundColor Yellow
Write-Host "2) Power-cycle the Cardputer (battery) so it boots in USB-Host mode." -ForegroundColor Yellow
Write-Host "3) Connect FM-1 with a USB-C data cable; it must be self-powered." -ForegroundColor Yellow
Write-Host "Never start the USB-Host build while the Cardputer is also connected to a PC." -ForegroundColor Red
