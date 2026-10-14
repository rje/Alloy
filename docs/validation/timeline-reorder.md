# Retained Timeline reorder

HIS-006/#280 adds one bounded history move: `feature.reorder` takes a retained
feature UUID and an `afterFeatureId` UUID in the same component. It moves the
unchanged authored feature object immediately after the anchor in the global
feature array. The normal full document evaluator validates the new order and
geometry before one undoable commit. A move already in place changes neither
revision nor history. Dependent moves refuse with structured diagnostics and
leave the prior document intact. The Timeline uses the same command when a
tile is dragged over another tile; a visible insertion marker identifies the
after-anchor position. Escape or release outside the strip cancels, and a
completed drag suppresses click-to-edit. This does not implement rollback
markers, suppression, multi-feature moves or automatic dependency repair.

The actual enclosure two-Mirror gate now passes in the real application:
Bottom inner Fillet R0.5 precedes the undercut Cut Mirror and the five-feature
Top snap Mirror, then moves after both. Its original retained edge remains
editable; adding the opposite retained edge as a separate R0.5 group produces
valid closed Top and Bottom solids. The 24-call API run checks atomic invalid
reorder refusal, history and native save/reopen. A separate evaluated-geometry
check finds zero positive-volume Top/Bottom intersection after the final edit.
The nominal board's pre-existing 0.7322403 mm³ connector overlap is unchanged;
this is not physical-fit certification.

The Qt native input gate passes initial press/move/Escape cancellation followed
by press/move/release after the last Mirror. The insertion marker is visible in
the inspected framebuffer and the final feature order is checked. One initial
harness activation and layout settle precede input; there is no focus repair
between gestures. The last tile needs four pixels of trailing layout margin so
its drop marker remains inside the scroll viewport. Earlier attempts exposed
that real last-tile drop bug; candidate 04 passes after the fix. This is Qt
input evidence, not OS mouse delivery or a complete tutorial recording.

External evidence is under
`../Alloy-reference-records/deliverables/timeline-reorder-2026-10-14/`:
`api-two-mirrors-01/` (report, transcript, saved model and capture),
`native-two-mirrors-04/` (test log, insertion marker and reordered captures),
and `final-clearance.json` with preserved geometry helper and build script.
Astra independently accepted candidate `6684dac` on 2026-10-14. Full
from-blank tutorial creation remains TUT-022.

The source record and exact public behavior are external at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/his006-reorder-handoff.md`.
The one-Mirror input and output are external at
`../Alloy-reference-records/deliverables/timeline-reorder-2026-10-13/core-01/`.
