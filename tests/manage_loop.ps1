[CmdletBinding()]
param([switch]$SkipUi)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$checklist = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\checklist.json') | ConvertFrom-Json
$manager = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\manager.json') | ConvertFrom-Json

$errors = [System.Collections.Generic.List[string]]::new()
$items = @($checklist.items)
$ids = @($items | ForEach-Object id)
if ($ids.Count -eq 0 -or (($ids | Sort-Object -Unique).Count -ne $ids.Count)) { $errors.Add('checklist IDs are empty or duplicated') }
foreach ($item in $items) {
    if ([string]::IsNullOrWhiteSpace($item.id)) { $errors.Add('checklist item has no id') }
    if ([string]::IsNullOrWhiteSpace($item.source)) { $errors.Add($item.id + ' has no source') }
    if (@('required','excluded') -notcontains [string]$item.status) { $errors.Add($item.id + ' has invalid status') }
    if ($item.status -eq 'required' -and @($item.test_ids).Count -eq 0) { $errors.Add($item.id + ' has no test links') }
}
if ($manager.authority -ne 'docs/loop/checklist.json') { $errors.Add('manager authority does not point to checklist.json') }
if (-not $manager.responsibilities -or @($manager.responsibilities).Count -lt 5) { $errors.Add('manager responsibilities are incomplete') }

$verification = $null
try {
    if ($SkipUi) {
        $verifyPath = Join-Path $repo 'tests\verify_goal.ps1'
        $outFile = [IO.Path]::GetTempFileName()
        $errFile = [IO.Path]::GetTempFileName()
        $child = Start-Process -FilePath 'powershell' -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',$verifyPath,'-SkipUi') -RedirectStandardOutput $outFile -RedirectStandardError $errFile -Wait -PassThru
        $raw = Get-Content -Raw -LiteralPath $outFile
        Remove-Item -LiteralPath $outFile,$errFile -Force -ErrorAction SilentlyContinue
        $jsonStart = $raw.IndexOf('{')
        $jsonEnd = $raw.LastIndexOf('}')
        if ($jsonStart -lt 0 -or $jsonEnd -le $jsonStart) { throw 'verify_goal output did not contain JSON' }
        $verification = $raw.Substring($jsonStart, $jsonEnd - $jsonStart + 1) | ConvertFrom-Json
    } else {
        $verifyPath = Join-Path $repo 'tests\verify_goal.ps1'
        $outFile = [IO.Path]::GetTempFileName()
        $errFile = [IO.Path]::GetTempFileName()
        $child = Start-Process -FilePath 'powershell' -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',$verifyPath) -RedirectStandardOutput $outFile -RedirectStandardError $errFile -Wait -PassThru
        $raw = Get-Content -Raw -LiteralPath $outFile
        Remove-Item -LiteralPath $outFile,$errFile -Force -ErrorAction SilentlyContinue
        $jsonStart = $raw.IndexOf('{')
        $jsonEnd = $raw.LastIndexOf('}')
        if ($jsonStart -lt 0 -or $jsonEnd -le $jsonStart) { throw 'verify_goal output did not contain JSON' }
        $verification = $raw.Substring($jsonStart, $jsonEnd - $jsonStart + 1) | ConvertFrom-Json
    }
} catch {
    $errors.Add('verify_goal.ps1 execution error: ' + $_.Exception.Message)
    $verification = [ordered]@{ status = 'FAIL'; checks = @([ordered]@{ id = 'verify_goal_execution'; status = 'FAIL'; detail = $_.Exception.Message }) }
}
$resultsById = @{}
foreach ($check in $verification.checks) { $resultsById[[string]$check.id] = [string]$check.status }
foreach ($item in $items | Where-Object status -eq 'required') {
    foreach ($testId in @($item.test_ids)) {
        if (-not $resultsById.ContainsKey([string]$testId)) { $errors.Add($item.id + ' links to missing test ' + $testId); continue }
        if ($resultsById[[string]$testId] -ne 'PASS') { $errors.Add($item.id + ' blocked by ' + $testId + '=' + $resultsById[[string]$testId]) }
    }
}
if ($verification.status -ne 'PASS') { $errors.Add('verify_goal.ps1 status=' + $verification.status) }

$unverified = @(
    [ordered]@{ id = 'metadata_real_exif'; status = 'UNVERIFIED'; detail = 'metadata_dynamic は実EXIF表示操作ではなく、ソースプローブ中心の検査' },
    [ordered]@{ id = 'resize_visual_dimensions'; status = 'UNVERIFIED'; detail = 'ui_dynamic はリサイズ後の実画像寸法を画面または出力で比較していない' }
)
$unverifiedRequirements = @{
    metadata_real_exif = @('REQ-017')
    resize_visual_dimensions = @('REQ-006','REQ-006A','REQ-007')
}
foreach ($fact in $unverified) { $errors.Add($fact.id + '=' + $fact.status + ': ' + $fact.detail) }

$status = if ($errors.Count -eq 0) { 'PASS' } else { 'FAIL' }
[string]$checkedAt = Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK'
[ordered]@{
    status = $status
    checked_at = $checkedAt
    next = if ($status -eq 'PASS') { '次のユーザー指示を待つ' } else { 'FAIL項目を解消して再検査する' }
    errors = @($errors)
    unverified = @($unverified)
    unverified_requirements = $unverifiedRequirements
    checks = @($verification.checks)
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $repo 'docs\loop\state.json') -Encoding UTF8
$dashboardScript = Join-Path $repo 'tests\generate_dashboard.ps1'
& powershell -NoProfile -ExecutionPolicy Bypass -File $dashboardScript | Out-Null
[ordered]@{ status=$status; checklist_items=$items.Count; errors=@($errors); manager=$manager.role; checks=@($verification.checks) } | ConvertTo-Json -Depth 8
exit $(if ($status -eq 'PASS') { 0 } else { 1 })
