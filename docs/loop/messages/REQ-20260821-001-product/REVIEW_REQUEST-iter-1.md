# REVIEW_REQUEST

message_type: REVIEW_REQUEST
request_id: REQ-20260821-001-product
iteration: 1
from_lane: implementation
to_lane: review
status: REVIEWING
created_at: 2026-08-21T08:28:30Z
source_docs:
- docs/loop/messages/REQ-20260821-001-product/IMPLEMENTATION_REQUEST.md
- docs/loop/messages/REQ-20260821-001-product/IMPLEMENTATION_DONE-iter-1.md
- docs/loop/evidence/REQ-20260821-001-product-audit-matrix.json
- plans/qa-audit-001.md
artifact_scope:
- tests/audit_current_state.ps1
- docs/loop/evidence/**
- docs/loop/lanes/implementation/evidence/**
acceptance_criteria:
- 監査マトリクスが各対象をPASS/BLOCKED/FAILで分類し、欠落証拠をPASSにしていないこと。
- CMake/CTestの実終了コードと証拠JSONが存在し、core/**が変更されていないこと。
- UI/Explorer実機証拠がない項目がBLOCKEDであること。
verification:
- completion-style audit: exit_code 0, docs/loop/evidence/REQ-20260821-001-product-audit-complete.json
- build: exit_code 0, docs/loop/evidence/REQ-20260821-001-product-build.json
- ctest: exit_code 0, docs/loop/evidence/REQ-20260821-001-product-ctest.json
- UI/Explorer evidence guard: exit_code 0, docs/loop/evidence/REQ-20260821-001-product-ui-explorer-required.json
review_focus:
- unmet_or_partial_criteria: verify the requested matrix coverage and evidence paths.
- scope_creep: confirm no changes under core/**, scripts/**, CMakeLists.txt, or README.md.
- looks_done_but_wrong: reject process-only or registry-only evidence as UI/Explorer PASS.
- ease_of_misuse: check whether missing UI/Explorer artifacts could be misclassified as PASS.
expected_reply:
- REVIEW_DONE with verdict PASS/FAIL, or FIX_REQUEST with severity and precise finding.
