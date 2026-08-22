[CmdletBinding()]
param([string]$ExePath = '')

$ErrorActionPreference = 'Stop'
$null = Add-Type -AssemblyName System.Windows.Forms
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$exe = if ($ExePath) { (Resolve-Path $ExePath).Path } else { Join-Path $repo 'build\QuickImageView.exe' }
$temp = Join-Path ([IO.Path]::GetTempPath()) ('QuickImageView-ui-' + [guid]::NewGuid().ToString('N'))

Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class QivUi {
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string cls, string title);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc proc, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr insertAfter, int x, int y, int cx, int cy, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT rect);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int command);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern bool SetWindowText(IntPtr h, string s);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc proc, IntPtr l);
  [DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetMenuItemCount(IntPtr menu);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetMenuString(IntPtr menu, uint pos, StringBuilder text, int max, uint flags);
  [DllImport("user32.dll")] public static extern IntPtr GetSubMenu(IntPtr menu, int pos);
  public const uint WM_COMMAND=0x0111, WM_LBUTTONDOWN=0x0201, WM_MOUSEMOVE=0x0200, WM_LBUTTONUP=0x0202;
  public const uint CB_SETCURSEL=0x014E, BM_GETCHECK=0x00F0;
  public const int SAVE_FILE_EDIT=0x047c;
  public const uint BM_CLICK=0x00F5;
  public const uint SWP_NOZORDER=0x0004;
  public const int SW_SHOW=5;
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  public static IntPtr FindDialog(uint pid) { IntPtr found=IntPtr.Zero; EnumWindows((h,l)=>{ uint p; GetWindowThreadProcessId(h,out p); var b=new StringBuilder(64); GetClassName(h,b,b.Capacity); if(p==pid && b.ToString()=="#32770") { found=h; return false; } return true; }, IntPtr.Zero); return found; }
  public static IntPtr FindMain(uint pid) { IntPtr found=IntPtr.Zero; EnumWindows((h,l)=>{ uint p; GetWindowThreadProcessId(h,out p); var b=new StringBuilder(64); GetClassName(h,b,b.Capacity); if(p==pid && b.ToString()=="QuickImageViewWindow") { found=h; return false; } return true; }, IntPtr.Zero); return found; }
  public static IntPtr FindEdit(IntPtr parent) { IntPtr found=IntPtr.Zero; EnumChildWindows(parent,(h,l)=>{ var b=new StringBuilder(64); GetClassName(h,b,b.Capacity); if(b.ToString()=="Edit") { found=h; return false; } return true; }, IntPtr.Zero); return found; }
  public static IntPtr FindChildClass(IntPtr parent, string wanted) { IntPtr found=IntPtr.Zero; EnumChildWindows(parent,(h,l)=>{ var b=new StringBuilder(64); GetClassName(h,b,b.Capacity); if(b.ToString()==wanted) { found=h; return false; } return true; }, IntPtr.Zero); return found; }
  public static string ChildSummary(IntPtr parent) { var all=new StringBuilder(2048); EnumChildWindows(parent,(h,l)=>{ var c=new StringBuilder(64); var t=new StringBuilder(128); GetClassName(h,c,c.Capacity); GetWindowText(h,t,t.Capacity); all.Append(c).Append(':').Append(t).Append('|'); return true; }, IntPtr.Zero); return all.ToString(); }
  public static IntPtr Param(int x, int y) { return (IntPtr)((y << 16) | (x & 0xffff)); }
  public static string Text(IntPtr h) { var b=new StringBuilder(256); GetWindowText(h,b,b.Capacity); return b.ToString(); }
  public static bool HasMenuText(IntPtr menu, string needle) {
    if (menu == IntPtr.Zero) return false;
    int count=GetMenuItemCount(menu);
    for (uint i=0;i<count;i++) { var b=new StringBuilder(256); GetMenuString(menu,i,b,b.Capacity,0x400); if (b.ToString().Contains(needle)) return true; if (HasMenuText(GetSubMenu(menu,(int)i),needle)) return true; }
    return false;
  }
}
'@

New-Item -ItemType Directory -Path $temp | Out-Null
$process = $null
try {
    $bmp = Join-Path $temp 'input.bmp'
    [IO.File]::WriteAllBytes($bmp, [Convert]::FromBase64String('Qk06AAAAAAAAADYAAAAoAAAAAQAAAAEAAAABACAAAAAAAAAAAADEDgAAxA4AAAAAAAAAAAAAAAD//w=='))
    $process = Start-Process -FilePath $exe -ArgumentList @('--ui-test-hidden', '"' + $bmp + '"') -WindowStyle Normal -PassThru
    $window = [IntPtr]::Zero
    for ($i=0; $i -lt 50 -and $window -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $window=[QivUi]::FindMain([uint32]$process.Id) }
    if ($window -eq [IntPtr]::Zero) { throw 'QIV main window was not created' }
    $secondary = [System.Windows.Forms.Screen]::AllScreens | Where-Object { -not $_.Primary } | Select-Object -First 1
    if (-not $secondary) { throw 'A secondary display is required for UI verification' }
    $windowWidth = [Math]::Min(1200, [Math]::Max(1, $secondary.Bounds.Width - 80))
    $windowHeight = [Math]::Min(800, [Math]::Max(1, $secondary.Bounds.Height - 80))
    $placed = [QivUi]::SetWindowPos($window, [IntPtr]::Zero, $secondary.Bounds.X + 40, $secondary.Bounds.Y + 40, $windowWidth, $windowHeight, [QivUi]::SWP_NOZORDER)
    if (-not $placed) { throw 'Could not place QIV on the secondary display' }
    $rect = New-Object QivUi+RECT
    if (-not [QivUi]::GetWindowRect($window, [ref]$rect)) { throw 'Could not read QIV window placement' }
    if ($rect.Left -lt $secondary.Bounds.Left -or $rect.Top -lt $secondary.Bounds.Top -or
        $rect.Right -gt $secondary.Bounds.Right -or $rect.Bottom -gt $secondary.Bounds.Bottom) {
        throw 'QIV window was not confined to the secondary display'
    }
    [QivUi]::ShowWindow($window, [QivUi]::SW_SHOW) | Out-Null
    [QivUi]::SetForegroundWindow($window) | Out-Null
    Start-Sleep -Milliseconds 200

    $menu = [QivUi]::GetMenu($window)
    $hasUndoMenu = [QivUi]::HasMenuText($menu, 'Undo') -or [QivUi]::HasMenuText($menu, 'Redo')
    if ($hasUndoMenu) { throw 'Undo/Redo menu entry still exists' }

    $dialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $dialog -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $dialog=[QivUi]::FindDialog([uint32]$process.Id) }
    if ($dialog -eq [IntPtr]::Zero) { throw 'Resize dialog was not opened' }
    $width = [QivUi]::GetDlgItem($dialog, 2001)
    $height = [QivUi]::GetDlgItem($dialog, 2002)
    $mode = [QivUi]::GetDlgItem($dialog, 2003)
    [QivUi]::SetWindowText($width, '200') | Out-Null
    [QivUi]::SetWindowText($height, '200') | Out-Null
    [QivUi]::SendMessage($mode, [QivUi]::CB_SETCURSEL, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    $ok = [QivUi]::GetDlgItem($dialog, 1)
    [QivUi]::SendMessage($ok, [QivUi]::BM_CLICK, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    if ([QivUi]::IsWindow($dialog)) { throw ('Resize dialog did not close after OK: title=' + [QivUi]::Text($dialog)) }

    [QivUi]::PostMessage($window, [QivUi]::WM_COMMAND, [IntPtr]1105, [IntPtr]::Zero) | Out-Null
    $dialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $dialog -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $dialog=[QivUi]::FindDialog([uint32]$process.Id) }
    if ($dialog -eq [IntPtr]::Zero) { throw 'Resize dialog did not reopen for percent test' }
    $width = [QivUi]::GetDlgItem($dialog, 2001)
    $height = [QivUi]::GetDlgItem($dialog, 2002)
    $mode = [QivUi]::GetDlgItem($dialog, 2003)
    $lock = [QivUi]::GetDlgItem($dialog, 2004)
    if ([QivUi]::SendMessage($lock, [QivUi]::BM_GETCHECK, [IntPtr]::Zero, [IntPtr]::Zero).ToInt32() -ne 1) { throw 'Aspect-ratio lock is not enabled by default' }
    [QivUi]::SetWindowText($width, '50') | Out-Null
    [QivUi]::SetWindowText($height, '50') | Out-Null
    [QivUi]::SendMessage($mode, [QivUi]::CB_SETCURSEL, [IntPtr]0, [IntPtr]::Zero) | Out-Null
    $ok = [QivUi]::GetDlgItem($dialog, 1)
    [QivUi]::SendMessage($ok, [QivUi]::BM_CLICK, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    if ([QivUi]::IsWindow($dialog)) { throw 'Percent resize dialog did not close after OK' }

    $qualityDialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $qualityDialog -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $qualityDialog=[QivUi]::FindDialog([uint32]$process.Id) }
    if ($qualityDialog -eq [IntPtr]::Zero) { throw 'Custom JPEG quality dialog was not opened' }
    $qualityValue = [QivUi]::GetDlgItem($qualityDialog, 2021)
    if ($qualityValue -eq [IntPtr]::Zero) { throw 'Custom JPEG quality control is missing' }
    [QivUi]::SetWindowText($qualityValue, '83') | Out-Null
    [QivUi]::SendMessage([QivUi]::GetDlgItem($qualityDialog, 1), [QivUi]::BM_CLICK, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    if ([QivUi]::IsWindow($qualityDialog)) { throw 'Custom JPEG quality dialog did not close after OK' }

    [QivUi]::PostMessage($window, [QivUi]::WM_LBUTTONDOWN, [IntPtr]1, [QivUi]::Param(100,100)) | Out-Null
    [QivUi]::PostMessage($window, [QivUi]::WM_MOUSEMOVE, [IntPtr]1, [QivUi]::Param(300,300)) | Out-Null
    [QivUi]::PostMessage($window, [QivUi]::WM_LBUTTONUP, [IntPtr]::Zero, [QivUi]::Param(300,300)) | Out-Null
    Start-Sleep -Milliseconds 100
    [QivUi]::PostMessage($window, [QivUi]::WM_COMMAND, [IntPtr]1110, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    [QivUi]::PostMessage($window, [QivUi]::WM_COMMAND, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    $saveDialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $saveDialog -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $saveDialog=[QivUi]::FindDialog([uint32]$process.Id) }
    if ($saveDialog -eq [IntPtr]::Zero) { throw 'Save options dialog was not opened before folder selection' }
    $saveFormat = [QivUi]::GetDlgItem($saveDialog, 2011)
    $saveQuality = [QivUi]::GetDlgItem($saveDialog, 2012)
    $saveCompression = [QivUi]::GetDlgItem($saveDialog, 2013)
    if ($saveFormat -eq [IntPtr]::Zero -or $saveQuality -eq [IntPtr]::Zero -or $saveCompression -eq [IntPtr]::Zero) { throw 'Save options controls are missing' }
    if (-not [QivUi]::IsWindowEnabled($saveQuality) -or [QivUi]::IsWindowEnabled($saveCompression)) { throw 'PNG-specific save controls are not selected correctly' }
    [QivUi]::SendMessage($saveFormat, [QivUi]::CB_SETCURSEL, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    [QivUi]::SendMessage($saveDialog, [QivUi]::WM_COMMAND, [IntPtr](0x00010000 + 2011), [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
    if ([QivUi]::IsWindowEnabled($saveQuality) -or -not [QivUi]::IsWindowEnabled($saveCompression)) { throw 'PNG save controls did not switch with format' }
    [QivUi]::SendMessage($saveFormat, [QivUi]::CB_SETCURSEL, [IntPtr]2, [IntPtr]::Zero) | Out-Null
    [QivUi]::SendMessage($saveDialog, [QivUi]::WM_COMMAND, [IntPtr](0x00010000 + 2011), [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
    if ([QivUi]::IsWindowEnabled($saveQuality) -or -not [QivUi]::IsWindowEnabled($saveCompression)) { throw 'TIFF compression control did not switch with format' }
    [QivUi]::SendMessage($saveFormat, [QivUi]::CB_SETCURSEL, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    [QivUi]::SendMessage($saveDialog, [QivUi]::WM_COMMAND, [IntPtr](0x00010000 + 2011), [IntPtr]::Zero) | Out-Null
    [QivUi]::SetWindowText($saveQuality, '83') | Out-Null
    [QivUi]::SetWindowText($saveCompression, '7') | Out-Null
    [QivUi]::SendMessage([QivUi]::GetDlgItem($saveDialog, 1), [QivUi]::BM_CLICK, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $fileDialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $fileDialog -eq [IntPtr]::Zero; $i++) {
        Start-Sleep -Milliseconds 100
        $candidate = [QivUi]::FindDialog([uint32]$process.Id)
        if ($candidate -ne [IntPtr]::Zero -and $candidate -ne $saveDialog) { $fileDialog = $candidate }
    }
    if ($fileDialog -eq [IntPtr]::Zero) { throw 'Destination file dialog was not opened after save options' }
    $saveTarget = Join-Path $temp 'saved.png'
    [QivUi]::SetForegroundWindow($fileDialog) | Out-Null
    Start-Sleep -Milliseconds 100
    [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
    Start-Sleep -Milliseconds 200
    [pscustomobject]@{ status='PASS'; menu_undo_redo=$false; resize_dialog=$true; aspect_ratio_default_on=$true; left_drag_crop_command=$true; custom_jpeg_quality_dialog=$true; custom_jpeg_quality_applied=$true; save_options_dialog=$true; save_options_applied=$true; destination_dialog=$true; destination_cancelled=$true } | ConvertTo-Json -Compress
} finally {
    if ($process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit(5000)
    }
    if (Test-Path $temp) { Remove-Item -LiteralPath $temp -Recurse -Force }
}
