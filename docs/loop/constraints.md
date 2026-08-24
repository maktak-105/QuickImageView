# Constraints

Boundaries every lane must respect. Read before implementing or reviewing.

## Hard Constraints

- Do not commit secrets, credentials, or tokens.
- Do not run destructive, billing, or production-deployment actions without explicit human approval.
- Stay inside each lane's declared write_scope in `agent-lanes.md`.

## Technical Constraints

- Record language, framework, runtime, and version pins the work must honor.
- Reserved loop infrastructure ports: 8765 (dashboard default; append any later manual dashboard port choice to this line).

## Process Constraints

- Only switch sessions at a checkpoint boundary.
- Update `tracker.md`, `handoff.md`, `requests.md`, and lane `current.md` before any handoff.
- Reuse the same `request_id` across fix cycles; increment `iteration`.

## Status Legend

- `[ ]` not started
- `[~]` in progress
- `[x]` done and verified
- `[!]` blocked

## Auto-Chain Permission

auto_chain_next_session: false
