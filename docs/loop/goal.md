# Goal

## Objective

- Windows向けQuickImageViewを、画像ファイルを確実に開いて表示・操作でき、原本を保護したまま配布できる状態へ仕上げる。
- 実装・検証・独立レビューを分離し、各完了判定を再現可能な証拠で記録する。

## Done When

- [ ] Windows上で単体起動からファイルを選択して表示できる。
- [ ] エクスプローラの画像右クリックから起動できる。
- [ ] マウスホイール拡大縮小、左ドラッグ移動、右クリックメニューが安定して動作する。
- [ ] スクロール・移動時に点滅せず、対応形式の表示が実機で確認できる。
- [ ] 変換保存は新規ファイルのみ作成し、原本の上書き選択肢を提供しない。
- [ ] Windows配布物とインストール・アンインストール手順を検証できる。

## Out Of Scope

- 原本の上書き保存、原本を自動削除する機能、macOS/Linux対応は対象外。
- 検証できていない機能を「実装済み」「完了」と扱わない。

## Verification Surface

- CMakeビルドとCTest、赤化可能な自己テスト、Windows実機での操作確認、画面録画またはスクリーンショット、独立レビューで確認する。

## Invariants

- 原本ファイルは絶対に上書きしない。
- 右クリック登録の存在だけでなく、Explorerから実際に起動できることを確認する。
- UI操作の検証は、プロセスが起動しただけでは合格にしない。
- 検証不能または証拠欠落はBLOCKEDとして扱う。

## Status Legend

- `[ ]` not started
- `[~]` in progress
- `[x]` done and verified
- `[!]` blocked

## Auto-Chain Permission

auto_chain_next_session: false
