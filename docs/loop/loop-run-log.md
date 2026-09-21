# Loop Run Log

Append-only transition log. Add one row per state transition; never edit or
delete prior rows. Use this to reconstruct loop history after compaction.

The `lane` column is the lane that performed the transition - the acting lane,
not the new owner. Human-gate transitions are always recorded by product.

| timestamp | request_id | iteration | from_status | to_status | lane | note |
| --- | --- | --- | --- | --- | --- | --- |
