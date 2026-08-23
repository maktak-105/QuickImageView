from __future__ import annotations

import json
from pathlib import Path

from catalog import readme_features

root = Path(__file__).resolve().parents[1]
features = readme_features(root / "README.md")
payload = {
    "schema_version": 1,
    "source": "README.md",
    "generated_when": "README.mdの機能箇条書き変更時のみ",
    "ui_required": True,
    "operations": features,
}
(root / "tests" / "operations.json").write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"操作台帳を再構築しました: {len(features)}件")
