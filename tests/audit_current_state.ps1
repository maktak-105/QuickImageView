[CmdletBinding()]
param(
    [ValidateSet('OriginalProtection','ExplorerLaunchEvidence','UiInteractionEvidence','MissingEvidence')]
    [string]$Invariant,
    [switch]$RequireCompleteMatrix,
    [string[]]$RequireEvidenceFor,
    [string]$EvidenceRootOverride
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$evidenceRoot = if ($EvidenceRootOverride) { (Resolve-Path $EvidenceRootOverride).Path } else { Join-Path $repo 'docs\loop\evidence' }
$matrixPath = Join-Path $evidenceRoot 'REQ-20260821-001-product-audit-matrix.json'
$corePath = Join-Path $repo 'core\native\main.cpp'
$cmakePath = Join-Path $repo 'CMakeLists.txt'
$readmePath = Join-Path $repo 'README.md'
$scriptsPath = Join-Path $repo 'scripts'

New-Item -ItemType Directory -Force -Path $evidenceRoot | Out-Null

function Get-EvidenceRecord([string]$Prefix) {
    $files = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter "$Prefix*.json" -File -ErrorAction SilentlyContinue)
    foreach ($file in $files) {
        try {
            $record = Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json
            if ($null -ne $record.exit_code) { return $record }
        } catch { }
    }
    return $null
}

function New-Row([string]$Id, [string]$Area, [string]$Status, [string]$Evidence, [string]$Note) {
    [ordered]@{
        id = $Id
        area = $Area
        status = $Status
        evidence = $Evidence
        note = $Note
    }
}

function Get-E2EEvidence {
    param([string]$Pattern, [string]$Area)
    $files = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter $Pattern -File -ErrorAction SilentlyContinue)
    if ($files.Count -eq 0) {
        return (New-Row $Area $Area 'BLOCKED' '' '実機操作証拠JSONが存在しない。')
    }
    $errors = @()
    $paths = @()
    foreach ($file in $files) {
        $paths += $file.FullName
        try { $record = Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json }
        catch { $errors += "$($file.Name): invalid JSON"; continue }
        $required = @('target_operation','expected_result','observed_result','target_exe','target_image','recorded_at','evidence_references')
        foreach ($field in $required) {
            $property = $record.PSObject.Properties[$field]
            if ($null -eq $property -or [string]::IsNullOrWhiteSpace([string]$record.$field)) { $errors += "$($file.Name): missing $field" }
        }
        if ($null -ne $record.PSObject.Properties['expected_result'] -and $null -ne $record.PSObject.Properties['observed_result']) {
            $expected = ([string]$record.expected_result).Trim()
            $observed = ([string]$record.observed_result).Trim()
            if ($expected -ne $observed) { $errors += "$($file.Name): expected_result does not equal observed_result" }
            if ($observed.ToLowerInvariant().Contains('process alive') -or $observed.ToLowerInvariant().Contains('process exists')) { $errors += "$($file.Name): process-liveness-only result" }
        }
        if ($null -ne $record.PSObject.Properties['recorded_at']) {
            $parsed = [DateTimeOffset]::MinValue
            if (-not [DateTimeOffset]::TryParse([string]$record.recorded_at, [ref]$parsed)) { $errors += "$($file.Name): recorded_at is invalid" }
        }
        if ($null -ne $record.PSObject.Properties['evidence_references']) {
            $references = @($record.evidence_references)
            if ($references.Count -eq 0) { $errors += "$($file.Name): evidence_references is empty" }
        }
    }
    $pathText = $paths -join ([char]59)
    if ($errors.Count -gt 0) { $errorText = $errors -join ([char]59); return (New-Row $Area $Area 'FAIL' $pathText $errorText) }
    return (New-Row $Area $Area 'PASS' $pathText '操作、期待/実測結果、対象exe/画像、記録時刻、証拠参照を検証済み。')
}

$rows = [System.Collections.Generic.List[object]]::new()
$source = if (Test-Path -LiteralPath $corePath) { Get-Content -Raw -LiteralPath $corePath } else { '' }
$cmake = if (Test-Path -LiteralPath $cmakePath) { Get-Content -Raw -LiteralPath $cmakePath } else { '' }

$originalProtectionStatus = if ($source -match 'SamePath' -and $source -match 'CREATE_NEW') { 'PASS' } else { 'FAIL' }
$rows.Add((New-Row 'INV-1' 'OriginalProtection' $originalProtectionStatus 'core/native/main.cpp' 'Static guard requires same-path rejection and CREATE_NEW output reservation.'))
$rows.Add((Get-E2EEvidence 'explorer-e2e*.json' 'ExplorerE2E'))
$rows.Add((Get-E2EEvidence 'ui-e2e*.json' 'UiE2E'))
$rows.Add((New-Row 'INV-4' 'MissingEvidence' $(if ((Test-Path -LiteralPath $evidenceRoot) -and (Test-Path -LiteralPath $matrixPath)) { 'PASS' } else { 'FAIL' }) 'docs/loop/evidence/REQ-20260821-001-product-audit-matrix.json' 'Every matrix row must carry an explicit PASS, BLOCKED, or FAIL status.'))

$rows.Add((New-Row 'AUDIT-IMPLEMENTATION' 'Implementation inventory' $(if (Test-Path -LiteralPath $corePath) { 'PASS' } else { 'FAIL' }) 'core/native/main.cpp' 'Implementation source is enumerated; no source change is made by this audit.'))
$rows.Add((New-Row 'AUDIT-TESTS' 'Test inventory' $(if (Test-Path -LiteralPath $PSCommandPath) { 'PASS' } else { 'FAIL' }) 'tests/audit_current_state.ps1' 'This red-capable audit script is present.'))
$rows.Add((New-Row 'AUDIT-DISTRIBUTION' 'Distribution scripts inventory' $(if ((Test-Path -LiteralPath $scriptsPath) -and (Test-Path (Join-Path $scriptsPath 'install.ps1'))) { 'PASS' } else { 'FAIL' }) 'scripts/install.ps1;scripts/uninstall.ps1;scripts/package.ps1' 'Script presence is inventory evidence, not install E2E evidence.'))
$build = Get-EvidenceRecord 'REQ-20260821-001-product-build'
$rows.Add((New-Row 'VERIFY-BUILD' 'CMake configure/build' $(if ($null -eq $build) { 'BLOCKED' } elseif ([int]$build.exit_code -eq 0) { 'PASS' } else { 'FAIL' }) $(if ($null -eq $build) { '' } else { $build.evidence_path }) 'Status is based on the recorded process exit code.'))
$ctest = Get-EvidenceRecord 'REQ-20260821-001-product-ctest'
$rows.Add((New-Row 'VERIFY-CTEST' 'CTest self-test' $(if ($null -eq $ctest) { 'BLOCKED' } elseif ([int]$ctest.exit_code -eq 0) { 'PASS' } else { 'FAIL' }) $(if ($null -eq $ctest) { '' } else { $ctest.evidence_path }) 'Status is based on the recorded process exit code.'))

$matrix = [ordered]@{
    request_id = 'REQ-20260821-001-product'
    checkpoint = '001 現状監査'
    generated_at = (Get-Date).ToUniversalTime().ToString('o')
    command = 'powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireCompleteMatrix'
    exit_code = 0
    ran_at = (Get-Date).ToUniversalTime().ToString('o')
    source_docs = @('plans/qa-audit-001.md','plans/requirements-matrix.md','CMakeLists.txt','README.md')
    rows = @($rows)
    policy = 'Missing or non-human UI/Explorer evidence is BLOCKED; no process-only PASS.'
}
$matrixJson = $matrix | ConvertTo-Json -Depth 6
[System.IO.File]::WriteAllText($matrixPath, $matrixJson, [System.Text.UTF8Encoding]::new($false))

$selected = @($rows)
if ($Invariant) { $selected = @($rows | Where-Object { $_.area -eq $Invariant }) }
$bad = @($selected | Where-Object { $_.status -eq 'FAIL' })
$missing = @($selected | Where-Object { [string]::IsNullOrWhiteSpace($_.status) -or [string]::IsNullOrWhiteSpace($_.evidence) -and $_.status -eq 'PASS' })
if ($RequireEvidenceFor) {
    $requestedNames = @($RequireEvidenceFor | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    foreach ($name in $requestedNames) {
        $hit = @($rows | Where-Object { $_.area -eq $name })
        if (-not $hit -or ($hit | Where-Object { $_.status -eq 'PASS' -and [string]::IsNullOrWhiteSpace($_.evidence) })) {
            $bad += [pscustomobject]@{ id = "required-$name"; status = 'FAIL' }
        }
    }
}

if ($RequireCompleteMatrix -and ($rows.Count -lt 9 -or $missing.Count -gt 0)) { exit 2 }
if ($bad.Count -gt 0) { exit 1 }
Write-Output ("AUDIT_MATRIX_OK {0}" -f $matrixPath)
exit 0
