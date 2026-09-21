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

// Writes the image with a WIC encoder. tiffCompression < 0 and imageQuality < 0 mean "not set".
// Returns S_OK on success, otherwise the failing HRESULT.
HRESULT saveWithWic(const QImage& image, const QString& filePath, const GUID& containerFormat,
                    int tiffCompression, float imageQuality) {
    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IPropertyBag2* properties = nullptr;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
    if (SUCCEEDED(result)) result = stream->InitializeFromFilename(filePath.toStdWString().c_str(), GENERIC_WRITE);
    if (SUCCEEDED(result)) result = factory->CreateEncoder(containerFormat, nullptr, &encoder);
    if (SUCCEEDED(result)) result = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    if (SUCCEEDED(result)) result = encoder->CreateNewFrame(&frame, &properties);
    if (SUCCEEDED(result) && properties && (tiffCompression >= 0 || imageQuality >= 0.0f)) {
        PROPBAG2 options[2] = {};
        VARIANT values[2];
        ULONG count = 0;
        if (tiffCompression >= 0) {
            options[count].pstrName = const_cast<LPOLESTR>(L"TiffCompressionMethod");
            VariantInit(&values[count]);
            values[count].vt = VT_UI1;
            values[count].bVal = static_cast<BYTE>(tiffCompression);
            ++count;
        }
        if (imageQuality >= 0.0f) {
            options[count].pstrName = const_cast<LPOLESTR>(L"ImageQuality");
            VariantInit(&values[count]);
            values[count].vt = VT_R4;
            values[count].fltVal = qBound(0.0f, imageQuality, 1.0f);
            ++count;
        }
        result = properties->Write(count, options, values);
    }
    if (SUCCEEDED(result)) result = frame->Initialize(properties);
    if (SUCCEEDED(result)) result = frame->SetSize(static_cast<UINT>(image.width()), static_cast<UINT>(image.height()));
    // The encoder answers with the pixel format it wants (the HEIF encoder wants 32bppBGR, not BGRA).
    GUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(result)) result = frame->SetPixelFormat(&pixelFormat);
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    IWICBitmap* bitmap = nullptr;
    IWICFormatConverter* converter = nullptr;
    if (SUCCEEDED(result)) {
        result = factory->CreateBitmapFromMemory(static_cast<UINT>(source.width()), static_cast<UINT>(source.height()),
                                                 GUID_WICPixelFormat32bppBGRA, static_cast<UINT>(source.bytesPerLine()),
                                                 static_cast<UINT>(source.sizeInBytes()),
                                                 const_cast<BYTE*>(source.constBits()), &bitmap);
    }
    if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converter);
    // Convert to whatever the encoder asked for; a plain copy when it accepted BGRA.
    if (SUCCEEDED(result)) {
        result = converter->Initialize(bitmap, pixelFormat, WICBitmapDitherTypeNone, nullptr, 0.0,
                                       WICBitmapPaletteTypeCustom);
    }
    if (SUCCEEDED(result)) result = frame->WriteSource(converter, nullptr);
    if (SUCCEEDED(result)) result = frame->Commit();
    if (SUCCEEDED(result)) result = encoder->Commit();
    if (converter) converter->Release();
    if (bitmap) bitmap->Release();
    if (properties) properties->Release();
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    return result;
}

HRESULT saveHeifWithWic(const QImage& image, const QString& filePath, int quality) {
    return saveWithWic(image, filePath, GUID_ContainerFormatHeif, -1,
                       static_cast<float>(qBound(0, quality, 100)) / 100.0f);
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
    const bool existedBefore = QFileInfo::exists(filePath);
    if (existedBefore) {
        // Never overwrite: the encoders open (and empty) the file before they write, so refuse up front.
        if (errorMessage) *errorMessage = QStringLiteral("Overwriting an existing file is not allowed.");
        return false;
    }
    const bool saved = saveWithoutCleanup(image, filePath, options, errorMessage);
    // An encoder creates the file before it writes anything. A failed save must not leave an empty file behind
    // (it would block the next attempt as "already exists").
    if (!saved) QFile::remove(filePath);
    return saved;
}

bool ImageEngine::saveWithoutCleanup(const QImage& image, const QString& filePath, const SaveOptions& options,
                                     QString* errorMessage) {
    if (errorMessage) errorMessage->clear();
    if (image.isNull()) {
        if (errorMessage) *errorMessage = QStringLiteral("The image is empty.");
        return false;
    }

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("tif") || suffix == QStringLiteral("tiff")) {
        // Qt's own TIFF support is a plugin (qtiff) that not every Qt build ships, so TIFF is written
        // with WIC. WIC has no compression level: 0 = uncompressed, 1-9 = LZW.
        const int method = options.compression > 0 ? WICTiffCompressionLZW : WICTiffCompressionNone;
        const HRESULT tiffResult = saveWithWic(image, filePath, GUID_ContainerFormatTiff, method, -1.0f);
        if (FAILED(tiffResult) && errorMessage) *errorMessage = hresultMessage(tiffResult);
        return SUCCEEDED(tiffResult);
    }
    QImageWriter writer(filePath);
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg") ||
        suffix == QStringLiteral("webp") || suffix == QStringLiteral("heic") ||
        suffix == QStringLiteral("heif")) {
        writer.setQuality(qBound(0, options.quality, 100));
    }
    if (suffix == QStringLiteral("png")) {
        writer.setCompression(qBound(0, options.compression, 9));
    }
    if (!writer.write(image)) {
        if (suffix == QStringLiteral("heic") || suffix == QStringLiteral("heif")) {
            const HRESULT heifResult = saveHeifWithWic(image, filePath, options.quality);
            if (SUCCEEDED(heifResult)) return true;
            if (errorMessage) {
                *errorMessage = QStringLiteral("HEIC/HEIF could not be saved. Saving needs the Windows HEIF encoder "
                                               "(HEIF Image Extensions and HEVC Video Extensions). %1")
                                    .arg(hresultMessage(heifResult));
            }
            return false;
        }
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

bool ImageEngine::convertFile(const QString& sourcePath, const QString& destinationPath,
                              QString* errorMessage) {
    if (errorMessage) errorMessage->clear();
    const QFileInfo source(sourcePath);
    const QFileInfo destination(destinationPath);
    if (!source.isFile()) {
        if (errorMessage) *errorMessage = QStringLiteral("The source image was not found.");
        return false;
    }
    if (source.absoluteFilePath().compare(destination.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        if (errorMessage) *errorMessage = QStringLiteral("The destination must differ from the source.");
        return false;
    }
    if (destination.exists()) {
        if (errorMessage) *errorMessage = QStringLiteral("Overwriting an existing file is not allowed.");
        return false;
    }
    const QImage image = load(source.absoluteFilePath(), errorMessage);
    if (image.isNull()) return false;
    return save(image, destination.absoluteFilePath(), SaveOptions{}, errorMessage);
}
