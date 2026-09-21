#include "context_menu_entry.h"

#include "native_file_dialog.h"

#include <QDir>
#include <QRegularExpression>

#include <windows.h>

#include <algorithm>
#include <string>

namespace ContextMenuEntry {

namespace {

const wchar_t kVerb[] = L"QuickImageView";
const QString kLegacyAssociation = QStringLiteral("image");  // the old image-wide place

std::wstring entryKey(const wchar_t* root, const QString& association) {
    return std::wstring(root) + L"\\" + association.toStdWString() + L"\\shell\\" + kVerb;
}

QString dotted(const QString& suffix) { return QLatin1Char('.') + suffix; }

QString commandValue(const QString& exePath) {
    return QStringLiteral("\"%1\" \"%2\"").arg(QDir::toNativeSeparators(exePath), QStringLiteral("%1"));
}

// The executable a "shell\<verb>\command" value runs: the quoted first part, or up to the first space.
QString commandExecutable(const QString& command) {
    const QString trimmed = command.trimmed();
    if (trimmed.startsWith(QLatin1Char('"'))) {
        const int end = trimmed.indexOf(QLatin1Char('"'), 1);
        return end < 0 ? trimmed.mid(1) : trimmed.mid(1, end - 1);
    }
    const int space = trimmed.indexOf(QLatin1Char(' '));
    return space < 0 ? trimmed : trimmed.left(space);
}

bool samePath(const QString& a, const QString& b) {
    return QDir::toNativeSeparators(a).compare(QDir::toNativeSeparators(b), Qt::CaseInsensitive) == 0;
}

// Reads a REG_SZ value ("" is the default value); a missing key or value gives a null string.
QString readString(const std::wstring& subKey, const wchar_t* valueName) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return {};
    wchar_t buffer[2048];
    DWORD size = sizeof(buffer) - sizeof(wchar_t);
    DWORD type = 0;
    QString result;
    if (RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS &&
        type == REG_SZ) {
        buffer[size / sizeof(wchar_t)] = L'\0';
        result = QString::fromWCharArray(buffer);
        if (result.isNull()) result = QString(QLatin1String(""));
    }
    RegCloseKey(key);
    return result;
}

bool keyExists(const std::wstring& subKey) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return false;
    RegCloseKey(key);
    return true;
}

bool writeString(HKEY key, const wchar_t* valueName, const QString& value) {
    const std::wstring wide = value.toStdWString();
    return RegSetValueExW(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(wide.c_str()),
                          static_cast<DWORD>((wide.length() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

bool writeEntry(const wchar_t* root, const QString& association, const QString& exePath, const QString& menuText) {
    const std::wstring key = entryKey(root, association);
    HKEY entry = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE | KEY_CREATE_SUB_KEY, nullptr, &entry, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    bool ok = writeString(entry, nullptr, menuText);
    ok = writeString(entry, L"Icon", iconValue(exePath)) && ok;
    HKEY command = nullptr;
    if (RegCreateKeyExW(entry, L"command", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &command,
                        nullptr) == ERROR_SUCCESS) {
        ok = writeString(command, nullptr, commandValue(exePath)) && ok;
        RegCloseKey(command);
    } else {
        ok = false;
    }
    RegCloseKey(entry);
    return ok;
}

// Deletes a key only when it has neither sub keys nor values (nothing of anyone else's is lost).
void deleteKeyIfEmpty(const std::wstring& subKey) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    DWORD subKeys = 0;
    DWORD values = 0;
    const bool queried = RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, &subKeys, nullptr, nullptr, &values,
                                          nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(key);
    if (queried && subKeys == 0 && values == 0) RegDeleteKeyW(HKEY_CURRENT_USER, subKey.c_str());
}

// Deletes the entry and, when they are left empty, the "shell" and extension keys that only held it.
void deleteEntry(const wchar_t* root, const QString& association) {
    const std::wstring key = entryKey(root, association);
    RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str());
    const std::wstring shellKey = key.substr(0, key.rfind(L'\\'));
    deleteKeyIfEmpty(shellKey);
    deleteKeyIfEmpty(shellKey.substr(0, shellKey.rfind(L'\\')));
}

bool entryRunsExecutable(const wchar_t* root, const QString& association, const QString& exePath) {
    const QString command = readString(entryKey(root, association) + L"\\command", nullptr);
    return !command.isEmpty() && samePath(commandExecutable(command), exePath);
}

}  // namespace

QStringList suffixes() {
    QStringList list = NativeFileDialog::openImageSuffixes();
    // Formats that WIC also decodes (see "Supported image formats" in docs/spec.md); the Open dialog does not list them.
    for (const QString& extra : {QStringLiteral("ico"), QStringLiteral("jxr"), QStringLiteral("wdp"),
                                 QStringLiteral("hdp"), QStringLiteral("dds")}) {
        if (!list.contains(extra)) list.append(extra);
    }
    return list;
}

QString iconValue(const QString& exePath) {
    return QStringLiteral("\"%1\",0").arg(QDir::toNativeSeparators(exePath));
}

QString iconTarget(const QString& iconValue) {
    const QString trimmed = iconValue.trimmed();
    if (trimmed.startsWith(QLatin1Char('"'))) {
        const int end = trimmed.indexOf(QLatin1Char('"'), 1);
        return end < 0 ? trimmed.mid(1) : trimmed.mid(1, end - 1);
    }
    QString path = trimmed;
    path.remove(QRegularExpression(QStringLiteral(",\\s*-?\\d+$")));
    return path;
}

bool isRegistered(const wchar_t* root) {
    if (keyExists(entryKey(root, kLegacyAssociation) + L"\\command")) return true;
    const QStringList all = suffixes();
    return std::any_of(all.cbegin(), all.cend(), [root](const QString& suffix) {
        return keyExists(entryKey(root, dotted(suffix)) + L"\\command");
    });
}

bool registerEntries(const QString& exePath, const QString& menuText, const wchar_t* root) {
    bool ok = true;
    for (const QString& suffix : suffixes()) ok = writeEntry(root, dotted(suffix), exePath, menuText) && ok;
    deleteEntry(root, kLegacyAssociation);
    return ok;
}

bool unregisterEntries(const wchar_t* root) {
    deleteEntry(root, kLegacyAssociation);
    for (const QString& suffix : suffixes()) deleteEntry(root, dotted(suffix));
    return !isRegistered(root);
}

bool repair(const QString& exePath, const wchar_t* root) {
    const QString legacy = kLegacyAssociation;
    QStringList associations{legacy};
    for (const QString& suffix : suffixes()) associations.append(dotted(suffix));

    // Only an entry that belongs to this executable is repaired.
    QString menuText;
    bool owned = false;
    for (const QString& association : associations) {
        if (!entryRunsExecutable(root, association, exePath)) continue;
        owned = true;
        if (menuText.isNull()) menuText = readString(entryKey(root, association), nullptr);
    }
    if (!owned) return false;
    if (menuText.isEmpty()) menuText = QStringLiteral("QuickImageView");

    bool changed = false;
    const QString wantedCommand = commandValue(exePath);
    const QString wantedIcon = iconValue(exePath);
    for (const QString& suffix : suffixes()) {
        const QString association = dotted(suffix);
        const std::wstring key = entryKey(root, association);
        const QString command = readString(key + L"\\command", nullptr);
        // Another executable's entry is not ours to change.
        if (!command.isEmpty() && !samePath(commandExecutable(command), exePath)) continue;
        if (command == wantedCommand && readString(key, L"Icon") == wantedIcon) continue;
        if (writeEntry(root, association, exePath, menuText)) changed = true;
    }
    if (entryRunsExecutable(root, legacy, exePath)) {
        deleteEntry(root, legacy);
        changed = true;
    }
    return changed;
}

}  // namespace ContextMenuEntry
