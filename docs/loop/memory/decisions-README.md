# Decision Memory

`decisions.jsonl` is the loop's only memory artifact: an append-only decision
log. It is a CACHE derived from the `docs/loop` source files, never a source of
truth. If it disagrees with the live sources, the sources win.

## One line per decision (append-only)

Each line is one JSON object. Append new lines only; NEVER edit or delete a
prior line. To change a decision, append a new line whose `supersedes` names the
old `decision_id`. Write lines with `record_decision.py` (it computes the hash
and appends in `'a'` mode).

Fields (one JSON object per line):

- `decision_id` -- stable id (derived from request_id + a per-request sequence).
- `request_id` -- the request this decision serves.
- `lane` -- which lane decided.
- `decision` -- what was decided (one line).
- `rationale` -- why.
- `alternatives_rejected` -- options considered and dropped.
- `supersedes` -- the `decision_id` this line replaces, or empty.
- `source_docs` -- the source files this decision derives from / depends on.
- `content_hash` -- `normalize_then_hash(source_docs)` at write time (CRLF->LF,
  trailing newlines stripped, sha256 hex).
- `gate_status` -- completion-gate token at write time: `SHIP_CHECK_OK`,
  `SHIP_CHECK_FAIL`, or `none` (so a decision made under FAIL reads as tentative).
- `created_at` -- ISO-8601 UTC.

## Known tradeoff: the hash only covers the listed `source_docs`

`content_hash` is computed over exactly the files named in `source_docs` and
nothing else. Drift detection (in `multi_agent_loop_doctor.py`) can only notice
a change in a file that was listed. If a decision truly depended on a file that
was omitted from `source_docs`, a later change to that file will NOT be flagged
as `stale_decision`. List every source a decision genuinely depends on.

## Drift is advisory, never a gate

The doctor recomputes the hash for every non-superseded decision with the SAME
`normalize_then_hash` helper (imported from `record_decision.py`, the single
canonical definition). A mismatch is a `stale_decision` WARNING; a missing
source doc is `missing_source_doc`; a bad line is `malformed_decision`. None of
these ever affect `handoff_ready` or `auto_chain_ready`. Verification fails
closed; memory fails open.
