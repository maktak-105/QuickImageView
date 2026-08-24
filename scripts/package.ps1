[CmdletBinding()]
param(
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot '..\build\package'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
$exe = Join-Path $root 'dist\binary\QuickImageView.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
$outputExe = Join-Path $output 'QuickImageView.exe'
if ([IO.Path]::GetFullPath($exe) -ne [IO.Path]::GetFullPath($outputExe)) {
    Copy-Item -LiteralPath $exe -Destination $outputExe -Force
}
Get-ChildItem -LiteralPath (Join-Path $root 'dist\documents') -File | Copy-Item -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $root 'installer\LICENSE.rtf') -Destination (Join-Path $output 'LICENSE.rtf') -Force
Copy-Item -LiteralPath (Join-Path $root 'resources\icons\QuickImageView.ico') -Destination (Join-Path $output 'QuickImageView-icon.ico') -Force
Write-Output "Package created: $output"
