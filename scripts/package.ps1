[CmdletBinding()]
param(
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot '..\dist\binary'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
$exe = Join-Path $root 'build\QuickImageView.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $output 'QuickImageView.exe') -Force
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination (Join-Path $output 'README.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'document\distribution_jp.md') -Destination (Join-Path $output 'distribution_jp.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'document\third_party_licenses.md') -Destination (Join-Path $output 'third_party_licenses.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\libwebp-1.6.0\COPYING') -Destination (Join-Path $output 'libwebp-COPYING') -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\libwebp-1.6.0\PATENTS') -Destination (Join-Path $output 'libwebp-PATENTS') -Force
Write-Output "Package created: $output"
