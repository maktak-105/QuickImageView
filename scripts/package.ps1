[CmdletBinding()]
param(
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot '..\build\intermediate\package'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
$exe = Join-Path $root 'dist\QuickImageView.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
$outputExe = Join-Path $output 'QuickImageView.exe'
if ([IO.Path]::GetFullPath($exe) -ne [IO.Path]::GetFullPath($outputExe)) {
    Copy-Item -LiteralPath $exe -Destination $outputExe -Force
}
$distributionDocs = Join-Path $root 'docs\distribution'
Get-ChildItem -LiteralPath $distributionDocs -File | Copy-Item -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $root 'installer\LICENSE.rtf') -Destination (Join-Path $output 'LICENSE.rtf') -Force
Copy-Item -LiteralPath (Join-Path $root 'src\app\QuickImageView.ico') -Destination (Join-Path $output 'QuickImageView-icon.ico') -Force
Write-Output "Package created: $output"
