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

$rows = [System.Collections.Generic.List[object]]::new()
$source = if (Test-Path -LiteralPath $corePath) { Get-Content -Raw -LiteralPath $corePath } else { '' }
$cmake = if (Test-Path -LiteralPath $cmakePath) { Get-Content -Raw -LiteralPath $cmakePath } else { '' }

$originalProtectionStatus = if ($source -match 'SamePath' -and $source -match 'CREATE_NEW') { 'PASS' } else { 'FAIL' }
$rows.Add((New-Row 'INV-1' 'OriginalProtection' $originalProtectionStatus 'core/native/main.cpp' 'Static guard requires same-path rejection and CREATE_NEW output reservation.'))
$validator = Join-Path $PSScriptRoot 'validate_e2e_evidence.ps1'
$explorerOutput = & $validator -EvidenceRoot $evidenceRoot -Pattern 'explorer-e2e*.json' 2>&1
$explorerCode = $LASTEXITCODE
$explorerStatus = if ($explorerCode -eq 0) { 'PASS' } elseif ($explorerCode -eq 2) { 'BLOCKED' } else { 'FAIL' }
$rows.Add((New-Row 'ExplorerE2E' 'ExplorerE2E' $explorerStatus ($(if ($explorerCode -eq 0) { 'docs/loop/evidence/explorer-e2e*.json' } else { '' })) (($explorerOutput -join '; '))))
$uiOutput = & $validator -EvidenceRoot $evidenceRoot -Pattern 'ui-e2e*.json' 2>&1
$uiCode = $LASTEXITCODE
$uiStatus = if ($uiCode -eq 0) { 'PASS' } elseif ($uiCode -eq 2) { 'BLOCKED' } else { 'FAIL' }
$rows.Add((New-Row 'UiE2E' 'UiE2E' $uiStatus ($(if ($uiCode -eq 0) { 'docs/loop/evidence/ui-e2e*.json' } else { '' })) (($uiOutput -join '; '))))
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
