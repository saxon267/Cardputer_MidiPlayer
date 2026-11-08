# Restore the normal USB-CDC/local-speaker build after USB-Host experiments.
#   1. Disconnect FM-1.
#   2. Connect Cardputer to the PC, hold G0 and reset/plug to enter download mode.
#   3. Run: pwsh tools/flash_normal.ps1 -Port COM3
param(
    [string]$Port = "COM3",
    [string]$Env = "cardputer_midi"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$proj = Join-Path $root "firmware"

pio run -d $proj -e $Env -t upload --upload-port $Port
if ($LASTEXITCODE -ne 0) { throw "upload failed" }

Write-Host ""
Write-Host "Normal build restored. Reconnect the USB-C cable and open the serial monitor." -ForegroundColor Green
