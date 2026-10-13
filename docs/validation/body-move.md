# Bounded editable body Move

Candidate 2026-10-12, SOL-019 (#169), child of #53. This supports one body's
Free Move without copy or align. Public evidence uses stable reference
`SRC-WARWICK-STRAW-BUNG`: external knowledge under
`../Alloy-reference-records/knowledge/tutorials/warwick`. Original public video
5:07–5:17 shows a 40 mm cutter, Y +10°, then local Z -20 mm; fields reset
between gestures. The PDF instead shows 60 mm and X -10°; its intermediate
Y 2 mm does not establish the final placement. These are separate variants.

## Contract and evidence

Modify → Move/Copy, Browser body right-click → Move/Copy, or a viewport body
right-click opens Bodies / Free Move. Select one body, edit distances/angles or
drag the GPU triad's axis arrows/rotation rings. Numeric editing completion and
mouse release accumulate a local rigid delta and reset the six live fields.
OK commits all gestures as one authored Move feature; Cancel discards the draft.
Escape also cancels. Timeline editing restores the accumulated steps; new deltas
modify that feature, while API clients may replace its `steps`.

The initial pivot is the selected solid's bounding-box bottom center; explicit
Pivot X/Y/Z fields can change it. That default and local-axis composition are
bounded Alloy rules inferred for this tutorial, not established universal
reference defaults. The video-supported cutter foot is (0,0,11). Local rotations
compose X, then Y, then Z within each delta; a point rotates first, then translates within
a mixed delta (`T * R`). Separate rotation and translation gestures match the researched
path. An editable authored Free Move is an Alloy requirement, not a claim that
the reference normally stores Free Move parametrically.

A Move retains the producer body identity, rigid dimensions/volume and feature
order. Downstream Combine uses its transformed BRep. Existing supported cap
lineage is invalidated: attaching sketches to moved caps is outside this slice,
and any resulting unresolved downstream support refuses the whole candidate.
Copy/align remain in parent #53. Moving several bodies and arbitrary pivot
selection/snapping remain outside this bounded one-body tutorial slice.

The focused `move_document` and existing `boolean_document` tests pass:
local-axis displacement, rigid volume, disposable evaluation, one-transaction
undo/redo, actual downstream Cut, exact save/reopen and consumed-tool refusal.
`tests/move_api.py` passes through the real application with Apple M4 GPU,
rotation→accept/reset→translation, Cancel, commit/history, editing the earlier
Move after Cut, and save/reopen. Captures are under the private worktree
`build-move/evidence/move/`: `move-preview.png`, `move-panel.png`,
`moved-cut.png`, `report.json` and `transcript.json`. The real preview and panel
were inspected; the manipulator is GPU geometry in the top overlay layer.
A bounded follow-up replaces platform-clamped wire handles with screen-sized
solid cylinders/cones and torus rings, using unlit saturated axis colors. The
post-build `move-preview-solid-triad.png` actual-app capture was inspected and
shows readable handles against the stock; projected hit targets are unchanged.
Astra independently accepted native Y10 and localZ−20 numeric completion/reset,
physical axis-arrow and rotation-ring gestures/reset, native OK/Cancel and
reopen. A narrow Escape follow-up removes duplicate shortcut lifetime, explicitly
focuses the dragged viewport and restores exact saved intent when canceled after
a drag; repeated-session widget regression and independent native Escape pass.
The moving body now uses established opaque selection blue; the stock remains
neutral. `move-preview-blue-solid-triad.png` and `native-escape-fixed.json` record
those bounded results. Native body context-menu activation remains inconclusive;
its implementation is not represented as a verified uninterrupted tutorial path.

## Local control

`solid.move` commits a feature; `solid.moveDialog` opens the shared draft.
Inputs: `targetBodyId`, optional `featureId`, `name`, explicit world pivot
`pivotXMm/pivotYMm/pivotZMm`, accumulated `steps` and current local delta
`xMm/yMm/zMm/xDeg/yDeg/zDeg`. Omitted delta fields are zero; partial step inputs
are normalized to six canonical numeric fields. `solid.moveApplyGesture`
accumulates/reset fields. Ordinary `command.update/preview/commit/cancel` apply.
`viewport.moveHandles` reports projected logical-pixel arrow/ring geometry.
`solid.moveHandle` is the shared native gesture input command.
