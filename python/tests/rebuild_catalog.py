from __future__ import annotations

import json
from pathlib import Path

from catalog import readme_features

root = Path(__file__).resolve().parents[2]
features = readme_features(root / "README.md")
for index, feature in enumerate(features, start=1):
    feature["id"] = f"feature_{index:03d}"
payload = {
    "schema_version": 1,
    "source": "README.md + README_jp.md",
    "generated_when": "README.mdの機能箇条書き変更時のみ",
    "ui_required": True,
    "operations": features,
}
(root / "python" / "tests" / "operations.json").write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"操作台帳を再構築しました: {len(features)}件")
