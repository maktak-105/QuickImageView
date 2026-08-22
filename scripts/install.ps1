[CmdletBinding()]
param(
    [string]$SourceExe,
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView',
    [switch]$NoRegisterContextMenu
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceExe)) {
    $SourceExe = Join-Path $PSScriptRoot '..\build\QuickImageView.exe'
}
$source = [IO.Path]::GetFullPath($SourceExe)
$target = [IO.Path]::GetFullPath($InstallDirectory)

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Built executable not found: $source"
}

New-Item -ItemType Directory -Force -Path $target | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $target 'QuickImageView.exe') -Force

$keyPath = 'HKCU:\Software\Classes\SystemFileAssociations\image\shell\' + $ContextMenuName
if (-not $NoRegisterContextMenu) {
    New-Item -Path $keyPath -Force | Out-Null
    Set-ItemProperty -Path $keyPath -Name '(default)' -Value 'Open with QuickImageView'
    New-Item -Path (Join-Path $keyPath 'command') -Force | Out-Null
    Set-ItemProperty -Path (Join-Path $keyPath 'command') -Name '(default)' `
        -Value ('"' + (Join-Path $target 'QuickImageView.exe') + '" "%1"')
}

Write-Output "QuickImageView installed: $target"
if (-not $NoRegisterContextMenu) { Write-Output 'Context menu registration enabled.' }
else { Write-Output 'Context menu registration skipped.' }
