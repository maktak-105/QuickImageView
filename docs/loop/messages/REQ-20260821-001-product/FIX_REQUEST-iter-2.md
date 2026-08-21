# FIX_REQUEST

message_type: FIX_REQUEST
request_id: REQ-20260821-001-product
iteration: 2
from_lane: review
to_lane: implementation
status: FIX_REQUESTED
created_at: 2026-08-21T08:31:00Z
source_docs:
- docs/loop/messages/REQ-20260821-001-product/REVIEW_REQUEST-iter-1.md
- docs/loop/messages/REQ-20260821-001-product/IMPLEMENTATION_DONE-iter-1.md
- tests/audit_current_state.ps1
- docs/loop/evidence/REQ-20260821-001-product-audit-matrix.json

verdict: FAIL

findings:
- severity: blocker
  category: looks-done-but-wrong
  title: UI/Explorer証拠JSONの存在だけでPASSへ到達できる
  evidence:
  - tests/audit_current_state.ps1:48-50
  - tests/audit_current_state.ps1:68-74
  - docs/loop/evidence/REQ-20260821-001-product-ui-explorer-required.json
  detail: >-
    `$explorerEvidence` と `$uiEvidence` はファイル名の存在だけを判定し、
    JSONの実測内容、対象exe、操作結果、日時、スクリーンショット/録画等を
    検証しない。そのため、プロセス生存だけを記録した ui-e2e*.json または
    explorer-e2e*.json を置けば、実機操作がない状態でも行列がPASSになる。
    現在のリポジトリでは該当ファイルがなく正しくBLOCKEDだが、監査ガード自体が
    要求された「証拠なしまたはプロセス生存のみのPASSでFAIL」を満たしていない。
  required_fix: >-
    UI/Explorer証拠JSONの必須スキーマと内容を定義し、実機操作結果を示す
    フィールド（少なくとも対象操作、期待結果、実測結果、対象exe/画像、
    記録時刻、証拠ファイル参照）を検証する赤化可能な監査を追加する。
    必須フィールド欠落、プロセス生存だけの記録、期待結果と実測結果の不一致は
    FAILまたはBLOCKEDにし、PASSにしない。テストで不正な証拠JSONを投入した場合に
    非0終了することを示すこと。

verification_observed:
- command: powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireCompleteMatrix
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-audit-complete.json
- command: cmake -S . -B build -G "MinGW Makefiles"; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; cmake --build build --clean-first
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-build.json
- command: ctest --test-dir build --output-on-failure
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-ctest.json
- command: powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireEvidenceFor UiE2E,ExplorerE2E
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-ui-explorer-required.json

matrix_review:
- audit matrix classification: PASS/BLOCKED/FAIL rows are present; UI/Explorer remain BLOCKED.
- CMake/CTest: independently rerun and both exited 0.
- core/**: no tracked diff observed; no core file was modified by this review.
- scope creep: no implementation scope creep observed in the declared changed files.
- ease-of-misuse: blocker found; a process-only or empty-content UI/Explorer evidence file can reach PASS.

expected_reply:
- IMPLEMENTATION_DONE for the same request_id with iteration 2.
- Updated red-capable evidence and exact commands/exit codes.
