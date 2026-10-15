# Trim region selection responsiveness

REL-026/#458 is independently accepted at `499d580`, with the focused window
teardown correction `e90e1a4`. An immutable partition belongs to the active Trim
session and its exact document revision and geometry inputs. Region selection,
repeated preview and naming reuse it; geometry changes rebuild it. Cancel,
replacement and stale revision invalidate it. Commit still runs authoritative
feature evaluation and downstream checks.

The actual saved bilateral iron workflow passes 40 API calls. Four region
toggles take 299–507 ms; repeated previews and naming take 194–566 ms.
The partition count remains one until a geometry input changes. The committed
result retains the accepted topology and face/edge IDs, with area difference
−2.73e−11 mm². Undo/redo and save/reopen pass.

Affected core checks and four native Qt checks pass. A failed test assumption
about swapping finite target/tool geometry was corrected without changing the
kernel refusal. Failure unwinding also exposed a late application focus callback;
disconnecting application signals at window teardown fixes it. The final focused
active-session teardown check passes all three Qt cases. Unchanged geometry runs
were retained for that lifecycle-only correction.

Actual Apple M4 GPU captures show readable controls, both red removed regions
and smooth reopened geometry. Independent Astra source and evidence review
accepted this bounded result. Evidence, captures and timing details are outside
the repository in `../Alloy-reference-records/deliverables/trim-session-reuse-2026-10-15/`.

Cold partition construction and authoritative commit still take about 11–12
seconds. The earlier baseline used a different crown edit, so no exact speedup
factor is claimed. This does not qualify the complete TUT-018 tutorial, physical
OS input or Windows/Linux GPU runtime.
