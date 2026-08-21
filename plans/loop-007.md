# ループ007 実施記録

## 対象

EXIFの代表項目を表示する。

## 実装

- WICメタデータクエリを追加。
- Make、Model、DateTimeOriginalを取得。
- DateTimeOriginalがない場合はDateTimeへフォールバック。
- JPEG系とTIFF系の代表クエリパスを試行。
- PROPVARIANTのLPSTR、LPWSTR、BSTRを安全に文字列化。
- EXIFがない・壊れている場合も画像表示を継続。
- 表示文字列の長さを制限。

## 検証

- CMake build: 成功
- CTest: 1件成功
- 実画像起動: プロセス維持を確認
- EXIFなし画像でも表示経路が失敗しない構成を確認。

## 次ループ

1. 総合テストを追加する。
2. 表示形式・巨大画像の性能を計測する。
3. 配布物を作成し、インストール後の右クリック起動を実機確認する。
