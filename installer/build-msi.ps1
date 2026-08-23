[CmdletBinding()]
param([string]$OutputPath = "$(Join-Path $PSScriptRoot '..\dist\binary\QuickImageView-1.0.0-x64.msi')")
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$stage = Join-Path $root 'dist\binary\.msi-stage'
$wixBin = @(
    (Get-Command candle.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue | Split-Path -Parent),
    'C:\Program Files (x86)\WiX Toolset v3.11\bin',
    'C:\Program Files (x86)\WiX Toolset v3.14\bin'
) | Where-Object { $_ -and (Test-Path -LiteralPath (Join-Path $_ 'candle.exe') -PathType Leaf) } | Select-Object -First 1
if (-not $wixBin) {
    throw "WiX Toolset v3 (candle.exe/light.exe) が見つかりません。管理者PowerShellで次を実行後、再度このスクリプトを実行してください: winget install --id WiXToolset.WiXToolset --exact --accept-package-agreements --accept-source-agreements"
}
$wix = Join-Path $wixBin 'candle.exe'
$light = Join-Path $wixBin 'light.exe'
& (Join-Path $root 'scripts\package.ps1') -OutputDirectory $stage
& $wix -nologo ("-dSourceDir=" + $stage) -out (Join-Path $stage 'QuickImageView.wixobj') (Join-Path $PSScriptRoot 'QuickImageView.wxs')
if ($LASTEXITCODE -ne 0) { throw "WiXコンパイルに失敗しました（exit code: $LASTEXITCODE）。" }
& $light -nologo -ext WixUIExtension -out $OutputPath (Join-Path $stage 'QuickImageView.wixobj')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) { throw "MSIリンクに失敗しました（exit code: $LASTEXITCODE）。" }
Write-Output "MSI created: $OutputPath"
