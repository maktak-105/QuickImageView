#pragma once

#include <QString>
#include <QStringList>

// The "Open with QuickImageView" entry of the Windows Explorer right-click menu (File > Settings).
//
// It is registered once per file extension, under <root>\.<ext>\shell\QuickImageView, and not once for all
// images (<root>\image\shell\QuickImageView, which 3.x and 4.0/4.1 used): on some PCs the shell does not
// apply the image-wide key to files such as .heic, .heif and .webp (measured with
// IContextMenu::QueryContextMenu), while a per-extension key is always applied.
//
// Every function takes the registry root as a HKCU sub key so that tests can work in a scratch tree.
namespace ContextMenuEntry {

constexpr const wchar_t* kDefaultRoot = L"Software\\Classes\\SystemFileAssociations";

// Extensions (lower case, without dot) that get the entry: every image the application opens.
QStringList suffixes();

// Registry "Icon" value for the entry: the icon embedded in the executable, "<exe>",0. It does not depend on a
// separate .ico file (the distribution has none).
QString iconValue(const QString& exePath);

// The file part of an "Icon" value: "C:\a b\x.exe",0 / C:\x.exe,0 / C:\x.ico -> the path only.
QString iconTarget(const QString& iconValue);

// True when the entry exists for at least one extension (or in the old image-wide place).
bool isRegistered(const wchar_t* root = kDefaultRoot);

// Writes the entry for every extension of suffixes() and removes the old image-wide entry. Overwrites what
// is there: the user asked for this copy to own the entry.
bool registerEntries(const QString& exePath, const QString& menuText, const wchar_t* root = kDefaultRoot);

// Removes the entry everywhere (every extension and the old image-wide place).
bool unregisterEntries(const wchar_t* root = kDefaultRoot);

// Startup repair of an entry that belongs to this executable (its command runs exePath): adds the missing
// per-extension entries, rewrites stale icons, and removes the old image-wide entry. Entries that run another
// executable, and a PC without the entry, are left alone. Returns true when something was changed.
bool repair(const QString& exePath, const wchar_t* root = kDefaultRoot);

}  // namespace ContextMenuEntry
