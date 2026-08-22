# QuickImageView 開発引き継ぎ

作成日: 2026-08-22
作業場所: `C:\Users\makta\source\QuickImageView`

## 次セッションの最優先事項

このファイルを読んだ後、`docs/loop/README.md`、`docs/loop/goal.json`、`docs/loop/checklist.json`、`docs/loop/invariants.json`、`docs/loop/state.json` を確認すること。

開発は一回の検査で停止せず、次のループを自動的に繰り返す。

1. 仕様・チェックリストを確認
2. 実装または不足テストを特定
3. `cmake --build build --clean-first` でビルド
4. `tests/verify_goal.ps1` で静的・CLI検査
5. `tests/verify_ui.ps1` で実機UI検査
6. `tests/manage_loop.ps1` を実行
7. `docs/loop/state.json` と `docs/loop/dashboard.html` を更新確認
8. FAIL が残る限り修正して再実行

ユーザーがダッシュボードを確認するだけで済むよう、進捗状態は管理ループから自動更新する。ダッシュボード生成だけではアプリを起動しない。

## ユーザーが確定した仕様

### 実装対象

- 画像表示
- 拡大・縮小
- パン
- 範囲選択による切り抜き
- リサイズ
  - パーセント指定
  - ピクセル指定
  - アスペクト比固定オプションを付ける
  - アスペクト比固定はデフォルトON
- 回転: 90度、180度、270度
- 反転・鏡像
  - 水平方向の鏡像: mirror horizontally
  - 垂直方向の反転: flip vertically
- JPEG品質指定
  - 50、75、90などの固定候補に限定しない
  - 任意の品質値を指定できるUIが必要
- EXIFコピー
- ファイル情報・EXIF情報の表示
- 色変換
  - フルカラー
  - 256色
  - グレースケール
- Undo / Redo
  - メニュー項目は不要
  - Windows標準ショートカットのみ: Ctrl+Z / Ctrl+Y
- 画像のクリップボードコピー・貼り付け
- WebP / HEIC変換
- 変換時の品質・圧縮設定
- 名前を付けて保存
  - 保存先フォルダへ移動する前に、形式・品質・圧縮率などの保存オプションを表示
- Explorerの画像右クリックメニューからQIV起動
- パン・拡大縮小・範囲選択時の点滅をなくし、滑らかに描画

### 実装しない対象

- Explorerのサムネイルシェル拡張
- フォルダ内画像の前後移動
- スライドショー
- 印刷
- EXIF・メタデータ編集
- 過去ログ、言い訳、作業履歴の保存
- ループ結果ログの保存

## 現在の実装状況

### 直近で実装済み

- ファイル情報とEXIF表示を `core/native/main.cpp` に復元済み
- リサイズのアスペクト比固定を実装済み。デフォルトON
- 保存オプションダイアログを追加済み
  - 形式
  - JPEG品質
  - 圧縮率
  - 設定後に保存先ファイルダイアログを表示
- `WM_PAINT` のダブルバッファリングと `WM_ERASEBKGND` によるちらつき対策を追加済み
- UI検査で以下を確認済み
  - リサイズダイアログ
  - アスペクト比固定の初期ON
  - 範囲選択による切り抜き操作
  - 保存オプション表示
  - 任意値 `83` の品質欄入力
  - 保存先ダイアログへの遷移
- 管理ループが検査後に `state.json` と `dashboard.html` を自動更新する構成に変更済み
- ダッシュボードは日本語表示
- 最新の管理ループ実行時点では `checklist_items=35`、全検査PASS

## 直近の未完了作業

JPEG品質UIに「指定...」を追加する作業が未完了。固定候補だけでなく任意値を正式に受け付ける必要がある。

対象ファイル:

- `core/native/main.cpp`
- `core/native/resource.h`
- `core/native/resource.rc`
- `tests/verify_goal.ps1`
- `tests/verify_ui.ps1`

実装案:

1. `kCommandQualityCustom = 1143` を追加
2. `QualityDialogState`、`QualityDialogProc`、`ShowQualityDialog` を追加
3. `IDD_QUALITY_DIALOG = 2020`、`IDC_QUALITY_VALUE = 2021` を追加
4. メインメニューと画像右クリックメニューのJPEG品質メニューへ「指定...」を追加
5. `ExecuteEditCommand` に任意の0〜100値を反映する処理を追加
6. `verify_goal.ps1` に `ShowQualityDialog` と `kCommandQualityCustom` の検査を追加
7. `verify_ui.ps1` で指定ダイアログを開き、`83` を入力して確定する検査を追加

直前にこの作業を一括パッチで適用しようとして失敗したが、失敗したパッチの変更は適用されていない。上記を小さいパッチに分けて実施すること。

## 検査・管理コマンド

PowerShellでリポジトリ直下から実行する。

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --clean-first
pwsh -NoProfile -ExecutionPolicy Bypass -File tests/verify_goal.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tests/verify_ui.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tests/manage_loop.ps1
```

通常は最後の `manage_loop.ps1` を使う。これは検査、PASS/FAIL判定、`docs/loop/state.json` 更新、ダッシュボード生成を一括で行う。

ダッシュボードだけを再生成する場合:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tests/generate_dashboard.ps1
```

これはアプリを起動しない。

## Git状態

直近の公開済みコミット:

- `68c49ec` — `Redesign requirements loop and clean historical artifacts`
- `origin/main` へpush済み

引き継ぎファイル作成時点では、次の変更が未コミットの可能性があるため、必ず `git status --short` で確認すること。

- `core/native/main.cpp`
- `core/native/resource.h`
- `core/native/resource.rc`
- `docs/loop/dashboard.html`
- `docs/loop/invariants.json`
- `docs/loop/state.json`
- `docs/loop/README.md`
- `tests/verify_goal.ps1`
- `tests/verify_ui.ps1`
- `tests/manage_loop.ps1`

ユーザーが明示的に要求するまで、勝手にcommit/pushしない。

## 運用上の禁止事項

- 1回のPASSで開発を終了しない。未実装・未検査の要件がないか次のループへ進む。
- ダッシュボードを手作業でPASSにしない。必ず管理ループの検査結果から更新する。
- 静的検査だけで実装済みと断定しない。UI操作が必要な項目は実機UI検査を追加する。
- EXIF・ファイル情報など、既に動いていた機能を削除しない。
- 実装していない機能を仕様・ダッシュボードでPASSにしない。
- WebView2をプロセス名だけで一括終了しない。
- 会話・調査・報告は日本語で行う。

## 再開時の最初の報告

次セッションでは、最初に次の3点だけを日本語で報告して作業を続ける。

1. `git status --short` の結果
2. `docs/loop/state.json` の現在値
3. JPEG品質任意指定の実装・検査が未完了であること

その後、質問待ちで止まらず、JPEG品質任意指定の実装から管理ループを再開する。
