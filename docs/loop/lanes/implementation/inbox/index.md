# Implementation Inbox Index

Append-only delivery log for inbox/new. One row per atomically delivered
message. Readers process inbox/new, then move each file to inbox/cur.

| delivered_at | message_id | request_id | iteration | from_lane | message_type | state |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-08-21T08:23:17Z | REQ-20260821-001-product--IMPLEMENTATION_REQUEST--iter-1 | REQ-20260821-001-product | 1 | product | IMPLEMENTATION_REQUEST | new |
