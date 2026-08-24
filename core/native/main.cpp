#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <webp/encode.h>
#include "resource.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cwchar>
#include <cstring>
#include <list>
#include <string>
#include <vector>

namespace {
constexpr wchar_t kClassName[] = L"QuickImageViewWindow";
constexpr UINT kCommandOpen = 1001;
constexpr UINT kCommandExit = 1002;
constexpr UINT kCommandResizeCustom = 1105;
constexpr UINT kCommandCrop = 1110;
constexpr UINT kCommandRotate90 = 1120;
constexpr UINT kCommandRotate180 = 1121;
constexpr UINT kCommandRotate270 = 1122;
constexpr UINT kCommandFlipHorizontal = 1123;
constexpr UINT kCommandFlipVertical = 1124;
constexpr UINT kCommandQuality50 = 1140;
constexpr UINT kCommandQuality75 = 1141;
constexpr UINT kCommandQuality90 = 1142;
constexpr UINT kCommandQualityCustom = 1143;
constexpr UINT kCommandColorFull = 1150;
constexpr UINT kCommandColor256 = 1151;
constexpr UINT kCommandColorGray = 1152;
constexpr UINT kCommandUndo = 1160;
constexpr UINT kCommandRedo = 1161;
constexpr UINT kCommandClipboardCopy = 1170;
constexpr UINT kCommandClipboardPaste = 1171;
constexpr UINT kCommandCompression1 = 1180;
constexpr UINT kCommandCompression5 = 1181;
constexpr UINT kCommandCompression9 = 1182;
constexpr UINT kCommandPasteCommit = 1190;
constexpr UINT kCommandPasteRetry = 1191;
constexpr UINT kCommandToggleLanguage = 1200;
constexpr UINT kCommandCopyExif = 1210;
constexpr UINT kCommandOpenExif = 1211;
constexpr UINT kCommandHelp = 1220;
constexpr UINT kCommandAbout = 1221;
constexpr int kInfoBarHeight = 34;
constexpr int kStatusBarHeight = 34;
constexpr COLORREF kDarkBackground = RGB(18, 22, 24);
constexpr COLORREF kDarkSurface = RGB(28, 34, 37);
constexpr COLORREF kDarkText = RGB(242, 246, 247);
constexpr COLORREF kDarkAccent = RGB(48, 190, 160);

using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);

void EnableDarkTheme(HWND window);

void PaintDarkMenuBoundary(HWND window) {
    if (!window) return;
    RECT windowRect{};
    RECT clientRect{};
    if (!GetWindowRect(window, &windowRect) || !GetClientRect(window, &clientRect)) return;
    POINT clientOrigin{0, 0};
    ClientToScreen(window, &clientOrigin);
    const int clientTop = clientOrigin.y - windowRect.top;
    if (clientTop <= 0) return;
    HDC dc = GetWindowDC(window);
    if (!dc) return;
    RECT boundary{0, clientTop - 1, windowRect.right - windowRect.left, clientTop};
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &boundary, black);
    DeleteObject(black);
    ReleaseDC(window, dc);
}

void ApplyDarkTitleBar(HWND window) {
    if (!window) return;
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    auto setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (setAttribute) {
        constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
        const BOOL enabled = TRUE;
        setAttribute(window, kDwmwaUseImmersiveDarkMode, &enabled, sizeof(enabled));
    }
    FreeLibrary(dwm);
    SetWindowPos(window, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

BOOL CALLBACK FindSecondaryMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    MONITORINFO info{sizeof(MONITORINFO)};
    if (GetMonitorInfoW(monitor, &info) && !(info.dwFlags & MONITORINFOF_PRIMARY)) {
        *reinterpret_cast<RECT*>(data) = info.rcWork;
        return FALSE;
    }
    return TRUE;
}
HBITMAP g_bitmap = nullptr;
UINT g_imageWidth = 0;
UINT g_imageHeight = 0;
double g_zoom = 1.0;
int g_panX = 0;
int g_panY = 0;
bool g_panning = false;
bool g_panMoved = false;
POINT g_lastPanPoint{};
std::wstring g_status = L"画像ファイルを指定して起動してください。";
std::wstring g_notice;
std::wstring g_fileName;
std::wstring g_sourcePath;
std::wstring g_formatName;
std::wstring g_exifMake;
std::wstring g_exifModel;
std::wstring g_exifDateTime;
ULONGLONG g_fileSize = 0;
UINT g_jpegQuality = 90;
UINT g_compressionLevel = 5;
std::vector<HBITMAP> g_undoStack;
std::vector<HBITMAP> g_redoStack;
bool g_selecting = false;
bool g_selectionActive = false;
POINT g_selectionStart{};
POINT g_selectionEnd{};
HBITMAP g_pasteBitmap = nullptr;
HBITMAP g_internalClipboardBitmap = nullptr;
UINT g_pasteWidth = 0;
UINT g_pasteHeight = 0;
int g_pasteX = 0;
int g_pasteY = 0;
bool g_pasteMoving = false;
POINT g_pasteDragStart{};
POINT g_pasteStart{};
HFONT g_uiFont = nullptr;
std::list<std::wstring> g_menuLabels;
bool g_ignoreNextContextMenu = false;
bool g_englishUi = false;
HWND g_languageButton = nullptr;
HWND g_exifWindow = nullptr;
HWND g_exifText = nullptr;
HWND g_exifCopyButton = nullptr;
HWND g_helpWindow = nullptr;
HWND g_helpText = nullptr;
HWND g_aboutWindow = nullptr;
HWND g_aboutOwner = nullptr;
HBITMAP g_aboutBadge = nullptr;

enum class LoadResult { success, fileNotFound, accessDenied, unsupportedFormat, decodeFailed };
LoadResult LoadImageFile(const wchar_t* path);
void ReleaseImage();

std::wstring ReadMetadataString(IWICMetadataQueryReader* reader, const std::wstring& path) {
    if (!reader) return L"";
    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring result;
    if (SUCCEEDED(reader->GetMetadataByName(path.c_str(), &value))) {
        if (value.vt == VT_LPWSTR && value.pwszVal) result.assign(value.pwszVal);
        else if (value.vt == VT_BSTR && value.bstrVal) result.assign(value.bstrVal, SysStringLen(value.bstrVal));
        else if (value.vt == VT_LPSTR && value.pszVal) {
            const int length = MultiByteToWideChar(CP_ACP, 0, value.pszVal, -1, nullptr, 0);
            if (length > 1) {
                result.resize(length);
                MultiByteToWideChar(CP_ACP, 0, value.pszVal, -1, result.data(), length);
                result.resize(length - 1);
            }
        }
    }
    PropVariantClear(&value);
    if (result.size() > 128) result.resize(128);
    return result;
}

void ReadExif(IWICBitmapFrameDecode* frame) {
    g_exifMake.clear();
    g_exifModel.clear();
    g_exifDateTime.clear();
    IWICMetadataQueryReader* reader = nullptr;
    if (!frame || FAILED(frame->GetMetadataQueryReader(&reader))) return;
    const wchar_t* roots[] = {L"/app1/ifd/", L"/ifd/"};
    for (const wchar_t* root : roots) {
        if (g_exifMake.empty()) g_exifMake = ReadMetadataString(reader, std::wstring(root) + L"{ushort=271}");
        if (g_exifModel.empty()) g_exifModel = ReadMetadataString(reader, std::wstring(root) + L"{ushort=272}");
        if (g_exifDateTime.empty()) g_exifDateTime = ReadMetadataString(reader, std::wstring(root) + L"{ushort=36867}");
        if (g_exifDateTime.empty()) g_exifDateTime = ReadMetadataString(reader, std::wstring(root) + L"{ushort=306}");
    }
    reader->Release();
}

const wchar_t* Ui(const wchar_t* japanese, const wchar_t* english);
std::wstring LocalizeNotice(const wchar_t* japanese);

std::wstring MetadataText() {
    if (!g_bitmap) return L"";
    wchar_t buffer[256]{};
    swprintf_s(buffer, L"%s  |  %ux%u  |  %s  |  %llu KB", g_fileName.c_str(), g_imageWidth,
               g_imageHeight, g_formatName.c_str(), static_cast<unsigned long long>((g_fileSize + 1023) / 1024));
    return buffer;
}

std::wstring ExifText() {
    if (g_exifMake.empty() && g_exifModel.empty() && g_exifDateTime.empty()) return Ui(L"EXIF: なし", L"EXIF: none");
    std::wstring text = L"EXIF: ";
    if (!g_exifMake.empty()) text += Ui(L"メーカー=", L"Make=") + g_exifMake + L"  ";
    if (!g_exifModel.empty()) text += Ui(L"機種=", L"Model=") + g_exifModel + L"  ";
    if (!g_exifDateTime.empty()) text += Ui(L"撮影日時=", L"Taken=") + g_exifDateTime;
    return text;
}

void EnableDarkTheme(HWND window) {
    HMODULE theme = LoadLibraryW(L"uxtheme.dll");
    if (theme) {
        using SetPreferredAppMode = int(WINAPI*)(int);
        using AllowDarkModeForWindow = BOOL(WINAPI*)(HWND, BOOL);
        using FlushMenuThemes = void(WINAPI*)();
        auto setPreferredAppMode = reinterpret_cast<SetPreferredAppMode>(
            GetProcAddress(theme, MAKEINTRESOURCEA(135)));
        auto allowWindow = reinterpret_cast<AllowDarkModeForWindow>(
            GetProcAddress(theme, MAKEINTRESOURCEA(133)));
        auto flushMenuThemes = reinterpret_cast<FlushMenuThemes>(
            GetProcAddress(theme, MAKEINTRESOURCEA(136)));
        // AllowDark is the Windows 10+ preferred app mode value.
        if (setPreferredAppMode) setPreferredAppMode(2);
        if (allowWindow) allowWindow(window, TRUE);
        if (flushMenuThemes) flushMenuThemes();
        SetWindowTheme(window, L"DarkMode_Explorer", nullptr);
        FreeLibrary(theme);
    }
    ApplyDarkTitleBar(window);
    InvalidateRect(window, nullptr, TRUE);
}

BOOL CALLBACK NormalizeDarkMenuPopup(HWND popup, LPARAM) {
    wchar_t className[64]{};
    if (GetClassNameW(popup, className, ARRAYSIZE(className)) == 0 ||
        wcscmp(className, L"#32768") != 0) return TRUE;
    HMODULE theme = LoadLibraryW(L"uxtheme.dll");
    if (theme) {
        using AllowDarkModeForWindow = BOOL(WINAPI*)(HWND, BOOL);
        auto allowWindow = reinterpret_cast<AllowDarkModeForWindow>(
            GetProcAddress(theme, MAKEINTRESOURCEA(133)));
        if (allowWindow) allowWindow(popup, TRUE);
        FreeLibrary(theme);
    }
    SetWindowTheme(popup, L"DarkMode_Explorer", nullptr);
    LONG_PTR windowStyle = GetWindowLongPtrW(popup, GWL_STYLE);
    windowStyle &= ~static_cast<LONG_PTR>(WS_BORDER);
    SetWindowLongPtrW(popup, GWL_STYLE, windowStyle);
    LONG_PTR extendedStyle = GetWindowLongPtrW(popup, GWL_EXSTYLE);
    extendedStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SetWindowLongPtrW(popup, GWL_EXSTYLE, extendedStyle);
    // SetWindowPos(SWP_FRAMECHANGED)はTrackPopupMenuの入力ループを
    // 中断することがあるため使わず、非クライアント枠だけ再描画する。
    RedrawWindow(popup, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
    return TRUE;
}

void DrawDarkButton(const DRAWITEMSTRUCT* draw) {
    if (!draw) return;
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    HBRUSH background = CreateSolidBrush(disabled ? RGB(48, 50, 54) :
                                         (pressed ? RGB(25, 100, 88) : RGB(35, 55, 62)));
    FillRect(draw->hDC, &draw->rcItem, background);
    DeleteObject(background);
    HPEN border = CreatePen(PS_SOLID, 1, disabled ? RGB(90, 92, 96) : RGB(70, 190, 165));
    HGDIOBJ oldPen = SelectObject(draw->hDC, border);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(HOLLOW_BRUSH));
    RoundRect(draw->hDC, draw->rcItem.left, draw->rcItem.top, draw->rcItem.right,
              draw->rcItem.bottom, 10, 10);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(border);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, disabled ? RGB(150, 154, 158) : RGB(240, 255, 250));
    HFONT oldFont = g_uiFont ? static_cast<HFONT>(SelectObject(draw->hDC, g_uiFont)) : nullptr;
    wchar_t label[64]{};
    GetWindowTextW(draw->hwndItem, label, ARRAYSIZE(label));
    RECT textRect = draw->rcItem;
    DrawTextW(draw->hDC, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (oldFont) SelectObject(draw->hDC, oldFont);
}

void DrawQuickLanguageButton(const DRAWITEMSTRUCT* draw) {
    if (!draw) return;
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    HBRUSH background = CreateSolidBrush(disabled ? RGB(31, 34, 40) :
                                         (pressed ? RGB(25, 53, 60) : RGB(22, 24, 28)));
    FillRect(draw->hDC, &draw->rcItem, background);
    DeleteObject(background);
    HPEN border = CreatePen(PS_SOLID, 1, disabled ? RGB(52, 56, 64) :
                            (pressed ? RGB(0, 240, 255) : RGB(42, 45, 51)));
    HGDIOBJ oldPen = SelectObject(draw->hDC, border);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(HOLLOW_BRUSH));
    RoundRect(draw->hDC, draw->rcItem.left, draw->rcItem.top, draw->rcItem.right,
              draw->rcItem.bottom, 6, 6);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(border);
    SetBkMode(draw->hDC, TRANSPARENT);
    wchar_t label[64]{};
    GetWindowTextW(draw->hwndItem, label, ARRAYSIZE(label));
    const wchar_t* iconText = L"🌐";
    const wchar_t* labelText = wcsstr(label, L"🌐 ") == label ? label + 2 : label;
    HFONT iconFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");
    HFONT textFont = CreateFontW(-11, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SIZE iconSize{};
    SIZE labelSize{};
    HGDIOBJ oldFont = iconFont ? SelectObject(draw->hDC, iconFont) : nullptr;
    GetTextExtentPoint32W(draw->hDC, iconText, 2, &iconSize);
    if (oldFont) SelectObject(draw->hDC, oldFont);
    oldFont = textFont ? SelectObject(draw->hDC, textFont) : nullptr;
    GetTextExtentPoint32W(draw->hDC, labelText, static_cast<int>(wcslen(labelText)), &labelSize);
    const int gap = 3;
    const int totalWidth = iconSize.cx + gap + labelSize.cx;
    const int left = (draw->rcItem.left + draw->rcItem.right - totalWidth) / 2;
    RECT iconRect{left, draw->rcItem.top, left + iconSize.cx, draw->rcItem.bottom};
    RECT labelRect{left + iconSize.cx + gap, draw->rcItem.top, left + totalWidth, draw->rcItem.bottom};
    SetTextColor(draw->hDC, disabled ? RGB(80, 120, 124) : RGB(0, 240, 255));
    if (iconFont) {
        SelectObject(draw->hDC, iconFont);
        DrawTextW(draw->hDC, iconText, 2, &iconRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SetTextColor(draw->hDC, disabled ? RGB(120, 125, 132) : RGB(243, 244, 246));
    if (textFont) {
        SelectObject(draw->hDC, textFont);
        DrawTextW(draw->hDC, labelText, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    if (oldFont) SelectObject(draw->hDC, oldFont);
    if (iconFont) DeleteObject(iconFont);
    if (textFont) DeleteObject(textFont);
}

bool HandleDarkButtonDraw(UINT message, LPARAM lParam) {
    if (message != WM_DRAWITEM || !lParam) return false;
    auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    if (draw->CtlType != ODT_BUTTON) return false;
    DrawDarkButton(draw);
    return true;
}

void PrepareDarkMenu(HMENU menu) {
    if (!menu) return;
    static HBRUSH popupBackground = CreateSolidBrush(RGB(18, 22, 24));
    MENUINFO menuInfo{sizeof(menuInfo)};
    menuInfo.fMask = MIM_BACKGROUND;
    menuInfo.hbrBack = popupBackground;
    SetMenuInfo(menu, &menuInfo);
    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index) {
        MENUITEMINFOW info{sizeof(info)};
        info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_SUBMENU | MIIM_STRING;
        wchar_t buffer[256]{};
        info.dwTypeData = buffer;
        info.cch = ARRAYSIZE(buffer);
        if (!GetMenuItemInfoW(menu, static_cast<UINT>(index), TRUE, &info)) continue;
        if (info.hSubMenu) PrepareDarkMenu(info.hSubMenu);
        if (info.fType & MFT_SEPARATOR) {
            MENUITEMINFOW separator{sizeof(separator)};
            separator.fMask = MIIM_FTYPE | MIIM_DATA;
            separator.fType = MFT_SEPARATOR | MFT_OWNERDRAW;
            separator.dwItemData = 0;
            SetMenuItemInfoW(menu, static_cast<UINT>(index), TRUE, &separator);
            continue;
        }
        g_menuLabels.emplace_back(buffer);
        MENUITEMINFOW ownerDraw{sizeof(ownerDraw)};
        ownerDraw.fMask = MIIM_FTYPE | MIIM_DATA;
        ownerDraw.fType = MFT_OWNERDRAW;
        ownerDraw.dwItemData = reinterpret_cast<ULONG_PTR>(&g_menuLabels.back());
        SetMenuItemInfoW(menu, static_cast<UINT>(index), TRUE, &ownerDraw);
    }
}

void DrawDarkMenuItem(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_MENU) return;
    const auto* label = reinterpret_cast<const std::wstring*>(draw->itemData);
    if (!label) {
        HBRUSH background = CreateSolidBrush(RGB(18, 22, 24));
        FillRect(draw->hDC, &draw->rcItem, background);
        DeleteObject(background);
        RECT separator = draw->rcItem;
        separator.top = (separator.top + separator.bottom) / 2;
        separator.bottom = separator.top + 1;
        HBRUSH line = CreateSolidBrush(RGB(55, 63, 66));
        FillRect(draw->hDC, &separator, line);
        DeleteObject(line);
        return;
    }
    const bool selected = (draw->itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    const COLORREF background = selected ? RGB(25, 53, 60) : RGB(18, 22, 24);
    HBRUSH brush = CreateSolidBrush(background);
    FillRect(draw->hDC, &draw->rcItem, brush);
    DeleteObject(brush);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, disabled ? RGB(120, 125, 128) : kDarkText);
    HFONT oldFont = g_uiFont ? static_cast<HFONT>(SelectObject(draw->hDC, g_uiFont)) : nullptr;
    RECT text = draw->rcItem;
    text.left += 10;
    DrawTextW(draw->hDC, label->c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (oldFont) SelectObject(draw->hDC, oldFont);
}

void MeasureDarkMenuItem(MEASUREITEMSTRUCT* measure) {
    if (!measure || measure->CtlType != ODT_MENU) return;
    const auto* label = reinterpret_cast<const std::wstring*>(measure->itemData);
    if (!label) {
        measure->itemWidth = 1;
        measure->itemHeight = 1;
        return;
    }
    HDC dc = GetDC(nullptr);
    SIZE size{};
    HFONT oldFont = g_uiFont ? static_cast<HFONT>(SelectObject(dc, g_uiFont)) : nullptr;
    GetTextExtentPoint32W(dc, label->c_str(), static_cast<int>(label->size()), &size);
    if (oldFont) SelectObject(dc, oldFont);
    ReleaseDC(nullptr, dc);
    measure->itemWidth = static_cast<UINT>(size.cx + 28);
    measure->itemHeight = static_cast<UINT>(std::max<LONG>(26, size.cy + 8));
}

void CopyExifTextToClipboard(HWND owner) {
    const std::wstring text = ExifText();
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (data) {
        void* target = GlobalLock(data);
        if (target) {
            memcpy(target, text.c_str(), bytes);
            GlobalUnlock(data);
            if (!SetClipboardData(CF_UNICODETEXT, data)) GlobalFree(data);
        } else {
            GlobalFree(data);
        }
    }
    CloseClipboard();
}

LRESULT CALLBACK ExifWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (HandleDarkButtonDraw(message, lParam)) return TRUE;
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN) {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, RGB(0, 0, 0));
        SetBkMode(dc, OPAQUE);
        static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        return reinterpret_cast<LRESULT>(brush);
    }
    if (message == WM_COMMAND && LOWORD(wParam) == kCommandCopyExif) {
        CopyExifTextToClipboard(window);
        SetWindowTextW(g_exifCopyButton, Ui(L"コピー済み", L"Copied"));
        return 0;
    }
    if (message == WM_CLOSE) {
        ShowWindow(window, SW_HIDE);
        return 0;
    }
    if (message == WM_DESTROY) {
        g_exifWindow = nullptr;
        g_exifText = nullptr;
        g_exifCopyButton = nullptr;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

std::wstring ReadUtf8File(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return L"";
    LARGE_INTEGER size{};
    std::wstring result;
    if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < 16 * 1024 * 1024) {
        std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        if (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read > 0) {
            const int count = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(read), nullptr, 0);
            if (count > 0) {
                result.resize(count);
                MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(read), result.data(), count);
            }
        }
    }
    CloseHandle(file);
    return result;
}

std::wstring ReadUtf8Resource(int resourceId) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) return L"";
    HGLOBAL loaded = LoadResource(nullptr, resource);
    const DWORD size = SizeofResource(nullptr, resource);
    const char* bytes = loaded ? static_cast<const char*>(LockResource(loaded)) : nullptr;
    if (!bytes || size == 0) return L"";
    const int count = MultiByteToWideChar(CP_UTF8, 0, bytes, static_cast<int>(size), nullptr, 0);
    if (count <= 0) return L"";
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, bytes, static_cast<int>(size), result.data(), count);
    return result;
}

HBITMAP LoadBitmapResource(int resourceId) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) return nullptr;
    HGLOBAL loaded = LoadResource(nullptr, resource);
    const DWORD size = SizeofResource(nullptr, resource);
    const BYTE* bytes = loaded ? static_cast<const BYTE*>(LockResource(loaded)) : nullptr;
    if (!bytes || size == 0) return nullptr;

    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    HBITMAP bitmap = nullptr;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateStream(&stream)) ||
            FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(bytes), size))) break;
        if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad,
                                                    &decoder))) break;
        if (FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter)) ||
            FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom))) break;
        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) break;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = static_cast<LONG>(width);
        info.bmiHeader.biHeight = -static_cast<LONG>(height);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        HDC screen = GetDC(nullptr);
        bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        ReleaseDC(nullptr, screen);
        if (!bitmap || FAILED(converter->CopyPixels(nullptr, width * 4, width * height * 4,
                                                    static_cast<BYTE*>(pixels)))) {
            if (bitmap) DeleteObject(bitmap);
            bitmap = nullptr;
        }
    } while (false);
    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    return bitmap;
}

std::wstring RenderHelpText(const std::wstring& markdown) {
    std::wstring rendered;
    bool previousHeading = false;
    size_t start = 0;
    while (start <= markdown.size()) {
        const size_t end = markdown.find(L'\n', start);
        std::wstring line = markdown.substr(start, end == std::wstring::npos ? end : end - start);
        if (!line.empty() && line.back() == L'\r') line.pop_back();

        if (line.empty() && previousHeading) {
            previousHeading = false;
            if (end == std::wstring::npos) break;
            start = end + 1;
            continue;
        }

        const bool heading = line.rfind(L"### ", 0) == 0 || line.rfind(L"## ", 0) == 0 ||
                             line.rfind(L"# ", 0) == 0;
        if (line.rfind(L"### ", 0) == 0) line.erase(0, 4);
        else if (line.rfind(L"## ", 0) == 0) line.erase(0, 3);
        else if (line.rfind(L"# ", 0) == 0) line.erase(0, 2);
        if (line.rfind(L"- ", 0) == 0) line.replace(0, 2, L"    • ");
        if (!line.empty() && !heading && line.rfind(L"    • ", 0) != 0) line = L"    " + line;
        if (heading && !line.empty()) line += L"\r\n";
        previousHeading = heading;

        for (size_t pos = 0; (pos = line.find(L"**", pos)) != std::wstring::npos;) {
            line.erase(pos, 2);
        }
        for (size_t pos = 0; (pos = line.find(L"__", pos)) != std::wstring::npos;) {
            line.erase(pos, 2);
        }
        for (size_t pos = 0; (pos = line.find(L'`', pos)) != std::wstring::npos;) {
            line.erase(pos, 1);
        }

        size_t linkStart = 0;
        while ((linkStart = line.find(L'[', linkStart)) != std::wstring::npos) {
            const size_t labelEnd = line.find(L']', linkStart + 1);
            const size_t urlStart = labelEnd == std::wstring::npos ? std::wstring::npos : line.find(L'(', labelEnd + 1);
            const size_t urlEnd = urlStart == std::wstring::npos ? std::wstring::npos : line.find(L')', urlStart + 1);
            if (labelEnd == std::wstring::npos || urlStart != labelEnd + 1 || urlEnd == std::wstring::npos) break;
            line.erase(urlStart, urlEnd - urlStart + 1);
            line.erase(labelEnd, 1);
            line.erase(linkStart, 1);
            linkStart += 1;
        }

        rendered += line;
        if (end == std::wstring::npos) break;
        rendered += L"\r\n";
        start = end + 1;
    }
    return rendered;
}

LRESULT CALLBACK HelpWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN) {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, RGB(0, 0, 0));
        SetBkMode(dc, OPAQUE);
        static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        return reinterpret_cast<LRESULT>(brush);
    }
    if (message == WM_CLOSE) {
        ShowWindow(window, SW_HIDE);
        return 0;
    }
    if (message == WM_SIZE && g_helpText) {
        const int width = std::max(0, LOWORD(lParam) - 24);
        const int height = std::max(0, HIWORD(lParam) - 24);
        MoveWindow(g_helpText, 12, 12, width, height, TRUE);
        return 0;
    }
    if (message == WM_DESTROY) {
        g_helpWindow = nullptr;
        g_helpText = nullptr;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void PlaceExifWindow(HWND owner) {
    if (!owner || !g_exifWindow) return;
    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    RECT workArea{};
    HMONITOR monitor = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(MONITORINFO)};
    if (monitor && GetMonitorInfoW(monitor, &monitorInfo)) workArea = monitorInfo.rcWork;
    else SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    constexpr int width = 440;
    constexpr int height = 150;
    constexpr int gap = 12;
    int x = ownerRect.right + gap;
    int y = std::max(workArea.top + 8, ownerRect.top + 40);
    if (x + width > workArea.right) x = ownerRect.left - width - gap;
    if (x < workArea.left) x = workArea.left + 8;
    if (y + height > workArea.bottom) y = std::max(workArea.top + 8, workArea.bottom - height - 8);
    SetWindowPos(g_exifWindow, HWND_TOP, x, y, width, height,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void UpdateExifWindow(HWND owner) {
    if (!owner) return;
    if (!g_bitmap) {
        if (g_exifWindow) ShowWindow(g_exifWindow, SW_HIDE);
        return;
    }
    if (!g_exifWindow) {
        WNDCLASSW klass{};
        klass.hInstance = GetModuleHandleW(nullptr);
        klass.lpfnWndProc = ExifWindowProc;
        klass.lpszClassName = L"QuickImageViewExifWindow";
        klass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        klass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        RegisterClassW(&klass);
        g_exifWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, klass.lpszClassName,
                                       Ui(L"EXIF情報", L"EXIF information"),
                                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
                                       0, 0, 440, 150, owner, nullptr, klass.hInstance, nullptr);
        if (!g_exifWindow) return;
        EnableDarkTheme(g_exifWindow);
        g_exifText = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
                                   WS_VSCROLL | WS_BORDER,
                                   12, 12, 400, 62, g_exifWindow, nullptr, klass.hInstance, nullptr);
        g_exifCopyButton = CreateWindowW(L"BUTTON", Ui(L"EXIFをコピー", L"Copy EXIF"),
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                         12, 84, 130, 30, g_exifWindow,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandCopyExif)),
                                         klass.hInstance, nullptr);
        if (g_uiFont) {
            SendMessageW(g_exifText, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            SendMessageW(g_exifCopyButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        }
    }
    SetWindowTextW(g_exifWindow, Ui(L"EXIF情報", L"EXIF information"));
    SetWindowTextW(g_exifText, ExifText().c_str());
    SetWindowTextW(g_exifCopyButton, Ui(L"EXIFをコピー", L"Copy EXIF"));
    PlaceExifWindow(owner);
}

void UpdateWindowTitle(HWND window) {
    std::wstring title = L"QuickImageView 2.1.0";
    if (!g_fileName.empty()) title += L" - " + g_fileName;
    if (g_bitmap) {
        wchar_t dimensions[64]{};
        swprintf_s(dimensions, L" [%ux%u]", g_imageWidth, g_imageHeight);
        title += dimensions;
    }
    SetWindowTextW(window, title.c_str());
}
void LoadImageIntoWindow(HWND window, const wchar_t* path);
double FitScale(HWND window);
void ClampPan(HWND window);
bool SamePath(const wchar_t* first, const wchar_t* second);

POINT CurrentClientCursor(HWND window, LPARAM fallback) {
    // WM_*BUTTON/WM_MOUSEMOVEのlParamは対象ウィンドウのクライアント座標。
    // GetCursorPos()を優先すると、合成入力や高速ドラッグで別イベントの
    // カーソル位置を拾い、選択範囲や貼り付け移動がずれる。
    const POINT eventPoint{GET_X_LPARAM(fallback), GET_Y_LPARAM(fallback)};
    RECT client{};
    GetClientRect(window, &client);
    if (eventPoint.x >= 0 && eventPoint.y >= 0 &&
        eventPoint.x < client.right && eventPoint.y < client.bottom) return eventPoint;
    POINT point{};
    if (GetCursorPos(&point) && ScreenToClient(window, &point)) return point;
    return eventPoint;
}

const wchar_t* Ui(const wchar_t* japanese, const wchar_t* english) {
    return g_englishUi ? english : japanese;
}

std::wstring LocalizeNotice(const wchar_t* japanese) {
    if (!japanese) return L"";
    struct NoticePair { const wchar_t* japanese; const wchar_t* english; };
    static const NoticePair pairs[] = {
        {L"Undoしました。再適用するにはRedoしてください。", L"Undo completed. Use Redo to apply it again."},
        {L"Redoしました。", L"Redo completed."},
        {L"リサイズしました。保存するには右クリックしてください。", L"Resized. Right-click to save."},
        {L"右へ90度回転しました。保存するには右クリックしてください。", L"Rotated right 90 degrees. Right-click to save."},
        {L"180度回転しました。保存するには右クリックしてください。", L"Rotated 180 degrees. Right-click to save."},
        {L"左へ90度回転しました。保存するには右クリックしてください。", L"Rotated left 90 degrees. Right-click to save."},
        {L"左右反転しました。保存するには右クリックしてください。", L"Mirrored horizontally. Right-click to save."},
        {L"上下反転しました。保存するには右クリックしてください。", L"Flipped vertically. Right-click to save."},
        {L"フルカラーへ変換しました。保存するには右クリックしてください。", L"Converted to full color. Right-click to save."},
        {L"256色へ変換しました。保存するには右クリックしてください。", L"Converted to 256 colors. Right-click to save."},
        {L"グレースケールへ変換しました。保存するには右クリックしてください。", L"Converted to grayscale. Right-click to save."}
    };
    for (const auto& pair : pairs) {
        if (wcscmp(japanese, pair.japanese) == 0) return Ui(pair.japanese, pair.english);
    }
    return japanese;
}

HICON CreateAppIcon() {
    constexpr int size = 32;
    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = size;
    header.bV5Height = -size;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;
    void* pixels = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(screen, reinterpret_cast<BITMAPINFO*>(&header), DIB_RGB_COLORS, &pixels, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!color || !pixels) return LoadIconW(nullptr, IDI_APPLICATION);
    auto* data = static_cast<DWORD*>(pixels);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int dx = x - 15;
            const int dy = y - 15;
            DWORD pixel = 0;
            if (dx * dx + dy * dy <= 14 * 14) {
                pixel = 0xFF30BEA0;
                if (y > 16 && y < 27 && x > 5 && x < 27 && (y - 16) > std::abs(x - 16) / 2) pixel = 0xFFFFFFFF;
            }
            data[y * size + x] = pixel;
        }
    }
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO info{};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = mask;
    HICON icon = CreateIconIndirect(&info);
    DeleteObject(color);
    DeleteObject(mask);
    return icon ? icon : LoadIconW(nullptr, IDI_APPLICATION);
}

RECT ImageViewport(HWND window) {
    RECT viewport{};
    GetClientRect(window, &viewport);
    viewport.top = std::min(viewport.bottom, viewport.top + kInfoBarHeight);
    viewport.bottom = std::max(viewport.top, viewport.bottom - kStatusBarHeight);
    return viewport;
}

void ClearPasteOverlay() {
    if (g_pasteBitmap) DeleteObject(g_pasteBitmap);
    g_pasteBitmap = nullptr;
    g_pasteWidth = 0;
    g_pasteHeight = 0;
    g_pasteX = 0;
    g_pasteY = 0;
    g_pasteMoving = false;
}

void ClearInternalClipboardBitmap() {
    if (g_internalClipboardBitmap) DeleteObject(g_internalClipboardBitmap);
    g_internalClipboardBitmap = nullptr;
}

void ClampPastePosition() {
    g_pasteX = std::clamp(g_pasteX, 0, std::max(0, static_cast<int>(g_imageWidth) - static_cast<int>(g_pasteWidth)));
    g_pasteY = std::clamp(g_pasteY, 0, std::max(0, static_cast<int>(g_imageHeight) - static_cast<int>(g_pasteHeight)));
}

std::wstring FileNameFromPath(const wchar_t* path) {
    const wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) slash = wcsrchr(path, L'/');
    return slash ? slash + 1 : path;
}

std::wstring FormatFromPath(const wchar_t* path) {
    const wchar_t* dot = wcsrchr(path, L'.');
    if (!dot || !dot[1]) return L"不明";
    std::wstring format(dot + 1);
    for (wchar_t& character : format) character = static_cast<wchar_t>(towlower(character));
    return format;
}

const wchar_t* DefaultExtensionForSaveFilter(DWORD filterIndex) {
    switch (filterIndex) {
    case 1: return L"jpg";
    case 2: return L"png";
    case 3: return L"tif";
    case 4: return L"bmp";
    case 5: return L"gif";
    case 6: return L"webp";
    case 7: return L"heic";
    default: return L"png";
    }
}

bool HasFileExtension(const wchar_t* path) {
    const wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) slash = wcsrchr(path, L'/');
    const wchar_t* dot = wcsrchr(path, L'.');
    return dot && (!slash || dot > slash + 1) && dot[1] != L'\0';
}

bool ReplaceBitmapFromSource(IWICBitmapSource* source, const wchar_t* notice) {
    if (!source) return false;
    IWICImagingFactory* factory = nullptr;
    IWICFormatConverter* converter = nullptr;
    HBITMAP replacement = nullptr;
    void* pixels = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom))) break;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) break;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = static_cast<LONG>(width);
        info.bmiHeader.biHeight = -static_cast<LONG>(height);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        HDC screen = GetDC(nullptr);
        replacement = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        ReleaseDC(nullptr, screen);
        if (!replacement || !pixels) break;
        if (FAILED(converter->CopyPixels(nullptr, width * 4, width * height * 4,
                                         static_cast<BYTE*>(pixels)))) break;
        if (g_bitmap) DeleteObject(g_bitmap);
        g_bitmap = replacement;
        replacement = nullptr;
        g_imageWidth = width;
        g_imageHeight = height;
        g_zoom = 1.0;
        g_panX = 0;
        g_panY = 0;
        g_selectionActive = false;
        g_notice = LocalizeNotice(notice);
        success = true;
    } while (false);
    if (replacement) DeleteObject(replacement);
    if (converter) converter->Release();
    if (factory) factory->Release();
    return success;
}

HBITMAP CopyBitmapRect(HBITMAP bitmap, RECT rect);

HGLOBAL CopyBitmapAsDib(HBITMAP bitmap) {
    if (!bitmap) return nullptr;
    BITMAP details{};
    if (!GetObjectW(bitmap, sizeof(details), &details) || details.bmWidth <= 0 || details.bmHeight <= 0) return nullptr;
    const SIZE_T pixelCount = static_cast<SIZE_T>(details.bmWidth) * static_cast<SIZE_T>(details.bmHeight);
    if (pixelCount > (static_cast<SIZE_T>(-1) - sizeof(BITMAPINFOHEADER)) / 4) return nullptr;
    const SIZE_T pixelBytes = pixelCount * 4;
    const SIZE_T totalBytes = sizeof(BITMAPINFOHEADER) + pixelBytes;
    HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, totalBytes);
    if (!data) return nullptr;
    auto* header = static_cast<BITMAPINFOHEADER*>(GlobalLock(data));
    if (!header) {
        GlobalFree(data);
        return nullptr;
    }
    *header = BITMAPINFOHEADER{};
    header->biSize = sizeof(BITMAPINFOHEADER);
    header->biWidth = details.bmWidth;
    header->biHeight = -details.bmHeight;
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;
    HDC screen = GetDC(nullptr);
    const int copiedLines = screen ? GetDIBits(screen, bitmap, 0, static_cast<UINT>(details.bmHeight),
                                                header + 1, reinterpret_cast<BITMAPINFO*>(header), DIB_RGB_COLORS) : 0;
    if (screen) ReleaseDC(nullptr, screen);
    GlobalUnlock(data);
    if (copiedLines != details.bmHeight) {
        GlobalFree(data);
        return nullptr;
    }
    return data;
}

HBITMAP CloneBitmap(HBITMAP bitmap) {
    if (!bitmap) return nullptr;
    BITMAP details{};
    if (!GetObjectW(bitmap, sizeof(details), &details) || details.bmWidth <= 0 || details.bmHeight <= 0) return nullptr;
    return CopyBitmapRect(bitmap, RECT{0, 0, details.bmWidth, details.bmHeight});
}

HBITMAP CopyBitmapRect(HBITMAP bitmap, RECT rect) {
    if (!bitmap || rect.right <= rect.left || rect.bottom <= rect.top) return nullptr;
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC screen = GetDC(nullptr);
    if (!screen) return nullptr;
    HDC source = CreateCompatibleDC(screen);
    HDC target = CreateCompatibleDC(screen);
    void* pixels = nullptr;
    HBITMAP copy = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HGDIOBJ oldSource = source ? SelectObject(source, bitmap) : nullptr;
    HGDIOBJ oldTarget = target && copy ? SelectObject(target, copy) : nullptr;
    const bool copied = source && target && copy && pixels && BitBlt(target, 0, 0, width, height, source,
                                                            rect.left, rect.top, SRCCOPY) != FALSE;
    if (oldSource) SelectObject(source, oldSource);
    if (oldTarget) SelectObject(target, oldTarget);
    if (source) DeleteDC(source);
    if (target) DeleteDC(target);
    ReleaseDC(nullptr, screen);
    if (!copied && copy) {
        DeleteObject(copy);
        copy = nullptr;
    }
    return copy;
}

HBITMAP CloneClipboardBitmap() {
    if (HBITMAP bitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP))) {
        return CloneBitmap(bitmap);
    }
    UINT formats[] = {CF_DIBV5, CF_DIB};
    UINT format = GetPriorityClipboardFormat(formats, 2);
    if (format != CF_DIBV5 && format != CF_DIB) return nullptr;
    HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(format));
    if (!data) return nullptr;
    const SIZE_T dataSize = GlobalSize(data);
    void* locked = GlobalLock(data);
    if (!locked) return nullptr;
    const auto* header = static_cast<const BITMAPINFOHEADER*>(locked);
    if (dataSize < sizeof(BITMAPINFOHEADER) ||
        header->biSize < sizeof(BITMAPINFOHEADER) || header->biSize > dataSize ||
        header->biWidth <= 0 || header->biHeight == 0 || header->biBitCount == 0 ||
        header->biBitCount > 32 ||
        (header->biCompression != BI_RGB && header->biCompression != BI_BITFIELDS)) {
        GlobalUnlock(data);
        return nullptr;
    }
    const size_t headerSize = header->biSize + (header->biBitCount <= 8 ?
        (static_cast<size_t>(1) << header->biBitCount) * sizeof(RGBQUAD) : 0);
    if (headerSize > dataSize) {
        GlobalUnlock(data);
        return nullptr;
    }
    const BYTE* pixels = static_cast<const BYTE*>(locked) + headerSize;
    HDC screen = GetDC(nullptr);
    if (!screen) {
        GlobalUnlock(data);
        return nullptr;
    }
    HBITMAP bitmap = CreateDIBitmap(screen, header, CBM_INIT, pixels,
                                    reinterpret_cast<const BITMAPINFO*>(header), DIB_RGB_COLORS);
    ReleaseDC(nullptr, screen);
    GlobalUnlock(data);
    return bitmap;
}

void ClearBitmapStack(std::vector<HBITMAP>& stack) {
    for (HBITMAP bitmap : stack) if (bitmap) DeleteObject(bitmap);
    stack.clear();
}

void RecordUndoState() {
    HBITMAP snapshot = CloneBitmap(g_bitmap);
    if (snapshot) g_undoStack.push_back(snapshot);
    ClearBitmapStack(g_redoStack);
}

bool ReplaceBitmapFromHBitmap(HBITMAP bitmap, const wchar_t* notice) {
    if (!bitmap) return false;
    IWICImagingFactory* factory = nullptr;
    IWICBitmap* source = nullptr;
    bool success = false;
    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateBitmapFromHBITMAP(bitmap, nullptr, WICBitmapUsePremultipliedAlpha, &source))) {
        success = ReplaceBitmapFromSource(source, notice);
    }
    if (source) source->Release();
    if (factory) factory->Release();
    return success;
}

bool UndoImage(HWND window) {
    if (g_undoStack.empty()) return false;
    HBITMAP current = CloneBitmap(g_bitmap);
    HBITMAP target = g_undoStack.back();
    g_undoStack.pop_back();
    if (current) g_redoStack.push_back(current);
    const bool success = ReplaceBitmapFromHBitmap(target, L"Undoしました。再適用するにはRedoしてください。");
    DeleteObject(target);
    if (success) {
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
    }
    return success;
}

bool RedoImage(HWND window) {
    if (g_redoStack.empty()) return false;
    HBITMAP current = CloneBitmap(g_bitmap);
    HBITMAP target = g_redoStack.back();
    g_redoStack.pop_back();
    if (current) g_undoStack.push_back(current);
    const bool success = ReplaceBitmapFromHBitmap(target, L"Redoしました。");
    DeleteObject(target);
    if (success) InvalidateRect(window, nullptr, FALSE);
    return success;
}

bool CreateCurrentWicSource(IWICImagingFactory* factory, IWICBitmapSource** source) {
    if (!factory || !g_bitmap || !source) return false;
    IWICBitmap* bitmap = nullptr;
    const HRESULT result = factory->CreateBitmapFromHBITMAP(g_bitmap, nullptr,
                                                             WICBitmapUsePremultipliedAlpha,
                                                             &bitmap);
    if (SUCCEEDED(result)) *source = bitmap;
    return SUCCEEDED(result);
}

bool ResizeCurrentImage(UINT width, UINT height) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapSource* source = nullptr;
    IWICBitmapScaler* scaler = nullptr;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (!CreateCurrentWicSource(factory, &source)) break;
        if (FAILED(factory->CreateBitmapScaler(&scaler))) break;
        if (FAILED(scaler->Initialize(source, width, height, WICBitmapInterpolationModeFant))) break;
        success = ReplaceBitmapFromSource(scaler, L"リサイズしました。保存するには右クリックしてください。");
    } while (false);
    if (scaler) scaler->Release();
    if (source) source->Release();
    if (factory) factory->Release();
    return success;
}

bool ResizeCurrentImageForDisplay(HWND window, UINT width, UINT height) {
    const double oldDisplayScale = window ? FitScale(window) * g_zoom : 0.0;
    const bool success = ResizeCurrentImage(width, height);
    if (success && window) {
        const double newFitScale = FitScale(window);
        if (oldDisplayScale > 0.0 && newFitScale > 0.0) {
            g_zoom = std::clamp(oldDisplayScale / newFitScale, 0.1, 20.0);
        }
        ClampPan(window);
    }
    return success;
}

bool ConvertColorCurrentImage(const GUID& pixelFormat, WICBitmapPaletteType paletteType,
                              const wchar_t* notice) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapSource* source = nullptr;
    IWICFormatConverter* converter = nullptr;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (!CreateCurrentWicSource(factory, &source)) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(source, pixelFormat, WICBitmapDitherTypeErrorDiffusion,
                                         nullptr, 0.0, paletteType))) break;
        success = ReplaceBitmapFromSource(converter, notice);
    } while (false);
    if (converter) converter->Release();
    if (source) source->Release();
    if (factory) factory->Release();
    return success;
}

bool TransformCurrentImage(WICBitmapTransformOptions options, const wchar_t* notice) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapSource* source = nullptr;
    IWICBitmapFlipRotator* transform = nullptr;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (!CreateCurrentWicSource(factory, &source)) break;
        if (FAILED(factory->CreateBitmapFlipRotator(&transform))) break;
        if (FAILED(transform->Initialize(source, options))) break;
        success = ReplaceBitmapFromSource(transform, notice);
    } while (false);
    if (transform) transform->Release();
    if (source) source->Release();
    if (factory) factory->Release();
    return success;
}

bool CropCurrentImage(RECT imageRect) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapSource* source = nullptr;
    IWICBitmapClipper* clipper = nullptr;
    bool success = false;
    do {
        if (imageRect.right <= imageRect.left || imageRect.bottom <= imageRect.top) break;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (!CreateCurrentWicSource(factory, &source)) break;
        if (FAILED(factory->CreateBitmapClipper(&clipper))) break;
        WICRect clipRect{imageRect.left, imageRect.top,
                         imageRect.right - imageRect.left,
                         imageRect.bottom - imageRect.top};
        if (FAILED(clipper->Initialize(source, &clipRect))) break;
        success = ReplaceBitmapFromSource(clipper, L"切り抜きました。保存するには右クリックしてください。");
    } while (false);
    if (clipper) clipper->Release();
    if (source) source->Release();
    if (factory) factory->Release();
    return success;
}

RECT SelectionImageRect(HWND window) {
    const RECT client = ImageViewport(window);
    const double scale = FitScale(window) * g_zoom;
    const int width = std::max(1, static_cast<int>(g_imageWidth * scale));
    const int height = std::max(1, static_cast<int>(g_imageHeight * scale));
    const int originX = client.left + (client.right - client.left - width) / 2 + g_panX;
    const int originY = client.top + (client.bottom - client.top - height) / 2 + g_panY;
    const int left = std::clamp(static_cast<int>(std::min(g_selectionStart.x, g_selectionEnd.x)), originX, originX + width);
    const int right = std::clamp(static_cast<int>(std::max(g_selectionStart.x, g_selectionEnd.x)), originX, originX + width);
    const int top = std::clamp(static_cast<int>(std::min(g_selectionStart.y, g_selectionEnd.y)), originY, originY + height);
    const int bottom = std::clamp(static_cast<int>(std::max(g_selectionStart.y, g_selectionEnd.y)), originY, originY + height);
    return RECT{static_cast<LONG>((left - originX) / scale), static_cast<LONG>((top - originY) / scale),
                static_cast<LONG>((right - originX) / scale), static_cast<LONG>((bottom - originY) / scale)};
}

bool CopyImageToClipboard(HWND window) {
    if (!g_bitmap || !OpenClipboard(window)) return false;
    EmptyClipboard();
    HBITMAP copy = g_selectionActive ? CopyBitmapRect(g_bitmap, SelectionImageRect(window)) : CloneBitmap(g_bitmap);
    if (!copy) {
        CloseClipboard();
        return false;
    }
    HBITMAP internalCopy = CloneBitmap(copy);
    if (!internalCopy) {
        DeleteObject(copy);
        CloseClipboard();
        return false;
    }
    ClearInternalClipboardBitmap();
    g_internalClipboardBitmap = internalCopy;
    HGLOBAL dib = CopyBitmapAsDib(copy);
    const bool success = SetClipboardData(CF_BITMAP, copy) != nullptr;
    if (success) {
        copy = nullptr;
        if (!dib || !SetClipboardData(CF_DIB, dib)) {
            if (dib) GlobalFree(dib);
        }
    }
    if (copy) DeleteObject(copy);
    CloseClipboard();
    if (success) g_notice = g_selectionActive ? Ui(L"選択範囲をクリップボードへコピーしました。", L"The selected area was copied to the clipboard.")
                                               : Ui(L"画像をクリップボードへコピーしました。", L"The image was copied to the clipboard.");
    return success;
}

bool PasteImageFromClipboard(HWND window) {
    HBITMAP copy = nullptr;
    if (g_internalClipboardBitmap) {
        // アプリ内コピーはWindowsクリップボードのロック状態に依存させない。
        copy = CloneBitmap(g_internalClipboardBitmap);
    } else if (OpenClipboard(window)) {
        // 内部コピーがない場合だけ、他アプリのクリップボードを読む。
        copy = CloneClipboardBitmap();
        CloseClipboard();
    }
    if (!copy || !g_bitmap) {
        if (copy) DeleteObject(copy);
        g_notice = Ui(L"貼り付ける画像を取得できませんでした。", L"The image could not be retrieved for pasting.");
        InvalidateRect(window, nullptr, FALSE);
        return false;
    }
    BITMAP details{};
    if (!GetObjectW(copy, sizeof(details), &details) || details.bmWidth <= 0 || details.bmHeight <= 0) {
        DeleteObject(copy);
        return false;
    }
    ClearPasteOverlay();
    g_pasteBitmap = copy;
    g_pasteWidth = static_cast<UINT>(details.bmWidth);
    g_pasteHeight = static_cast<UINT>(details.bmHeight);
    g_pasteX = (static_cast<int>(g_imageWidth) - static_cast<int>(g_pasteWidth)) / 2;
    g_pasteY = (static_cast<int>(g_imageHeight) - static_cast<int>(g_pasteHeight)) / 2;
    ClampPastePosition();
    g_selectionActive = false;
    g_notice = Ui(L"貼り付け画像を左ドラッグで移動し、右クリックのOKで確定してください。",
                  L"Drag the pasted image with the left button, then choose OK from the right-click menu.");
    InvalidateRect(window, nullptr, FALSE);
    UpdateWindow(window);
    return true;
}

bool CommitPasteOverlay(HWND window) {
    if (!g_bitmap || !g_pasteBitmap) return false;
    RecordUndoState();
    HDC screen = GetDC(nullptr);
    HDC target = CreateCompatibleDC(screen);
    HDC source = CreateCompatibleDC(screen);
    HGDIOBJ oldTarget = target ? SelectObject(target, g_bitmap) : nullptr;
    HGDIOBJ oldSource = source ? SelectObject(source, g_pasteBitmap) : nullptr;
    const bool success = target && source && BitBlt(target, g_pasteX, g_pasteY,
                                                     g_pasteWidth, g_pasteHeight,
                                                     source, 0, 0, SRCCOPY) != FALSE;
    if (oldTarget) SelectObject(target, oldTarget);
    if (oldSource) SelectObject(source, oldSource);
    if (target) DeleteDC(target);
    if (source) DeleteDC(source);
    ReleaseDC(nullptr, screen);
    if (success) {
        ClearPasteOverlay();
    g_notice = Ui(L"貼り付けを確定しました。保存するには右クリックしてください。",
                  L"The paste was committed. Right-click to save.");
    } else if (!g_undoStack.empty()) {
        DeleteObject(g_undoStack.back());
        g_undoStack.pop_back();
    }
    InvalidateRect(window, nullptr, FALSE);
    UpdateWindow(window);
    return success;
}

struct ResizeDialogState {
    UINT width = 100;
    UINT height = 100;
    UINT originalWidth = 1;
    UINT originalHeight = 1;
    bool percent = true;
    bool keepAspectRatio = true;
    bool accepted = false;
};

LRESULT DarkDialogColor(WPARAM wParam) {
    HDC dc = reinterpret_cast<HDC>(wParam);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    SetBkMode(dc, OPAQUE);
    static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    return reinterpret_cast<LRESULT>(brush);
}

INT_PTR CALLBACK ResizeDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (HandleDarkButtonDraw(message, lParam)) return TRUE;
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN ||
        message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLORDLG) return DarkDialogColor(wParam);
    auto* state = reinterpret_cast<ResizeDialogState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        EnableDarkTheme(dialog);
        state = reinterpret_cast<ResizeDialogState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        wchar_t value[32]{};
        swprintf_s(value, L"%u", state->width);
        SetDlgItemTextW(dialog, IDC_RESIZE_WIDTH, value);
        swprintf_s(value, L"%u", state->height);
        SetDlgItemTextW(dialog, IDC_RESIZE_HEIGHT, value);
        SendDlgItemMessageW(dialog, IDC_RESIZE_MODE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Percent"));
        SendDlgItemMessageW(dialog, IDC_RESIZE_MODE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Pixels"));
        SendDlgItemMessageW(dialog, IDC_RESIZE_MODE, CB_SETCURSEL, state->percent ? 0 : 1, 0);
        CheckDlgButton(dialog, IDC_RESIZE_LOCK, state->keepAspectRatio ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }
    if (message == WM_COMMAND && state && LOWORD(wParam) == IDOK) {
        wchar_t widthText[32]{}, heightText[32]{};
        GetDlgItemTextW(dialog, IDC_RESIZE_WIDTH, widthText, ARRAYSIZE(widthText));
        GetDlgItemTextW(dialog, IDC_RESIZE_HEIGHT, heightText, ARRAYSIZE(heightText));
        const unsigned long width = wcstoul(widthText, nullptr, 10);
        const unsigned long height = wcstoul(heightText, nullptr, 10);
        if (width == 0 || height == 0 || width > 100000 || height > 100000) return TRUE;
        state->width = static_cast<UINT>(width);
        state->height = static_cast<UINT>(height);
        state->percent = SendDlgItemMessageW(dialog, IDC_RESIZE_MODE, CB_GETCURSEL, 0, 0) == 0;
        state->keepAspectRatio = IsDlgButtonChecked(dialog, IDC_RESIZE_LOCK) == BST_CHECKED;
        if (state->keepAspectRatio) {
            if (state->percent) {
                state->height = state->width;
            } else if (state->originalWidth > 0) {
                state->height = std::max(1u, static_cast<UINT>(
                    std::lround(static_cast<double>(state->width) * state->originalHeight / state->originalWidth)));
            }
        }
        state->accepted = true;
        EndDialog(dialog, IDOK);
        return TRUE;
    }
    if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

bool ShowResizeDialog(HWND owner, UINT* width, UINT* height, bool* percent) {
    ResizeDialogState state{*width, *height, g_imageWidth, g_imageHeight, *percent, true, false};
    const INT_PTR result = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_RESIZE_DIALOG),
                                           owner, ResizeDialogProc, reinterpret_cast<LPARAM>(&state));
    if (result != IDOK || !state.accepted) return false;
    *width = state.width;
    *height = state.height;
    *percent = state.percent;
    return true;
}

struct QualityDialogState {
    UINT quality = 90;
    bool accepted = false;
};

bool ParseBoundedUnsigned(const wchar_t* text, unsigned long maximum, unsigned long* value) {
    if (!text || !*text || !value) return false;
    wchar_t* end = nullptr;
    const unsigned long parsed = wcstoul(text, &end, 10);
    if (end == text || *end != L'\0' || parsed > maximum) return false;
    *value = parsed;
    return true;
}

INT_PTR CALLBACK QualityDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (HandleDarkButtonDraw(message, lParam)) return TRUE;
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN ||
        message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLORDLG) return DarkDialogColor(wParam);
    auto* state = reinterpret_cast<QualityDialogState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        EnableDarkTheme(dialog);
        state = reinterpret_cast<QualityDialogState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        wchar_t value[32]{};
        swprintf_s(value, L"%u", state->quality);
        SetDlgItemTextW(dialog, IDC_QUALITY_VALUE, value);
        return TRUE;
    }
    if (message == WM_COMMAND && state && LOWORD(wParam) == IDOK) {
        wchar_t qualityText[32]{};
        GetDlgItemTextW(dialog, IDC_QUALITY_VALUE, qualityText, ARRAYSIZE(qualityText));
        unsigned long quality = 0;
        if (!ParseBoundedUnsigned(qualityText, 100, &quality)) return TRUE;
        state->quality = static_cast<UINT>(quality);
        state->accepted = true;
        EndDialog(dialog, IDOK);
        return TRUE;
    }
    if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

bool ShowQualityDialog(HWND owner, UINT* quality) {
    QualityDialogState state{*quality, false};
    const INT_PTR result = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_QUALITY_DIALOG),
                                           owner, QualityDialogProc, reinterpret_cast<LPARAM>(&state));
    if (result != IDOK || !state.accepted) return false;
    *quality = state.quality;
    return true;
}

struct SaveOptionsState {
    int formatIndex = 1;
    UINT quality = 90;
    UINT compression = 5;
    bool accepted = false;
};

void UpdateSaveOptionsControls(HWND dialog) {
    const int formatIndex = static_cast<int>(SendDlgItemMessageW(dialog, IDC_SAVE_FORMAT, CB_GETCURSEL, 0, 0)) + 1;
    const bool jpegLike = formatIndex == 1 || formatIndex == 6 || formatIndex == 7;
    const bool png = formatIndex == 2;
    const bool tiff = formatIndex == 3;
    EnableWindow(GetDlgItem(dialog, IDC_SAVE_QUALITY), jpegLike ? TRUE : FALSE);
    EnableWindow(GetDlgItem(dialog, IDC_SAVE_COMPRESSION), (png || tiff) ? TRUE : FALSE);
    SetDlgItemTextW(dialog, IDC_SAVE_SETTING_LABEL,
                    png ? L"PNG compression (0-9)" : (tiff ? L"TIFF compression (1-9)" : L"Compression (PNG/TIFF only)"));
}

INT_PTR CALLBACK SaveOptionsDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (HandleDarkButtonDraw(message, lParam)) return TRUE;
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN ||
        message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLORDLG) return DarkDialogColor(wParam);
    auto* state = reinterpret_cast<SaveOptionsState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        EnableDarkTheme(dialog);
        state = reinterpret_cast<SaveOptionsState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        const wchar_t* formats[] = {L"JPEG", L"PNG", L"TIFF", L"BMP", L"GIF", L"WebP", L"HEIC/HEIF"};
        for (const wchar_t* format : formats) {
            SendDlgItemMessageW(dialog, IDC_SAVE_FORMAT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(format));
        }
        SendDlgItemMessageW(dialog, IDC_SAVE_FORMAT, CB_SETCURSEL, state->formatIndex - 1, 0);
        wchar_t value[32]{};
        swprintf_s(value, L"%u", state->quality);
        SetDlgItemTextW(dialog, IDC_SAVE_QUALITY, value);
        swprintf_s(value, L"%u", state->compression);
        SetDlgItemTextW(dialog, IDC_SAVE_COMPRESSION, value);
        UpdateSaveOptionsControls(dialog);
        return TRUE;
    }
    if (message == WM_COMMAND && state && LOWORD(wParam) == IDC_SAVE_FORMAT && HIWORD(wParam) == CBN_SELCHANGE) {
        UpdateSaveOptionsControls(dialog);
        return TRUE;
    }
    if (message == WM_COMMAND && state && LOWORD(wParam) == IDOK) {
        wchar_t qualityText[32]{}, compressionText[32]{};
        GetDlgItemTextW(dialog, IDC_SAVE_QUALITY, qualityText, ARRAYSIZE(qualityText));
        GetDlgItemTextW(dialog, IDC_SAVE_COMPRESSION, compressionText, ARRAYSIZE(compressionText));
        unsigned long quality = 0;
        unsigned long compression = 0;
        if (!ParseBoundedUnsigned(qualityText, 100, &quality) ||
            !ParseBoundedUnsigned(compressionText, 9, &compression)) return TRUE;
        state->formatIndex = static_cast<int>(SendDlgItemMessageW(dialog, IDC_SAVE_FORMAT, CB_GETCURSEL, 0, 0)) + 1;
        state->quality = static_cast<UINT>(quality);
        state->compression = static_cast<UINT>(compression);
        state->accepted = true;
        EndDialog(dialog, IDOK);
        return TRUE;
    }
    if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

bool ShowSaveOptionsDialog(HWND owner, SaveOptionsState* state) {
    const INT_PTR result = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SAVE_OPTIONS_DIALOG),
                                           owner, SaveOptionsDialogProc, reinterpret_cast<LPARAM>(state));
    return result == IDOK && state->accepted;
}

bool SamePath(const wchar_t* first, const wchar_t* second) {
    wchar_t firstFull[MAX_PATH * 4]{};
    wchar_t secondFull[MAX_PATH * 4]{};
    if (!GetFullPathNameW(first, ARRAYSIZE(firstFull), firstFull, nullptr) ||
        !GetFullPathNameW(second, ARRAYSIZE(secondFull), secondFull, nullptr)) return false;
    return _wcsicmp(firstFull, secondFull) == 0;
}

bool IsSupportedOutputFormat(const std::wstring& extension) {
    return extension == L"jpg" || extension == L"jpeg" || extension == L"png" ||
           extension == L"tif" || extension == L"tiff" || extension == L"bmp" ||
           extension == L"gif" || extension == L"webp" || extension == L"heic" || extension == L"heif";
}

bool ConvertWebPFile(const wchar_t* outputPath) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmap* source = nullptr;
    IWICFormatConverter* converter = nullptr;
    uint8_t* encoded = nullptr;
    size_t encodedSize = 0;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateBitmapFromHBITMAP(g_bitmap, nullptr,
                                                    WICBitmapUsePremultipliedAlpha, &source))) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(source, GUID_WICPixelFormat32bppBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom))) break;
        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) break;
        const UINT stride = width * 4;
        std::vector<BYTE> pixels(static_cast<size_t>(stride) * height);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data()))) break;
        encodedSize = WebPEncodeBGRA(pixels.data(), static_cast<int>(width), static_cast<int>(height),
                                     static_cast<int>(stride), g_jpegQuality, &encoded);
        if (encodedSize == 0 || encoded == nullptr) break;
        HANDLE file = CreateFileW(outputPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) break;
        DWORD written = 0;
        success = WriteFile(file, encoded, static_cast<DWORD>(encodedSize), &written, nullptr) &&
                  written == encodedSize;
        CloseHandle(file);
    } while (false);
    if (encoded) WebPFree(encoded);
    if (converter) converter->Release();
    if (source) source->Release();
    if (factory) factory->Release();
    if (!success) DeleteFileW(outputPath);
    return success;
}

const GUID* EncoderFormat(const std::wstring& extension) {
    if (extension == L"jpg" || extension == L"jpeg") return &GUID_ContainerFormatJpeg;
    if (extension == L"png") return &GUID_ContainerFormatPng;
    if (extension == L"tif" || extension == L"tiff") return &GUID_ContainerFormatTiff;
    if (extension == L"bmp") return &GUID_ContainerFormatBmp;
    if (extension == L"gif") return &GUID_ContainerFormatGif;
    if (extension == L"webp") return &GUID_ContainerFormatWebp;
    if (extension == L"heic" || extension == L"heif") return &GUID_ContainerFormatHeif;
    return nullptr;
}

bool ConvertImageFile(const wchar_t* outputPath) {
    const std::wstring extension = FormatFromPath(outputPath);
    if (!IsSupportedOutputFormat(extension) || !g_bitmap) return false;
    if (extension == L"webp") return ConvertWebPFile(outputPath);
    const GUID* format = EncoderFormat(extension);
    if (!format) return false;

    HANDLE reservation = CreateFileW(outputPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
    if (reservation == INVALID_HANDLE_VALUE) return false;
    CloseHandle(reservation);

    IWICImagingFactory* factory = nullptr;
    IStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IWICBitmap* source = nullptr;
    IPropertyBag2* options = nullptr;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(SHCreateStreamOnFileEx(outputPath,
                                          STGM_WRITE | STGM_SHARE_DENY_WRITE,
                                          FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream))) break;
        if (FAILED(factory->CreateEncoder(*format, nullptr, &encoder))) break;
        if (FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache))) break;
        if (FAILED(encoder->CreateNewFrame(&frame, &options))) break;
        if (extension == L"jpg" || extension == L"jpeg") {
            PROPBAG2 property{};
            property.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
            VARIANT value;
            VariantInit(&value);
            value.vt = VT_R4;
            value.fltVal = g_jpegQuality / 100.0f;
            options->Write(1, &property, &value);
            VariantClear(&value);
        }
        if (extension == L"png") {
            PROPBAG2 property{};
            property.pstrName = const_cast<LPOLESTR>(L"CompressionLevel");
            VARIANT value;
            VariantInit(&value);
            value.vt = VT_UI1;
            value.bVal = static_cast<BYTE>(g_compressionLevel);
            options->Write(1, &property, &value);
            VariantClear(&value);
        }
        if (extension == L"tif" || extension == L"tiff") {
            PROPBAG2 property{};
            property.pstrName = const_cast<LPOLESTR>(L"TiffCompressionMethod");
            VARIANT value;
            VariantInit(&value);
            value.vt = VT_UI1;
            value.bVal = static_cast<BYTE>(std::clamp(g_compressionLevel + 2, 3u, 7u));
            options->Write(1, &property, &value);
            VariantClear(&value);
        }
        if (extension == L"webp" || extension == L"heic" || extension == L"heif") {
            PROPBAG2 property{};
            property.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
            VARIANT value;
            VariantInit(&value);
            value.vt = VT_R4;
            value.fltVal = g_jpegQuality / 100.0f;
            options->Write(1, &property, &value);
            VariantClear(&value);
        }
        if (FAILED(frame->Initialize(nullptr))) break;
        if (FAILED(frame->SetSize(g_imageWidth, g_imageHeight))) break;
        WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(frame->SetPixelFormat(&pixelFormat))) break;
        if (FAILED(factory->CreateBitmapFromHBITMAP(g_bitmap, nullptr,
                                                    WICBitmapUsePremultipliedAlpha, &source))) break;
        if (FAILED(frame->WriteSource(source, nullptr))) break;
        if (FAILED(frame->Commit())) break;
        if (FAILED(encoder->Commit())) break;
        success = true;
    } while (false);
    if (source) source->Release();
    if (options) options->Release();
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (!success) DeleteFileW(outputPath);
    return success;
}

bool RunEditSelfTest(const wchar_t* inputPath) {
    if (LoadImageFile(inputPath) != LoadResult::success || !g_bitmap) return false;
    const UINT originalWidth = g_imageWidth;
    const UINT originalHeight = g_imageHeight;
    if (originalWidth == 0 || originalHeight == 0) return false;
    RecordUndoState();
    if (!ResizeCurrentImage(originalWidth + 1, originalHeight + 1)) return false;
    RecordUndoState();
    if (!TransformCurrentImage(WICBitmapTransformRotate90, L"")) return false;
    RecordUndoState();
    if (!TransformCurrentImage(WICBitmapTransformFlipHorizontal, L"")) return false;
    RecordUndoState();
    if (!ConvertColorCurrentImage(GUID_WICPixelFormat8bppIndexed, WICBitmapPaletteTypeFixedHalftone256, L"")) return false;
    RecordUndoState();
    if (!ConvertColorCurrentImage(GUID_WICPixelFormat8bppGray, WICBitmapPaletteTypeCustom, L"")) return false;
    const RECT crop{0, 0, static_cast<LONG>(std::max(1u, g_imageWidth - 1)), static_cast<LONG>(std::max(1u, g_imageHeight - 1))};
    RecordUndoState();
    if (!CropCurrentImage(crop)) return false;
    if (!CopyImageToClipboard(nullptr) || !PasteImageFromClipboard(nullptr) || !CommitPasteOverlay(nullptr)) return false;
    g_jpegQuality = 50;
    g_compressionLevel = 1;
    wchar_t temporaryPath[MAX_PATH]{};
    if (!GetTempFileNameW(L".", L"qiv", 0, temporaryPath)) return false;
    DeleteFileW(temporaryPath);
    std::wstring jpgPath = std::wstring(temporaryPath) + L".jpg";
    std::wstring pngPath = std::wstring(temporaryPath) + L".png";
    const bool qualityPass = ConvertImageFile(jpgPath.c_str()) && ConvertImageFile(pngPath.c_str());
    DeleteFileW(jpgPath.c_str());
    DeleteFileW(pngPath.c_str());
    if (!qualityPass) return false;
    if (!UndoImage(nullptr) || !RedoImage(nullptr)) return false;
    return g_imageWidth > 0 && g_imageHeight > 0;
}

bool RunSmokeSelfTest() {
    IWICImagingFactory* factory = nullptr;
    IWICBitmap* bitmap = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool passed = false;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateBitmap(1, 1, GUID_WICPixelFormat32bppPBGRA,
                                         WICBitmapCacheOnLoad, &bitmap))) break;
        if (FAILED(bitmap->GetSize(&width, &height))) break;
        passed = width == 1 && height == 1;
    } while (false);
    if (bitmap) bitmap->Release();
    if (factory) factory->Release();
    CoUninitialize();
    return passed;
}

bool RunResizeTest(const wchar_t* inputPath, UINT width, UINT height) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool loaded = LoadImageFile(inputPath) == LoadResult::success;
    const bool resized = loaded && ResizeCurrentImage(width, height);
    const bool passed = resized && g_imageWidth == width && g_imageHeight == height;
    ReleaseImage();
    CoUninitialize();
    return passed;
}

bool RunMetadataTest(const wchar_t* inputPath) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool loaded = LoadImageFile(inputPath) == LoadResult::success;
    const bool passed = loaded && !g_exifMake.empty() && !g_exifModel.empty() && !g_exifDateTime.empty();
    ReleaseImage();
    CoUninitialize();
    return passed;
}

void ConvertWithSaveDialog(HWND window) {
    SaveOptionsState options{};
    options.formatIndex = 1;
    options.quality = g_jpegQuality;
    options.compression = g_compressionLevel;
    if (!ShowSaveOptionsDialog(window, &options)) return;
    g_jpegQuality = options.quality;
    g_compressionLevel = options.compression;
    wchar_t outputPath[MAX_PATH * 4]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"JPEG画像 (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0"
                         L"PNG画像 (*.png)\0*.png\0"
                         L"TIFF画像 (*.tif;*.tiff)\0*.tif;*.tiff\0"
                         L"BMP画像 (*.bmp)\0*.bmp\0"
                         L"GIF画像 (*.gif)\0*.gif\0"
                         L"WebP画像 (*.webp)\0*.webp\0"
                         L"HEIC/HEIF画像 (*.heic;*.heif)\0*.heic;*.heif\0"
                         L"すべてのファイル (*.*)\0*.*\0";
    dialog.lpstrFile = outputPath;
    dialog.nMaxFile = ARRAYSIZE(outputPath);
    dialog.nFilterIndex = static_cast<DWORD>(options.formatIndex);
    dialog.lpstrDefExt = nullptr;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) return;
    if (!HasFileExtension(outputPath)) {
        const std::wstring extension = L"." + std::wstring(DefaultExtensionForSaveFilter(dialog.nFilterIndex));
        const size_t currentLength = wcslen(outputPath);
        if (currentLength + extension.size() + 1 < ARRAYSIZE(outputPath)) {
            wcscat_s(outputPath, extension.c_str());
        }
    }
    if (SamePath(g_sourcePath.c_str(), outputPath)) {
        g_notice = Ui(L"原本と同じ場所には保存できません。原本は変更していません。",
                      L"The source path cannot be used. The original was not changed.");
        InvalidateRect(window, nullptr, FALSE);
        return;
    }
    if (ConvertImageFile(outputPath)) {
        LoadImageIntoWindow(window, outputPath);
        g_notice = Ui(L"別ファイルとして保存し、保存先ファイルを再読み込みしました。原本は変更していません。",
                      L"Saved as a separate file and reloaded it. The original was not changed.");
        UpdateWindowTitle(window);
    } else {
        g_notice = Ui(L"保存できませんでした。既存ファイルへの上書きは禁止されています。",
                      L"The file was not saved. Overwriting an existing file is not allowed.");
    }
    InvalidateRect(window, nullptr, FALSE);
}

void ReleaseImage() {
    if (g_bitmap != nullptr) {
        DeleteObject(g_bitmap);
        g_bitmap = nullptr;
    }
    g_imageWidth = 0;
    g_imageHeight = 0;
    g_zoom = 1.0;
    g_panX = 0;
    g_panY = 0;
    g_panning = false;
    g_panMoved = false;
    g_selectionActive = false;
    ClearPasteOverlay();
    ClearInternalClipboardBitmap();
    g_fileName.clear();
    g_sourcePath.clear();
    g_formatName.clear();
    g_exifMake.clear();
    g_exifModel.clear();
    g_exifDateTime.clear();
    g_fileSize = 0;
    g_notice.clear();
    ClearBitmapStack(g_undoStack);
    ClearBitmapStack(g_redoStack);
}

const wchar_t* LoadErrorMessage(LoadResult result) {
    switch (result) {
    case LoadResult::fileNotFound: return Ui(L"ファイルが見つかりません。パスを確認してください。", L"The file was not found. Check the path.");
    case LoadResult::accessDenied: return Ui(L"ファイルを読み取る権限がありません。原本は変更していません。", L"The file cannot be read. The original was not changed.");
    case LoadResult::unsupportedFormat: return Ui(L"対応していない画像形式です。原本は変更していません。", L"This image format is not supported. The original was not changed.");
    case LoadResult::decodeFailed: return Ui(L"画像データを読み込めませんでした。原本は変更していません。", L"The image data could not be decoded. The original was not changed.");
    default: return L"";
    }
}

void LoadImageIntoWindow(HWND window, const wchar_t* path) {
    ReleaseImage();
    const LoadResult result = LoadImageFile(path);
    if (result != LoadResult::success) {
        g_status = LoadErrorMessage(result);
    }
    std::wstring title = L"QuickImageView 2.1.0";
    if (result == LoadResult::success) title += L" - " + g_fileName;
    SetWindowTextW(window, title.c_str());
    UpdateExifWindow(window);
    InvalidateRect(window, nullptr, TRUE);
}

void ExecuteEditCommand(HWND window, UINT command) {
    if (command == kCommandUndo) {
        UndoImage(window);
        return;
    }
    if (command == kCommandRedo) {
        RedoImage(window);
        return;
    }
    if (command == kCommandClipboardCopy) {
        CopyImageToClipboard(window);
        InvalidateRect(window, nullptr, FALSE);
        return;
    }
    if (command == kCommandClipboardPaste) {
        PasteImageFromClipboard(window);
        return;
    }
    if (!g_bitmap) return;
    switch (command) {
    case kCommandResizeCustom: {
        UINT width = 100;
        UINT height = 100;
        bool percent = true;
        if (!ShowResizeDialog(window, &width, &height, &percent)) break;
        if (percent) {
            width = std::max(1u, static_cast<UINT>(g_imageWidth * (width / 100.0)));
            height = std::max(1u, static_cast<UINT>(g_imageHeight * (height / 100.0)));
        }
        RecordUndoState();
        ResizeCurrentImageForDisplay(window, width, height);
        break;
    }
    case kCommandCrop:
        if (g_selectionActive) {
            RecordUndoState();
            CropCurrentImage(SelectionImageRect(window));
        }
        break;
    case kCommandRotate90:
        RecordUndoState();
        TransformCurrentImage(WICBitmapTransformRotate90, L"右へ90度回転しました。保存するには右クリックしてください。");
        break;
    case kCommandRotate180:
        RecordUndoState();
        TransformCurrentImage(WICBitmapTransformRotate180, L"180度回転しました。保存するには右クリックしてください。");
        break;
    case kCommandRotate270:
        RecordUndoState();
        TransformCurrentImage(WICBitmapTransformRotate270, L"左へ90度回転しました。保存するには右クリックしてください。");
        break;
    case kCommandFlipHorizontal:
        RecordUndoState();
        TransformCurrentImage(WICBitmapTransformFlipHorizontal, L"左右反転しました。保存するには右クリックしてください。");
        break;
    case kCommandFlipVertical:
        RecordUndoState();
        TransformCurrentImage(WICBitmapTransformFlipVertical, L"上下反転しました。保存するには右クリックしてください。");
        break;
    case kCommandColorFull:
        RecordUndoState();
        ConvertColorCurrentImage(GUID_WICPixelFormat32bppPBGRA, WICBitmapPaletteTypeCustom, L"フルカラーへ変換しました。保存するには右クリックしてください。");
        break;
    case kCommandColor256:
        RecordUndoState();
        ConvertColorCurrentImage(GUID_WICPixelFormat8bppIndexed, WICBitmapPaletteTypeFixedHalftone256, L"256色へ変換しました。保存するには右クリックしてください。");
        break;
    case kCommandColorGray:
        RecordUndoState();
        ConvertColorCurrentImage(GUID_WICPixelFormat8bppGray, WICBitmapPaletteTypeCustom, L"グレースケールへ変換しました。保存するには右クリックしてください。");
        break;
    case kCommandQuality50:
        g_jpegQuality = 50;
        g_notice = Ui(L"JPEG品質を50に設定しました。", L"JPEG quality set to 50.");
        break;
    case kCommandQuality75:
        g_jpegQuality = 75;
        g_notice = Ui(L"JPEG品質を75に設定しました。", L"JPEG quality set to 75.");
        break;
    case kCommandQuality90:
        g_jpegQuality = 90;
        g_notice = Ui(L"JPEG品質を90に設定しました。", L"JPEG quality set to 90.");
        break;
    case kCommandQualityCustom: {
        UINT quality = g_jpegQuality;
        if (!ShowQualityDialog(window, &quality)) break;
        g_jpegQuality = quality;
        g_notice = Ui(L"指定したJPEG品質を設定しました。", L"Custom JPEG quality applied.");
        break;
    }
    case kCommandCompression1:
        g_compressionLevel = 1;
        g_notice = Ui(L"圧縮レベルを1に設定しました。", L"PNG compression level set to 1.");
        break;
    case kCommandCompression5:
        g_compressionLevel = 5;
        g_notice = Ui(L"圧縮レベルを5に設定しました。", L"PNG compression level set to 5.");
        break;
    case kCommandCompression9:
        g_compressionLevel = 9;
        g_notice = Ui(L"圧縮レベルを9に設定しました。", L"PNG compression level set to 9.");
        break;
    default:
        return;
    }
    UpdateWindowTitle(window);
    InvalidateRect(window, nullptr, FALSE);
    UpdateWindow(window);
}

void OpenImageDialog(HWND window) {
    wchar_t path[MAX_PATH * 4]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"画像ファイル\0*.jpg;*.jpeg;*.png;*.tif;*.tiff;*.bmp;*.gif;*.webp;*.heic;*.heif\0すべてのファイル\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = ARRAYSIZE(path);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&dialog)) LoadImageIntoWindow(window, path);
}

constexpr int kAboutAppName = 5001;
constexpr int kAboutVersion = 5002;
constexpr int kAboutEnvironment = 5003;
constexpr int kAboutAuthor = 5004;

void UpdateAboutTexts() {
    if (!g_aboutWindow) return;
    SetWindowTextW(GetDlgItem(g_aboutWindow, kAboutAppName), L"QuickImageView");
    SetWindowTextW(GetDlgItem(g_aboutWindow, kAboutVersion), L"Ver. 2.1.0");
    SetWindowTextW(GetDlgItem(g_aboutWindow, kAboutEnvironment),
                   Ui(L"【開発環境】\r\n・C++17 (MinGW-w64 / g++)\r\n・Win32 API / Windows Imaging Component\r\n・CMake / libwebp 1.6.0",
                      L"[Development environment]\r\n・C++17 (MinGW-w64 / g++)\r\n・Win32 API / Windows Imaging Component\r\n・CMake / libwebp 1.6.0"));
    SetWindowTextW(GetDlgItem(g_aboutWindow, kAboutAuthor),
                   Ui(L"【制作者】\r\nGitHub: maktak-105", L"[Author]\r\nGitHub: maktak-105"));
    SetWindowTextW(GetDlgItem(g_aboutWindow, IDOK), L"OK");
}

LRESULT CALLBACK AboutWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (HandleDarkButtonDraw(message, lParam)) return TRUE;
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN) {
        return DarkDialogColor(wParam);
    }
    if (message == WM_CREATE) {
        g_aboutBadge = LoadBitmapResource(IDR_CREATOR_BADGE);
        CreateWindowW(L"STATIC", L"QuickImageView", WS_CHILD | WS_VISIBLE | SS_CENTER,
                      24, 24, 332, 30, window, reinterpret_cast<HMENU>(kAboutAppName),
                      GetModuleHandleW(nullptr), nullptr);
        CreateWindowW(L"STATIC", L"Ver. 2.1.0", WS_CHILD | WS_VISIBLE | SS_CENTER,
                      24, 58, 332, 24, window, reinterpret_cast<HMENU>(kAboutVersion),
                      GetModuleHandleW(nullptr), nullptr);
        CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                      24, 100, 332, 72, window, reinterpret_cast<HMENU>(kAboutEnvironment),
                      GetModuleHandleW(nullptr), nullptr);
        CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                      24, 360, 332, 44, window, reinterpret_cast<HMENU>(kAboutAuthor),
                      GetModuleHandleW(nullptr), nullptr);
        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                      24, 438, 72, 30, window, reinterpret_cast<HMENU>(IDOK),
                      GetModuleHandleW(nullptr), nullptr);
        if (g_uiFont) {
            for (const int id : {kAboutAppName, kAboutVersion, kAboutEnvironment, kAboutAuthor, IDOK}) {
                SendMessageW(GetDlgItem(window, id), WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            }
        }
        UpdateAboutTexts();
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        HBRUSH background = CreateSolidBrush(RGB(15, 18, 24));
        FillRect(dc, &client, background);
        DeleteObject(background);
        if (g_aboutBadge) {
            BITMAP source{};
            GetObjectW(g_aboutBadge, sizeof(source), &source);
            HDC memory = CreateCompatibleDC(dc);
            HGDIOBJ old = SelectObject(memory, g_aboutBadge);
            SetStretchBltMode(dc, HALFTONE);
            StretchBlt(dc, 105, 180, 170, 170, memory, 0, 0, source.bmWidth, source.bmHeight, SRCCOPY);
            SelectObject(memory, old);
            DeleteDC(memory);
        }
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_COMMAND && LOWORD(wParam) == IDOK) {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        if (g_aboutBadge) {
            DeleteObject(g_aboutBadge);
            g_aboutBadge = nullptr;
        }
        g_aboutWindow = nullptr;
        if (g_aboutOwner) {
            EnableWindow(g_aboutOwner, TRUE);
            SetForegroundWindow(g_aboutOwner);
            g_aboutOwner = nullptr;
        }
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void OpenAbout(HWND owner) {
    if (g_aboutWindow) {
        UpdateAboutTexts();
        ShowWindow(g_aboutWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_aboutWindow);
        return;
    }
    WNDCLASSW klass{};
    klass.hInstance = GetModuleHandleW(nullptr);
    klass.lpfnWndProc = AboutWindowProc;
    klass.lpszClassName = L"QuickImageViewAboutWindow";
    klass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    klass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&klass);
    g_aboutOwner = owner;
    EnableWindow(owner, FALSE);
    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    const int width = 380;
    const int height = 510;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;
    g_aboutWindow = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW,
                                   klass.lpszClassName, Ui(L"QuickImageView バージョン情報", L"QuickImageView About"),
                                   WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                   x, y, width, height, owner, nullptr, klass.hInstance, nullptr);
    if (!g_aboutWindow) {
        EnableWindow(owner, TRUE);
        g_aboutOwner = nullptr;
        return;
    }
    EnableDarkTheme(g_aboutWindow);
    UpdateAboutTexts();
    ShowWindow(g_aboutWindow, SW_SHOWNORMAL);
    SetForegroundWindow(g_aboutWindow);
}

void OpenHelp(HWND window) {
    const std::wstring contents = ReadUtf8Resource(g_englishUi ? IDR_HELP_EN : IDR_HELP_JP);
    if (contents.empty()) {
        MessageBoxW(window, Ui(L"埋め込みヘルプを読み込めません。", L"The embedded help could not be read."),
                    L"QuickImageView", MB_OK | MB_ICONWARNING);
        return;
    }
    if (!g_helpWindow) {
        WNDCLASSW klass{};
        klass.hInstance = GetModuleHandleW(nullptr);
        klass.lpfnWndProc = HelpWindowProc;
        klass.lpszClassName = L"QuickImageViewHelpWindow";
        klass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        klass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        RegisterClassW(&klass);
        g_helpWindow = CreateWindowExW(WS_EX_TOOLWINDOW, klass.lpszClassName,
                                       Ui(L"QuickImageView ヘルプ", L"QuickImageView Help"),
                                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                       CW_USEDEFAULT, CW_USEDEFAULT, 760, 620,
                                       window, nullptr, klass.hInstance, nullptr);
        if (g_helpWindow) {
            EnableDarkTheme(g_helpWindow);
             g_helpText = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
                                        12, 12, 720, 540, g_helpWindow, nullptr, klass.hInstance, nullptr);
            if (g_uiFont) SendMessageW(g_helpText, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        }
    }
    if (g_helpWindow && g_helpText) {
        SetWindowTextW(g_helpWindow, Ui(L"QuickImageView ヘルプ", L"QuickImageView Help"));
        const std::wstring rendered = RenderHelpText(contents);
        SetWindowTextW(g_helpText, rendered.c_str());
        ShowWindow(g_helpWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_helpWindow);
    }
}

void OpenExifWindow(HWND window) {
    if (!g_bitmap) {
        MessageBoxW(window, Ui(L"画像を開いてからEXIF情報を表示してください。", L"Open an image before viewing EXIF information."),
                    L"QuickImageView", MB_OK | MB_ICONINFORMATION);
        return;
    }
    UpdateExifWindow(window);
    if (g_exifWindow) {
        ShowWindow(g_exifWindow, SW_SHOWNOACTIVATE);
        SetWindowPos(g_exifWindow, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
}

void BuildMenu(HWND window) {
    HMENU menu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, kCommandOpen, Ui(L"ファイルを開く\tCtrl+O", L"Open image\tCtrl+O"));
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, kCommandExit, Ui(L"終了", L"Exit"));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), Ui(L"ファイル", L"File"));
    HMENU editMenu = CreatePopupMenu();
    AppendMenuW(editMenu, MF_STRING, kCommandResizeCustom, Ui(L"リサイズを指定...", L"Custom resize..."));
    AppendMenuW(editMenu, MF_STRING, kCommandCrop, Ui(L"選択範囲を切り抜く", L"Crop selection"));
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, kCommandRotate90, Ui(L"右へ90度回転", L"Rotate right 90°"));
    AppendMenuW(editMenu, MF_STRING, kCommandRotate180, Ui(L"180度回転", L"Rotate 180°"));
    AppendMenuW(editMenu, MF_STRING, kCommandRotate270, Ui(L"左へ90度回転", L"Rotate left 90°"));
    AppendMenuW(editMenu, MF_STRING, kCommandFlipHorizontal, Ui(L"ミラー（左右反転 / Mirror）", L"Mirror horizontally"));
    AppendMenuW(editMenu, MF_STRING, kCommandFlipVertical, Ui(L"上下反転", L"Flip vertically"));
    HMENU colorMenu = CreatePopupMenu();
    AppendMenuW(colorMenu, MF_STRING, kCommandColorFull, Ui(L"フルカラー", L"Full color"));
    AppendMenuW(colorMenu, MF_STRING, kCommandColor256, Ui(L"256色", L"256 colors"));
    AppendMenuW(colorMenu, MF_STRING, kCommandColorGray, Ui(L"グレースケール", L"Grayscale"));
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(colorMenu), Ui(L"色変換", L"Color mode"));
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    HMENU qualityMenu = CreatePopupMenu();
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality50, L"50");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality75, L"75");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality90, L"90");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQualityCustom, Ui(L"指定...", L"Custom..."));
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(qualityMenu), Ui(L"JPEG品質", L"JPEG quality"));
    HMENU compressionMenu = CreatePopupMenu();
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression1, Ui(L"1（低圧縮）", L"1 (low)"));
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression5, Ui(L"5（標準）", L"5 (normal)"));
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression9, Ui(L"9（高圧縮）", L"9 (high)"));
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(compressionMenu), Ui(L"PNG圧縮", L"PNG compression"));
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, kCommandClipboardCopy, Ui(L"画像をコピー", L"Copy image"));
    AppendMenuW(editMenu, MF_STRING, kCommandClipboardPaste, Ui(L"画像を貼り付け", L"Paste image"));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(editMenu), Ui(L"編集", L"Edit"));
    HMENU helpMenu = CreatePopupMenu();
    AppendMenuW(helpMenu, MF_STRING, kCommandOpenExif, Ui(L"EXIF情報", L"EXIF information"));
    AppendMenuW(helpMenu, MF_STRING, kCommandHelp, Ui(L"ヘルプ", L"Help"));
    AppendMenuW(helpMenu, MF_STRING, kCommandAbout, Ui(L"バージョン情報", L"About"));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu), Ui(L"ヘルプ", L"Help"));
    PrepareDarkMenu(menu);
    static HBRUSH menuBarBrush = CreateSolidBrush(RGB(10, 12, 16));
    MENUINFO menuBarInfo{sizeof(menuBarInfo)};
    menuBarInfo.fMask = MIM_BACKGROUND;
    menuBarInfo.hbrBack = menuBarBrush;
    SetMenuInfo(menu, &menuBarInfo);
    SetMenu(window, menu);
}

void ToggleLanguage(HWND window) {
    g_englishUi = !g_englishUi;
    HMENU oldMenu = GetMenu(window);
    BuildMenu(window);
    if (oldMenu) DestroyMenu(oldMenu);
    SetWindowTextW(g_languageButton, g_englishUi ? L"🌐 日本語" : L"🌐 English");
    UpdateExifWindow(window);
    if (g_helpWindow && IsWindowVisible(g_helpWindow)) OpenHelp(window);
    if (g_aboutWindow && IsWindowVisible(g_aboutWindow)) UpdateAboutTexts();
    g_notice = Ui(L"表示言語を日本語に切り替えました。", L"Display language changed to English.");
    DrawMenuBar(window);
    InvalidateRect(window, nullptr, FALSE);
}

void ShowImageContextMenu(HWND window, int x, int y) {
    if (!g_bitmap) return;
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, Ui(L"別形式で保存...", L"Save as..."));
    HMENU resizeMenu = CreatePopupMenu();
    AppendMenuW(resizeMenu, MF_STRING, kCommandResizeCustom, Ui(L"指定...", L"Custom..."));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(resizeMenu), Ui(L"リサイズ", L"Resize"));
    AppendMenuW(menu, MF_STRING, kCommandResizeCustom, Ui(L"リサイズを指定...", L"Custom resize..."));
    AppendMenuW(menu, MF_STRING | (g_selectionActive ? 0 : MF_GRAYED), kCommandCrop,
                Ui(L"選択範囲を切り抜く", L"Crop selection"));
    AppendMenuW(menu, MF_STRING, kCommandRotate90, Ui(L"右へ90度回転", L"Rotate right 90°"));
    AppendMenuW(menu, MF_STRING, kCommandRotate180, Ui(L"180度回転", L"Rotate 180°"));
    AppendMenuW(menu, MF_STRING, kCommandRotate270, Ui(L"左へ90度回転", L"Rotate left 90°"));
    AppendMenuW(menu, MF_STRING, kCommandFlipHorizontal, Ui(L"ミラー（左右反転）", L"Mirror horizontally"));
    AppendMenuW(menu, MF_STRING, kCommandFlipVertical, Ui(L"上下反転", L"Flip vertically"));
    HMENU colorMenu = CreatePopupMenu();
    AppendMenuW(colorMenu, MF_STRING, kCommandColorFull, Ui(L"フルカラー", L"Full color"));
    AppendMenuW(colorMenu, MF_STRING, kCommandColor256, Ui(L"256色", L"256 colors"));
    AppendMenuW(colorMenu, MF_STRING, kCommandColorGray, Ui(L"グレースケール", L"Grayscale"));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(colorMenu), Ui(L"色変換", L"Color mode"));
    HMENU qualityMenu = CreatePopupMenu();
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality50, L"50");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality75, L"75");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality90, L"90");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQualityCustom, L"指定...");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(qualityMenu), Ui(L"JPEG品質", L"JPEG quality"));
    HMENU compressionMenu = CreatePopupMenu();
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression1, Ui(L"1（低圧縮）", L"1 (low)"));
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression5, Ui(L"5（標準）", L"5 (normal)"));
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression9, Ui(L"9（高圧縮）", L"9 (high)"));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(compressionMenu), Ui(L"PNG圧縮", L"PNG compression"));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCommandClipboardCopy, Ui(L"画像をコピー", L"Copy image"));
    AppendMenuW(menu, MF_STRING, kCommandClipboardPaste, Ui(L"画像を貼り付け", L"Paste image"));
    PrepareDarkMenu(menu);
    const int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, window, nullptr);
    DestroyMenu(menu);
    if (command == 1) ConvertWithSaveDialog(window);
    else if (command == kCommandOpenExif) OpenExifWindow(window);
    else if (command == kCommandHelp) OpenHelp(window);
    else if (command != 0) SendMessageW(window, WM_COMMAND, command, 0);
}

void ShowPasteContextMenu(HWND window, int x, int y) {
    if (!g_pasteBitmap) return;
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kCommandPasteCommit, L"OK");
    AppendMenuW(menu, MF_STRING, kCommandPasteRetry, Ui(L"やり直し", L"Retry"));
    PrepareDarkMenu(menu);
    const int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, window, nullptr);
    DestroyMenu(menu);
    if (command == kCommandPasteCommit) CommitPasteOverlay(window);
    else if (command == kCommandPasteRetry) {
        g_notice = Ui(L"貼り付け画像を左ドラッグで移動して、右クリックのOKで確定してください。",
                       L"Drag the pasted image with the left button, then choose OK with the right button.");
        InvalidateRect(window, nullptr, FALSE);
    }
}

double FitScale(HWND window) {
    const RECT client = ImageViewport(window);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (!g_imageWidth || !g_imageHeight || width <= 0 || height <= 0) return 1.0;
    return std::min(width / static_cast<double>(g_imageWidth),
                    height / static_cast<double>(g_imageHeight));
}

void ClampPan(HWND window) {
    if (!g_bitmap) return;
    const RECT client = ImageViewport(window);
    const double scale = FitScale(window) * g_zoom;
    const double imageWidth = g_imageWidth * scale;
    const double imageHeight = g_imageHeight * scale;
    const double maxX = std::max(0.0, (imageWidth - (client.right - client.left)) / 2.0);
    const double maxY = std::max(0.0, (imageHeight - (client.bottom - client.top)) / 2.0);
    g_panX = std::clamp(g_panX, static_cast<int>(-maxX), static_cast<int>(maxX));
    g_panY = std::clamp(g_panY, static_cast<int>(-maxY), static_cast<int>(maxY));
}

LoadResult LoadImageFile(const wchar_t* path) {
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes)) {
        const DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) return LoadResult::accessDenied;
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return LoadResult::fileNotFound;
        }
        return LoadResult::decodeFailed;
    }
    if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return LoadResult::decodeFailed;

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    LoadResult result = LoadResult::decodeFailed;

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return result;
    }
    do {
        HRESULT hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                                        WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) {
            if (hr == WINCODEC_ERR_UNKNOWNIMAGEFORMAT) result = LoadResult::unsupportedFormat;
            else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || hr == E_ACCESSDENIED) result = LoadResult::accessDenied;
            break;
        }
        if (FAILED(decoder->GetFrame(0, &frame))) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom))) break;

        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) break;
        HBITMAP bitmap = nullptr;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = static_cast<LONG>(width);
        info.bmiHeader.biHeight = -static_cast<LONG>(height);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        HDC screen = GetDC(nullptr);
        bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        ReleaseDC(nullptr, screen);
        if (bitmap == nullptr || pixels == nullptr) break;

        const UINT stride = width * 4;
        if (FAILED(converter->CopyPixels(nullptr, stride, stride * height,
                                         static_cast<BYTE*>(pixels)))) {
            DeleteObject(bitmap);
            break;
        }
        ReleaseImage();
        g_bitmap = bitmap;
        g_imageWidth = width;
        g_imageHeight = height;
        g_fileName = FileNameFromPath(path);
        g_sourcePath = path;
        g_formatName = FormatFromPath(path);
        ULARGE_INTEGER size{};
        size.HighPart = attributes.nFileSizeHigh;
        size.LowPart = attributes.nFileSizeLow;
        g_fileSize = size.QuadPart;
        ReadExif(frame);
        g_status.clear();
        result = LoadResult::success;
    } while (false);

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    factory->Release();
    return result;
}

void Paint(HWND window, HDC dc) {
    RECT client{};
    GetClientRect(window, &client);
    FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    HGDIOBJ oldFont = g_uiFont ? SelectObject(dc, g_uiFont) : nullptr;
    if (!g_bitmap) {
        SetTextColor(dc, RGB(210, 210, 210));
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, g_status.c_str(), -1, &client,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (oldFont) SelectObject(dc, oldFont);
        return;
    }

    const RECT viewport = ImageViewport(window);
    const int clientWidth = viewport.right - viewport.left;
    const int clientHeight = viewport.bottom - viewport.top;
    const double fitScale = FitScale(window);
    const double scale = fitScale * g_zoom;
    const int width = std::max(1, static_cast<int>(g_imageWidth * scale));
    const int height = std::max(1, static_cast<int>(g_imageHeight * scale));
    const int x = viewport.left + (clientWidth - width) / 2 + g_panX;
    const int y = viewport.top + (clientHeight - height) / 2 + g_panY;
    HDC source = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(source, g_bitmap);
    SetStretchBltMode(dc, HALFTONE);
    StretchBlt(dc, x, y, width, height, source, 0, 0,
               static_cast<int>(g_imageWidth), static_cast<int>(g_imageHeight), SRCCOPY);
    SelectObject(source, old);
    DeleteDC(source);
    RECT infoBackground{0, 0, client.right, kInfoBarHeight};
    FillRect(dc, &infoBackground, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);
    RECT metadata{12, 10, client.right - 70, 34};
    DrawTextW(dc, MetadataText().c_str(), -1, &metadata, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    if (g_selecting || g_selectionActive) {
        RECT selection{std::min(g_selectionStart.x, g_selectionEnd.x), std::min(g_selectionStart.y, g_selectionEnd.y),
                       std::max(g_selectionStart.x, g_selectionEnd.x), std::max(g_selectionStart.y, g_selectionEnd.y)};
        HPEN pen = CreatePen(PS_DASH, 1, RGB(255, 220, 80));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, selection.left, selection.top, selection.right, selection.bottom);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
        const RECT selectedImage = SelectionImageRect(window);
        wchar_t selectionSize[64]{};
        swprintf_s(selectionSize, L"%ld×%ld", selectedImage.right - selectedImage.left,
                   selectedImage.bottom - selectedImage.top);
        RECT sizeLabel{selection.left + 3, std::max(viewport.top, selection.top - 22), selection.right, selection.top};
        SetTextColor(dc, RGB(255, 220, 80));
        DrawTextW(dc, selectionSize, -1, &sizeLabel, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        SetTextColor(dc, RGB(255, 255, 255));
    }
    if (g_pasteBitmap) {
        HDC paste = CreateCompatibleDC(dc);
        HGDIOBJ oldPaste = SelectObject(paste, g_pasteBitmap);
        const int pasteX = x + static_cast<int>(std::lround(g_pasteX * scale));
        const int pasteY = y + static_cast<int>(std::lround(g_pasteY * scale));
        const int pasteWidth = std::max(1, static_cast<int>(std::lround(g_pasteWidth * scale)));
        const int pasteHeight = std::max(1, static_cast<int>(std::lround(g_pasteHeight * scale)));
        SetStretchBltMode(dc, HALFTONE);
        StretchBlt(dc, pasteX, pasteY, pasteWidth, pasteHeight, paste, 0, 0,
                   static_cast<int>(g_pasteWidth), static_cast<int>(g_pasteHeight), SRCCOPY);
        SelectObject(paste, oldPaste);
        DeleteDC(paste);
        HPEN pen = CreatePen(PS_DASH, 1, RGB(120, 220, 255));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, pasteX, pasteY, pasteX + pasteWidth, pasteY + pasteHeight);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }
    if (!g_notice.empty()) {
        RECT statusBackground{0, client.bottom - kStatusBarHeight, client.right, client.bottom};
        FillRect(dc, &statusBackground, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        SetTextColor(dc, RGB(255, 255, 255));
        RECT notice{12, client.bottom - kStatusBarHeight + 8, client.right - 12, client.bottom - 8};
        DrawTextW(dc, g_notice.c_str(), -1, &notice, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    }
    if (oldFont) SelectObject(dc, oldFont);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_NCPAINT:
    case WM_NCACTIVATE: {
        const LRESULT result = DefWindowProcW(window, message, wParam, lParam);
        PaintDarkMenuBoundary(window);
        return result;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, kDarkText);
        SetBkColor(dc, kDarkSurface);
        SetBkMode(dc, OPAQUE);
        static HBRUSH brush = CreateSolidBrush(kDarkSurface);
        return reinterpret_cast<LRESULT>(brush);
    }
    case WM_CREATE:
        DragAcceptFiles(window, TRUE);
        g_status = Ui(L"画像ファイルを指定して起動してください。", L"Pass an image file to QuickImageView.");
        g_uiFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_languageButton = CreateWindowW(L"BUTTON", L"🌐 English", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                         0, 0, 84, 26, window,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandToggleLanguage)),
                                         GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_languageButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        EnableDarkTheme(window);
        UpdateExifWindow(window);
        return 0;
    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wParam);
        wchar_t path[MAX_PATH * 4]{};
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        if (count > 0 && DragQueryFileW(drop, 0, path, ARRAYSIZE(path)) > 0) {
            const bool shouldOpen = !g_bitmap ||
                MessageBoxW(window,
                            Ui(L"現在開いている画像を閉じて、ドロップした画像を開きますか？",
                               L"Close the current image and open the dropped image?"),
                            Ui(L"画像を開く確認", L"Open image"), MB_YESNO | MB_ICONQUESTION) == IDYES;
            if (shouldOpen) LoadImageIntoWindow(window, path);
        }
        DragFinish(drop);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kCommandOpen) OpenImageDialog(window);
        else if (LOWORD(wParam) == kCommandExit) DestroyWindow(window);
        else if (LOWORD(wParam) == kCommandHelp) OpenHelp(window);
        else if (LOWORD(wParam) == kCommandAbout) OpenAbout(window);
        else if (LOWORD(wParam) == kCommandOpenExif) OpenExifWindow(window);
        else if (LOWORD(wParam) == kCommandToggleLanguage) ToggleLanguage(window);
        else if (LOWORD(wParam) == 1) ConvertWithSaveDialog(window);
        else if ((LOWORD(wParam) >= kCommandResizeCustom && LOWORD(wParam) <= kCommandFlipVertical) ||
                 (LOWORD(wParam) >= kCommandQuality50 && LOWORD(wParam) <= kCommandColorGray) ||
                 (LOWORD(wParam) >= kCommandCompression1 && LOWORD(wParam) <= kCommandCompression9)) {
            ExecuteEditCommand(window, LOWORD(wParam));
        }
        else if (LOWORD(wParam) == kCommandPasteCommit) CommitPasteOverlay(window);
        else if (LOWORD(wParam) == kCommandPasteRetry && g_pasteBitmap) {
            g_notice = Ui(L"貼り付け画像を左ドラッグで移動して、右クリックのOKで確定してください。",
                           L"Drag the pasted image with the left button, then choose OK with the right button.");
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_DRAWITEM:
        if (lParam && reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
            DrawDarkMenuItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        if (wParam == kCommandToggleLanguage) {
            DrawQuickLanguageButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_MEASUREITEM:
        if (lParam && reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
            MeasureDarkMenuItem(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) OpenImageDialog(window);
        else if (wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandClipboardCopy);
        else if (wParam == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandClipboardPaste);
        else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandUndo);
        else if (wParam == 'Y' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandRedo);
        return 0;
    case WM_ENTERIDLE:
        // メニューのネストした入力ループ中にポップアップのスタイルを
        // 変更すると、TrackPopupMenuの入力を奪って操作不能になる。
        // ウィンドウ枠・位置は変更せず、Windowsのダークテーマだけを適用する。
        if (wParam == MSGF_MENU) EnumThreadWindows(GetCurrentThreadId(), NormalizeDarkMenuPopup, 0);
        return 0;
    case WM_CHAR:
        // 一部の入力方式・UI自動化ではCtrlキーの状態がWM_KEYDOWNで取得できないため、
        // Windowsが生成する制御文字も同じショートカットとして扱う。
        if (wParam == 0x03) ExecuteEditCommand(window, kCommandClipboardCopy);      // Ctrl+C
        else if (wParam == 0x16) ExecuteEditCommand(window, kCommandClipboardPaste); // Ctrl+V
        else if (wParam == 0x1A) ExecuteEditCommand(window, kCommandUndo);           // Ctrl+Z
        else if (wParam == 0x19) ExecuteEditCommand(window, kCommandRedo);           // Ctrl+Y
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;
        HDC back = CreateCompatibleDC(dc);
        HBITMAP backBitmap = CreateCompatibleBitmap(dc, std::max(1, width), std::max(1, height));
        HGDIOBJ oldBitmap = SelectObject(back, backBitmap);
        Paint(window, back);
        BitBlt(dc, 0, 0, width, height, back, 0, 0, SRCCOPY);
        SelectObject(back, oldBitmap);
        DeleteObject(backBitmap);
        DeleteDC(back);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (g_languageButton) MoveWindow(g_languageButton, std::max(8, LOWORD(lParam) - 96), 14, 84, 26, TRUE);
        if (g_exifWindow && IsWindowVisible(g_exifWindow)) PlaceExifWindow(window);
        ClampPan(window);
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    case WM_MOUSEWHEEL: {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (!delta || !g_bitmap) return 0;
        const double oldZoom = g_zoom;
        const double steps = delta / static_cast<double>(WHEEL_DELTA);
        g_zoom = std::clamp(g_zoom * std::pow(1.15, steps), 0.1, 20.0);
        if (oldZoom != g_zoom) {
            POINT cursor{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window, &cursor);
            const double oldScale = FitScale(window) * oldZoom;
            const double newScale = FitScale(window) * g_zoom;
            const RECT client = ImageViewport(window);
            const double oldOriginX = client.left + (client.right - client.left - g_imageWidth * oldScale) / 2.0 + g_panX;
            const double oldOriginY = client.top + (client.bottom - client.top - g_imageHeight * oldScale) / 2.0 + g_panY;
            const double imageX = (cursor.x - oldOriginX) / oldScale;
            const double imageY = (cursor.y - oldOriginY) / oldScale;
            const double newOriginX = cursor.x - imageX * newScale;
            const double newOriginY = cursor.y - imageY * newScale;
            g_panX = static_cast<int>(newOriginX - client.left - (client.right - client.left - g_imageWidth * newScale) / 2.0);
            g_panY = static_cast<int>(newOriginY - client.top - (client.bottom - client.top - g_imageHeight * newScale) / 2.0);
            ClampPan(window);
        }
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
        if (g_bitmap) {
            const POINT point = CurrentClientCursor(window, lParam);
            if (g_pasteBitmap) {
                g_pasteMoving = true;
                g_pasteDragStart = point;
                g_pasteStart = POINT{g_pasteX, g_pasteY};
            } else {
                g_selecting = true;
                g_selectionActive = false;
                g_selectionStart = point;
                g_selectionEnd = point;
            }
            SetCapture(window);
        }
        return 0;
    case WM_LBUTTONUP: {
        const POINT point = CurrentClientCursor(window, lParam);
        if (g_selecting) {
            g_selectionEnd = point;
            g_selectionActive = std::abs(g_selectionEnd.x - g_selectionStart.x) > 4 &&
                                std::abs(g_selectionEnd.y - g_selectionStart.y) > 4;
        }
        g_selecting = false;
        g_pasteMoving = false;
        g_panning = false;
        if (GetCapture() == window) ReleaseCapture();
        g_panMoved = false;
        return 0;
    }
    case WM_MBUTTONDOWN:
        if (g_bitmap) {
            g_panning = true;
            g_panMoved = false;
            g_lastPanPoint = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            SetCapture(window);
        }
        return 0;
    case WM_MBUTTONUP:
        g_panning = false;
        if (GetCapture() == window) ReleaseCapture();
        g_panMoved = false;
        return 0;
    case WM_RBUTTONUP: {
        if (!g_bitmap) return 0;
        POINT screenPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ClientToScreen(window, &screenPoint);
        g_ignoreNextContextMenu = true;
        if (g_pasteBitmap) ShowPasteContextMenu(window, screenPoint.x, screenPoint.y);
        else ShowImageContextMenu(window, screenPoint.x, screenPoint.y);
        return 0;
    }
    case WM_CONTEXTMENU: {
        if (g_ignoreNextContextMenu) {
            g_ignoreNextContextMenu = false;
            return 0;
        }
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        if (g_pasteBitmap) ShowPasteContextMenu(window, x, y);
        else ShowImageContextMenu(window, x, y);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_selecting) {
            g_selectionEnd = CurrentClientCursor(window, lParam);
            InvalidateRect(window, nullptr, FALSE);
        } else if (g_pasteMoving && g_pasteBitmap) {
            const POINT current{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const double scale = FitScale(window) * g_zoom;
            if (scale > 0.0) {
                g_pasteX = g_pasteStart.x + static_cast<int>(std::lround((current.x - g_pasteDragStart.x) / scale));
                g_pasteY = g_pasteStart.y + static_cast<int>(std::lround((current.y - g_pasteDragStart.y) / scale));
                ClampPastePosition();
                InvalidateRect(window, nullptr, FALSE);
            }
        } else if (g_panning) {
            const POINT current{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            g_panX += current.x - g_lastPanPoint.x;
            g_panY += current.y - g_lastPanPoint.y;
            if (current.x != g_lastPanPoint.x || current.y != g_lastPanPoint.y) g_panMoved = true;
            g_lastPanPoint = current;
            ClampPan(window);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_CAPTURECHANGED:
        g_panning = false;
        g_selecting = false;
        g_pasteMoving = false;
        return 0;
    case WM_DESTROY:
        if (g_exifWindow) DestroyWindow(g_exifWindow);
        if (g_helpWindow) DestroyWindow(g_helpWindow);
        if (g_aboutWindow) DestroyWindow(g_aboutWindow);
        ReleaseImage();
        if (g_uiFont) {
            DeleteObject(g_uiFont);
            g_uiFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
    return 0;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand) {
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--self-test") == 0) {
        LocalFree(arguments);
        return RunSmokeSelfTest() ? 0 : 2;
    }
    if (arguments && argumentCount == 3 && wcscmp(arguments[1], L"--self-test-edit") == 0) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool passed = RunEditSelfTest(arguments[2]);
        ReleaseImage();
        LocalFree(arguments);
        CoUninitialize();
        return passed ? 0 : 2;
    }
    if (arguments && argumentCount == 5 && wcscmp(arguments[1], L"--resize-test") == 0) {
        const unsigned long width = wcstoul(arguments[3], nullptr, 10);
        const unsigned long height = wcstoul(arguments[4], nullptr, 10);
        const bool passed = width > 0 && height > 0 && width <= 100000 && height <= 100000 &&
            RunResizeTest(arguments[2], static_cast<UINT>(width), static_cast<UINT>(height));
        LocalFree(arguments);
        return passed ? 0 : 2;
    }
    if (arguments && argumentCount == 3 && wcscmp(arguments[1], L"--metadata-test") == 0) {
        const bool passed = RunMetadataTest(arguments[2]);
        LocalFree(arguments);
        return passed ? 0 : 2;
    }
    const bool convertMode = arguments && argumentCount == 4 && wcscmp(arguments[1], L"--convert") == 0;
    if (convertMode) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool samePath = SamePath(arguments[2], arguments[3]);
        const bool loaded = !samePath && LoadImageFile(arguments[2]) == LoadResult::success;
        const bool converted = loaded && ConvertImageFile(arguments[3]);
        LocalFree(arguments);
        CoUninitialize();
        return converted ? 0 : 2;
    }
    const bool uiTestHidden = arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--ui-test-hidden") == 0;
    std::wstring uiTestFile = uiTestHidden && argumentCount >= 3 ? arguments[2] : L"";
    if (arguments) LocalFree(arguments);
    if (commandLine && wcscmp(commandLine, L"--help") == 0) {
        MessageBoxW(nullptr, Ui(L"QuickImageView 2.1.0\n画像ファイルを引数に指定してください。",
                                L"QuickImageView 2.1.0\nPass an image file as an argument."),
                    L"QuickImageView", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const wchar_t* filePath = uiTestHidden ? uiTestFile.c_str() : commandLine;
    if (filePath && *filePath == L'"') {
        ++filePath;
        wchar_t* end = const_cast<wchar_t*>(wcschr(filePath, L'"'));
        if (end) *end = L'\0';
    }
    if (filePath && *filePath) {
        const LoadResult result = LoadImageFile(filePath);
        if (result != LoadResult::success) g_status = LoadErrorMessage(result);
    }

    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = kClassName;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_QUICKIMAGEVIEW));
    if (!windowClass.hIcon) windowClass.hIcon = CreateAppIcon();
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&windowClass);

    const DWORD extendedStyle = WS_EX_COMPOSITED;
    RECT secondaryWorkArea{};
    EnumDisplayMonitors(nullptr, nullptr, FindSecondaryMonitor, reinterpret_cast<LPARAM>(&secondaryWorkArea));
    const bool hasSecondary = secondaryWorkArea.right > secondaryWorkArea.left && secondaryWorkArea.bottom > secondaryWorkArea.top;
    const int windowX = uiTestHidden && hasSecondary ? secondaryWorkArea.left + 40 : CW_USEDEFAULT;
    const int windowY = uiTestHidden && hasSecondary ? secondaryWorkArea.top + 40 : CW_USEDEFAULT;
    const int windowWidth = uiTestHidden && hasSecondary ? std::min(1200, static_cast<int>(secondaryWorkArea.right - secondaryWorkArea.left - 80)) : 960;
    const int windowHeight = uiTestHidden && hasSecondary ? std::min(800, static_cast<int>(secondaryWorkArea.bottom - secondaryWorkArea.top - 80)) : 720;
    HWND window = CreateWindowExW(extendedStyle, kClassName, L"QuickImageView 2.1.0",
                                  WS_OVERLAPPEDWINDOW, windowX, windowY,
                                  windowWidth, windowHeight, nullptr, nullptr, instance, nullptr);
    if (!window) {
        CoUninitialize();
        return 1;
    }
    BuildMenu(window);
    if (filePath && *filePath && g_bitmap) SetWindowTextW(window, (L"QuickImageView 2.1.0 - " + g_fileName).c_str());
    ShowWindow(window, uiTestHidden ? SW_SHOW : showCommand);
    UpdateWindow(window);
    if (uiTestHidden) {
        UINT testWidth = 100;
        UINT testHeight = 100;
        bool testPercent = true;
        ShowResizeDialog(window, &testWidth, &testHeight, &testPercent);
        ShowResizeDialog(window, &testWidth, &testHeight, &testPercent);
        ShowQualityDialog(window, &g_jpegQuality);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    CoUninitialize();
    return static_cast<int>(message.wParam);
}
