# File Leases

Advisory write leases. A lane should acquire a lease before editing files
outside an obviously exclusive scope, and release it when done. Leases are
advisory: pair them with the git pre-commit write_scope check for enforcement.

| file_glob | lane | request_id | acquired_at | status |
| --- | --- | --- | --- | --- |

Status values: `active` (held/enforced) or `released`/`expired`/`done`/`revoked` (ignored). A blank status is ignored; any other non-blank value is treated as held, so the guard fails closed on a status it does not recognize.
