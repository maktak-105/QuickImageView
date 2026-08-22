#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include "resource.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cwchar>
#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr wchar_t kClassName[] = L"QuickImageViewWindow";
constexpr UINT kCommandOpen = 1001;
constexpr UINT kCommandExit = 1002;
constexpr UINT kCommandResize50 = 1101;
constexpr UINT kCommandResize75 = 1102;
constexpr UINT kCommandResize125 = 1103;
constexpr UINT kCommandResize200 = 1104;
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

std::wstring MetadataText() {
    if (!g_bitmap) return L"";
    wchar_t buffer[256]{};
    swprintf_s(buffer, L"%s  |  %ux%u  |  %s  |  %llu KB", g_fileName.c_str(), g_imageWidth,
               g_imageHeight, g_formatName.c_str(), static_cast<unsigned long long>((g_fileSize + 1023) / 1024));
    return buffer;
}

std::wstring ExifText() {
    if (g_exifMake.empty() && g_exifModel.empty() && g_exifDateTime.empty()) return L"EXIF: なし";
    std::wstring text = L"EXIF: ";
    if (!g_exifMake.empty()) text += L"メーカー=" + g_exifMake + L"  ";
    if (!g_exifModel.empty()) text += L"機種=" + g_exifModel + L"  ";
    if (!g_exifDateTime.empty()) text += L"撮影日時=" + g_exifDateTime;
    return text;
}

void UpdateWindowTitle(HWND window) {
    std::wstring title = L"QuickImageView 0.1.0";
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
bool SamePath(const wchar_t* first, const wchar_t* second);

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
        g_notice = notice;
        success = true;
    } while (false);
    if (replacement) DeleteObject(replacement);
    if (converter) converter->Release();
    if (factory) factory->Release();
    return success;
}

HBITMAP CloneBitmap(HBITMAP bitmap) {
    return bitmap ? static_cast<HBITMAP>(CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)) : nullptr;
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
    if (success) InvalidateRect(window, nullptr, FALSE);
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
    RECT client{};
    GetClientRect(window, &client);
    const double scale = FitScale(window) * g_zoom;
    const int width = std::max(1, static_cast<int>(g_imageWidth * scale));
    const int height = std::max(1, static_cast<int>(g_imageHeight * scale));
    const int originX = (client.right - client.left - width) / 2 + g_panX;
    const int originY = (client.bottom - client.top - height) / 2 + g_panY;
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
    HBITMAP copy = CloneBitmap(g_bitmap);
    const bool success = copy && SetClipboardData(CF_BITMAP, copy) != nullptr;
    if (!success && copy) DeleteObject(copy);
    CloseClipboard();
    if (success) g_notice = L"画像をクリップボードへコピーしました。";
    return success;
}

bool PasteImageFromClipboard(HWND window) {
    if (!OpenClipboard(window)) return false;
    HBITMAP clipboardBitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
    HBITMAP copy = CloneBitmap(clipboardBitmap);
    CloseClipboard();
    if (!copy) return false;
    RecordUndoState();
    const bool success = ReplaceBitmapFromHBitmap(copy, L"クリップボードから貼り付けました。保存するには右クリックしてください。");
    DeleteObject(copy);
    if (!success && !g_undoStack.empty()) {
        DeleteObject(g_undoStack.back());
        g_undoStack.pop_back();
    }
    InvalidateRect(window, nullptr, FALSE);
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

INT_PTR CALLBACK ResizeDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<ResizeDialogState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
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
    auto* state = reinterpret_cast<QualityDialogState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
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
    auto* state = reinterpret_cast<SaveOptionsState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
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
    if (!CopyImageToClipboard(nullptr) || !PasteImageFromClipboard(nullptr)) return false;
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
        g_notice = L"原本と同じ場所には保存できません。原本は変更していません。";
        InvalidateRect(window, nullptr, FALSE);
        return;
    }
    if (ConvertImageFile(outputPath)) {
        g_notice = L"別ファイルとして保存しました。原本は変更していません。";
    } else {
        g_notice = L"保存できませんでした。既存ファイルへの上書きは禁止されています。";
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
    case LoadResult::fileNotFound: return L"ファイルが見つかりません。パスを確認してください。";
    case LoadResult::accessDenied: return L"ファイルを読み取る権限がありません。原本は変更していません。";
    case LoadResult::unsupportedFormat: return L"対応していない画像形式です。原本は変更していません。";
    case LoadResult::decodeFailed: return L"画像データを読み込めませんでした。原本は変更していません。";
    default: return L"";
    }
}

void LoadImageIntoWindow(HWND window, const wchar_t* path) {
    ReleaseImage();
    const LoadResult result = LoadImageFile(path);
    if (result != LoadResult::success) {
        g_status = LoadErrorMessage(result);
    }
    std::wstring title = L"QuickImageView 0.1.0";
    if (result == LoadResult::success) title += L" - " + g_fileName;
    SetWindowTextW(window, title.c_str());
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
    case kCommandResize50:
        RecordUndoState();
        ResizeCurrentImage(std::max(1u, g_imageWidth / 2), std::max(1u, g_imageHeight / 2));
        break;
    case kCommandResize75:
        RecordUndoState();
        ResizeCurrentImage(std::max(1u, g_imageWidth * 3 / 4), std::max(1u, g_imageHeight * 3 / 4));
        break;
    case kCommandResize125:
        RecordUndoState();
        ResizeCurrentImage(std::max(1u, g_imageWidth * 5 / 4), std::max(1u, g_imageHeight * 5 / 4));
        break;
    case kCommandResize200:
        RecordUndoState();
        ResizeCurrentImage(std::max(1u, g_imageWidth * 2), std::max(1u, g_imageHeight * 2));
        break;
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
        ResizeCurrentImage(width, height);
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
        g_notice = L"JPEG品質を50に設定しました。";
        break;
    case kCommandQuality75:
        g_jpegQuality = 75;
        g_notice = L"JPEG品質を75に設定しました。";
        break;
    case kCommandQuality90:
        g_jpegQuality = 90;
        g_notice = L"JPEG品質を90に設定しました。";
        break;
    case kCommandQualityCustom: {
        UINT quality = g_jpegQuality;
        if (!ShowQualityDialog(window, &quality)) break;
        g_jpegQuality = quality;
        g_notice = L"指定したJPEG品質を設定しました。";
        break;
    }
    case kCommandCompression1:
        g_compressionLevel = 1;
        g_notice = L"圧縮レベルを1に設定しました。";
        break;
    case kCommandCompression5:
        g_compressionLevel = 5;
        g_notice = L"圧縮レベルを5に設定しました。";
        break;
    case kCommandCompression9:
        g_compressionLevel = 9;
        g_notice = L"圧縮レベルを9に設定しました。";
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

void BuildMenu(HWND window) {
    HMENU menu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, kCommandOpen, L"ファイルを開く\tCtrl+O");
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, kCommandExit, L"終了");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"ファイル");
    HMENU editMenu = CreatePopupMenu();
    AppendMenuW(editMenu, MF_STRING, kCommandResize50, L"リサイズ 50%\tCtrl+1");
    AppendMenuW(editMenu, MF_STRING, kCommandResize75, L"リサイズ 75%\tCtrl+2");
    AppendMenuW(editMenu, MF_STRING, kCommandResize125, L"リサイズ 125%\tCtrl+3");
    AppendMenuW(editMenu, MF_STRING, kCommandResize200, L"リサイズ 200%\tCtrl+4");
    AppendMenuW(editMenu, MF_STRING, kCommandResizeCustom, L"リサイズを指定...");
    AppendMenuW(editMenu, MF_STRING, kCommandCrop, L"選択範囲を切り抜く");
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, kCommandRotate90, L"右へ90度回転");
    AppendMenuW(editMenu, MF_STRING, kCommandRotate180, L"180度回転");
    AppendMenuW(editMenu, MF_STRING, kCommandRotate270, L"左へ90度回転");
    AppendMenuW(editMenu, MF_STRING, kCommandFlipHorizontal, L"ミラー（左右反転 / Mirror）");
    AppendMenuW(editMenu, MF_STRING, kCommandFlipVertical, L"上下反転");
    HMENU colorMenu = CreatePopupMenu();
    AppendMenuW(colorMenu, MF_STRING, kCommandColorFull, L"フルカラー");
    AppendMenuW(colorMenu, MF_STRING, kCommandColor256, L"256色");
    AppendMenuW(colorMenu, MF_STRING, kCommandColorGray, L"グレースケール");
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(colorMenu), L"色変換");
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    HMENU qualityMenu = CreatePopupMenu();
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality50, L"50");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality75, L"75");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality90, L"90");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQualityCustom, L"指定...");
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(qualityMenu), L"JPEG品質");
    HMENU compressionMenu = CreatePopupMenu();
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression1, L"1（低圧縮）");
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression5, L"5（標準）");
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression9, L"9（高圧縮）");
    AppendMenuW(editMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(compressionMenu), L"PNG圧縮");
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, kCommandClipboardCopy, L"画像をコピー");
    AppendMenuW(editMenu, MF_STRING, kCommandClipboardPaste, L"画像を貼り付け");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(editMenu), L"編集");
    SetMenu(window, menu);
}

void ShowImageContextMenu(HWND window, int x, int y) {
    if (!g_bitmap) return;
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"別形式で保存...");
    HMENU resizeMenu = CreatePopupMenu();
    AppendMenuW(resizeMenu, MF_STRING, kCommandResize50, L"50%");
    AppendMenuW(resizeMenu, MF_STRING, kCommandResize75, L"75%");
    AppendMenuW(resizeMenu, MF_STRING, kCommandResize125, L"125%");
    AppendMenuW(resizeMenu, MF_STRING, kCommandResize200, L"200%");
    AppendMenuW(resizeMenu, MF_STRING, kCommandResizeCustom, L"指定...");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(resizeMenu), L"リサイズ");
    AppendMenuW(menu, MF_STRING | (g_selectionActive ? 0 : MF_GRAYED), kCommandCrop, L"選択範囲を切り抜く");
    AppendMenuW(menu, MF_STRING, kCommandRotate90, L"右へ90度回転");
    AppendMenuW(menu, MF_STRING, kCommandRotate180, L"180度回転");
    AppendMenuW(menu, MF_STRING, kCommandRotate270, L"左へ90度回転");
    AppendMenuW(menu, MF_STRING, kCommandFlipHorizontal, L"ミラー（左右反転 / Mirror）");
    AppendMenuW(menu, MF_STRING, kCommandFlipVertical, L"上下反転");
    HMENU colorMenu = CreatePopupMenu();
    AppendMenuW(colorMenu, MF_STRING, kCommandColorFull, L"フルカラー");
    AppendMenuW(colorMenu, MF_STRING, kCommandColor256, L"256色");
    AppendMenuW(colorMenu, MF_STRING, kCommandColorGray, L"グレースケール");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(colorMenu), L"色変換");
    HMENU qualityMenu = CreatePopupMenu();
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality50, L"50");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality75, L"75");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQuality90, L"90");
    AppendMenuW(qualityMenu, MF_STRING, kCommandQualityCustom, L"指定...");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(qualityMenu), L"JPEG品質");
    HMENU compressionMenu = CreatePopupMenu();
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression1, L"1（低圧縮）");
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression5, L"5（標準）");
    AppendMenuW(compressionMenu, MF_STRING, kCommandCompression9, L"9（高圧縮）");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(compressionMenu), L"PNG圧縮");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCommandClipboardCopy, L"画像をコピー");
    AppendMenuW(menu, MF_STRING, kCommandClipboardPaste, L"画像を貼り付け");
    const int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, window, nullptr);
    DestroyMenu(menu);
    if (command == 1) ConvertWithSaveDialog(window);
    else SendMessageW(window, WM_COMMAND, command, 0);
}

double FitScale(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (!g_imageWidth || !g_imageHeight || width <= 0 || height <= 0) return 1.0;
    return std::min(width / static_cast<double>(g_imageWidth),
                    height / static_cast<double>(g_imageHeight));
}

void ClampPan(HWND window) {
    if (!g_bitmap) return;
    RECT client{};
    GetClientRect(window, &client);
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
    if (!g_bitmap) {
        SetTextColor(dc, RGB(210, 210, 210));
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, g_status.c_str(), -1, &client,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        return;
    }

    const int clientWidth = client.right - client.left;
    const int clientHeight = client.bottom - client.top;
    const double fitScale = FitScale(window);
    const double scale = fitScale * g_zoom;
    const int width = std::max(1, static_cast<int>(g_imageWidth * scale));
    const int height = std::max(1, static_cast<int>(g_imageHeight * scale));
    const int x = (clientWidth - width) / 2 + g_panX;
    const int y = (clientHeight - height) / 2 + g_panY;
    HDC source = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(source, g_bitmap);
    SetStretchBltMode(dc, HALFTONE);
    StretchBlt(dc, x, y, width, height, source, 0, 0,
               static_cast<int>(g_imageWidth), static_cast<int>(g_imageHeight), SRCCOPY);
    SelectObject(source, old);
    DeleteDC(source);
    SetTextColor(dc, RGB(230, 230, 230));
    SetBkMode(dc, TRANSPARENT);
    RECT metadata{12, 10, client.right - 12, 34};
    DrawTextW(dc, MetadataText().c_str(), -1, &metadata, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    RECT exif{12, 34, client.right - 12, 58};
    DrawTextW(dc, ExifText().c_str(), -1, &exif, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
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
    }
    if (!g_notice.empty()) {
        SetTextColor(dc, RGB(150, 230, 160));
        RECT notice{12, client.bottom - 34, client.right - 12, client.bottom - 10};
        DrawTextW(dc, g_notice.c_str(), -1, &notice, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == kCommandOpen) OpenImageDialog(window);
        else if (LOWORD(wParam) == kCommandExit) DestroyWindow(window);
        else if (LOWORD(wParam) == 1) ConvertWithSaveDialog(window);
        else if (LOWORD(wParam) >= kCommandResize50 && LOWORD(wParam) <= kCommandCompression9) ExecuteEditCommand(window, LOWORD(wParam));
        return 0;
    case WM_KEYDOWN:
        if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) OpenImageDialog(window);
        else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandUndo);
        else if (wParam == 'Y' && (GetKeyState(VK_CONTROL) & 0x8000)) ExecuteEditCommand(window, kCommandRedo);
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam >= '1' && wParam <= '4') {
            ExecuteEditCommand(window, kCommandResize50 + (wParam - '1'));
        }
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
            RECT client{};
            GetClientRect(window, &client);
            const double oldOriginX = (client.right - client.left - g_imageWidth * oldScale) / 2.0 + g_panX;
            const double oldOriginY = (client.bottom - client.top - g_imageHeight * oldScale) / 2.0 + g_panY;
            const double imageX = (cursor.x - oldOriginX) / oldScale;
            const double imageY = (cursor.y - oldOriginY) / oldScale;
            const double newOriginX = cursor.x - imageX * newScale;
            const double newOriginY = cursor.y - imageY * newScale;
            g_panX = static_cast<int>(newOriginX - (client.right - client.left - g_imageWidth * newScale) / 2.0);
            g_panY = static_cast<int>(newOriginY - (client.bottom - client.top - g_imageHeight * newScale) / 2.0);
            ClampPan(window);
        }
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
        if (g_bitmap) {
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            g_selecting = true;
            g_selectionActive = false;
            g_selectionStart = point;
            g_selectionEnd = point;
            SetCapture(window);
        }
        return 0;
    case WM_LBUTTONUP: {
        const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (g_selecting) {
            g_selectionEnd = point;
            g_selectionActive = std::abs(g_selectionEnd.x - g_selectionStart.x) > 4 &&
                                std::abs(g_selectionEnd.y - g_selectionStart.y) > 4;
        }
        g_selecting = false;
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
        if (g_bitmap) {
            POINT screenPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(window, &screenPoint);
            ShowImageContextMenu(window, screenPoint.x, screenPoint.y);
        }
        return 0;
    }
    case WM_CONTEXTMENU: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        ShowImageContextMenu(window, x, y);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_selecting) {
            g_selectionEnd = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            InvalidateRect(window, nullptr, FALSE);
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
        return 0;
    case WM_DESTROY:
        ReleaseImage();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
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
        MessageBoxW(nullptr, L"QuickImageView 0.1.0\n画像ファイルを引数に指定してください。",
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
    HWND window = CreateWindowExW(extendedStyle, kClassName, L"QuickImageView 0.1.0",
                                  WS_OVERLAPPEDWINDOW, windowX, windowY,
                                  windowWidth, windowHeight, nullptr, nullptr, instance, nullptr);
    if (!window) {
        CoUninitialize();
        return 1;
    }
    BuildMenu(window);
    if (filePath && *filePath && g_bitmap) SetWindowTextW(window, (L"QuickImageView 0.1.0 - " + g_fileName).c_str());
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
