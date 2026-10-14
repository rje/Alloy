# Retained enclosure snap workflow

Independently accepted on 2026-10-14 as the bounded adapted API modeling
checkpoint. The full source workflow remains TUT-016.

`tests/enclosure_port_card_workflow_api.py --through-snaps` continues the
accepted from-blank imported-board and ported-Case workflow in the same real
application document. The prefix path without that flag remains unchanged.
The continued path uses the two actual outer Case faces for Midplane and
Split, names the resulting bodies Bottom and Top, and projects a current
Split seam and an imported component edge to position a 4 mm construction
guide and 6 × 3 mm catch. The catch Cut resolves To Object against the current
inner wall. Its side face drives a 0.5 mm plane, three linked Cut-edge
projections, the stepped 8/2/10/6/2 outline, and a Join of three selected
sketch regions into Top.

The upper snap finishes use current Join edge identities in source order:
Chamfer 2 mm at the lower front, 1 mm at the upper front, 2 mm at the exact
attachment root, then inner Fillet 0.5 mm. A separate Bottom inner Fillet
precedes the Cut-only Mirror and the ordered five-feature Top Mirror. The
Bottom Fillet then moves after both Mirrors and receives the opposite retained
inner edge as a separate 0.5 mm group. The origin YZ mirror plane and the
catch's horizontal placement adapt the independently authored board and Case;
the public source does not establish those adapted world coordinates.

The focused real-app continuation probe through both Mirrors passed with 14
evaluated bodies and valid closed Top and Bottom solids. Its actual ledger and
captures are external at
`../Alloy-reference-records/deliverables/enclosure-snap-workflow-2026-10-14/probe-05/`.
The source-sized geometry and final reorder/clearance ingredients have
separate accepted receipts in [source-sized-snap-group.md](source-sized-snap-group.md)
and [timeline-reorder.md](timeline-reorder.md).

The continuous from-blank recording under
`../Alloy-reference-records/deliverables/enclosure-snap-workflow-2026-10-14/from-blank-01/`
authored and saved both completed halves after the two Mirrors, Timeline
reorder, and opposite Bottom Fillet group. It stopped at a late verification
assertion: visibility changes followed the Fillet edit, so one undo reverted
visibility instead of the Fillet. The saved model and 23-minute original movie
are preserved, with the exact stop described in `partial-run-receipt.json`.
The same directory contains a labeled 431-second share edit and its
`enclosure-through-snaps-share.edit-map.json`; this trims repeated/idle frames
from the original, without replacing the raw recording. Its final Top-section
caption accompanies an **unavailable** face-linked section. The working
same-position section appears only in the separate post-save tail capture.
The runner now checks undo immediately after the Fillet edit. A separate
36-call real-app post-save tail (`tail-02/tail-result.json`) passed a geometric
Fillet edit, undo/redo, authored reopen, and separate Bottom/Top STL and STEP
exports. Each STL is independently manifold with one connected component.
The halves share their split-seam face; a combined STL therefore contains
coincident triangles and is not the printable deliverable.

Read-only OCCT intersection of the saved model reports **0 mm³** positive
overlap between Top and Bottom (`final-clearance.json`). It also reports
0.73224 mm³ overlap between Top and the independently authored imported
SIDE-ETH-001 part, an already known collision in the nominal adapted board
fixture. This qualifies the adapted electronics fit; the two-half
clearance result does not establish whole-board clearance.

The original face-linked Top section becomes unavailable after Split. The
post-save tail used a disclosed origin-XY inspection plane at the same
z=24.5 mm position, facing −Z, with 14 clipped actors; it did not repair the
lost retained-face link. The projected upper outline uses the actual lower
Cut span and two uprights to form four selectable regions. That edge choice,
the YZ mirror plane, and the catch's horizontal placement are adaptations to
the independently authored board and Case rather than exact observed source
gestures. Native typed body rename remains a separate DOC-031 gate. This
receipt does not claim complete source parity. DOC-031 and DOC-032 preserve
native naming and the original retained-section requirements in the full parent
gate. STEP export passed; independent STEP readback was not performed here.

Public source evidence and documented versus inferred steps are kept outside
the repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/enclosure-split-snap-contract.md`
and `enclosure-final-order-clarification.md` in the same directory. No
proprietary application was operated for this validation.

The local video gallery `../Alloy-reference-records/demos/2026-10-13/index.html#enclosure-snaps`
links the share and raw recordings, editable assembly, separate printable halves
and corrected post-save section capture. Its local links were checked.
