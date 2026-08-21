# Loop Budget

Cost and effort stop gate. The loop must stop and report when any budget is
exhausted, regardless of remaining tracker work.

## Limits

max_total_tokens: 0
max_total_usd: 0
max_attempts_per_request: 3
max_loop_iterations: 0

A limit of `0` means "unset / no enforced cap"; set a positive number to enforce.

## Spent

tokens_spent: 0
usd_spent: 0
loop_iterations: 0

## Stop Flag

budget_exhausted: false

## Rules

- Update `Spent` as work proceeds.
- Set `budget_exhausted: true` and stop when any positive limit is reached or exceeded.
- Do not auto-chain a continuation session while `budget_exhausted: true`.
