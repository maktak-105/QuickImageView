[CmdletBinding()]
param([string]$OutputPath = '')

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $OutputPath) { $OutputPath = Join-Path $repo 'docs\loop\dashboard.html' }

# Documentation-only generation. It never runs the application or tests.
$goal = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\goal.json') | ConvertFrom-Json
$checklist = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\checklist.json') | ConvertFrom-Json
$invariants = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\invariants.json') | ConvertFrom-Json

function Encode([object]$Value) { [System.Net.WebUtility]::HtmlEncode([string]$Value) }
$generated = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
$checklistRows = foreach ($item in @($checklist.items)) {
    $status = if ($item.status -eq 'excluded') { 'EXCLUDED' } else { 'PENDING' }
    $class = if ($status -eq 'EXCLUDED') { 'excluded' } else { 'pending' }
    "<tr class='$class'><td><code>$(Encode $item.id)</code></td><td><span class='badge $class'>$(Encode $status)</span></td><td>$(Encode $item.description)</td><td>$(Encode ($item.test_ids -join ', '))</td></tr>"
}
$invariantRows = foreach ($item in @($invariants.invariants)) {
    "<tr class='pending'><td><code>$(Encode $item.id)</code></td><td><span class='badge pending'>PENDING</span></td><td>$(Encode $item.description)</td></tr>"
}
$requiredCount = @($checklist.items | Where-Object status -eq 'required').Count
$excludedCount = @($checklist.items | Where-Object status -eq 'excluded').Count

$html = @"
<!doctype html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>QuickImageView &#38283;&#30330;&#12480;&#12483;&#12471;&#12517;&#12508;&#12540;&#12489;</title>
<style>
:root { color-scheme: dark; font-family: Segoe UI, sans-serif; background:#10151c; color:#e8edf3; }
body { max-width:1400px; margin:0 auto; padding:28px; }
h1 { margin:0 0 8px; } h2 { margin-top:30px; }
.muted { color:#9aa8b8; } .summary { display:flex; gap:12px; flex-wrap:wrap; margin:20px 0; }
.card { background:#18212c; border:1px solid #2c3b4c; border-radius:10px; padding:16px 20px; min-width:180px; }
.card strong { display:block; font-size:28px; margin-top:4px; }
.pending { color:#f6cf72; } .excluded { color:#9aa8b8; }
.badge { display:inline-block; border-radius:999px; padding:3px 9px; font-size:12px; font-weight:700; background:#283544; }
table { width:100%; border-collapse:collapse; background:#151d27; border:1px solid #2c3b4c; }
th,td { text-align:left; padding:10px 12px; border-bottom:1px solid #273341; vertical-align:top; }
th { color:#aebdca; background:#1d2936; } tr.pending { background:#302a1d; }
code { color:#b9d7ff; } .note { border-left:4px solid #f6cf72; padding:10px 14px; background:#302a1d; }
</style>
</head>
<body>
<h1>QuickImageView &#38283;&#30330;&#12480;&#12483;&#12471;&#12517;&#12508;&#12540;&#12489;</h1>
<div class="muted">&#29983;&#25104;&#26085;&#26178;: $(Encode $generated) / &#29694;&#22312;&#12398;&#20184;&#27096;&#12539;&#12481;&#12455;&#12483;&#12463;&#12522;&#12473;&#12488;&#29366;&#24907;（&#26908;&#26597;&#26410;&#23455;&#34892;）</div>
<div class="summary">
  <div class="card">&#31649;&#29702;&#12466;&#12540;&#12488;<strong class="pending">&#26410;&#23455;&#34892;</strong></div>
  <div class="card">&#24517;&#38920;&#38917;&#30446;<strong class="pending">$requiredCount</strong></div>
  <div class="card">&#23550;&#35937;&#22806;<strong class="excluded">$excludedCount</strong></div>
  <div class="card">&#12468;&#12540;&#12523;&#21463;&#20837;&#38917;&#30446;<strong class="pending">$(@($goal.acceptance).Count)</strong></div>
</div>
<div class="note"><strong>&#27880;&#24847;:</strong> &#12371;&#12398;&#12506;&#12540;&#12472;&#12398;&#29983;&#25104;&#12391;&#12399;&#12450;&#12503;&#12522;&#12418;&#12486;&#12473;&#12488;&#12418;&#36215;&#21205;&#12375;&#12414;&#12379;&#12435;&#12290;&#23455;&#27231;&#26908;&#26597;&#12399; <code>tests/manage_loop.ps1</code> &#12434;&#26126;&#31034;&#30340;&#12395;&#23455;&#34892;&#12375;&#12390;&#12367;&#12384;&#12373;&#12356;&#12290;</div>
<h2>&#32173;&#25345;&#22865;&#32004;</h2>
<table><thead><tr><th>ID</th><th>&#29366;&#24907;</th><th>&#22865;&#32004;</th></tr></thead><tbody>$($invariantRows -join ([Environment]::NewLine))</tbody></table>
<h2>&#12481;&#12455;&#12483;&#12463;&#12522;&#12473;&#12488;</h2>
<table><thead><tr><th>ID</th><th>&#29366;&#24907;</th><th>&#35201;&#27714;</th><th>&#26908;&#26597;&#12522;&#12531;&#12463;</th></tr></thead><tbody>$($checklistRows -join ([Environment]::NewLine))</tbody></table>
</body></html>
"@

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) | Out-Null
Set-Content -LiteralPath $OutputPath -Value $html -Encoding UTF8
Write-Output $OutputPath
exit 0
