# サードパーティライセンス

[English version third_party_licenses.md](third_party_licenses.md)

## Qt 6.10.3

QuickImageViewはQt 6（Core、Gui、Qml、Quick、Quick Controls 2、Quick Dialogs、Svgなどのモジュール）を使用します。Qtライブラリは動的ライブラリ（DLL）として実行ファイルの横に配置し、QuickImageViewでは改変していません。

- QtはGNU Lesser General Public License version 3などのライセンスで提供されています。https://www.qt.io/licensing/ と https://doc.qt.io/qt-6/lgpl.html を参照してください。

## libwebp（QtのWebP画像フォーマットプラグインに含まれる）

WebPファイルの読み書きは、Qtのqtimageformatsモジュールに含まれるWebP画像フォーマットプラグイン`imageformats/qwebp.dll`が行います。このプラグインはGoogleのlibwebpを含んでいます。

- ライセンス本文: 配布文書内の`libwebp-COPYING`
- 特許ライセンス: 配布文書内の`libwebp-PATENTS`
- ライセンス種別: BSD系の3条項ライセンス

バイナリ再配布時も、libwebpの著作権表示、ライセンス条件、免責事項を本書または配布ドキュメントに含めます。Googleまたは貢献者の名称を製品の推薦・承認を示す目的では使用しません。

`PATENTS`に記載された特許ライセンスの条件も適用されます。Qtのバージョン更新時は、`libwebp-COPYING`、`libwebp-PATENTS`、本書の内容を再確認します。

## MinGW-w64 C++ ランタイム

配布ZIPには、Qt 6.10.3 MinGW版のビルドに使われたMinGW 13.1.0ツールチェーンのMinGW-w64ランタイムDLL（`libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`）を、実行ファイルの横へアプリローカルで同梱します。`libgcc`と`libstdc++`はGCC Runtime Library Exception付きのGNU General Public License version 3で、`libwinpthread`はMinGW-w64プロジェクトの一部で寛容なライセンスで配布されています。ライセンス本文はGCCおよびMinGW-w64プロジェクトを参照してください。
