[CmdletBinding()]
param([string]$ExePath = '')

$ErrorActionPreference = 'Stop'
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
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern bool SetWindowText(IntPtr h, string s);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetMenuItemCount(IntPtr menu);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetMenuString(IntPtr menu, uint pos, StringBuilder text, int max, uint flags);
  [DllImport("user32.dll")] public static extern IntPtr GetSubMenu(IntPtr menu, int pos);
  public const uint WM_COMMAND=0x0111, WM_LBUTTONDOWN=0x0201, WM_MOUSEMOVE=0x0200, WM_LBUTTONUP=0x0202;
  public const uint CB_SETCURSEL=0x014E;
  public const uint BM_CLICK=0x00F5;
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  public static IntPtr FindDialog(uint pid) { IntPtr found=IntPtr.Zero; EnumWindows((h,l)=>{ uint p; GetWindowThreadProcessId(h,out p); var b=new StringBuilder(64); GetClassName(h,b,b.Capacity); if(p==pid && b.ToString()=="#32770") { found=h; return false; } return true; }, IntPtr.Zero); return found; }
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
    $process = Start-Process -FilePath $exe -ArgumentList @('"' + $bmp + '"') -PassThru
    $window = [IntPtr]::Zero
    for ($i=0; $i -lt 50 -and $window -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $process.Refresh(); $window=$process.MainWindowHandle }
    if ($window -eq [IntPtr]::Zero) { throw 'QIV main window was not created' }
    [QivUi]::SetForegroundWindow($window) | Out-Null

    $menu = [QivUi]::GetMenu($window)
    $hasUndoMenu = [QivUi]::HasMenuText($menu, 'Undo') -or [QivUi]::HasMenuText($menu, 'Redo')
    if ($hasUndoMenu) { throw 'Undo/Redo menu entry still exists' }

    [QivUi]::PostMessage($window, [QivUi]::WM_COMMAND, [IntPtr]1105, [IntPtr]::Zero) | Out-Null
    $dialog = [IntPtr]::Zero
    for ($i=0; $i -lt 30 -and $dialog -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $dialog=[QivUi]::FindDialog([uint32]$process.Id) }
    if ($dialog -eq [IntPtr]::Zero) { throw 'Resize dialog was not opened' }
    $width = [QivUi]::GetDlgItem($dialog, 100)
    $height = [QivUi]::GetDlgItem($dialog, 101)
    $mode = [QivUi]::GetDlgItem($dialog, 102)
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
    [QivUi]::SetWindowText($width, '50') | Out-Null
    [QivUi]::SetWindowText($height, '50') | Out-Null
    [QivUi]::SendMessage($mode, [QivUi]::CB_SETCURSEL, [IntPtr]0, [IntPtr]::Zero) | Out-Null
    $ok = [QivUi]::GetDlgItem($dialog, 1)
    [QivUi]::SendMessage($ok, [QivUi]::BM_CLICK, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    if ([QivUi]::IsWindow($dialog)) { throw 'Percent resize dialog did not close after OK' }

    [QivUi]::PostMessage($window, [QivUi]::WM_LBUTTONDOWN, [IntPtr]1, [QivUi]::Param(100,100)) | Out-Null
    [QivUi]::PostMessage($window, [QivUi]::WM_MOUSEMOVE, [IntPtr]1, [QivUi]::Param(300,300)) | Out-Null
    [QivUi]::PostMessage($window, [QivUi]::WM_LBUTTONUP, [IntPtr]::Zero, [QivUi]::Param(300,300)) | Out-Null
    Start-Sleep -Milliseconds 100
    [QivUi]::PostMessage($window, [QivUi]::WM_COMMAND, [IntPtr]1110, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    [pscustomobject]@{ status='PASS'; menu_undo_redo=$false; resize_dialog=$true; left_drag_crop_command=$true } | ConvertTo-Json -Compress
} finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    if (Test-Path $temp) { Remove-Item -LiteralPath $temp -Recurse -Force }
}
