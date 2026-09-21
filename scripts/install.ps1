[CmdletBinding()]
param(
    [string]$SourceExe,
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView',
    [string]$RegistryRoot = 'HKCU:\Software\Classes\SystemFileAssociations',
    [switch]$NoRegisterContextMenu
)

$ErrorActionPreference = 'Stop'
$exeName = 'QuickImageView.exe'
if ([string]::IsNullOrWhiteSpace($SourceExe)) {
    $SourceExe = Join-Path $PSScriptRoot ('..\dist\' + $exeName)
}
$source = [IO.Path]::GetFullPath($SourceExe)
$target = [IO.Path]::GetFullPath($InstallDirectory)

# Every image the application opens (kept in step with ContextMenuEntry::suffixes() in src/app/context_menu_entry.cpp;
# a test compares the two lists). The entry is registered per extension: on some PCs the shell does not apply an
# image-wide key to .heic, .heif and .webp files.
$suffixes = @('jpg', 'jpeg', 'png', 'tif', 'tiff', 'bmp', 'gif', 'webp', 'heic', 'heif', 'ico', 'jxr', 'wdp', 'hdp', 'dds')

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Built executable not found: $source"
}

# The deployed Qt runtime (windeployqt output) sits next to the executable.
$runtimeSource = Split-Path -Parent $source
$deployedFiles = Get-ChildItem -LiteralPath $runtimeSource -Force | Where-Object { $_.Name -ne '.gitkeep' }
if (-not ($deployedFiles | Where-Object { $_.Name -eq $exeName })) {
    throw "Qt runtime files were not found under $runtimeSource"
}

New-Item -ItemType Directory -Force -Path $target | Out-Null
$installedExe = Join-Path $target $exeName

# Remove leftovers from earlier installs (for example the pre-release QuickImageViewQt.exe or a separate .ico).
$staleItems = Get-ChildItem -LiteralPath $target -Force | Where-Object {
    $name = $_.Name
    -not ($deployedFiles | Where-Object { $_.Name -eq $name })
}
foreach ($item in $staleItems) {
    Remove-Item -LiteralPath $item.FullName -Recurse -Force
}

foreach ($file in $deployedFiles) {
    Copy-Item -LiteralPath $file.FullName -Destination $target -Recurse -Force
}
Copy-Item -LiteralPath $source -Destination $installedExe -Force

if (-not $NoRegisterContextMenu) {
    foreach ($suffix in $suffixes) {
        $keyPath = Join-Path $RegistryRoot ".$suffix\shell\$ContextMenuName"
        New-Item -Path $keyPath -Force | Out-Null
        Set-ItemProperty -Path $keyPath -Name '(default)' -Value 'Open with QuickImageView'
        # The icon embedded in the executable; there is no separate .ico file.
        Set-ItemProperty -Path $keyPath -Name 'Icon' -Value ('"' + $installedExe + '",0')
        New-Item -Path (Join-Path $keyPath 'command') -Force | Out-Null
        Set-ItemProperty -Path (Join-Path $keyPath 'command') -Name '(default)' `
            -Value ('"' + $installedExe + '" "%1"')
    }
    # The image-wide entry of versions 3.x to 4.1 is replaced by the per-extension entries.
    $oldKeyPath = Join-Path $RegistryRoot "image\shell\$ContextMenuName"
    if (Test-Path -LiteralPath $oldKeyPath) {
        Remove-Item -LiteralPath $oldKeyPath -Recurse -Force
    }
}

Write-Output "QuickImageView installed: $target"
if (-not $NoRegisterContextMenu) {
    Write-Output 'Context menu registration enabled.'
} else {
    Write-Output 'Context menu registration skipped.'
}
