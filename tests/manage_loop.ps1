[CmdletBinding()]
param()

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

$raw = @(& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'tests\verify_goal.ps1') 2>&1 | Out-String)
$verification = $raw | ConvertFrom-Json
$resultsById = @{}
foreach ($check in $verification.checks) { $resultsById[[string]$check.id] = [string]$check.status }
foreach ($item in $items | Where-Object status -eq 'required') {
    foreach ($testId in @($item.test_ids)) {
        if (-not $resultsById.ContainsKey([string]$testId)) { $errors.Add($item.id + ' links to missing test ' + $testId); continue }
        if ($resultsById[[string]$testId] -ne 'PASS') { $errors.Add($item.id + ' blocked by ' + $testId + '=' + $resultsById[[string]$testId]) }
    }
}
if ($verification.status -ne 'PASS') { $errors.Add('verify_goal.ps1 status=' + $verification.status) }

$status = if ($errors.Count -eq 0) { 'PASS' } else { 'FAIL' }
[ordered]@{ status=$status; checklist_items=$items.Count; errors=@($errors); manager=$manager.role; checks=@($verification.checks) } | ConvertTo-Json -Depth 8
exit $(if ($status -eq 'PASS') { 0 } else { 1 })
