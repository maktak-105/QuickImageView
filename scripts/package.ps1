[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [string]$ArchivePath
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot '..\build\intermediate\package'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
$distDir = Join-Path $root 'dist'
$exe = Join-Path $distDir 'QuickImageViewQt.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
$qtRuntime = Get-ChildItem -LiteralPath $distDir -Force | Where-Object {
    $_.Name -eq 'QuickImageViewQt.exe' -or
    $_.Name -like 'Qt6*.dll' -or
    $_.Name -like 'libgcc*' -or
    $_.Name -like 'libstdc++*' -or
    $_.Name -like 'libwinpthread*' -or
    $_.Name -like 'vcruntime*.dll' -or
    $_.Name -like 'msvcp*.dll' -or
    $_.Name -like 'concrt*.dll' -or
    $_.Name -like 'vccorlib*.dll' -or
    $_.Name -eq 'D3Dcompiler_47.dll' -or
    $_.Name -in @('platforms', 'imageformats', 'qml', 'iconengines')
}
$qtRuntime | Copy-Item -Destination $output -Recurse -Force

$distributionDocs = Join-Path $root 'docs\distribution'
Get-ChildItem -LiteralPath $distributionDocs -File | Copy-Item -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $root 'src\app\QuickImageView.ico') -Destination (Join-Path $output 'QuickImageView-icon.ico') -Force
Write-Output "Package created: $output"
if (-not [string]::IsNullOrWhiteSpace($ArchivePath)) {
    $archive = [IO.Path]::GetFullPath($ArchivePath)
    if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
    Compress-Archive -Path (Join-Path $output '*') -DestinationPath $archive -CompressionLevel Optimal
    Write-Output "Archive created: $archive"
}
