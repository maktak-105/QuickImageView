from __future__ import annotations

import json
import re
from pathlib import Path

SECTIONS = {"表示と基本操作", "リサイズ", "画像編集", "クリップボード", "保存と変換", "Windows連携", "完成・配布検査"}
ADDITIONAL_TEXTS = {
    "EXIF情報を画像本体と重ならないフローティングウィンドウへ表示する",
    "EXIF情報ウィンドウにメーカー、機種、撮影日時などの表示テキストを直接表示する",
    "EXIF情報ウィンドウのコピー操作でEXIFテキストをクリップボードへ格納する",
    "EXIFが存在しない場合も、EXIFなしであることをフローティングウィンドウへ表示する",
    "EXIF情報の確認はOCRではなく、UIコントロールとクリップボード内容を検査する",
}
AUDIT_TEXTS = {
    "アプリ全体をダークテーマで表示する", "タイトルバーをダークテーマで表示する",
    "メニューバーとメニュー項目をダークテーマで表示する", "日英切替ボタンをダークな専用UIとして表示する",
    "EXIF情報ウィンドウをアプリのメニューから開ける", "アプリのヘルプメニューから同梱ヘルプを開ける",
    "日本語ヘルプと英語ヘルプをそれぞれ開ける", "ヘルプの説明が実装済み機能と一致する",
    "英語READMEと日本語READMEの機能・制約・手順が同期している", "英語仕様書と日本語仕様書の機能・制約・手順が同期している",
    "英語配布READMEと日本語配布READMEの内容が同期している", "英語履歴と日本語履歴の内容が同期している",
    "MIT LicenseとlibwebpのCOPYING・PATENTSを配布物へ同梱する", "対応形式とWindows側のWICコーデック依存を文書へ記載する",
    "MSIをRelease版EXEから生成できる", "MSIへEXE、ヘルプ、日英文書、ライセンスを同梱する",
    "MSIの右クリック登録を任意選択としてインストールできる", "MSIの拡張子関連付けを拡張子ごとに任意選択できる",
    "MSIを既定の関連付けなしでインストールできる", "MSIインストール後のEXEから画像とヘルプを開ける",
    "アンインストール時にQuickImageView専用の登録だけを削除する",
}

def readme_features(readme: Path) -> list[dict]:
    # README.md is the English project document; the UI ledger uses the
    # synchronized Japanese document so the operation names remain stable for
    # the existing Japanese UI assertions. Never silently accept an empty
    # catalog when the selected README has English headings.
    candidate = readme
    japanese = any(line.startswith("## 表示と基本操作") for line in readme.read_text(encoding="utf-8-sig").splitlines())
    if not japanese:
        japanese_readme = readme.with_name("README_jp.md")
        if japanese_readme.exists():
            candidate = japanese_readme
    section = ""
    result = []
    for line_number, line in enumerate(candidate.read_text(encoding="utf-8-sig").splitlines(), 1):
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
    base = [feature for feature in result if feature["text"] not in ADDITIONAL_TEXTS and feature["text"] not in AUDIT_TEXTS]
    additional = [feature for feature in result if feature["text"] in ADDITIONAL_TEXTS]
    audit = [feature for feature in result if feature["text"] in AUDIT_TEXTS]
    for index, feature in enumerate(base + additional + audit, 1):
        feature["id"] = f"feature_{index:03d}"
    features = base + additional + audit
    if not features:
        raise RuntimeError(f"機能台帳を生成できません: {candidate}")
    return features

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
