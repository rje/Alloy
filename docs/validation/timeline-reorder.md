# Retained Timeline reorder candidate

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

The first headless proof uses the accepted rounded-Shell/Split snap fixture:
Bottom inner Fillet R0.5, then the five-source snap-group Mirror. Moving the
Bottom Fillet after that Mirror retains its original inner-edge ID. Editing the
moved Fillet to include the opposite retained inner edge produces valid closed
Top and Bottom solids; an invalid Join move after its dependent Mirror refuses.
The proof also checks one transaction, no-op, exact feature identity/order,
undo/redo and save/reopen. This is a one-Mirror mechanism proof. The source
sequence has a preceding undercut Cut Mirror and an opposite Bottom edge
affected by it; that actual two-Mirror reorder/edit is still required before
HIS-006 acceptance. Native and real-app API checks are also pending.

The source record and exact public behavior are external at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/his006-reorder-handoff.md`.
The one-Mirror input and output are external at
`../Alloy-reference-records/deliverables/timeline-reorder-2026-10-13/core-01/`.
