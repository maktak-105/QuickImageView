[CmdletBinding()]
param(
    [string]$SourceExe,
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView',
    [switch]$NoRegisterContextMenu
)

$ErrorActionPreference = 'Stop'
$exeName = 'QuickImageViewQt.exe'
if ([string]::IsNullOrWhiteSpace($SourceExe)) {
    $SourceExe = Join-Path $PSScriptRoot ('..\dist\' + $exeName)
}
$source = [IO.Path]::GetFullPath($SourceExe)
$target = [IO.Path]::GetFullPath($InstallDirectory)
$iconSource = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\src\app\QuickImageView.ico'))

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Built executable not found: $source"
}
if (-not (Test-Path -LiteralPath $iconSource -PathType Leaf)) {
    throw "Application icon not found: $iconSource"
}

# The deployed Qt runtime (windeployqt output) sits next to the executable.
$runtimeSource = Split-Path -Parent $source
$deployedFiles = Get-ChildItem -LiteralPath $runtimeSource -Force | Where-Object { $_.Name -ne '.gitkeep' }
if (-not ($deployedFiles | Where-Object { $_.Name -eq $exeName })) {
    throw "Qt runtime files were not found under $runtimeSource"
}

New-Item -ItemType Directory -Force -Path $target | Out-Null
$installedExe = Join-Path $target $exeName
$installedIcon = Join-Path $target 'QuickImageView.ico'

# Remove leftovers from earlier installs (for example the pre-4.0 Win32 executable).
$staleItems = Get-ChildItem -LiteralPath $target -Force | Where-Object {
    $name = $_.Name
    -not ($deployedFiles | Where-Object { $_.Name -eq $name }) -and $name -ne 'QuickImageView.ico'
}
foreach ($item in $staleItems) {
    Remove-Item -LiteralPath $item.FullName -Recurse -Force
}

foreach ($file in $deployedFiles) {
    Copy-Item -LiteralPath $file.FullName -Destination $target -Recurse -Force
}
Copy-Item -LiteralPath $source -Destination $installedExe -Force
Copy-Item -LiteralPath $iconSource -Destination $installedIcon -Force

$keyPath = 'HKCU:\Software\Classes\SystemFileAssociations\image\shell\' + $ContextMenuName
if (-not $NoRegisterContextMenu) {
    New-Item -Path $keyPath -Force | Out-Null
    Set-ItemProperty -Path $keyPath -Name '(default)' -Value 'Open with QuickImageView'
    Set-ItemProperty -Path $keyPath -Name 'Icon' -Value $installedIcon
    New-Item -Path (Join-Path $keyPath 'command') -Force | Out-Null
    Set-ItemProperty -Path (Join-Path $keyPath 'command') -Name '(default)' `
        -Value ('"' + $installedExe + '" "%1"')
}

Write-Output "QuickImageView installed: $target"
if (-not $NoRegisterContextMenu) {
    Write-Output 'Context menu registration enabled.'
} else {
    Write-Output 'Context menu registration skipped.'
}
