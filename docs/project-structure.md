# QuickImageView フォルダ構成

Quickアプリ標準構成に合わせ、ビルドの出力先はリポジトリ直下の `dist/` に固定する。ルート直下に `build/` フォルダは作成しない。

```text
QuickImageView/
├── .github/                 CI・Release定義
├── assets/                  README用スクリーンショット・アイコン原本
├── core/native/             C++/Win32アプリ本体
├── document/                仕様・ヘルプ・配布原本（日英）
├── docs/                    検査ループ・構成記録
├── installer/               MSI定義とビルドスクリプト
├── plans/                   計画・実施結果
├── python/
│   ├── tests/               Python検査コード
│   └── tools/               補助ツール
├── scripts/                 インストール・パッケージ化
├── static/                  開発用静的ファイル
├── third_party/             libwebp等の第三者コード
├── dist/
│   ├── binary/              ビルド・配布パッケージ成果物
│   │   └── QuickImageView.exe
│   └── documents/           配布README（日英）のみ
├── build-tools/             ビルド・ヘルプ埋め込みスクリプト
├── build/                   CMake中間生成物（Git管理外）
├── build.bat                ビルド入口
└── CMakeLists.txt           CMake定義
```

ルートの`README.md`、`README_jp.md`、`history.md`、`history_jp.md`が正式なREADME・履歴原本である。`resources/help/help.md`と`help_jp.md`はアプリ内ヘルプのビルド入力で、ビルド時にEXEへ埋め込む。`dist/documents/`には配布用README、履歴、ライセンス、第三者通知を置き、`dist/binary/`にはバイナリだけを置く。libwebpのライセンス原本は`third_party/libwebp-1.6.0/COPYING`と`PATENTS`である。

`build/`内のCMake中間生成物とMSI stagingはGit管理対象外とする。`dist/`直下には`binary/`と`documents/`だけを置き、ソース・文書・検査コードをビルド生成物と混在させない。
