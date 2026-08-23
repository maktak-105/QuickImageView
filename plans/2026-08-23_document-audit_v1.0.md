# QuickImageView 文書監査・更新計画 v1.0

実施日: 2026-08-23  
担当: 文書作成  
対象: `C:\Users\makta\source\QuickImageView`

## 1. 前提

- `README.md` と `python/tests/operations.json` の `feature_001`〜`feature_072`を、既存の正式ベースラインとして扱う。
- 追加案件（EXIFフローティング表示、EXIFコピー、EXIFなし表示、UI要素・クリップボードの直接検査）は未テストであり、完了済み・PASS済みとして文書化しない。
- 今回の担当範囲はREADME、`document/`、`dist/documents/`、HISTORY、`plans/`の文書だけとし、コード・テストプログラム・操作台帳は変更しない。
- 既存の未コミット変更は保持する。commit/pushは行わない。

## 2. 現状調査

### 2.1 既存ベースライン

- `README.md`には72件の日本語機能箇条書きがある。
- `python/tests/operations.json`も72件で、READMEの機能箇条書きと対応している。
- `README_jp.md`は29行の概要文書で、README.mdの72件と内容・構成が一致していない。
- `docs/loop/current.json`と`docs/loop/handoff.md`は古い検査世代の記録であるため、今回の文書更新では72件の正式ベースラインや追加案件の完了判定に使用しない。

### 2.2 Quickテンプレートとの差分

- README: 英語版をベース名、`_jp`を日本語版とする構成、スクリーンショット、配布版、SHA-256、ビルド、ライセンス、免責事項の標準節が不足している。さらに現在はREADME.mdが日本語の機能台帳であるため、台帳との連携を保った同期方法が必要。
- `document/`: `spec.md`と`spec_jp.md`の内容量・構成が不一致。`environment`と`about`は日英ペアが存在するが、記載は最小限。
- 配布手順: `document/distribution.md`が欠落していた。日本語版には重複したインストールコマンドと誤解を招く改行があった。
- 第三者ライセンス: `document/third_party_licenses_jp.md`が欠落していた。
- `dist/documents/`: README・HISTORY・MIT Licenseは存在するが、libwebpのCOPYING/PATENTSがソース管理下の配布文書として存在していなかった。MSI staging側には生成物として存在する。
- assets: Quickテンプレートが求める日本語UI・英語UIのスクリーンショットは未配置。存在するのは`assets/QuickImageView-icon.svg`のみ。
- plans: 過去計画は存在するが、今回の文書監査で確認した差分と未記載事項をまとめた計画書がなかった。

## 3. 今回実施した文書変更

- `document/distribution.md`を追加し、日本語版と同じ配布手順を英語で記載した。
- `document/third_party_licenses_jp.md`を追加し、英語版と対になる日本語文書を作成した。
- `document/distribution_jp.md`の重複したインストールコマンドを整理し、`-NoRegisterContextMenu`の使い方を明確化した。
- `dist/documents/libwebp-COPYING`と`dist/documents/libwebp-PATENTS`を追加した。内容は`third_party/libwebp-1.6.0/`の原文と一致させた。

## 4. 優先して更新する文書一覧

### P0: 台帳を壊さずに日英の正本を同期

1. `README.md` / `README_jp.md`
   - 72件の順序・ID対応を維持する。
   - README.mdを英語、README_jp.mdを日本語とするテンプレート構成へ整理する。
   - 追加案件は追記せず、未テストであることを別記する。
   - READMEの機能箇条書きを変更した場合は、実装担当・検査担当と合意のうえで`python/tests/operations.json`を再生成し、72件を維持する。
2. `document/spec.md` / `document/spec_jp.md`
   - 72件のベースライン、対象外、対応形式、WICコーデック依存を同じ内容で記載する。
   - 実在しない正本ファイルや、未検証の完了判定を記載しない。

### P1: 配布・ライセンスの正確性

3. `document/distribution.md` / `document/distribution_jp.md`
   - 今回追加済み。次にMSI・PowerShell配布物の実物と最終照合する。
4. `dist/documents/readme.txt` / `readme_jp.txt`
   - 配布実物のファイル一覧、起動方法、対応形式、インストール選択肢を日英で同期する。
   - GitHub Releases URLとSHA-256は、実在するリリースと計測結果が確認できるまで追加しない。
5. `document/third_party_licenses.md` / `third_party_licenses_jp.md`、`dist/documents/libwebp-COPYING`、`libwebp-PATENTS`
   - 今回追加済み。配布物とMSI展開物の両方に実際に入っていることを別担当が監査する。

### P2: 補助文書・公開品質

6. `HISTORY.md` / `HISTORY_jp.md`および`dist/documents/history.txt` / `history_jp.txt`
   - 実装・検証済みの変更だけを記載し、未テストの追加案件を完了履歴に入れない。
7. `document/environment.md` / `environment_jp.md`、`document/about.md` / `about_jp.md`
   - バージョン、作者、ビルド方法、実際のツールチェーンを最終確認する。
8. `assets/`とREADMEのスクリーンショット参照
   - 実UI検証担当が取得した日本語・英語の実画面だけを追加する。生成画像や未確認画面は使わない。

## 5. 未記載・未確認事項

- READMEの日英完全同期は未完了。
- `document/spec.md` / `spec_jp.md`の日英完全同期は未完了。
- 配布用READMEの日英完全同期、GitHub Releases URL、SHA-256は未確定。
- 日英UIスクリーンショットは未配置。
- 72件の最新実UI検査結果は本担当では確定していない。
- EXIFフローティング表示、EXIFコピー、EXIFなし表示、直接UI要素検査は未テストであり、未完了扱いとする。
- `docs/loop/handoff.md`には古い件数・バージョン記載が残っているが、今回の担当範囲外のため変更していない。

## 6. 判定

文書ペアの欠落補完と配布手順の明確化は完了した。ただし、Quickテンプレート準拠の文書整備全体、および72件の実UI検証・追加案件の検証は未完了である。したがって、この文書作業だけでリリース完了とは判定しない。
