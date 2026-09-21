[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [switch]$Qt,
    [string]$ArchivePath
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot '..\build\intermediate\package'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
$exe = if ($Qt) {
    Join-Path $root 'dist\binary\QuickImageViewQt.exe'
} else {
    Join-Path $root 'dist\QuickImageView.exe'
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build output not found: $exe" }

New-Item -ItemType Directory -Force -Path $output | Out-Null
if ($Qt) {
    $binaryDir = Join-Path $root 'dist\binary'
    $qtRuntime = Get-ChildItem -LiteralPath $binaryDir -Force | Where-Object {
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
    if (-not ($qtRuntime | Where-Object { $_.Name -eq 'QuickImageViewQt.exe' })) {
        throw "Qt runtime files were not found under $binaryDir"
    }
    $qtRuntime | Copy-Item -Destination $output -Recurse -Force
} else {
    $outputExe = Join-Path $output 'QuickImageView.exe'
    if ([IO.Path]::GetFullPath($exe) -ne [IO.Path]::GetFullPath($outputExe)) {
        Copy-Item -LiteralPath $exe -Destination $outputExe -Force
    }
}
$distributionDocs = Join-Path $root 'docs\distribution'
Get-ChildItem -LiteralPath $distributionDocs -File | Copy-Item -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $root 'installer\LICENSE.rtf') -Destination (Join-Path $output 'LICENSE.rtf') -Force
Copy-Item -LiteralPath (Join-Path $root 'src\app\QuickImageView.ico') -Destination (Join-Path $output 'QuickImageView-icon.ico') -Force
Write-Output "Package created: $output"
if (-not [string]::IsNullOrWhiteSpace($ArchivePath)) {
    $archive = [IO.Path]::GetFullPath($ArchivePath)
    if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
    Compress-Archive -Path (Join-Path $output '*') -DestinationPath $archive -CompressionLevel Optimal
    Write-Output "Archive created: $archive"
}
