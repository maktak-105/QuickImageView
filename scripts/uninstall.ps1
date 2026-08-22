[CmdletBinding()]
param(
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView'
)

$ErrorActionPreference = 'Stop'
$target = [IO.Path]::GetFullPath($InstallDirectory)
$keyPath = 'HKCU:\Software\Classes\SystemFileAssociations\image\shell\' + $ContextMenuName

if (Test-Path -LiteralPath $keyPath) {
    Remove-Item -LiteralPath $keyPath -Recurse -Force
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
