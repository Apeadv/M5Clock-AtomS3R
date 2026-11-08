$vbs = Join-Path $PSScriptRoot "M5Clock-Spectrum.vbs"
if (-not (Test-Path $vbs)) {
    Write-Host "M5Clock-Spectrum.vbs not found."
    exit 1
}

$startup = [Environment]::GetFolderPath("Startup")
$linkPath = Join-Path $startup "M5Clock Spectrum.lnk"

$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($linkPath)
$shortcut.TargetPath = Join-Path $env:SystemRoot "System32\wscript.exe"
$shortcut.Arguments = "`"$vbs`""
$shortcut.WorkingDirectory = $PSScriptRoot
$shortcut.WindowStyle = 7
$shortcut.Description = "M5Clock USB music spectrum (silent)"
$shortcut.Save()

Write-Host "Autostart installed silently:"
Write-Host "  $linkPath"
Write-Host ""
Write-Host "It starts hidden after Windows login and waits for AtomS3R."
