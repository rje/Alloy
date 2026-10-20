# Atomic in-plane sketch Move

SKT-098/#503, 2026-10-20. This bounded workflow translates one isolated closed
four-Line profile in an active XY sketch as an atomic authored coordinate edit.
An independent architecture review preceded implementation. Retained Move
membership/definition and same-definition editing are accepted separately in
SKT-184/#707; Copy/Rotate/Scale remain SKT-179–181/#702–704.

Public evidence and engineering review are external in
`../Alloy-reference-records/knowledge/specs/skt098-sketch-transform-2026-10-20.md`
(R1/R2), `skt098-move-public-followup-2026-10-20.md`, and
`skt098-move-architecture-2026-10-20.md`, retrieved 2026-10-20. Public sources
establish active-sketch Modify > Move/Copy, Sketch Objects, Translate, selection,
numeric distances/manipulators, M invocation and OK/Cancel. They do not establish
active-sketch defaults, exact constraint migration, or sketch-specific retained
Timeline semantics. The atomic constraint/transaction policy below is Alloy
engineering behavior. Exact panel styling is unknown; no proprietary application
was operated.

## Shared atomic workflow

Sketch > Modify > Move/Copy or M opens a canvas-owned dock showing Sketch Objects
and Translate, X/Y distances in sketch units, and disabled Z explicitly labeled
out-of-plane unavailable. X/Y start at zero as an Alloy policy. Native curve
selection is either preselected authored Lines or invoke-then-click four edges;
clicking again removes an edge. The selected-count prompt and curve/profile
highlight show membership. Accepting leaves the four curves selected and returns
focus to the viewport so M can begin another relative move. Distances are always
relative to the current profile position. A second (+2,+1) after (+5,−3) is an
independent edit; it does not edit or reapply a retained definition.

`sketch.moveStart`, `sketch.movePick`, `sketch.movePreview`,
`sketch.moveAccept`, `sketch.moveCancel` and `sketch.moveExit` use the same native
controller and authoritative document. Preview takes `entityIds`, `x`, `y` and
optional `units=mm|in`, and returns the disposable draft, diagnostics and explicit
`retained=false`. Invalid preview removes the candidate and disables OK. Cancel,
Escape and dock close restore accepted draft/history. OK publishes one ordinary
sketch-history edit; Finish Sketch uses the existing document authority. No new
saved transform metadata or alternate automation model exists.

## Rigid translation and refusal

`SketchGeometry::translateProfile` requires four distinct normal authored Lines
forming one simple closed loop with four shared point identities. Mixed-sketch,
projected, construction, disconnected/open, shared-unselected-endpoint and
retained-owned inputs refuse with structured command errors and actionable text.
Coordinate-coincident but separately identified vertices are outside this bounded
connected-topology family. XY only, finite displacement/coordinates within
±100000 mm, and a nonzero displacement are explicit limits.

The method translates every selected unique point simultaneously, solves the
candidate once through PlaneGCS, then checks all selected outputs against their
rigid targets within 1e-6 mm. Every unselected point and all entity, constraint,
dimension and owner definitions must remain unchanged. Solver acceptance alone
is insufficient: any relaxed shape, changed displacement or moved unselected
anchor refuses and returns the exact original draft. No constraint is removed.
The existing entity/point IDs preserve profile lineage and downstream Extrude
references. Downstream evaluation validates before publishing preview or commit.

## Candidate evidence

Build is `build-preview-release`, Release/private Qt 6.11.2, OCCT 7.9.3, Apple
Clang, `-j4`. External evidence:
`../Alloy-reference-records/validation/skt098-2026-10-20/`.

- `sketch_move_test`, `core-final.log`: a 20×12 rectangle with side dimensions and H/V constraints
  retains exactly two translational DOFs. (+5,−3) and a separate (+2,+1) reach
  total (+7,−2), preserving entity/constraint/dimension/profile identities.
  Invalid/mixed/disconnected selections, a shared unselected endpoint, retained
  Mirror ownership, zero displacement, a driving placement dimension, and a
  relation to an unselected free anchor refuse atomically. Authored history and
  save/reopen remain exact.
- `sketch_move_controls_test`, `native-final/` and `native-final.log`: 3/3
  functions pass in 10.134 s. Checks cover real Modify popup/action hits, four native edge
  picks, numeric X/Y preview, Cancel/Escape, M, selected-curve prefill, OK, separate
  relative moves, sketch/document Undo/Redo, same downstream profile/body and
  8 mm Extrude volume 1920 mm³, save/reopen and STL. It verifies actual hit widgets,
  command-produced focus and popup activation/closure without interstep repair.
- `tests/sketch_move_api.py`, `api-final/`: 55 real-executable calls pass. The app opens the exact native-produced
  model, exercises local preview/Cancel/Accept and undo, rejects invalid/no-op
  selections and an unselected Coincident anchor, and preserves the committed
  model. Independent STL readback checks one consistently wound watertight solid,
  Euler characteristic 2, 12 triangles, bounds (17,6,0)..(37,18,8) mm and mesh
  volume exactly 1920 mm³ (analytic OCCT volume 1919.9999999999998 mm³).
  Generic inspection/Look At preserve the candidate, and Finish Sketch refuses
  while Move is pending.
- Native preview and solid captures are inspected separately from authored,
  solver, exact geometry and mesh evidence. Hardware diagnostics identify Apple
  M4 / Apple, OpenGL 4.1 Metal, MSAA4 and DPR2. Detailed public-reference visual
  parity is unknown because state/scale/theme are unmatched.
- Initial failed fixture receipts are retained: temporary QJsonValueRef lookups
  caused test-only crashes and were corrected to owned `.value()` lookups; a
  later fixture export omitted required STL tolerance fields and was corrected
  to the accepted 0.01 mm / 5° settings. Neither is claimed as a passing gate.

## Limits

Retained definition/editing is accepted separately in SKT-184/#707. Numeric translation is the supported
input; direct axis manipulator dragging, wider planar/support frames, multiple
profiles, arbitrary curve families and 3D modes remain broader workflow scope.
Physical AppKit pointer/trackpad, native DPR1 and packaged Windows/Linux runtime
are unqualified release gates. No CI, tag or release was run. Private fixture
editors closed; user-owned PID 72618 preserved. Independent acceptance is
recorded below.

## Independent-review lifecycle corrections

The review of `9952f07` found two command-boundary defects: generic
`command.commit` bypassed the local Finish Sketch pending-Move check, and direct
API Accept/Cancel left the native panel alive with stale interaction state.
The pending-Move rule now lives in shared `finishSession` authority, before any
candidate/session mutation. Both finish routes refuse with `pending_move`,
preserving the visible candidate, authored draft and history.

One terminal helper clears transient state, disconnects/hides/retires the native
panel, restores the current draft and returns viewport focus. Native buttons,
API Accept/Cancel, Escape, dock close and sketch/session cancellation all use it.
API completion therefore permits M to open a fresh zero-displacement panel;
Accept preserves the four selected curves for the next relative move. Deferred
widget deletion is separated from terminal state cleanup.

Focused evidence is under external `validation/skt098-2026-10-20/lifecycle-fix/`.
The native fixture adds both finish-route refusals, API Accept→M, API Cancel→M,
and both sketch/general session cancellation with panel closure; it also repeats
the existing actual-hit/print workflow. The real-executable receipt adds generic
`command.commit` refusal to its pending-Move checks. Core transform and mesh
contracts are unchanged.

The expanded full fixture passes 4/4 in 8.282 s (`native.log`). After narrowing
signal disconnection to the terminal rejected handler, the final focused
lifecycle check passes 3/3 in 2.615 s without warnings
(`native-terminal-final.log`), including actual deferred widget retirement before
reopening. The fresh zero-X/Y, four-selected-curve panel capture was inspected at
`native-terminal/api-accept-reopened-move.png`. Core remains green (`core.log`);
the real-executable receipt passes 56 calls (`api/`) and independently retains
STL bounds (17,6,0)..(37,18,8), watertight topology and exactly 1920 mm³ mesh
volume. Private editors closed; only preserved user PID 72618 remains.

Independent Sol re-review accepted `9952f07` plus corrective `55628da` after
checking source, focused receipts and the fresh reopened-panel capture. The
reviewer did not claim a fresh build or application run. Exact reference styling,
physical input and packaged platform behavior remain unqualified.
