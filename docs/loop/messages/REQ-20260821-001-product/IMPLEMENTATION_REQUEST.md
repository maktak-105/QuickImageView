# IMPLEMENTATION_REQUEST

message_type: IMPLEMENTATION_REQUEST
request_id: REQ-20260821-001-product
parent_request_id:
iteration: 1
from_lane: product
to_lane: implementation
status: REQUESTED
created_at: 2026-08-21T08:22:08Z
source_docs:
- docs/loop/goal.md
- docs/loop/tracker.md
- docs/loop/constraints.md
- plans/qa-audit-001.md
- CMakeLists.txt
- README.md
delivery:
- channel: send_message_to_thread | lane_inbox
- target_thread_id: 01a02368-aa20-79e1-be3e-d6e15dfa89c7
- delivery_status: sent
- sent_at: 2026-08-21T08:22:08Z

goal: 既存QuickImageViewの現状を監査し、要求・実装・自動検証・配布導線・実機証拠の現在地を再現可能な証拠として記録する。core/**の実装変更は行わない。
user_facing: false
scope:
- tests/**
- docs/loop/lanes/implementation/**
- docs/loop/messages/REQ-20260821-001-product/**
- docs/loop/evidence/**
non_goals:
- core/**の実装・修正・リファクタリングをしない。
- scripts/**、CMakeLists.txt、README.md、配布物を変更しない。
- UI/Explorer E2Eを実行したと捏造しない。証拠がない項目をPASSにしない。
- 原本画像、個人データ、資格情報をリポジトリ・ログ・証拠へコピーしない。
invariants:
- INV-1 原本ファイルは絶対に上書きしない: VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -Invariant OriginalProtection` (違反・未判定をFAILにする)。
- INV-2 Explorer登録は存在確認だけで合格にしない: VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -Invariant ExplorerLaunchEvidence` (実際のExplorer起動証拠がなければBLOCKED判定を要求する)。
- INV-3 UI操作はプロセス起動だけで合格にしない: VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -Invariant UiInteractionEvidence` (画像表示・操作の実機証拠がなければBLOCKED判定を要求する)。
- INV-4 検証不能または証拠欠落はBLOCKED: VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -Invariant MissingEvidence` (証拠欠落項目がPASSになる場合にFAILする)。
acceptance_criteria:
- 監査成果物が実装ファイル・テスト・配布スクリプト・Explorer登録・UI E2E・原本保護を列挙し、各項目をPASS/BLOCKED/FAILで分類する。VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireCompleteMatrix` (必須分類・証拠パス・判定が欠けるとFAILする)。
- CMake構成とビルドが再現可能で、失敗を隠さず記録する。VERIFY `cmake -S . -B build -G "MinGW Makefiles"; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; cmake --build build --clean-first` (構成またはビルド失敗で非ゼロ終了)。
- CTestの既存self-testを実行し、実終了コードと出力を証拠化する。VERIFY `ctest --test-dir build --output-on-failure` (テスト失敗で非ゼロ終了)。
- 単体起動・ファイル選択・画像表示・ズーム/パン/右クリック、Explorer右クリック起動は、実機操作証拠がない限りBLOCKEDとする。VERIFY `powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireEvidenceFor UiE2E,ExplorerE2E` (証拠なしPASSまたはプロセス生存のみのPASSでFAILする)。
- 変更一覧がscope内で、core/**・scripts/**・CMakeLists.txt・README.mdが変更されていない。VERIFY `git diff --name-only -- core scripts CMakeLists.txt README.md` (対象変更があればFAILする)。
expected_reply:
- changed_files
- verification commands, exact exit codes, and evidence paths
- PASS/BLOCKED/FAIL matrix
- blockers, especially missing Windows実機/UI/Explorer evidence
- confirmation that core/** was not changed
