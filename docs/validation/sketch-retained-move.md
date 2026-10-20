# Retained sketch Move candidate

SKT-184/#707, 2026-10-20. One retained Translate owner on an isolated XY
four-Line closed profile. The accepted atomic Move remains the default.
Root owns continuation context/backlog integration and independent acceptance.

Public provenance is external in `knowledge/specs/skt098-sketch-transform-2026-10-20.md`
and `skt098-move-public-followup-2026-10-20.md`, retrieved 2026-10-20, under
`../Alloy-reference-records/`. The accepted engineering boundary is
`skt184-retained-sketch-move-design-2026-10-20.md` in that directory. Public
sources document Move/Copy, Sketch Objects, Translate, M, numeric distances,
selection and OK/Cancel. Active-sketch retained Timeline semantics, constraint
migration and matched visual defaults remain unknown. The owner/source model,
anchor, whitelist, explicit checkbox and Edit Move route are Alloy policies.
No proprietary application was operated; no Timeline parity is claimed.

## Authority and editing

`parameters.moveGroups` retains at most one owner UUID, four original entity IDs,
four authoritative pre-Move `sourcePoints`, a recorded source anchor, unchanged
endpoint topology and original internal constraint/dimension membership IDs.
`parameters.points` keeps the same IDs with derived output coordinates as a
compatibility cache. Public validation rejects inconsistent caches rather than
repairing them. Solver input restores the source coordinates and removes the
Move group; after source solve, output is materialized as source + displacement
once. Solver diagnostics mark owned output points/curves as derived.

Existing internal driving side-size dimensions solve in source space with the
recorded source point fixed. Supported original internal constraints are
Horizontal, Vertical, Parallel, Perpendicular and Equal. Existing driving
horizontal/vertical/linear side dimensions are supported. Topology/role edits,
new output constraints/dimensions, cross-boundary relations, shared unselected
vertices, another retained owner, direct output dragging, atomic Move and owner
Delete refuse. Generic `feature.edit` cannot replace owned source/cache arrays
or the Move group. The bounded owner requires a solver-backed sketch; legacy
text/loop-offset short-circuit sketches refuse. Finite coordinates/displacements
are limited to ±100000 mm, including derived output bounds.

Native Sketch > Modify > Move/Copy or M opens the shared dock. Retain Move is
explicit and initially unchecked; four ordinary Line selections and X/Y input
create the retained owner. Invalid preview disables OK. Cancel/Escape/dock close
leave accepted draft/history unchanged. After acceptance, Modify > Edit Move is
a visible active-sketch route to the same group, with its total source-relative
X/Y displacement populated and membership locked. M/Move/Copy also reopen an
existing group. Replacing (+5,-3) with (+7,-2) replaces that group's values.
Zero is valid for an existing owner and restores source placement without
removing ownership; new creation requires nonzero displacement. Z is visibly
unavailable. OK publishes one sketch journal step; Finish Sketch publishes one
document history change. Both finish authorities preserve a pending Move by
refusing until it is accepted/cancelled.

API `sketch.movePreview` adds `retain=true` for creation or `groupId` for edit;
`sketch.moveEdit` opens the native editor. Existing Start/Pick/Accept/Cancel/Exit
commands share the same authority and terminal cleanup. A valid creation preview
keeps its owner UUID across subsequent numeric previews. Inspection exposes the
retained definition and source/output cache, including after save/reopen.

## Evidence

External evidence root: `../Alloy-reference-records/validation/skt184-2026-10-20/`.
Build: accepted `build-preview-release`, Release, private Qt 6.11.2/OCCT 7.9.3,
Apple Clang, `-j4`. No CI or push.

- `core-tests.log`: retained Move, atomic Move, rectangular Pattern and circular
  Pattern all pass (4/4). Retained core checks replacement/source-size edits,
  source-anchor placement, repeated-solve exact idempotence, zero edit, stable
  topology/profile, drag/Fix/constraint/new-dimension/Delete/role refusal,
  cross-boundary Coincident refusal, invalid-cache rejection, exact document
  Undo/Redo and save/reopen.
- `native-final.log`, `native-final/native-report.json`: 3/3 Qt test functions
  pass. Real Modify popup/action, four edge hit widgets, checkbox indicator,
  typed numeric preview, Cancel/Escape/OK, visible Edit Move action, actual hit
  on the translated bottom edge, initial values, replacement Cancel/OK, owner
  identity, sketch/document Undo/Redo and exact save/reopen are checked. Initial
  activation is separate; no focus or selection repair occurs between steps.
- `api-final/report.json` and `transcript.json`: real executable opens the exact
  native model; checks total displacement, cache equation, pending finish-route
  refusal, Cancel, zero edit/history, invalid range refusal, source width edit
  20→24, unchanged owner/entity/profile/body identities, atomic output refusal,
  generic feature edit refusal, exact authored save/reopen and printable export.
- Native model `native-final/retained-profile.alloy` retains the original
  (10,8)..(30,20) source with displacement (+7,-2). Output bounds are
  (17,6)..(37,18); its existing 8 mm Extrude remains 1920 mm³. Independent STL
  readback of `retained-profile.stl` verifies one consistently wound watertight
  component, Euler 2, bounds (17,6,0)..(37,18,8), signed volume exactly 1920.
- `api-final/source-sized-retained-move.alloy` changes source width to 24 with
  anchor (10,8) and the same displacement. The preserved Extrude becomes
  2304 mm³. Independent STL readback verifies one consistently wound watertight
  component, Euler 2, bounds (17,6,0)..(41,18,8), signed volume exactly 2304.
- `atomic-regression.log`: existing atomic native workflow and shared lifecycle
  checks pass 4/4, including API Accept/Cancel→M and sketch/session cancellation.
- Inspected native `replacement-edit-preview.png`, `move5-minus3-preview.png`
  and API `source-sized-solid.png` at native size. Labels and dimension lines
  remain readable; the dock groups its controls above the bottom buttons.
  Captures include actual OCCT OpenGL framebuffer composition and SHA256 via
  `capture.screenshot`. Hardware evidence identifies Apple M4 / Apple,
  OpenGL 4.1 Metal - 90.5, MSAA4, DPR2. Native fixture window is 1500×950 logical
  before platform adjustments; capture responses retain actual pixels.
- Failed receipts are preserved: initial native checkbox click hit blank row
  space and left atomic mode selected; test now hits/asserts the indicator.
  Initial API fixture expected `invalid_parameters` for an out-of-range schema
  value; corrected to the actual shared `invalid_arguments` contract.

Matched within Alloy: documented general selection/numeric/preview/OK/Cancel
flow and bounded equivalent editing. Unknown: exact reference styling/defaults,
sketch Timeline behavior and constraint migration. This is macOS Qt-delivered
input/GPU evidence, not physical AppKit pointer/trackpad or Windows/Linux runtime
qualification. Wider planes, multiple owners/profiles, output inverse editing,
3D Move and direct manipulator input remain broader ADV-005 scope;
Copy/Rotate/Scale remain SKT-179–181. Independent Sol review is pending.
Private fixture editors closed; preserved user PID 72618 remains running.

## Independent-review creation-boundary correction

Review of `cd0e682` found inconsistent creation/edit discrimination: the command
used presence of `groupId`, while the owner builder used its empty value. An
explicit empty string could bypass rigid creation qualification and reach an
empty-array `first()`. Both layers now use the same empty-string creation rule;
non-string core IDs reject. The builder independently requires four distinct
Line IDs and nonzero creation displacement before indexing. Existing nonempty
owner-ID edits still permit zero displacement.

Focused evidence is external in `empty-group-fix/`: retained core passes,
including empty/absent group IDs, missing/empty/partial selection, zero creation,
valid empty-ID creation and direct empty builder refusal. The real executable
API regression (`tests/sketch_retained_move_empty_group_api.py`) passes six
invalid creation cases without a crash or draft/history/authored mutation,
then accepts a qualified nonzero creation and refuses empty-ID zero creation
against that owned profile. Session cancellation preserves the document.
Build remains Release/private Qt/OCCT `-j4`; `api/report.json` and transcript
record the actual calls. These focused checks do not repeat unchanged GPU/STL
qualification. Private test editor closed; user PID 72618 preserved. No CI/push.

Independent Astra review accepted `cd0e682` plus correction `5143acb` after
checking source/cache authority, input/refusal paths, native/API receipts and
captures. The review was read-only and did not claim a fresh application run.
Bounded XY creation and same-definition editing are accepted; exact public
Timeline parity, wider modes, physical input and other platform runtime remain
open.
