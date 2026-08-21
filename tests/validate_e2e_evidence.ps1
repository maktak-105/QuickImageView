[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EvidenceRoot,
    [Parameter(Mandatory=$true)][string]$Pattern
)

$ErrorActionPreference = 'Stop'
$files = @(Get-ChildItem -LiteralPath $EvidenceRoot -Filter $Pattern -File -ErrorAction SilentlyContinue)
if ($files.Count -eq 0) {
    Write-Output 'BLOCKED: no evidence JSON found'
    exit 2
}

$errors = @()
foreach ($file in $files) {
    try { $record = Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json }
    catch { $errors += "$($file.Name): invalid JSON"; continue }

    $required = @('target_operation','expected_result','observed_result','target_exe','target_image','recorded_at','evidence_references')
    foreach ($field in $required) {
        if ($null -eq $record.PSObject.Properties[$field] -or [string]::IsNullOrWhiteSpace([string]$record.PSObject.Properties[$field].Value)) {
            $errors += "$($file.Name): missing $field"
        }
    }
    if ($null -ne $record.PSObject.Properties['expected_result'] -and $null -ne $record.PSObject.Properties['observed_result']) {
        if (([string]$record.expected_result).Trim() -ne ([string]$record.observed_result).Trim()) { $errors += "$($file.Name): expected/observed mismatch" }
        $observedLower = ([string]$record.observed_result).ToLowerInvariant()
        if ($observedLower.Contains('process alive') -or $observedLower.Contains('process exists') -or $observedLower.Contains('process running')) { $errors += "$($file.Name): process-liveness-only" }
    }
    if ($null -ne $record.PSObject.Properties['recorded_at']) {
        $parsed = [DateTimeOffset]::MinValue
        if (-not [DateTimeOffset]::TryParse([string]$record.recorded_at, [ref]$parsed)) { $errors += "$($file.Name): invalid recorded_at" }
    }
    if ($null -ne $record.PSObject.Properties['evidence_references']) {
        $references = @($record.evidence_references)
        if ($references.Count -eq 0) { $errors += "$($file.Name): empty evidence_references" }
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Output "FAIL: $_" }
    exit 1
}
Write-Output 'PASS: evidence schema and content valid'
exit 0
