# Retained planar sketch Rotate

SKT-180/#703, 2026-10-20. One retained Rotate owns an isolated closed four-Line
XY profile and uses its sketch's authored normal +Z construction Line. Angle
and eligible-axis edits are supported; later source-size composition is
explicitly deferred to SKT-188/#711. General 3D Sketch remains broader scope.
Root owns continuation-context/backlog integration and independent acceptance.

Public provenance is external in
`../Alloy-reference-records/knowledge/specs/skt180-sketch-rotate-2026-10-20.md`
(R1–R8, retrieved 2026-10-20). R1 documents Sketch Objects, Rotate, axis and
angle in Move/Copy. R2 documents the same-sketch 3D Line workaround and excludes
a coplanar point as its axis center. R4 supports constraint/fixed-geometry
refusal, including H/V conflicts. R6's signed numeric angles concern bodies;
it does not establish sketch sign convention. Exact sketch defaults, signed
orientation, preview cadence, edit/history semantics, control styling and
angle manipulator gestures remain unknown. Retained ownership, degree units,
right-hand sign, transactional preview and UI details here are Alloy policy.
No proprietary application was operated. No matched active-sketch Rotate
screenshot or exact Timeline parity is claimed.

## Command and authority

In an active XY sketch, enter Move/Copy through Solid > Modify, Sketch > Modify
or M. Move Object is Sketch Objects; choose Rotate from Move Type. Click the
four ordinary profile Lines, activate the axis selector, then click the authored
3D +Z marker. A coplanar point or ordinary 2D Line is ineligible. The axis is
reference-only and never part of the selected profile. The 3D Sketch checkbox
controls helper authoring/ordinary reference picking; Rotate's explicit axis
role can pick that marker while its authoring checkbox is off.

Enter a signed Angle in degrees. Positive is right-hand about +Z,
counterclockwise from the +normal side looking toward the sketch. Initial 0°
is an explicit Alloy display default; creation requires nonzero input. An
existing owner can be edited to zero while retaining its identity. Finite angle
input is bounded by ±360000 degrees; source/output coordinates remain within
±100000 mm. Height/axis eligibility follows the normal-axis prerequisite.

Preview changes the displayed evaluated profile while preserving the accepted
draft and history. Consecutive valid previews retain the pending owner UUID.
Invalid geometry/ownership requests discard the candidate and disable OK.
Cancel/Escape/dock close and API/session cancellation share the Move/Copy
terminal cleanup. Both sketch.finish and command.commit refuse pending input.
Other geometry edits refuse while a Rotate candidate is pending. Switching
Move Type cancels the old candidate before opening the other mode. OK publishes
one solved sketch journal entry through the same authority as native Move/Copy.
Edit Rotate in Sketch > Modify opens the same owner with locked membership;
entering Rotate again also discovers that owner.

Local commands extend normal discovery:

- `sketch.rotateStart` / `sketch.rotateEdit {groupId?}` open shared controls.
- `sketch.rotatePick {x,y}` performs the active objects/axis role hit.
- `sketch.rotatePreview {entityIds?,axisLineId,angleDeg,groupId?}` previews;
  provide the existing group ID to replace its angle rather than creating one.
- `sketch.rotateAccept` / `sketch.rotateCancel` share Move/Copy publication and
  cleanup. Responses identify degree angle, retained group and sign policy.

The authoritative `parameters.rotateGroups` record stores one UUID, original
entity IDs/topology, pre-Rotate `sourcePoints`, unchanged internal constraint/
dimension snapshots, `axisLineId`, both axis endpoint IDs and `angleDeg`.
Normal `parameters.points` are the same-ID derived output cache:
`pivot + R(angle) * (source - pivot)`. Editing +30→−15 replaces the angle over
that original source. It does not rotate the already rotated cache. Solver
input restores source coordinates and removes the Rotate owner/reference axis;
materialization then applies Rotate once. A second solver residual check at
output coordinates verifies preserved relations without adopting normalization
as new source. Rigid-target/metadata checks preserve unrelated points and
records. Displayed annotation anchors follow Rotate while their authored source
coordinates remain unchanged.

Supported internal relations are Coincident, Parallel, Perpendicular and Equal;
all their references must remain inside the four-Line membership. Internal
point-to-point Linear dimensions, including a diagonal, are retained unchanged.
H/V, fixed, projected, placement/non-invariant dimensions and external relations
refuse with the implicated relation ID when available. No constraint is deleted
or relaxed. Direct point edits, Delete, new relations and existing source-size
updates refuse owned output. Snapshot changes refuse; #711 preserves future
source-size editing. Other retained owners cannot acquire these members.
The group must remain a simple nondegenerate closed loop bounding one isolated
four-edge profile; downstream evaluation is checked before accepting preview.

Dedicated normal-axis pivot/height edits retain its three IDs and rematerialize
the same Rotate once. Missing, foreign, reversed, degenerate, coplanar and tilted
axes cannot pass the normal-axis/owner validation. Axis Delete remains refused
while referenced. Generic feature.edit cannot replace an existing Rotate owner
or construction-axis collection. This adds no separately claimed sketch
Timeline feature.

## Evidence

External root: `../Alloy-reference-records/validation/skt180-2026-10-20/`.
Accepted build: `build-preview-release`, Release/private Qt 6.11.2 and OCCT
7.9.3, Apple Clang, `-j4`; no CI or push.

- `sketch_rotate_test`: exact +30/−15 and moved-pivot analytic vertices,
  idempotent source-only solve, same source/owner/profile identity, five internal
  driving Linear dimensions, exact unselected standalone point preservation,
  owned output/Delete/source-size refusal, referenced axis Delete refusal,
  invalid membership/axis/empty group/zero creation, H/V and external Coincident
  conflict, exact document history/save/reopen.
- `native-final.log` and `native-final/native-report.json`: 3/3 Qt functions
  pass in 12.027 s. Actual Solid Modify
  menu/action, Move Type combobox, four viewport edge hits, explicit axis-role
  marker hit, numeric +30 preview and stable preview UUID, Cancel/Escape/OK,
  actual Edit Rotate action and −15 replacement, sketch/document Undo/Redo,
  downstream body/profile identity and exact reopen. Tests deliver to the real
  hit widget and separate initial activation; no interstep focus/selection
  repair. Final +30, −15 and finished-solid captures were inspected at native
  size. Initial default annotations crowded the profile; final source anchors
  sit outside its sides and their display follows the owner. The selected axis
  color also follows the explicit Rotate axis role during edit.
- `api-final/report.json` and its 143-call transcript: real executable checks both signed
  analytic vertex sets, exact constraints/source snapshot, invalid/foreign
  membership and axis, both finish guards, pending-input and owned-output
  refusals, source-size/Delete refusal, API panel Accept/Cancel/session cleanup,
  H/V and external-anchor conflicts, Create Copy→failed Rotate capture/candidate
  invalidation, Rotate→Edit Move panel transition, history/reopen, same profile/body lineage,
  and helper pivot `(4,3)→(6,5)` with angle −15 before exact restoration.
  An early harness assertion treated the existing empty/null interaction state
  as a present active panel; the final assertion checks active content.
- The proposed asymmetric source vertices are `(12,6),(32,10),(27,22),(9,18)`
  mm, with four side lengths and one diagonal driver (three rigid DOF). Area
  remains 244 mm² and the same 8 mm Extrude remains 1952 mm³. The +30 expected
  vertices are `(9.428203230,9.598076211)`, `(24.748711306,23.062177826)`,
  `(14.418584287,30.954482672)`, `(0.830127019,18.490381057)` mm. API/core
  analytic tolerance is 1e-6 mm; source/profile/curve/body IDs are preserved.
- Independent binary STL readback at +30 and −15 returns one watertight,
  consistently wound component, 12 triangles and Euler 2 each. +30 bounds are
  approximately `(0.830127,9.598076,0)..(24.748711,30.954483,8)`, signed volume
  1952.00003256 mm³. −15 bounds are
  `(12.503863,2.514548,0)..(32.857655,16.194792,8)`, signed volume
  1951.99987979 mm³. Checks use 3e-6 mm float32 bound tolerance and 0.001 mm³
  volume tolerance. The axis contributes no profile or exported facets.
- GPU receipt: Apple / Apple M4, OpenGL 4.1 Metal - 90.5, hardware accelerated,
  4x MSAA, 2x resolution ratio, native 1188×608 / framebuffer 2376×1216. Native
  panel/axis and separate real solid captures were inspected. This is prototype
  OCCT OpenGL evidence; physical input and Windows/Linux runtime are unqualified.
- Focused normal-axis, Copy, atomic Move and retained Move core regressions and
  shared Move/Copy/normal-axis native regressions pass separately: 3/3 in
  25.14 s (`native-regressions.log`).

Reproduce via `sketch_rotate_test`, `sketch_rotate_controls_test` and:

```sh
ALLOY_ROTATE_EVIDENCE=/absolute/evidence/native ./build-preview-release/sketch_rotate_controls_test
python3 tests/sketch_rotate_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --native-evidence /absolute/evidence/native --output /absolute/evidence/api
```

This accepts only the recorded normal-axis planar scope. Arbitrary 3D axes,
source-size composition (#711), general 3D Sketch, angle grips and exact
reference history/visual parity are not claimed. Private fixture editors are
closed and user PID 72618 is preserved.

Independent Astra read-only review accepted `34f36c6` for this bounded scope.
It checked source/cache authority, same-owner replacement, axis identity and
refusals, post-rotation relation validation, native/API lifecycle and the saved
geometry/STL receipts. The reviewer did not rerun the application. Source-size
composition remains explicitly owned by SKT-188/#711.
