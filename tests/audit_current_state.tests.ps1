[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$script = Join-Path $PSScriptRoot 'audit_current_state.ps1'
$fixtureRoot = Join-Path $PSScriptRoot 'fixtures'
$output = & $script -EvidenceRootOverride $fixtureRoot -RequireEvidenceFor UiE2E 2>&1
if ($LASTEXITCODE -eq 0) { throw 'invalid UI evidence was accepted' }
$validator = Join-Path $PSScriptRoot 'validate_e2e_evidence.ps1'
$validatorOutput = & $validator -EvidenceRoot $fixtureRoot -Pattern 'ui-e2e*.json' 2>&1
if ($LASTEXITCODE -eq 0 -or ($validatorOutput -join "`n") -notmatch 'FAIL|process-liveness-only|expected/observed') { throw 'invalid evidence failure was not reported' }
Write-Output 'AUDIT_INVALID_EVIDENCE_RED_TEST_PASS'
exit 0
