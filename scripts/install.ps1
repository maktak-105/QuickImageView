[CmdletBinding()]
param(
    [string]$SourceExe,
    [string]$InstallDirectory = "$(Join-Path $env:LOCALAPPDATA 'QuickImageView')",
    [string]$ContextMenuName = 'QuickImageView',
    [switch]$Qt,
    [switch]$NoRegisterContextMenu
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceExe)) {
    $SourceExe = if ($Qt) {
        Join-Path $PSScriptRoot '..\dist\binary\QuickImageViewQt.exe'
    } else {
        Join-Path $PSScriptRoot '..\dist\QuickImageView.exe'
    }
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

New-Item -ItemType Directory -Force -Path $target | Out-Null
$installedExeName = if ($Qt) { 'QuickImageViewQt.exe' } else { 'QuickImageView.exe' }
$installedExe = Join-Path $target $installedExeName
$installedIcon = Join-Path $target 'QuickImageView.ico'
Copy-Item -LiteralPath $source -Destination $installedExe -Force
Copy-Item -LiteralPath $iconSource -Destination $installedIcon -Force

if ($Qt) {
    $runtimeSource = Split-Path -Parent $source
    $deployedFiles = Get-ChildItem -LiteralPath $runtimeSource -Force
    if (-not ($deployedFiles | Where-Object { $_.Name -eq 'QuickImageViewQt.exe' })) {
        throw "Qt runtime files were not found under $runtimeSource"
    }
    $staleItems = Get-ChildItem -LiteralPath $target -Force | Where-Object {
        $name = $_.Name
        $name -ne 'QuickImageView.exe' -and (-not ($deployedFiles | Where-Object { $_.Name -eq $name }))
    }
    foreach ($item in $staleItems) {
        Remove-Item -LiteralPath $item.FullName -Recurse -Force
    }

    foreach ($file in $deployedFiles) {
        if ($file.Name -ne 'QuickImageViewQt.exe' -and $file.Name -ne 'QuickImageView.ico') {
            Copy-Item -LiteralPath $file.FullName -Destination $target -Recurse -Force
        }
    }
}

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
