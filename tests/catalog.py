from __future__ import annotations

import json
import re
from pathlib import Path

SECTIONS = {"表示と基本操作", "リサイズ", "画像編集", "クリップボード", "保存と変換", "Windows連携"}

def readme_features(readme: Path) -> list[dict]:
    section = ""
    result = []
    for line_number, line in enumerate(readme.read_text(encoding="utf-8-sig").splitlines(), 1):
        heading = re.match(r"^##\s+(.+?)\s*$", line)
        if heading:
            section = heading.group(1).strip()
            continue
        item = re.match(r"^-\s+(.+?)\s*$", line)
        if section in SECTIONS and item:
            result.append({
                "id": f"feature_{len(result) + 1:03d}",
                "section": section,
                "text": item.group(1).strip(),
                "source": "README.md",
                "source_line": line_number,
                "ui_required": True,
            })
    return result

def load_and_validate(readme: Path, ledger: Path) -> list[dict]:
    expected = readme_features(readme)
    data = json.loads(ledger.read_text(encoding="utf-8-sig"))
    actual = data.get("operations", [])
    if len(expected) != len(actual):
        raise RuntimeError(f"README機能数({len(expected)})と固定台帳数({len(actual)})が不一致です")
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        for key in ("id", "section", "text"):
            if want[key] != got.get(key):
                raise RuntimeError(f"固定台帳の{index}件目がREADMEと不一致です。README変更後に明示再構築してください")
    return actual
