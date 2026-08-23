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
$exe = Join-Path $root 'dist\binary\QuickImageView.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
$outputExe = Join-Path $output 'QuickImageView.exe'
if ([IO.Path]::GetFullPath($exe) -ne [IO.Path]::GetFullPath($outputExe)) {
    Copy-Item -LiteralPath $exe -Destination $outputExe -Force
}
Copy-Item -LiteralPath (Join-Path $root 'dist\documents\readme.txt') -Destination (Join-Path $output 'readme.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'dist\documents\readme_jp.txt') -Destination (Join-Path $output 'readme_jp.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'history.md') -Destination (Join-Path $output 'history.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'history_jp.md') -Destination (Join-Path $output 'history_jp.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination (Join-Path $output 'LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'document\LICENSE_jp.txt') -Destination (Join-Path $output 'LICENSE_jp.txt') -Force
Copy-Item -LiteralPath (Join-Path $root 'document\help.md') -Destination (Join-Path $output 'help.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'document\help_jp.md') -Destination (Join-Path $output 'help_jp.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\libwebp-1.6.0\COPYING') -Destination (Join-Path $output 'libwebp-COPYING') -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\libwebp-1.6.0\PATENTS') -Destination (Join-Path $output 'libwebp-PATENTS') -Force
Copy-Item -LiteralPath (Join-Path $root 'installer\LICENSE.rtf') -Destination (Join-Path $output 'LICENSE.rtf') -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\QuickImageView-icon.ico') -Destination (Join-Path $output 'QuickImageView-icon.ico') -Force
Write-Output "Package created: $output"
