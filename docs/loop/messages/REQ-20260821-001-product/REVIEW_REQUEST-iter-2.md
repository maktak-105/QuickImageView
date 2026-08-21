# REVIEW_REQUEST

message_type: REVIEW_REQUEST
request_id: REQ-20260821-001-product
iteration: 2
from_lane: product
to_lane: review
status: REVIEWING
created_at: 2026-08-21T08:47:45Z
source_docs:
- docs/loop/messages/REQ-20260821-001-product/FIX_REQUEST-iter-2.md
- docs/loop/messages/REQ-20260821-001-product/IMPLEMENTATION_DONE-iter-2.md
- tests/audit_current_state.ps1
- tests/validate_e2e_evidence.ps1
- tests/audit_current_state.tests.ps1
- docs/loop/evidence/REQ-20260821-001-product-iter-2-audit-complete.json
- docs/loop/evidence/REQ-20260821-001-product-iter-2-red-test.json
artifact_scope:
- tests/audit_current_state.ps1
- tests/validate_e2e_evidence.ps1
- tests/audit_current_state.tests.ps1
- tests/fixtures/ui-e2e-invalid.json
- docs/loop/evidence/**
- docs/loop/lanes/implementation/evidence/**
acceptance_criteria:
- UI/Explorer証拠JSONの必須フィールドと内容一致を検証し、存在だけでPASSできないこと。
- 不正fixture、プロセス生存のみ、必須フィールド欠落、時刻/参照不正がFAILまたは非0になること。
- 実機証拠がない通常行列はUI/ExplorerをBLOCKEDとすること。
- CMake/CTest/全VERIFY/completion gateがexit 0であること。
- core/**、scripts/**、CMakeLists.txt、README.mdに変更がないこと。
review_focus:
- unmet_or_partial_criteria: iter-2のblocker修正と全VERIFYを確認。
- scope_creep: artifact_scope外の変更がないか確認。
- looks_done_but_wrong: 不正fixtureやプロセス生存だけの証拠がPASSへ抜けないことを確認。
- ease_of_misuse: 将来の不正UI/Explorer証拠投入で誤PASSする経路が残っていないか確認。
expected_reply:
- REVIEW_DONE with verdict PASS/FAIL, or FIX_REQUEST with severity and precise finding.
