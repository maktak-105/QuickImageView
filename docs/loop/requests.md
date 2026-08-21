# Requests

Use the table below as the durable queue for cross-agent work. Recover from
here instead of from chat memory.

## Schema

Required columns, in order: request_id, status, owner_lane, iteration,
source_docs, last_message, next_action, updated_at.

Rules:

- request_id is stable across fix cycles: REQ-YYYYMMDD-HHMMSS-<lane>.
- status lifecycle: PLANNED -> REQUESTED -> IMPLEMENTING -> IMPLEMENTATION_DONE -> REVIEWING; REVIEWING -> FIX_REQUESTED | ACCEPTED | BLOCKED; FIX_REQUESTED -> IMPLEMENTING; BLOCKED -> FIX_REQUESTED | ABANDONED.
- BLOCKED is a human-gate pause, not a terminal state. It has exactly one legal edge back into work: BLOCKED -> FIX_REQUESTED, carrying a recorded human authorization. The authorizing run-log note is exactly `human_authorization: approved` or starts with `human_authorization: approved | ` followed by an evidence pointer.
- ACCEPTED is the success terminal. ABANDONED is the explicit human-declared dead-end terminal. BLOCKED -> ABANDONED requires a recorded human decision; this terminal disposition is not a resume edge, and ABANDONED rows keep their evidence.
- Only the current owner_lane moves a request forward.
- Increment iteration when a request returns to implementation after review.
- next_action must let any lane resume after compaction or a new session.
- updated_at is ISO-8601 UTC, e.g. 2026-06-23T11:00:00Z.
- loop-run-log.md is the authoritative transition history.
- requests.md is a coarse current-state snapshot at checkpoint granularity.
  PLANNED and IMPLEMENTATION_DONE may never appear in it - that is legal.

This file must contain exactly one Markdown table (the queue below). Keep the
schema described as prose above so recovery tooling reads only real rows.

## Queue

| request_id | status | owner_lane | iteration | source_docs | last_message | next_action | updated_at |
| --- | --- | --- | --- | --- | --- | --- | --- |
| REQ-20260821-001-product | FIX_REQUESTED | implementation | 2 | docs/loop/goal.md; docs/loop/tracker.md; docs/loop/constraints.md; plans/qa-audit-001.md; CMakeLists.txt; README.md; docs/loop/messages/REQ-20260821-001-product/FIX_REQUEST-iter-2.md | reviewがblocker判定。UI/Explorer証拠JSONの存在だけでPASSへ到達でき、内容・対象exe・操作結果・時刻・証拠参照を検証していない。 | tests/audit_current_state.ps1で証拠JSONスキーマと内容を検証し、不正証拠を投入したテストを非0にする。iter-2のIMPLEMENTATION_DONEを返す。 | 2026-08-21T08:32:08Z |
