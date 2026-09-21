# QuickImageView フォルダ構成

Quickアプリ標準構成（`___appli-template`）に合わせ、ソース、ビルドスクリプト、開発・配布文書を分離します。CMake中間生成物は`build/intermediate/`、完成した実行ファイルとQtランタイムは`dist/`直下に置きます。

```text
QuickImageView/
├── .github/workflows/       CI・Release定義
├── assets/                  アイコン原本・作者ワッペン
├── src/
│   ├── app/                 C++エンジン・Qt連携・エントリーポイント・アイコン
│   │   └── help/            EXEへ埋め込む日英ヘルプ原稿
│   └── ui/                  QML画面・共通部品
├── docs/                    仕様・環境・About・配布原本
│   └── distribution/        配布README・履歴・ライセンス・第三者通知
├── plans/                   計画・実施結果
├── scripts/                 ビルド・Qtデプロイ・インストール・パッケージ化
├── build/intermediate/      CMake・ログ・配布ステージ中間生成物（Git管理外）
├── dist/                    完成したEXEとQtランタイム（Git管理外）
├── tests/
│   ├── qt/                  コントローラー・エンジンのQt Test
│   └── qml/                 Qt Quick Test
├── CMakeLists.txt           CMake定義
├── HISTORY.md               英語変更履歴
└── HISTORY_jp.md            日本語変更履歴
```

ルートの`README.md`と`README_jp.md`が正式な説明書です。`HISTORY.md`と`HISTORY_jp.md`が日英の履歴です。`src/app/help/help.md`と`help_jp.md`はアプリ内ヘルプのビルド入力で、ビルド時にEXEへ埋め込みます。`docs/distribution/`の文書はZIPへ同梱します。

`scripts/`にはアプリ全体のビルド（`build.bat`、`build.py`）、Qtランタイム配置（`deploy.py`）、インストール・アンインストール、ZIP作成処理、パッケージ検査（`verify_package.ps1`）を置きます。CMake中間生成物とZIPステージングは`build/intermediate/`へ、完成したバイナリは`dist/`直下へ出力します。
