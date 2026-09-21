#include "native_file_dialog.h"

#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>

namespace {

QString itemPath(IShellItem* item) {
    PWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw)) || !raw) return {};
    const QString path = QString::fromWCharArray(raw);
    CoTaskMemFree(raw);
    return path;
}

QString patternOf(const QStringList& suffixes) {
    QStringList patterns;
    for (const QString& suffix : suffixes) patterns.append(QStringLiteral("*.") + suffix);
    return patterns.join(QLatin1Char(';'));
}

// Filter entries own their strings, so the pointers handed to COMDLG_FILTERSPEC stay valid.
class FilterSpecs {
public:
    void add(const QString& label, const QStringList& suffixes) {
        const QString pattern = patternOf(suffixes);
        names_.push_back((label + QStringLiteral(" (") + QString(pattern).replace(QLatin1Char(';'), QLatin1Char(' '))
                          + QLatin1Char(')')).toStdWString());
        patterns_.push_back(pattern.toStdWString());
    }
    std::vector<COMDLG_FILTERSPEC> specs() const {
        std::vector<COMDLG_FILTERSPEC> result;
        for (size_t i = 0; i < names_.size(); ++i) result.push_back({names_[i].c_str(), patterns_[i].c_str()});
        return result;
    }

private:
    std::vector<std::wstring> names_;
    std::vector<std::wstring> patterns_;
};

}  // namespace

namespace NativeFileDialog {

QList<FileType> saveFileTypes() {
    return {
        {QStringLiteral("PNG"), {QStringLiteral("png")}},
        {QStringLiteral("JPEG"), {QStringLiteral("jpg"), QStringLiteral("jpeg")}},
        {QStringLiteral("BMP"), {QStringLiteral("bmp")}},
        {QStringLiteral("TIFF"), {QStringLiteral("tif"), QStringLiteral("tiff")}},
        {QStringLiteral("WebP"), {QStringLiteral("webp")}},
        {QStringLiteral("HEIC/HEIF"), {QStringLiteral("heic"), QStringLiteral("heif")}},
    };
}

QStringList openImageSuffixes() {
    return {QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"), QStringLiteral("tif"),
            QStringLiteral("tiff"), QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("webp"),
            QStringLiteral("heic"), QStringLiteral("heif")};
}

QString pickOpenFile(const QString& title, const QString& typeLabel, const QStringList& suffixes) {
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return {};
    }
    QString path;
    FilterSpecs filters;
    filters.add(typeLabel, suffixes);
    const std::vector<COMDLG_FILTERSPEC> specs = filters.specs();
    const std::wstring wideTitle = title.toStdWString();
    dialog->SetTitle(wideTitle.c_str());
    dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    dialog->SetFileTypeIndex(1);
    FILEOPENDIALOGOPTIONS options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
    if (SUCCEEDED(dialog->Show(GetActiveWindow()))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            path = itemPath(item);
            item->Release();
        }
    }
    dialog->Release();
    return path;
}

SaveResult pickSaveFile(const QString& title, const QList<FileType>& types) {
    SaveResult result;
    if (types.isEmpty()) return result;
    IFileSaveDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return result;
    }
    FilterSpecs filters;
    for (const FileType& type : types) filters.add(type.name, type.suffixes);
    const std::vector<COMDLG_FILTERSPEC> specs = filters.specs();
    const std::wstring wideTitle = title.toStdWString();
    dialog->SetTitle(wideTitle.c_str());
    dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    dialog->SetFileTypeIndex(1);
    // The dialog adds the extension of the selected file type when none is typed. Overwriting is refused by
    // the caller, so no overwrite prompt is needed here.
    FILEOPENDIALOGOPTIONS options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    if (SUCCEEDED(dialog->Show(GetActiveWindow()))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            result.path = itemPath(item);
            item->Release();
            UINT index = 0;
            if (SUCCEEDED(dialog->GetFileTypeIndex(&index)) && index >= 1 && index <= static_cast<UINT>(types.size())) {
                result.suffix = types.at(static_cast<int>(index) - 1).suffixes.first();
            }
            result.accepted = !result.path.isEmpty();
        }
    }
    dialog->Release();
    return result;
}

}  // namespace NativeFileDialog
