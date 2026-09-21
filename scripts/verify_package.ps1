[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory
)

# Checks the staged package before it is zipped: required files are present and unused modules are not shipped.
$ErrorActionPreference = 'Stop'
$dir = [IO.Path]::GetFullPath($PackageDirectory)

$required = @(
    'QuickImageView.exe', 'Qt6Core.dll', 'platforms\qwindows.dll', 'imageformats\qwebp.dll',
    'readme.txt', 'readme_jp.txt', 'history.txt', 'history_jp.txt',
    'LICENSE.txt', 'LICENSE_jp.txt', 'third_party_licenses.md'
)
# The C++ runtime must ship with the package: MinGW Qt needs the MinGW runtime, MSVC Qt the Visual C++ runtime.
if (Test-Path -LiteralPath (Join-Path $dir 'libstdc++-6.dll')) {
    $required += 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll'
} else {
    $required += 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll'
}
$forbidden = @('Qt6Svg.dll', 'imageformats\qsvg.dll', 'QuickImageViewQt.exe')

$errors = @()
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $dir $file))) { $errors += "Missing distribution file: $file" }
}
foreach ($file in $forbidden) {
    if (Test-Path -LiteralPath (Join-Path $dir $file)) { $errors += "Unexpected file in the distribution: $file" }
}
if ($errors.Count -gt 0) { throw ($errors -join [Environment]::NewLine) }

$count = (Get-ChildItem -LiteralPath $dir -Recurse -File | Measure-Object).Count
Write-Output "Package verified: $dir ($count files)"
