[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $PSScriptRoot 'audit_current_state.ps1'
$fixtureRoot = Join-Path $env:TEMP 'QuickImageView-audit-invalid-evidence'
if (Test-Path -LiteralPath $fixtureRoot) { Remove-Item -LiteralPath $fixtureRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $fixtureRoot | Out-Null

try {
    @{
        target_operation = 'Open image and zoom';
        expected_result = 'Image is displayed and zoom changes';
        observed_result = 'process alive';
        target_exe = 'build/QuickImageView.exe';
        target_image = 'fixture.png';
        recorded_at = '2026-08-21T08:00:00Z';
        evidence_references = @('fixture.mp4')
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $fixtureRoot 'ui-e2e-invalid.json') -Encoding utf8
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $script -EvidenceRootOverride $fixtureRoot -RequireEvidenceFor UiE2E 2>&1
    if ($LASTEXITCODE -eq 0) { throw 'invalid UI evidence was accepted' }
    if (($output -join "`n") -notmatch 'FAIL|process-liveness-only|expected_result') { throw 'invalid evidence failure was not reported' }
    Write-Output 'AUDIT_INVALID_EVIDENCE_RED_TEST_PASS'
    exit 0
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) { Remove-Item -LiteralPath $fixtureRoot -Recurse -Force }
}
