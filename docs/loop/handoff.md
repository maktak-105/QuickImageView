# QuickImageView 引き継ぎ

## 現在の状態

- リポジトリ: `C:\Users\makta\source\QuickImageView`
- ブランチ: `main`
- 最新コミット: `eb12d60`（`origin/main`へプッシュ済み）
- 直近コミット時点の作業ツリー: クリーン（この引き継ぎ更新自体は未コミット）
- PowerShell: 7.6.5
- アプリ: QuickImageView 2.1.0

## 正本と検査の入口

- 機能仕様の正本: `README.md`
- 日本語仕様: `README_jp.md`、`docs/spec_jp.md`
- 固定操作台帳: `tests/python/operations.json`
- 台帳再生成: `python tests/python/rebuild_catalog.py`
- ループ設計: `plans/test-rebuild-python-001.md`
- 正式ループ入口: `python tests/python/run_loop.py`
- UI操作本体: `tests/python/ui_test.py`
- 直近結果: `docs/loop/current.json`
- HTMLレポート: `docs/loop/report.html`

READMEの実装対象は現在98件で、`tests/python/operations.json`の操作台帳と1:1で対応している。通常の検査実行では台帳を再生成しない。READMEの機能を追加・変更・削除した場合だけ、明示的に次を実行して台帳差分を確認する。

```powershell
python .\tests\python\rebuild_catalog.py
```

## 直近の記録

以下はREADME全64項目だった時点の過去記録で、現在の98項目全体に対する検査結果ではありません。フォルダ移行後の再検査は未実施です。実行順は次のとおり。

1. CMake configure
2. ビルド
3. ビルド成果物をインストール
4. ビルド版とインストール版の実行ファイル同一性を確認
5. インストール済みアプリを起動し、UI操作を機能ごとに独立プロセスで1回ずつ実行
6. UI検査の後にCTestと`--self-test`を実行
7. `docs/loop/current.json`と`docs/loop/report.html`へ直近結果を出力

直近結果:

- UI対象: 57件
- UI PASS: 57件
- UI FAIL/ERROR/UNCHECKED: 0件
- CTest: PASS
- `--self-test`: PASS
- install-time専用の除外: 7件（過去記録の64件には含まれるが、当時のアプリ起動後UI検査対象外）
- 除外ID: `feature_057`～`feature_063`

UIでFAIL・ERROR・UNCHECKEDになった機能を、CLI・静的検査・関数検査で合格扱いにしてはいけない。

## 完了済みの主な変更

- 起動後の画像ファイルD&Dを実装
- 画像表示中のD&Dで確認メッセージを表示し、了承時だけ現在画像を閉じて新画像を開く処理を実装
- D&DをREADME・操作台帳・UI検査へ追加
- libwebp 1.6.0をソース組込みし、WebP保存を実装
- libwebpの`COPYING`・`PATENTS`と配布時の注意を`docs/third_party_licenses.md`へ記録
- 旧PowerShell検査プログラム、旧台帳、旧ループ管理ファイルを削除
- Python + pywinautoによる実UI検査へ再構築
- 保存拒否、パン、EXIFのUI検査判定を実画面の挙動に合わせて修正

## 次に扱う未着手事項: EXIF表示の改善

ユーザーから次の方向性が提示されているが、まだREADMEへの仕様追加も実装もしていない。

- EXIFを画像本体と重ならないフローティングウィンドウで表示する
- EXIF情報のコピーボタンを付ける
- コピーボタンでEXIFテキストをクリップボードへコピーする
- EXIFなしの場合の表示を定義する
- OCRではなく、フローティングウィンドウのUI要素とクリップボード内容を直接検査する

現行READMEにあるのは「EXIFが存在する場合、メーカー、機種、撮影日時など代表的な情報を表示する」だけであり、EXIFコピーは未記載である。この引継ぎ時点の未実装指摘であり、現在はREADMEと仕様書へ反映済みである。仕様変更時は `python tests/python/rebuild_catalog.py` で台帳を更新すること。

現行のEXIF UI検査は画像上部の文字をOCRしているため、背景色・画像内容・アンチエイリアスの影響を受ける。これは暫定的な既存検査であり、フローティングウィンドウ実装後はOCR判定を残さず、UIコントロールの存在・表示テキスト・コピー結果を直接検査する。

## 運用上の注意

- 作業開始時に`C:\Users\makta\.codex\AGENTS.md`とリポジトリのREADME・関連仕様を確認する。
- 1ループの変更は1つの明確な変更に限定し、Plan / Act / Observe / Reflectで記録する。
- 実機確認できていないものをPASSと報告しない。
- 作業時の企画・計画・実装・評価は`plans/`または本ファイルなど所定ドキュメントへ残す。
- `msedgewebview2.exe`をプロセス名だけで終了しない。対象アプリのPIDをコマンドラインで特定してから扱う。
- commit・push・アプリ実装変更はユーザーの明示指示がある場合だけ行う。
