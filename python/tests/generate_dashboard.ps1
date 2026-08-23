[CmdletBinding()]
param(
    [string]$InputPath = (Join-Path $PSScriptRoot '..\docs\loop\current.json'),
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\docs\loop\report.html')
)
$ErrorActionPreference = 'Stop'
$data = Get-Content -Raw -LiteralPath $InputPath | ConvertFrom-Json
$rows = foreach ($item in $data.ui_results) {
    $status = [System.Net.WebUtility]::HtmlEncode([string]$item.status)
    $text = [System.Net.WebUtility]::HtmlEncode([string]$item.text)
    $detail = [System.Net.WebUtility]::HtmlEncode([string]$item.detail)
    "<tr class='$($status.ToLowerInvariant())'><td>$($item.id)</td><td>$text</td><td>$status</td><td>$detail</td></tr>"
}
$html = "<!doctype html><meta charset='utf-8'><title>QuickImageView verification</title><style>body{font-family:'Segoe UI';background:#121618;color:#f2f6f7}table{border-collapse:collapse;width:100%}td,th{border:1px solid #526064;padding:6px;text-align:left}.pass{background:#173d32}.fail,.error{background:#5a2222}.unchecked{background:#5a4a22}</style><h1>QuickImageView verification</h1><table><tr><th>ID</th><th>Requirement</th><th>Status</th><th>Evidence</th></tr>$($rows -join '')</table>"
Set-Content -LiteralPath $OutputPath -Value $html -Encoding UTF8
