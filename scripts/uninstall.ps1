[CmdletBinding()]
param(
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView',
    [string]$RegistryRoot = 'HKCU:\Software\Classes\SystemFileAssociations'
)

$ErrorActionPreference = 'Stop'
$target = [IO.Path]::GetFullPath($InstallDirectory)

# Every image the application opens (kept in step with ContextMenuEntry::suffixes() in src/app/context_menu_entry.cpp;
# a test compares the two lists). "image" is the old image-wide entry of versions 3.x to 4.1.
$suffixes = @('jpg', 'jpeg', 'png', 'tif', 'tiff', 'bmp', 'gif', 'webp', 'heic', 'heif', 'ico', 'jxr', 'wdp', 'hdp', 'dds')

function Remove-EmptyKey([string]$Path) {
    if ((Test-Path -LiteralPath $Path) -and
        -not (Get-ChildItem -LiteralPath $Path -ErrorAction SilentlyContinue) -and
        (Get-Item -LiteralPath $Path).ValueCount -eq 0) {
        Remove-Item -LiteralPath $Path -Force
    }
}

foreach ($association in (@('image') + ($suffixes | ForEach-Object { ".$_" }))) {
    $keyPath = Join-Path $RegistryRoot "$association\shell\$ContextMenuName"
    if (Test-Path -LiteralPath $keyPath) {
        Remove-Item -LiteralPath $keyPath -Recurse -Force
    }
    # Keys that only held the entry go away; keys with anything else in them stay.
    Remove-EmptyKey (Join-Path $RegistryRoot "$association\shell")
    Remove-EmptyKey (Join-Path $RegistryRoot $association)
}

if (Test-Path -LiteralPath $target) {
    $resolved = (Resolve-Path -LiteralPath $target).Path
    $localAppData = [IO.Path]::GetFullPath($env:LOCALAPPDATA)
    if ($resolved.StartsWith($localAppData, [StringComparison]::OrdinalIgnoreCase) -and
        $resolved.TrimEnd('\') -ne $localAppData.TrimEnd('\')) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
Write-Output 'QuickImageView uninstalled.'
