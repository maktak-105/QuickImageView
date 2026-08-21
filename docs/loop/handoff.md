# Handoff

Continuation state for the next session or lane. Keep this current enough that
the next actor can continue from repo files plus the latest message alone.

## Current State

- REQ-20260821-001-productのiter-2 IMPLEMENTATION_DONEを確認した。証拠JSONの必須スキーマ/内容一致、不正fixture赤化テスト、全VERIFY、completion gateはPASS。UI/Explorer実機証拠なしはBLOCKEDのまま。

## Next Action

- [ ] reviewがiter-2の修正と証拠を独立レビューする。

## Active Request

- request_id: REQ-20260821-001-product
- owner_lane: review
- iteration: 2

## Blockers

- なし。検証不能項目は要求のルールに従いBLOCKEDへ遷移する。

## Pending Inbox Deliveries

- None.

## Status Legend

- `[ ]` not started
- `[~]` in progress
- `[x]` done and verified
- `[!]` blocked

## Done When

- [ ] REVIEW_DONEを受領し、review verdictに基づいて受入可否を判定する。

## Memory Protocol

The decision memory (`memory/decisions.jsonl`) is an append-only cache, never a
source of truth. Follow this protocol so it survives compaction and never lies:

1. Before deciding, grep `memory/decisions.jsonl` for this request_id and
   follow the `supersedes` chain to the newest live decision.
2. Before trusting any recorded `gate_status`, re-run
   `completion_gate.py --request-id <id>` and `multi_agent_loop_doctor.py`.
   The recorded token is only a hint; the live gate is the authority.
3. If the doctor reports a `stale_decision`, discard that cached decision and
   re-read the live source docs before acting on it.
4. At checkpoint close, append EXACTLY one line via `record_decision.py`. Never
   edit or delete an old line. To change a prior decision, append a new line
   whose `supersedes` names the old `decision_id`.

## Auto-Chain Permission

auto_chain_next_session: false
