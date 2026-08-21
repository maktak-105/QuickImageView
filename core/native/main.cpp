#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commdlg.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cwchar>
#include <cstring>
#include <string>

namespace {
constexpr wchar_t kClassName[] = L"QuickImageViewWindow";
constexpr UINT kCommandOpen = 1001;
constexpr UINT kCommandExit = 1002;
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

enum class LoadResult { success, fileNotFound, accessDenied, unsupportedFormat, decodeFailed };
LoadResult LoadImageFile(const wchar_t* path);

std::wstring ReadMetadataString(IWICMetadataQueryReader* reader, const std::wstring& path) {
    if (!reader) return L"";
    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring result;
    if (SUCCEEDED(reader->GetMetadataByName(path.c_str(), &value))) {
        if (value.vt == VT_LPWSTR && value.pwszVal) {
            result.assign(value.pwszVal);
        } else if (value.vt == VT_BSTR && value.bstrVal) {
            result.assign(value.bstrVal, SysStringLen(value.bstrVal));
        } else if (value.vt == VT_LPSTR && value.pszVal) {
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
    if (FAILED(frame->GetMetadataQueryReader(&reader))) return;
    const wchar_t* roots[] = {L"/app1/ifd/", L"/ifd/"};
    for (const wchar_t* root : roots) {
        if (g_exifMake.empty()) g_exifMake = ReadMetadataString(reader, std::wstring(root) + L"{ushort=271}");
        if (g_exifModel.empty()) g_exifModel = ReadMetadataString(reader, std::wstring(root) + L"{ushort=272}");
        if (g_exifDateTime.empty()) g_exifDateTime = ReadMetadataString(reader, std::wstring(root) + L"{ushort=36867}");
        if (g_exifDateTime.empty()) g_exifDateTime = ReadMetadataString(reader, std::wstring(root) + L"{ushort=306}");
    }
    reader->Release();
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
           extension == L"gif" || extension == L"webp" || extension == L"heic";
}

const GUID* EncoderFormat(const std::wstring& extension) {
    if (extension == L"jpg" || extension == L"jpeg") return &GUID_ContainerFormatJpeg;
    if (extension == L"png") return &GUID_ContainerFormatPng;
    if (extension == L"tif" || extension == L"tiff") return &GUID_ContainerFormatTiff;
    if (extension == L"bmp") return &GUID_ContainerFormatBmp;
    if (extension == L"gif") return &GUID_ContainerFormatGif;
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
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IWICBitmap* source = nullptr;
    bool success = false;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateStream(&stream))) break;
        if (FAILED(stream->InitializeFromFilename(outputPath, GENERIC_WRITE))) break;
        if (FAILED(factory->CreateEncoder(*format, nullptr, &encoder))) break;
        if (FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache))) break;
        if (FAILED(encoder->CreateNewFrame(&frame, nullptr))) break;
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
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (!success) DeleteFileW(outputPath);
    return success;
}

void ConvertWithSaveDialog(HWND window) {
    wchar_t outputPath[MAX_PATH * 4]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"画像ファイル\0*.jpg;*.jpeg;*.png;*.tif;*.tiff;*.bmp;*.gif;*.webp;*.heic\0すべてのファイル\0*.*\0";
    dialog.lpstrFile = outputPath;
    dialog.nMaxFile = ARRAYSIZE(outputPath);
    dialog.lpstrDefExt = L"png";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) return;
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

std::wstring MetadataText() {
    if (!g_bitmap) return L"";
    wchar_t buffer[256]{};
    swprintf_s(buffer, L"%s  |  %ux%u  |  %s  |  %llu KB",
               g_fileName.c_str(), g_imageWidth, g_imageHeight, g_formatName.c_str(),
               static_cast<unsigned long long>((g_fileSize + 1023) / 1024));
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

void OpenImageDialog(HWND window) {
    wchar_t path[MAX_PATH * 4]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"画像ファイル\0*.jpg;*.jpeg;*.png;*.tif;*.tiff;*.bmp;*.gif;*.webp;*.heic\0すべてのファイル\0*.*\0";
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
    SetMenu(window, menu);
}

void ShowImageContextMenu(HWND window, int x, int y) {
    if (!g_bitmap) return;
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"別形式で保存...");
    const int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, window, nullptr);
    DestroyMenu(menu);
    if (command == 1) ConvertWithSaveDialog(window);
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
                                                        WICDecodeMetadataCacheOnLoad, &decoder);
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
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &attributes)) {
            ULARGE_INTEGER size{};
            size.HighPart = attributes.nFileSizeHigh;
            size.LowPart = attributes.nFileSizeLow;
            g_fileSize = size.QuadPart;
        } else {
            g_fileSize = 0;
        }
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
        return 0;
    case WM_KEYDOWN:
        if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) OpenImageDialog(window);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        Paint(window, dc);
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
    case WM_RBUTTONDOWN:
        if (g_bitmap) {
            g_panning = true;
            g_panMoved = false;
            g_lastPanPoint = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            SetCapture(window);
        }
        return 0;
    case WM_RBUTTONUP: {
        const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        g_panning = false;
        if (GetCapture() == window) ReleaseCapture();
        if (g_bitmap && !g_panMoved) {
            POINT screenPoint = point;
            ClientToScreen(window, &screenPoint);
            ShowImageContextMenu(window, screenPoint.x, screenPoint.y);
        }
        g_panMoved = false;
        return 0;
    }
    case WM_CONTEXTMENU: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        ShowImageContextMenu(window, x, y);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_panning) {
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
        return 0;
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
    if (arguments) LocalFree(arguments);
    if (commandLine && wcscmp(commandLine, L"--help") == 0) {
        MessageBoxW(nullptr, L"QuickImageView 0.1.0\n画像ファイルを引数に指定してください。",
                    L"QuickImageView", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const wchar_t* filePath = commandLine;
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

    HWND window = CreateWindowExW(WS_EX_COMPOSITED, kClassName, L"QuickImageView 0.1.0",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  960, 720, nullptr, nullptr, instance, nullptr);
    if (!window) {
        CoUninitialize();
        return 1;
    }
    BuildMenu(window);
    if (filePath && *filePath && g_bitmap) SetWindowTextW(window, (L"QuickImageView 0.1.0 - " + g_fileName).c_str());
    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    CoUninitialize();
    return static_cast<int>(message.wParam);
}
