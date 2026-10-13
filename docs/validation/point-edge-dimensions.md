# Retained point-to-edge dimension candidate

SKT-034/#205, 2026-10-13. Public source TUT-001 step05, video10:21–11:08.
The source-linked contract and provenance remain external in
`knowledge/specs/point-edge-dimension-next-slice.md` and
`knowledge/records/point-edge-dimensions.json`. This is a bounded candidate,
awaiting independent compatibility review.

## Workflow and authored contract

In an attached planar sketch, D starts Sketch Dimension. Select a circle center,
then a retained straight model edge. Move the pointer to preview its dimension;
a separate click places the label and opens the numeric editor. Type3.9 and Enter,
then repeat against the adjacent perpendicular edge. The existing draft command,
annotation editing, Finish/Cancel and shared undo/storage authority are reused.
The palette exposes the same relation as Point to Edge.

`sketch.dimension` now accepts `kind: pointLineDistance`, `pointIds: [pointId]`
and `entityId: projectedLineId`, with normal `value`, `units`, `driving` and
`annotation` inputs. Native selection and `sketch.selectionSet` use those same
stable IDs. Existing `dimensionPreview`, `dimensionPlace`, `dimensionAccept`,
`dimensionUpdate`, `dimensionEdit` and annotation commands operate on the relation.
Pending state exposes point/line IDs, kind, measured value and placing/editing phase.

Authored dimensions retain one point ID, one straight line ID and `side: +1/-1`.
Alloy defines a positive perpendicular distance to the infinite supporting line.
For a driving dimension, the initial side is retained relative to that line's
stable endpoint direction. Projected cap edges already orient their endpoints
by retained contact IDs, preventing a kernel traversal reversal from silently
flipping this branch during the qualified resize. PlaneGCS P2LDistance supplies
the equation. Solver residuals independently check the retained signed equation;
the displayed value is positive. A driven dimension measures absolute distance
and adds no equation. These are explicit Alloy decisions, not observed reference
solver internals. Supporting-line/end-point and signed-zero parity remain unknown.

The witness foot is recomputed by projecting the actual solved center onto the
current line at redraw. It uses existing linear witness/arrow/label presentation;
no synthetic foot or frozen center is authored as a replacement for the equation.
The document continues its existing normalization of solved coordinate seeds,
while dimension/projection IDs and equations remain authoritative. Conflicts,
curved targets and degenerate references fail transactionally. Curved dimensions,
reference repair and broader zero/endpoint behavior are outside this child.

## Verification

Isolated Debug application build passes with the coherent private Qt runtime.
Focused `point_edge_dimensions` and existing `sketch_solver` checks pass.
The core fixture verifies free-circle DOF3→2→1 after perpendicular point-edge
relations while radius stays3; radius driving then removes the remaining freedom.
It covers dimension value edit, retained side, conflicting-equation refusal,
actual40×25×5 plate Cut, width40→50, center35→45, stable equation/projection IDs,
Undo/Redo and exact save/reopen. Volumes are5000−45π and6250−45π.

`tests/point_edge_dimensions_api.py` passes through the real app entrypoint.
Actual measured Cut volumes are4858.62833058846 and6108.628330588459mm³.
It separately verifies preview/place/accept, value edit/Undo, annotation-only
movement, upstream width edit, retained IDs, history and exact storage.
Artifacts: isolated `build-point-edge/evidence/api/report.json`,
`transcript.json`, `retained-hole.alloy`, `point-edge-dimensions.png` and
`resized-retained-hole.png`. Captures were inspected by the implementer.

`point_edge_controls_test` passes3Qt results,0failures,2.30seconds. On the public
31.8×15.8×9.6 base with a5mm circle initially at(8,8), actual synthesized Qt
D, center click, outer-edge click, separate pointer move/placement and3.9/Enter
run twice. Center reaches(3.9,11.9); radius remains2.5 and one radius DOF remains.
The resulting dimensions exercise both stored sides(+1 and−1). Escape after a
new point/edge selection preserves the draft, and dragging a real label changes
only annotation placement, preserving points and circle radius.

Qt's native-child motion helper did not reliably deliver MouseMove, so the test
sends explicit Qt MouseMove events to the actual viewport. This is synthesized
Qt interaction evidence, distinct from physical OS input. D initially exposed
an inactive outer-window QShortcut limitation; a bounded delivered-viewport-key
fallback now routes D and pending-dimension Escape through the same commands.
No menu substitution is used for the passing D receipt.
Widget artifacts: `build-point-edge/evidence/widget/toy-point-edge.json` and
`toy-point-edge.png`. The capture shows two readable3.9 labels with actual
witnesses/arrows on the GPU solid; matched public viewport/scale is unavailable,
so exact pixel compatibility remains unknown. No full tutorial, broader feature
parent closure, other-platform runtime, release or push is claimed.
