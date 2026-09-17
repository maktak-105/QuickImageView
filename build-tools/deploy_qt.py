"""Collect the Qt runtime next to QuickImageViewQt.exe with windeployqt."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
QT_DIST = ROOT / "dist" / "binary"
QT_BINARY = QT_DIST / "QuickImageViewQt.exe"

UNUSED_QML_STYLES = (
    "Fusion",
    "Imagine",
    "Material",
    "Universal",
    "FluentWinUI3",
    "Windows",
)
UNUSED_DLL_PREFIXES = (
    "Qt6QuickControls2Fusion",
    "Qt6QuickControls2Imagine",
    "Qt6QuickControls2Material",
    "Qt6QuickControls2Universal",
    "Qt6QuickControls2FluentWinUI3",
    "Qt6QuickControls2Windows",
    "dxcompiler",
    "dxil",
    "D3Dcompiler_47",
    "Qt6LabsFolderListModel",
    "Qt6QuickEffects",
    "Qt6QuickShapes",
)
UNUSED_QML_DIRS = (
    Path("qml") / "QtQuick" / "LocalStorage",
    Path("qml") / "QtQuick" / "NativeStyle",
    Path("qml") / "QtQuick" / "Particles",
    Path("qml") / "QtQuick" / "tooling",
    Path("qml") / "QtQuick" / "VectorImage",
    Path("qml") / "QtQml" / "XmlListModel",
    Path("qml") / "QtQuick" / "Dialogs" / "quickimpl" / "qml" / "+Fusion",
    Path("qml") / "QtQuick" / "Dialogs" / "quickimpl" / "qml" / "+Imagine",
    Path("qml") / "QtQuick" / "Dialogs" / "quickimpl" / "qml" / "+Material",
    Path("qml") / "QtQuick" / "Dialogs" / "quickimpl" / "qml" / "+Universal",
    Path("qml") / "Qt" / "labs",
    Path("qml") / "QtQuick" / "Effects",
    Path("qml") / "QtQuick" / "Shapes",
)


def require_file(path: Path, description: str) -> None:
    if not path.is_file():
        raise SystemExit(f"{description} was not found: {path}")


def find_windeployqt() -> Path:
    qt_root = os.environ.get("QT_ROOT")
    if qt_root:
        candidate = Path(qt_root) / "bin" / "windeployqt.exe"
        if candidate.is_file():
            return candidate
    found = shutil.which("windeployqt") or shutil.which("windeployqt.exe")
    if found:
        return Path(found)
    raise SystemExit("windeployqt was not found. Set QT_ROOT or add Qt bin to PATH.")


def deploy() -> None:
    require_file(QT_BINARY, "Qt build output")
    windeployqt = find_windeployqt()
    keep_names = {QT_BINARY.name.lower()}
    QT_DIST.mkdir(parents=True, exist_ok=True)
    for item in QT_DIST.iterdir():
        if item.name.lower() in keep_names:
            continue
        if item.is_dir():
            shutil.rmtree(item)
        else:
            item.unlink()

    command = [
        str(windeployqt),
        "--release",
        "--force",
        "--qmldir", str(ROOT / "qml"),
        "--no-translations",
        "--no-opengl-sw",
        "--skip-plugin-types",
        "qmltooling,generic,networkinformation,tls,qmllint,qmlls,designer,help,sqldrivers,styles",
        "--exclude-plugins", "qicns,qtga,qwbmp,qsvgicon",
        "--no-quickcontrols2imagine",
        "--no-quickcontrols2imaginestyleimpl",
        "--no-quickcontrols2material",
        "--no-quickcontrols2materialstyleimpl",
        "--no-quickcontrols2universal",
        "--no-quickcontrols2universalstyleimpl",
        "--no-quickcontrols2fusion",
        "--no-quickcontrols2fusionstyleimpl",
        "--no-quickcontrols2fluentwinui3styleimpl",
        "--no-quickcontrols2windowsstyleimpl",
        str(QT_BINARY),
    ]
    result = subprocess.run(command, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)

    controls_dir = QT_DIST / "qml" / "QtQuick" / "Controls"
    if controls_dir.is_dir():
        for style in UNUSED_QML_STYLES:
            style_dir = controls_dir / style
            if style_dir.exists():
                shutil.rmtree(style_dir)

    for prefix in UNUSED_DLL_PREFIXES:
        for leftover in QT_DIST.glob(prefix + "*"):
            leftover.unlink()

    for relative in UNUSED_QML_DIRS:
        leftover_dir = QT_DIST / relative
        if leftover_dir.exists():
            shutil.rmtree(leftover_dir)

    icon_source = ROOT / "resources" / "icons" / "QuickImageView.ico"
    if icon_source.is_file():
        shutil.copy2(icon_source, QT_DIST / "QuickImageView.ico")


if __name__ == "__main__":
    deploy()
    sys.exit(0)
