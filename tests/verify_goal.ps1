[CmdletBinding()]
param([switch]$SkipUi)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$results = [System.Collections.Generic.List[object]]::new()

function Add-Result([string]$Id, [bool]$Pass, [string]$Detail) {
    $results.Add([ordered]@{ id = $Id; status = if ($Pass) { 'PASS' } else { 'FAIL' }; detail = $Detail })
}

function Run-Command([string]$Id, [string]$File, [string[]]$Arguments) {
    try {
        $output = @(& $File @Arguments 2>&1 | Out-String)
        $code = $LASTEXITCODE
        Add-Result $Id ($code -eq 0) ("exit_code={0}; {1}" -f $code, $output.Trim())
    } catch {
        Add-Result $Id $false $_.Exception.Message
    }
}

function New-TestBmp([string]$Path) {
    $bytes = [Convert]::FromBase64String('Qk06AAAAAAAAADYAAAAoAAAAAQAAAAEAAAABACAAAAAAAAAAAADEDgAAxA4AAAAAAAAAAAAAAAD//w==')
    [IO.File]::WriteAllBytes($Path, $bytes)
}

try {
    $goal = Get-Content -Raw (Join-Path $repo 'docs\loop\goal.json') | ConvertFrom-Json
    $ids = @($goal.acceptance | ForEach-Object id)
    Add-Result 'goal_definition' ($goal.schema_version -eq 1 -and $ids.Count -gt 0 -and (($ids | Sort-Object -Unique).Count -eq $ids.Count)) 'schema and unique acceptance IDs'
} catch {
    Add-Result 'goal_definition' $false $_.Exception.Message
}

$checklist = $null
$manager = $null
try {
    $checklist = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\checklist.json') | ConvertFrom-Json
    $manager = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\manager.json') | ConvertFrom-Json
    $checklistSchemaPass = $checklist.schema_version -eq 1 -and @($checklist.items).Count -gt 0
    $managerContractPass = $manager.schema_version -eq 1 -and $manager.role -eq 'requirements_and_release_manager' -and
        -not [string]::IsNullOrWhiteSpace($manager.authority) -and -not [string]::IsNullOrWhiteSpace($manager.release_gate)
    Add-Result 'checklist_schema' $checklistSchemaPass 'Checklist schema and non-empty item set'
    Add-Result 'manager_contract' $managerContractPass 'Manager contract schema'
} catch {
    Add-Result 'checklist_schema' $false $_.Exception.Message
    Add-Result 'manager_contract' $false $_.Exception.Message
}

Run-Command 'build_configure' 'cmake' @('-S', $repo, '-B', (Join-Path $repo 'build'), '-G', 'MinGW Makefiles')
Run-Command 'build' 'cmake' @('--build', (Join-Path $repo 'build'), '--clean-first')

$exe = Join-Path $repo 'build\QuickImageView.exe'
if (Test-Path $exe) { Run-Command 'self_test' $exe @('--self-test') } else { Add-Result 'self_test' $false 'build executable is missing' }
Run-Command 'ctest' 'ctest' @('--test-dir', (Join-Path $repo 'build'), '--output-on-failure')
if ($SkipUi) {
    Add-Result 'ui_dynamic' $false 'UI検査を省略（監査モード）'
} else {
    Run-Command 'ui_dynamic' 'powershell' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $repo 'tests\verify_ui.ps1'))
}

$source = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'core\native\main.cpp')

$invariantsPass = $true
try {
    $invariants = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'docs\loop\invariants.json') | ConvertFrom-Json
    $probeSource = $source + (Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'scripts\install.ps1')) + (Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'scripts\uninstall.ps1'))
    $invariantIds = @($invariants.invariants | ForEach-Object id)
    if ($invariants.schema_version -ne 1 -or $invariantIds.Count -eq 0 -or (($invariantIds | Sort-Object -Unique).Count -ne $invariantIds.Count)) {
        $invariantsPass = $false
    }
    foreach ($invariant in $invariants.invariants) {
        $missing = @($invariant.source_probes | Where-Object { -not $probeSource.Contains([string]$_) })
        $pass = $missing.Count -eq 0
        $probeDetail = if ($pass) { 'all source probes present' } else { 'missing: ' + ($missing -join ', ') }
        Add-Result ('invariant_' + $invariant.id) $pass $probeDetail
        if (-not $pass) { $invariantsPass = $false }
    }
} catch {
    $invariantsPass = $false
    Add-Result 'invariant_schema' $false $_.Exception.Message
}
Add-Result 'regression_invariants' $invariantsPass 'Existing feature preservation contract'

$conversionTemp = Join-Path ([IO.Path]::GetTempPath()) ('QuickImageView-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $conversionTemp | Out-Null
    $input = Join-Path $conversionTemp 'input.bmp'
    $output = Join-Path $conversionTemp 'output.png'
    New-TestBmp $input
    $before = (Get-FileHash -LiteralPath $input -Algorithm SHA256).Hash
    & $exe '--self-test-edit' $input 2>&1 | Out-Null
    $editSelfTestCode = $LASTEXITCODE
    Add-Result 'edit_dynamic' ($editSelfTestCode -eq 0) ("edit_self_test_exit={0}" -f $editSelfTestCode)
    & $exe '--resize-test' $input '2' '3' 2>&1 | Out-Null
    $resizeTestCode = $LASTEXITCODE
    Add-Result 'resize_dimensions_dynamic' ($resizeTestCode -eq 0) ("resize_test_exit={0}; target=2x3" -f $resizeTestCode)
    & $exe '--convert' $input $output 2>&1 | Out-Null
    $convertCode = $LASTEXITCODE
    $after = (Get-FileHash -LiteralPath $input -Algorithm SHA256).Hash
    $created = Test-Path -LiteralPath $output -PathType Leaf
    & $exe '--convert' $input $input 2>&1 | Out-Null
    $samePathOutput = $LASTEXITCODE
    & $exe '--convert' $input $output 2>&1 | Out-Null
    $existingOutput = $LASTEXITCODE
    $conversionPass = $convertCode -eq 0 -and $created -and $before -eq $after -and $samePathOutput -ne 0 -and $existingOutput -ne 0
    Add-Result 'conversion_cli' $conversionPass ("convert={0}; created={1}; source_unchanged={2}; same_path_exit={3}; existing_exit={4}" -f $convertCode, $created, ($before -eq $after), $samePathOutput, $existingOutput)

    $formatResults = @()
    foreach ($extension in @('jpg', 'png', 'tif', 'bmp', 'gif')) {
        $formatOutput = Join-Path $conversionTemp ('format.' + $extension)
        & $exe '--convert' $input $formatOutput 2>&1 | Out-Null
        $convertExit = $LASTEXITCODE
        $exists = Test-Path -LiteralPath $formatOutput -PathType Leaf
        $decodeExit = if ($exists) { & $exe '--self-test-edit' $formatOutput 2>&1 | Out-Null; $LASTEXITCODE } else { 2 }
        $formatResults += [ordered]@{ format = $extension; exit_code = $convertExit; exists = $exists; decode_exit_code = $decodeExit }
    }
    $formatPass = @($formatResults | Where-Object { $_.exit_code -ne 0 -or -not $_.exists -or $_.decode_exit_code -ne 0 }).Count -eq 0
    Add-Result 'format_matrix' $formatPass (($formatResults | ConvertTo-Json -Compress))

    $installTarget = Join-Path $env:LOCALAPPDATA ('QuickImageView-verify-' + [guid]::NewGuid().ToString('N'))
    $contextMenuName = 'QuickImageViewVerify-' + [guid]::NewGuid().ToString('N')
    $keyPath = 'HKCU:\Software\Classes\SystemFileAssociations\image\shell\' + $contextMenuName
    $installScript = Join-Path $repo 'scripts\install.ps1'
    $uninstallScript = Join-Path $repo 'scripts\uninstall.ps1'
    & powershell -NoProfile -ExecutionPolicy Bypass -File $installScript -SourceExe $exe -InstallDirectory $installTarget -ContextMenuName $contextMenuName 2>&1 | Out-Null
    $installCode = $LASTEXITCODE
    $command = ''
    if (Test-Path -LiteralPath (Join-Path $keyPath 'command')) {
        $command = [string](Get-Item -LiteralPath (Join-Path $keyPath 'command')).GetValue('')
    }
    $commandPass = $command -eq ('"' + (Join-Path $installTarget 'QuickImageView.exe') + '" "%1"')
    $installedHash = if (Test-Path -LiteralPath (Join-Path $installTarget 'QuickImageView.exe')) { (Get-FileHash (Join-Path $installTarget 'QuickImageView.exe') -Algorithm SHA256).Hash } else { '' }
    $buildHash = if (Test-Path -LiteralPath $exe) { (Get-FileHash $exe -Algorithm SHA256).Hash } else { '' }
    Add-Result 'installed_parity' ($installedHash -ne '' -and $installedHash -eq $buildHash) 'Installed executable matches the tested build'
    Run-Command 'installed_ui_dynamic' 'powershell' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $repo 'tests\verify_ui.ps1'), '-ExePath', (Join-Path $installTarget 'QuickImageView.exe'))
    & powershell -NoProfile -ExecutionPolicy Bypass -File $uninstallScript -InstallDirectory $installTarget -ContextMenuName $contextMenuName 2>&1 | Out-Null
    $uninstallCode = $LASTEXITCODE
    $cleanupPass = -not (Test-Path -LiteralPath $keyPath) -and -not (Test-Path -LiteralPath $installTarget)
    Add-Result 'context_menu_install' ($installCode -eq 0 -and $commandPass -and $uninstallCode -eq 0 -and $cleanupPass) ("install={0}; command_ok={1}; uninstall={2}; cleanup_ok={3}" -f $installCode, $commandPass, $uninstallCode, $cleanupPass)
} catch {
    Add-Result 'edit_dynamic' $false $_.Exception.Message
    Add-Result 'conversion_cli' $false $_.Exception.Message
    Add-Result 'format_matrix' $false $_.Exception.Message
    Add-Result 'context_menu_install' $false $_.Exception.Message
} finally {
    if (Test-Path -LiteralPath $conversionTemp) { Remove-Item -LiteralPath $conversionTemp -Recurse -Force }
}

Add-Result 'source_safety' ($source.Contains('SamePath') -and $source.Contains('CREATE_NEW')) 'SamePath and CREATE_NEW guards'

$scriptNames = @('install.ps1', 'uninstall.ps1', 'package.ps1')
$scriptPass = @($scriptNames | Where-Object { -not (Test-Path (Join-Path $repo ('scripts\' + $_))) }).Count -eq 0
Add-Result 'distribution_scripts' $scriptPass ($scriptNames -join ', ')

$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$runtimePass = $cmake.Contains('-static') -and $cmake.Contains('-static-libgcc') -and $cmake.Contains('-static-libstdc++')
Add-Result 'runtime_independence' $runtimePass 'MinGW static runtime flags'

$saveFilterPass = $source.Contains('lpstrFilter') -and $source.Contains('nFilterIndex') -and
    $source.Contains('DefaultExtensionForSaveFilter') -and $source.Contains('HasFileExtension')
Add-Result 'save_format_filters' $saveFilterPass 'Separate save filters and selected-format extension fallback'

Add-Result 'resize' ($source.Contains('ResizeCurrentImage') -and $source.Contains('kCommandResize50') -and $source.Contains('ShowResizeDialog') -and $source.Contains('percent') -and $source.Contains('width')) 'Preset and custom percent/pixel resize operations'
Add-Result 'crop' ($source.Contains('CropCurrentImage') -and $source.Contains('IWICBitmapClipper') -and $source.Contains('WM_LBUTTONDOWN') -and $source.Contains('g_selectionActive')) 'Left-drag crop selection'
Add-Result 'rotate_flip' ($source.Contains('WICBitmapTransformRotate90') -and $source.Contains('WICBitmapTransformFlipHorizontal')) 'Rotation and flip operations'
$jpegQualityPass = $source.Contains('g_jpegQuality') -and $source.Contains('ImageQuality') -and
    $source.Contains('kCommandQualityCustom') -and $source.Contains('QualityDialogProc') -and
    $source.Contains('ShowQualityDialog') -and $source.Contains('IDD_QUALITY_DIALOG')
Add-Result 'jpeg_quality' $jpegQualityPass 'User-specified JPEG quality dialog and encoder property'
Add-Result 'color_conversion' ($source.Contains('ConvertColorCurrentImage') -and $source.Contains('kCommandColorFull') -and $source.Contains('GUID_WICPixelFormat8bppIndexed') -and $source.Contains('GUID_WICPixelFormat8bppGray')) 'Full color, 256-color, and grayscale conversion'
Add-Result 'undo_redo' ($source.Contains('g_undoStack') -and $source.Contains('g_redoStack') -and $source.Contains('UndoImage') -and $source.Contains('RedoImage')) 'Undo and redo snapshots'
Add-Result 'clipboard_image' ($source.Contains('CopyImageToClipboard') -and $source.Contains('PasteImageFromClipboard') -and $source.Contains('CF_BITMAP')) 'Image clipboard copy and paste'
Add-Result 'webp_heic_conversion' ($source.Contains('GUID_ContainerFormatWebp') -and $source.Contains('GUID_ContainerFormatHeif') -and $source.Contains('webp') -and $source.Contains('heic')) 'WebP and HEIC/HEIF WIC routes'
Add-Result 'compression_settings' ($source.Contains('g_compressionLevel') -and $source.Contains('CompressionLevel') -and $source.Contains('ImageQuality')) 'Conversion compression and quality settings'
Add-Result 'undo_redo_shortcuts' ($source.Contains("wParam == 'Z'") -and $source.Contains("wParam == 'Y'") -and -not $source.Contains('L"Undo"') -and -not $source.Contains('L"Redo"')) 'Undo/Redo shortcuts without menu entries'
Add-Result 'metadata_dynamic' ($source.Contains('MetadataText') -and $source.Contains('ReadExif') -and $source.Contains('ExifText') -and $source.Contains('g_fileSize')) 'File information and EXIF display regression'
Add-Result 'scope_exclusions' (-not $source.Contains('NavigateSibling') -and -not $source.Contains('BuildSiblingList') -and -not $source.Contains('StartSlideshow') -and -not $source.Contains('PrintDlg')) 'Excluded features remain excluded'
$specJp = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'document\spec_jp.md')
$testingJp = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'document\testing_jp.md')
$readme = Get-Content -Raw -Encoding UTF8 (Join-Path $repo 'README.md')
Add-Result 'spec_consistency' ($specJp.Contains('EXIF') -and $testingJp.Contains('verify_goal.ps1') -and $readme.Contains('WebP')) 'Specification and README contract'
$dashboardPath = Join-Path $repo 'docs\loop\dashboard.html'
$dashboardRaw = ''
if (Test-Path -LiteralPath $dashboardPath) { $dashboardRaw = Get-Content -Raw -Encoding UTF8 $dashboardPath }
Add-Result 'dashboard_contract' ($dashboardRaw.Contains('QuickImageView') -and $dashboardRaw.Contains('<table') -and $dashboardRaw.Contains('manager')) 'Static dashboard exists with current verification sections'
$aspectRatioPass = $source.Contains('IDC_RESIZE_LOCK') -and $source.Contains('keepAspectRatio') -and $source.Contains('BST_CHECKED')
Add-Result 'aspect_ratio_dynamic' $aspectRatioPass 'Resize aspect-ratio lock option and default state'
$smoothPass = $source.Contains('CreateCompatibleBitmap') -and $source.Contains('BitBlt') -and $source.Contains('WM_ERASEBKGND')
Add-Result 'smooth_rendering_dynamic' $smoothPass 'Double-buffered pan, zoom, and selection rendering'
$saveOptionsPosition = $source.IndexOf('ShowSaveOptionsDialog')
$saveDialogPosition = $source.IndexOf('GetSaveFileNameW')
$saveOptionsPass = $source.Contains('IDD_SAVE_OPTIONS_DIALOG') -and $saveOptionsPosition -ge 0 -and $saveDialogPosition -gt $saveOptionsPosition
Add-Result 'save_options_dynamic' $saveOptionsPass 'Save options dialog precedes destination folder selection'

$knownResultIds = @($results | ForEach-Object id) + 'checklist_completeness'
$checklistComplete = $true
if ($checklist) {
    $checklistIds = @($checklist.items | ForEach-Object id)
    if ($checklistIds.Count -eq 0 -or (($checklistIds | Sort-Object -Unique).Count -ne $checklistIds.Count)) { $checklistComplete = $false }
    foreach ($item in $checklist.items) {
        if ([string]::IsNullOrWhiteSpace($item.id) -or [string]::IsNullOrWhiteSpace($item.source)) { $checklistComplete = $false; continue }
        $linkedTests = @($item.test_ids)
        if ($item.status -eq 'required' -and $linkedTests.Count -eq 0) { $checklistComplete = $false }
        foreach ($testId in $linkedTests) {
            if ($knownResultIds -notcontains [string]$testId) { $checklistComplete = $false }
        }
    }
} else { $checklistComplete = $false }
Add-Result 'checklist_completeness' $checklistComplete 'Every checklist item has source, status, and executable test links'

$failed = @($results | Where-Object status -eq 'FAIL')
[ordered]@{ status = if ($failed.Count -eq 0) { 'PASS' } else { 'FAIL' }; failed = @($failed | ForEach-Object id); checks = @($results) } | ConvertTo-Json -Depth 8
exit $(if ($failed.Count -eq 0) { 0 } else { 1 })
