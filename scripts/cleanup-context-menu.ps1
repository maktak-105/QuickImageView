[CmdletBinding()]
param(
    [switch]$NoExplorerRestart
)

$ErrorActionPreference = 'Stop'

# PowerToys registers both the legacy COM handler and a Windows 11 sparse-package
# handler.  The supported way to keep PowerRename and File Locksmith out of the
# default menu is their module setting; Image Resizer has no such setting, so it
# is disabled as a context-menu module and exposed as a normal Open with app.
$powerToysRoot = Join-Path $env:LOCALAPPDATA 'Microsoft\PowerToys'

$powerRenameSettings = Join-Path $powerToysRoot 'PowerRename\power-rename-settings.json'
if (Test-Path -LiteralPath $powerRenameSettings) {
    $settings = Get-Content -Raw -LiteralPath $powerRenameSettings | ConvertFrom-Json
    $settings.ExtendedContextMenuOnly = $true
    [IO.File]::WriteAllText($powerRenameSettings, ($settings | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
}

$fileLocksmithSettings = Join-Path $powerToysRoot 'File Locksmith\file-locksmith-settings.json'
if (Test-Path -LiteralPath $fileLocksmithSettings) {
    $settings = Get-Content -Raw -LiteralPath $fileLocksmithSettings | ConvertFrom-Json
    $settings.showInExtendedContextMenu = $true
    [IO.File]::WriteAllText($fileLocksmithSettings, ($settings | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
}

# Image Resizer has no "extended menu only" option in PowerToys v0.100.2.
# Keep the GUI available through Open with, but remove its duplicate shell verbs.
$powerToysSettings = Join-Path $powerToysRoot 'settings.json'
if (Test-Path -LiteralPath $powerToysSettings) {
    $settings = Get-Content -Raw -LiteralPath $powerToysSettings | ConvertFrom-Json
    $settings.enabled.'Image Resizer' = $false
    [IO.File]::WriteAllText($powerToysSettings, ($settings | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
}

$imageResizerExe = 'C:\Program Files\PowerToys\WinUI3Apps\PowerToys.ImageResizer.exe'
if (Test-Path -LiteralPath $imageResizerExe) {
    $appKey = 'HKCU:\Software\Classes\Applications\PowerToys.ImageResizer.exe'
    New-Item -Path (Join-Path $appKey 'shell\open\command') -Force | Out-Null
    Set-ItemProperty -Path (Join-Path $appKey 'shell\open\command') -Name '(default)' -Value ('"' + $imageResizerExe + '" "%1"')
    New-Item -Path (Join-Path $appKey 'DefaultIcon') -Force | Out-Null
    Set-ItemProperty -Path (Join-Path $appKey 'DefaultIcon') -Name '(default)' -Value $imageResizerExe
    $supportedTypes = @('.jpg', '.jpeg', '.png', '.gif', '.bmp', '.tif', '.tiff', '.webp', '.heic', '.heif')
    New-Item -Path (Join-Path $appKey 'SupportedTypes') -Force | Out-Null
    foreach ($extension in $supportedTypes) {
        New-ItemProperty -Path (Join-Path $appKey 'SupportedTypes') -Name $extension -PropertyType String -Value '' -Force | Out-Null
    }
}

# Block the modern package handlers as a second line of defense.  The module
# setting above hides Image Resizer; these values also prevent stale package
# registrations from surfacing if PowerToys briefly re-registers them.
$blockedPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Blocked'
$modernPowerToysHandlers = [ordered]@{
    '{8F491918-259F-451A-950F-8C3EBF4864AF}' = 'Hide duplicate PowerToys Image Resizer modern entry'
    '{1861E28B-A1F0-4EF4-A1FE-4C8CA88E2174}' = 'Hide duplicate PowerToys PowerRename modern entry'
    '{AAF1E27D-4976-49C2-8895-AAFA743C0A7E}' = 'Hide duplicate PowerToys File Locksmith modern entry'
}

New-Item -Path $blockedPath -Force | Out-Null
foreach ($handler in $modernPowerToysHandlers.GetEnumerator()) {
    New-ItemProperty -Path $blockedPath -Name $handler.Key -PropertyType String -Value $handler.Value -Force | Out-Null
}

# Preserve the user's classic-menu preference.  On Windows builds that honor it,
# this keeps the existing old-style menu; on builds that ignore it, the key is
# harmless and remains available for a future OS update.
$legacyMenuOverride = 'HKCU:\Software\Classes\CLSID\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\InprocServer32'
New-Item -Path $legacyMenuOverride -Force | Out-Null
Set-ItemProperty -Path $legacyMenuOverride -Name '(default)' -Value ''

if (-not $NoExplorerRestart) {
    Get-Process -Name explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Process -FilePath 'explorer.exe'
}

Write-Output 'PowerToys modern duplicate handlers blocked for the current user.'
if (-not $NoExplorerRestart) { Write-Output 'Explorer restarted.' }
