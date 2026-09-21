#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// Windows common file dialogs (IFileDialog). They replace QtQuick.Dialogs, which pulled three Qt DLLs and
// a set of QML modules into the distribution only to open the same native dialog.
namespace NativeFileDialog {

struct FileType {
    QString name;          // for example "JPEG"
    QStringList suffixes;  // without dot; the first one is the preferred extension
};

struct SaveResult {
    bool accepted = false;
    QString path;
    QString suffix;  // preferred extension of the file type that was selected in the dialog
};

// File types offered by Save as, in dialog order. Every entry must be writable by ImageEngine::save().
QList<FileType> saveFileTypes();

// Extensions offered by Open image.
QStringList openImageSuffixes();

// Both calls are modal and use the active window as owner. An empty path / not accepted means cancelled.
QString pickOpenFile(const QString& title, const QString& typeLabel, const QStringList& suffixes);
// initialFolder: the folder the dialog opens in (every time, not only the first). Empty = Windows' choice.
SaveResult pickSaveFile(const QString& title, const QList<FileType>& types, const QString& initialFolder = QString());

}  // namespace NativeFileDialog
