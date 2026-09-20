# QuickImageView フォルダ構成

Quickアプリ標準構成に合わせ、ソース、ビルドスクリプト、開発・配布文書を分離します。CMake中間生成物は`build/intermediate/`、完成した配布バイナリは`dist/`直下に置きます。

```text
QuickImageView/
├── .agents/skills/          リポジトリ同梱のCodexスキル
├── .github/                 CI・Release定義
├── assets/                  README用日英スクリーンショット・アイコン原本・作者ワッペン
├── src/app/                 C++/Win32/WICアプリとリソース
│   └── help/                EXEへ埋め込む日英ヘルプ原稿
├── docs/                    仕様・環境・About・配布原本
│   ├── distribution/        配布README・履歴・ライセンス・第三者通知
│   └── loop/                統合検査ループの状態・計画・実行記録
├── installer/               MSI定義とMSIビルドスクリプト
├── plans/                   計画・実施結果
├── scripts/                 アプリのビルド・インストール・パッケージ化
├── build/intermediate/      CMake・ログ・配布ステージ中間生成物（Git管理外）
├── dist/                    完成したEXE・MSI（Git管理外）
├── tests/
│   ├── python/              Python UI検査・固定操作台帳
│   └── tools/               検査補助ツール
├── third_party/             libwebp等の第三者コード
├── CMakeLists.txt           CMake定義
├── HISTORY.md               英語変更履歴
└── HISTORY_jp.md            日本語変更履歴
```

ルートの`README.md`と`README_jp.md`が正式な説明書です。`HISTORY.md`と`HISTORY_jp.md`が日英の履歴です。`src/app/help/help.md`と`help_jp.md`はアプリ内ヘルプのビルド入力で、ビルド時にEXEへ埋め込みます。`docs/distribution/`の文書はZIPとMSIへ同梱します。libwebpのライセンス原本は`third_party/libwebp-1.6.0/COPYING`と`PATENTS`です。

`installer/`にはMSIのWiX定義、MSI専用ライセンス、MSIビルドスクリプトをまとめます。`scripts/`にはアプリ全体のビルド・インストール・ZIP作成処理を置きます。UIはWin32コントロールで実装しており、独立した`src/ui/`素材ディレクトリは設けません。Codexスキルは`.agents/skills/`、検査計画とループ状態は`docs/loop/`に置きます。

このアプリはWin32/WICネイティブ実装のため、CMake定義はルートに置きます。CMake中間生成物・MSI staging・ZIP stagingは`build/intermediate/`へ、配布する完成バイナリは`dist/`直下へ出力します。
