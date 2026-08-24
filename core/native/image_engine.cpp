#include "image_engine.h"

#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QImageReader>
#include <QStringList>

#include <windows.h>
#include <wincodec.h>
#include <propvarutil.h>

namespace {

QString hresultMessage(HRESULT result) {
    return QStringLiteral("WIC error 0x%1").arg(static_cast<quint32>(result), 8, 16, QLatin1Char('0'));
}

QString metadataString(IWICMetadataQueryReader* reader, const wchar_t* query) {
    if (!reader) return {};
    PROPVARIANT value;
    PropVariantInit(&value);
    QString result;
    if (SUCCEEDED(reader->GetMetadataByName(query, &value))) {
        if (value.vt == VT_LPWSTR && value.pwszVal) result = QString::fromWCharArray(value.pwszVal);
        else if (value.vt == VT_BSTR && value.bstrVal) result = QString::fromWCharArray(value.bstrVal);
        else if (value.vt == VT_LPSTR && value.pszVal) result = QString::fromLocal8Bit(value.pszVal);
    }
    PropVariantClear(&value);
    return result.left(128);
}

bool saveHeifWithWic(const QImage& image, const QString& filePath, QString* errorMessage) {
    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IPropertyBag2* properties = nullptr;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
    if (SUCCEEDED(result)) result = stream->InitializeFromFilename(filePath.toStdWString().c_str(), GENERIC_WRITE);
    if (SUCCEEDED(result)) result = factory->CreateEncoder(GUID_ContainerFormatHeif, nullptr, &encoder);
    if (SUCCEEDED(result)) result = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    if (SUCCEEDED(result)) result = encoder->CreateNewFrame(&frame, &properties);
    if (SUCCEEDED(result)) result = frame->Initialize(properties);
    if (SUCCEEDED(result)) result = frame->SetSize(static_cast<UINT>(image.width()), static_cast<UINT>(image.height()));
    GUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(result)) result = frame->SetPixelFormat(&pixelFormat);
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    if (SUCCEEDED(result)) result = frame->WritePixels(static_cast<UINT>(source.height()),
                                                        static_cast<UINT>(source.bytesPerLine()),
                                                        static_cast<UINT>(source.sizeInBytes()),
                                                        const_cast<BYTE*>(source.constBits()));
    if (SUCCEEDED(result)) result = frame->Commit();
    if (SUCCEEDED(result)) result = encoder->Commit();
    if (properties) properties->Release();
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (FAILED(result) && errorMessage) *errorMessage = hresultMessage(result);
    return SUCCEEDED(result);
}

} // namespace

ImageEngine::ExifFields ImageEngine::readExif(const QString& filePath) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICMetadataQueryReader* reader = nullptr;
    ExifFields fields;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) {
        const std::wstring path = filePath.toStdWString();
        result = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                     WICDecodeMetadataCacheOnLoad, &decoder);
    }
    if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(result)) result = frame->GetMetadataQueryReader(&reader);
    if (SUCCEEDED(result)) {
        const wchar_t* roots[] = {L"/app1/ifd/", L"/ifd/"};
        for (const wchar_t* root : roots) {
            if (fields.make.isEmpty()) fields.make = metadataString(reader, (std::wstring(root) + L"{ushort=271}").c_str());
            if (fields.model.isEmpty()) fields.model = metadataString(reader, (std::wstring(root) + L"{ushort=272}").c_str());
            if (fields.taken.isEmpty()) fields.taken = metadataString(reader, (std::wstring(root) + L"{ushort=36867}").c_str());
            if (fields.taken.isEmpty()) fields.taken = metadataString(reader, (std::wstring(root) + L"{ushort=306}").c_str());
        }
    }
    if (reader) reader->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();
    return fields;
}

QString ImageEngine::formatExifText(const ExifFields& fields, bool english) {
    if (fields.isEmpty()) {
        return english ? QStringLiteral("EXIF: none") : QStringLiteral("EXIF: なし");
    }
    QStringList parts;
    if (!fields.make.isEmpty()) {
        parts.append((english ? QStringLiteral("Make=") : QStringLiteral("メーカー=")) + fields.make);
    }
    if (!fields.model.isEmpty()) {
        parts.append((english ? QStringLiteral("Model=") : QStringLiteral("機種=")) + fields.model);
    }
    if (!fields.taken.isEmpty()) {
        parts.append((english ? QStringLiteral("Taken=") : QStringLiteral("撮影日時=")) + fields.taken);
    }
    return QStringLiteral("EXIF: ") + parts.join(QStringLiteral("  "));
}

QImage ImageEngine::load(const QString& filePath, QString* errorMessage) {
    if (errorMessage) errorMessage->clear();
    if (!QFileInfo::exists(filePath)) {
        if (errorMessage) *errorMessage = QStringLiteral("The image file was not found.");
        return {};
    }

    if (QFileInfo(filePath).suffix().compare(QStringLiteral("webp"), Qt::CaseInsensitive) == 0) {
        QImageReader reader(filePath);
        const QImage image = reader.read();
        if (image.isNull() && errorMessage) *errorMessage = reader.errorString();
        return image;
    }

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) {
        const std::wstring path = filePath.toStdWString();
        result = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                    WICDecodeMetadataCacheOnLoad, &decoder);
    }
    if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(result)) {
        result = converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
                                       WICBitmapDitherTypeNone, nullptr, 0.0,
                                       WICBitmapPaletteTypeCustom);
    }

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(result)) result = converter->GetSize(&width, &height);
    QImage image;
    if (SUCCEEDED(result) && width > 0 && height > 0) {
        image = QImage(static_cast<int>(width), static_cast<int>(height), QImage::Format_ARGB32);
        if (image.isNull()) {
            result = E_OUTOFMEMORY;
        } else {
            result = converter->CopyPixels(nullptr, static_cast<UINT>(image.bytesPerLine()),
                                           static_cast<UINT>(image.sizeInBytes()), image.bits());
        }
    }

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();

    if (FAILED(result) || image.isNull()) {
        if (errorMessage) *errorMessage = hresultMessage(result);
        return {};
    }
    return image;
}

bool ImageEngine::save(const QImage& image, const QString& filePath, QString* errorMessage) {
    return save(image, filePath, SaveOptions{}, errorMessage);
}

bool ImageEngine::save(const QImage& image, const QString& filePath, const SaveOptions& options,
                       QString* errorMessage) {
    if (errorMessage) errorMessage->clear();
    if (image.isNull()) {
        if (errorMessage) *errorMessage = QStringLiteral("The image is empty.");
        return false;
    }

    QImageWriter writer(filePath);
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg") ||
        suffix == QStringLiteral("webp") || suffix == QStringLiteral("heic") ||
        suffix == QStringLiteral("heif")) {
        writer.setQuality(qBound(0, options.quality, 100));
    }
    if (suffix == QStringLiteral("png") || suffix == QStringLiteral("tif") ||
        suffix == QStringLiteral("tiff")) {
        writer.setCompression(qBound(0, options.compression, 9));
    }
    if (!writer.write(image)) {
        if ((suffix == QStringLiteral("heic") || suffix == QStringLiteral("heif")) &&
            saveHeifWithWic(image, filePath, errorMessage)) return true;
        if (errorMessage) *errorMessage = writer.errorString();
        return false;
    }
    return true;
}

QImage ImageEngine::resize(const QImage& image, int width, int height) {
    if (image.isNull() || width <= 0 || height <= 0 || width > 100000 || height > 100000) return {};
    return image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage ImageEngine::convertColor(const QImage& image, int mode) {
    if (image.isNull()) return {};
    if (mode == 0) return image.convertToFormat(QImage::Format_ARGB32);
    if (mode == 1) return image.convertToFormat(QImage::Format_Indexed8, Qt::DiffuseDither);
    if (mode == 2) return image.convertToFormat(QImage::Format_Grayscale8);
    return {};
}
