# QuickImageView v3.1.2 Qt Quick移行 実施結果

対象計画書：[`2026-08-24_qt-quick-migration_v1.0.md`](2026-08-24_qt-quick-migration_v1.0.md)

## 2026-08-24 初期移行基盤

- `QuickImageViewQt` を既存Win32版とは別ターゲット・別実行ファイルとして追加した。既存の`QuickImageView.exe`は変更していない。
- Qt 6.10.3 / Qt Quick / QMLによる暗色テーマのメイン画面、日英切替、ヘルプ、中央モーダルのAboutを追加した。Aboutには`Ver. 3.1.2`、開発環境、制作者、作者ワッペンを表示する。
- QMLの主要操作部品へ`objectName`と`Accessible.name`を付与し、表示文言や座標へ依存しないUIテストの基礎を追加した。
- 画像読込はQMLの標準デコーダーを使わず、C++の`ImageEngine`を経由する構成にした。PNG等はWICでデコードし、WebPはリポジトリ同梱の静的libwebpでデコードする。QML表示は専用の`image://quickimage`プロバイダーを経由する。
- `build.bat qt`からQt用CMakeビルド、CTest、`windeployqt`によるQt DLL・プラグイン収集を実行するようにした。中間生成物は`build/qt-mingw1310/`、配布対象バイナリは`dist/binary/`に置く。
- GitHub ActionsにはQt版のC++テスト、QMLテスト、Cppcheckを追加した。

## 2026-08-24 操作機能とヘルプの更新

- Aboutの作者ワッペンを最大112pxへ制限し、モーダルカードからはみ出さないようにした。
- 埋め込みヘルプをv3.1.2の実装内容へ更新した。見出し・項目の直後に空白行を置かず、次の項目との間だけを空ける段落構成に統一した。
- Qt版のC++コントローラーに、右・左90度回転、左右・上下反転、Windowsクリップボードへのコピー・貼り付けを追加した。QMLの編集メニュー、操作パネル、`Ctrl+C`、`Ctrl+V`へ接続した。
- 回転後の画像サイズと画像プロバイダーへの反映を確認するC++テストを追加した。
- 左側の操作パネルを廃止し、操作を上部メニューバーと画像領域の右クリックメニューへ集約した。Win32版の操作導線に合わせた。
- 編集履歴をC++層で管理し、回転・反転に対するUndo/Redoと`Ctrl+Z`/`Ctrl+Y`を追加した。履歴の状態遷移をC++テストで検証した。

## 検証結果

| 項目 | 結果 |
| --- | --- |
| `build.bat qt` | 成功。Qt Quick EXEと依存DLL・`platforms/qwindows.dll`を`dist/binary/`へ出力。 |
| CTest | 成功（既存自己診断、C++単体テスト、QMLテストの3件）。C++テストにはWIC、同梱libwebp、回転後の画像サイズ検証を含む。 |
| WIC読込テスト | 成功。テスト生成PNGをWIC経由で読み込み、寸法と画素を検証。 |
| 同梱libwebp読込テスト | 成功。テスト生成WebPをlibwebp経由で読み込み、寸法と画素を検証。 |
| 配布フォルダからの起動 | 成功。Qtインストールの`bin`をPATHへ加えない状態で`QuickImageViewQt.exe`が起動・応答することを確認。 |
| Cppcheck | 成功。Qtマクロの解析不能警告は`unknownMacro`として抑制し、警告・性能・可搬性・スタイルを検査。 |
| `git diff --check` | 成功。 |

## 未実装・継続対象

この時点は移行の初期基盤である。現行Win32版の切り抜き、リサイズ、Undo/Redo、EXIF表示、別形式での保存、原本・既存出力保護、クラッシュログ、キーボード操作の完全移植は未完了であり、Qt版の完成機能としては扱わない。以後は計画書の手順5以降を、この基盤に接続して進める。
