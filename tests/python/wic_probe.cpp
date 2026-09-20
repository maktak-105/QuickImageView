#include <windows.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

static void PrintHr(const wchar_t* label, HRESULT hr) {
    std::wcout << label << L": 0x" << std::hex << static_cast<unsigned long>(hr) << std::dec << L"\n";
}

static HRESULT Encode(IWICImagingFactory* factory, IWICBitmapSource* source,
                      REFGUID container, const wchar_t* output) {
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IPropertyBag2* options = nullptr;
    IStream* stream = nullptr;
    HRESULT hr = SHCreateStreamOnFileEx(output, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE,
                                        FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream);
    if (SUCCEEDED(hr)) hr = factory->CreateEncoder(container, nullptr, &encoder);
    if (SUCCEEDED(hr)) hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    if (SUCCEEDED(hr)) hr = encoder->CreateNewFrame(&frame, &options);
    if (SUCCEEDED(hr)) hr = frame->Initialize(options);
    if (SUCCEEDED(hr)) hr = frame->WriteSource(source, nullptr);
    if (SUCCEEDED(hr)) hr = frame->Commit();
    if (SUCCEEDED(hr)) hr = encoder->Commit();
    if (options) options->Release();
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    return hr;
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    PrintHr(L"factory", hr);
    if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromFilename(argv[1], nullptr, GENERIC_READ,
                                                                 WICDecodeMetadataCacheOnLoad, &decoder);
    PrintHr(L"decoder", hr);
    if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
    PrintHr(L"frame", hr);
    if (SUCCEEDED(hr)) {
        HRESULT webp = Encode(factory, frame, GUID_ContainerFormatWebp, L"wic-probe.webp");
        HRESULT heif = Encode(factory, frame, GUID_ContainerFormatHeif, L"wic-probe.heic");
        PrintHr(L"webp_encode", webp);
        PrintHr(L"heif_encode", heif);
    }
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();
    CoUninitialize();
    return 0;
}
