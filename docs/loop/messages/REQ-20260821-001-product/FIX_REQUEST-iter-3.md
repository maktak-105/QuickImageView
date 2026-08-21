# FIX_REQUEST

message_type: FIX_REQUEST
request_id: REQ-20260821-001-product
iteration: 3
from_lane: review
to_lane: implementation
status: FIX_REQUESTED
created_at: 2026-08-21T08:55:00Z
source_docs:
- docs/loop/messages/REQ-20260821-001-product/REVIEW_REQUEST-iter-2.md
- docs/loop/messages/REQ-20260821-001-product/IMPLEMENTATION_DONE-iter-2.md
- tests/validate_e2e_evidence.ps1
- tests/audit_current_state.tests.ps1
- docs/loop/evidence/REQ-20260821-001-product-iter-2-red-test.json

verdict: FAIL
defect_class: looks-done-but-wrong

findings:
- severity: blocker
  category: unmet_or_partial_criteria
  title: evidence_references の参照先を検証していない
  evidence:
  - tests/validate_e2e_evidence.ps1:35-38
  - tests/audit_current_state.tests.ps1:11-14
  detail: >-
    validator は evidence_references の配列が空でないことだけを確認し、各参照先の
    ファイル存在、許可された相対パス、または証拠種別を検証しない。そのため
    evidence_references: ["does-not-exist.mp4"] のような不正参照でも、他の必須値と
    expected/observed が一致すればPASSへ到達できる。REVIEW_REQUEST-iter-2の
    「時刻/参照不正がFAILまたは非0」という受入条件を満たしていない。
  required_fix: >-
    evidence_references の各要素を検証し、空値・不正形式・許可範囲外パス・存在しない
    ファイルをFAILにする。存在確認を行わない参照形式を採用する場合は、参照が実在する
    証拠識別子であることを別の必須フィールドと赤化可能なテストで検証する。不正参照の
    fixtureを追加し、validatorまたはwrapperが非0になることを証拠化する。

- severity: blocker
  category: unmet_or_partial_criteria
  title: implementing-lane evidence mirror と命名規約が未達
  evidence:
  - doctor output: evidence_mirror_gap for REQ-20260821-001-product-iter-2-*.json
  - doctor output: evidence_naming for REQ-20260821-001-product-iter-2-*.json
  - docs/loop/lanes/implementation/evidence/
  detail: >-
    completion gateはroot evidenceだけでSHIP_CHECK_OKを返すが、loopのReview To Fix Gateは
    request 1以降、各VERIFYのlane evidenceを実装lane側に保存し、root mirrorとbyte-for-byte
    一致させることを要求する。iter-2 root evidenceには対応する実装lane twinがなく、root
    ファイル名も正規形に一致しない。gateが通ったことだけではこのレビュー要件を満たさない。
  required_fix: >-
    iter-2の全VERIFY evidenceを implementation/evidence に保存し、rootとのSHA-256または
    byte-for-byte一致を確認する。命名規約に合う正規ファイル名へ揃え、対応する正本・
    IMPLEMENTATION_DONEのchanged_filesと証拠パスを更新する。doctorを再実行し、これらの
    warningが解消された結果を提出する。

verification_observed:
- command: powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.tests.ps1
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-iter-2-red-test.json
- command: powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit_current_state.ps1 -RequireCompleteMatrix
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-iter-2-audit-complete.json
- command: cmake --build build --clean-first
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-iter-2-build.json
- command: ctest --test-dir build --output-on-failure
  exit_code: 0
  evidence: docs/loop/evidence/REQ-20260821-001-product-iter-2-ctest.json
- command: python completion_gate.py --request-id REQ-20260821-001-product
  exit_code: 0
  evidence: SHIP_CHECK_OK REQ-20260821-001-product
- command: python multi_agent_loop_doctor.py --loop-dir docs/loop
  exit_code: 0
  evidence: warnings include evidence_mirror_gap and evidence_naming

matrix_review:
- normal audit matrix: UI/Explorer remain BLOCKED; no real-device evidence was treated as PASS.
- invalid fixture: process-liveness-only fixture is rejected with non-zero through the wrapper.
- CMake/CTest/completion gate: PASS.
- core/scripts/CMakeLists.txt/README.md: no forbidden diff observed.
- ease-of-misuse: invalid reference path remains a reachable wrong-but-accepted outcome.

expected_reply:
- IMPLEMENTATION_DONE for the same request_id with iteration 3.
- Updated validator/red fixtures, byte-identical lane/root evidence mirrors, and clean doctor output.
