# Retained active-sketch Scale

SKT-181/#704, 2026-10-20. This bounded command uniformly scales one isolated,
closed four-Line XY profile about a separate ordinary authored Point. The
source and result keep their point/curve/profile IDs, and one retained Scale
owner supports replacement factor edits and intentional source-point edits.
Root owns the independent review and continuation/backlog integration.

## Workflow and evidence boundary

Public R1/R2 document active Sketch > Modify > Scale, entity selection, a separate
point, a uniform factor, and OK/Enter. R3 documents that an explicitly selected
point cannot simultaneously be the pivot. R5 confirms uniform factor semantics.
External provenance, URLs and retrieval date (2026-10-20) remain in
`../Alloy-reference-records/knowledge/specs/skt181-sketch-scale-2026-10-20.md`;
the corrected implementation contract is alongside it in
`skt181-scale-implementation-contract-2026-10-20.md`.

Native **Sketch > Modify > Scale** shows Entities, Scale Point, Scale Factor,
preview status and OK/Cancel. Select the four actual canvas edges, click Scale
Point and pick a separate Point. Typing a complete factor refreshes a disposable
GPU preview; OK or Enter accepts, Cancel/Escape/dock close discards. **Modify >
Edit Scale** retains the same owner and locked profile membership. There is no
new shortcut or Scale drag grip. Factor input is dimensionless, initially 1.00,
with supported positive range `[0.000001,100000]`, finite output coordinates
within ±100000 mm, and ordinary profile-validity limits still applying.

The local commands use the same draft/document authority:
`sketch.scaleStart`, `scalePick {role:entities|pivot,x,y}`,
`scalePreview {entityIds,pivotPointId,factor,groupId?}`, `scaleAccept`,
`scaleCancel`, and `scaleEdit {groupId?}` (all with `sketch.` prefix).
Preview returns the draft/group identity without authored/history change.
Repeated factor previews retain that identity. Invalid geometric preview clears
the candidate and redraws the accepted draft. Schema/type/range refusal leaves
the last accepted candidate intact, following the shared command validator. Shared Finish Sketch/command.commit
refuse pending Scale; session cancellation also closes the native panel. Other
transform input and unrelated sketch edits cannot silently consume its preview.

The command vocabulary/sequence and one-factor geometry are **matched** to the
specific written public evidence. Retained ownership/edit, factor default/range,
constraint policy, preview timing, Escape/history, inverse source edits and exact
current visual appearance remain **unknown** in that evidence and are explicit
Alloy policy. Public images have unmatched generation/viewport/theme/DPI, so
there is no pixel-parity claim. No proprietary application was operated or
captured. Broader multi-loop/nonuniform/3D/body scaling stays outside this slice.

## Authored and evaluated authority

`parameters.scaleGroups` stores one UUID/type, four ordered source entity IDs,
four authoritative `{pointId,uMm,vMm}` source records, four endpoint topology
records, separate pivot ID, factor, exact internal constraint snapshot and empty
dimensions array. Ordinary `points` hold the same-ID derived compatibility cache.
Validation checks its exact formula; load/evaluation never repairs the cache.
The pivot resolves from its current separate authored Point, preserving stable
identity and composing later eligible pivot moves.

`SketchScale::source` restores pre-Scale coordinates for the ordinary solver;
`materialize` applies `c + s*(p-c)` once. Source verification refuses silent
solver drift. An independent output-space residual solve checks preserved
relations and exact targets within 1e-6 mm. Geometry qualification requires one
matching simple profile and stable profile membership/identity. Preview and
source edits evaluate existing downstream features before publication.

Supported internal geometric relations are Coincident, Horizontal, Vertical,
Parallel, Perpendicular and Equal. Shared endpoint IDs provide exact coincident
joins in the positive fixture; its explicit Horizontal relation remains intact.
Fixed, external/projected, cross-boundary or unknown relations, dimensions
(driving and reference), other ownership, shared unselected endpoints, missing
pivot/source and unsupported geometry refuse atomically. Relations are never
dropped. Delete/new constraint on owned geometry or pivot and generic
`feature.edit` of owner/cache/source arrays refuse.

`sketch.movePoint` is the explicit source-edit route for a Scale output point.
A requested visible target q maps to `p=c+(q-c)/s`; only that authoritative
source entry changes. It solves with that point locked, requires all other
points to remain in place and refuses conflicting constraints. It then derives
the output once, validates profiles and downstream references, and publishes one
history step. Direct low-level cache mutation remains refused. This receipt
proves the requested local source edit, not a new general point-drag gesture.

## Verification receipt

Evidence root: `../Alloy-reference-records/validation/skt181-2026-10-20/`.
Build uses existing `build-preview-release`, Release/private Qt 6.11.2,
OCCT 7.9.3, Apple Clang and `-j4`. No new build tree or CI.

- `sketch_scale_test` and five existing core targets (Rotate, Rotate source-size,
  Copy, atomic Move, retained Move) pass: 6/6, 2.69 s. Scale covers exact vertices,
  idempotent solves, factor replacement, inverse source edit, conflicting source
  edit, pivot edit, ownership/selection/fixed/external/driving/reference-dimension
  refusal, cache tamper, exact history and authored reopen.
- `native-final/native-report.json` and `native-final.log`: 3/3 Qt functions pass,
  22.891 s. The real Modify menu opens Scale. Actual Qt hit receivers receive the
  four edge and separate pivot clicks; focus produced by controls receives keys.
  It checks repeated stable preview IDs, no authored/history change before
  acceptance, Cancel/Escape, OK/Enter, same-owner factor edits, the explicit
  source-C request, draft/document Undo/Redo and exact save/reopen. API
  Accept/Cancel are also checked against actual panel closure and a fresh
  native Edit Scale opening; cancelling the sketch closes the active panel. Initial
  activation is separate; there is no interstep focus/selection repair.
- `api-final/report.json` and its 167-call transcript pass command discovery,
  schema/geometric refusal behavior, both Finish Sketch and command.commit
  pending guards, other input refusal, API terminal panel cleanup/reopen,
  source/pivot/constraint/cache ownership, source edits, stable body/profile
  identity and independent mesh readback. Source-only fixed, external-anchor,
  driving and reference dimension negatives preserve draft/history/downstream
  authored document. Session cancellation closes the Scale panel.
- The asymmetric source is A=(10,10), B=(30,10), C=(27,20), D=(12,24) mm,
  area 209 mm², with separate P=(4,6). At factor 1.5, exact output is
  (13,12), (43,12), (38.5,27), (16,33), area 470.25 mm². The original referenced
  8 mm Extrude becomes 3762 mm³ with the same profile/body lineage.
  After factor 2.0 then restoration to 1.5, visible C's requested (35.5,27)
  inverse-maps to source (25,20), making area 438.75 mm² and volume 3510 mm³.
  Only that authoritative source entry changes; owner, factor, pivot and IDs
  remain stable.
- `native-final/scaled.stl` and `source-edited-scale.stl` independently read back
  as one consistently wound watertight component each, 12 triangles, 8 vertices,
  18 edges, Euler characteristic 2. Signed volumes are exactly 3762 and 3510 mm³;
  both bounds are (13,12,0)..(43,33,8) mm. Declared bounds tolerance is 3e-6 mm,
  volume tolerance 0.001 mm³. These are standalone exported meshes, separate
  from the kernel summaries and screenshots.
- `factor15-preview.png`, `edit-factor2-preview.png`, `source-c-edit.png` and
  `source-edited-solid.png` were inspected at native size: full asymmetric
  profile, distinct selected pivot, readable numeric/status controls and solid.
  Apple / Apple M4, OpenGL 4.1 Metal - 90.5, hardware accelerated, 4x MSAA,
  2x framebuffer ratio. The real-app API viewport is 1188×608 logical /
  2376×1216 framebuffer; native docked preview is 932×616 / 1864×1232.
  This is prototype OCCT OpenGL evidence, not RHI migration, physical input,
  matched public-image styling or Windows/Linux runtime qualification.
- Existing Rotate native input regression passes 3/3 (15.271 s); its evidence
  is in `rotate-regression/` and `rotate-regression.log`.

Early failures are preserved separately. The first native start guard mistook
null Move/Pattern interaction keys for active input; it now checks explicit
active flags. The initial test continued after a missing-panel assertion and
crashed; it now stops with a state receipt. The next diagnostic proved actual
hit-widget delivery and Scale Pick dispatch, exposing missing numeric schema
bounds: the shared validator treated absent bounds as zero. Explicit pixel
bounds and factor policy fixed the refusal, without focus or selection repair.
A core reference-dimension fixture initially supplied a driving value and was
corrected to omit it. An accidentally overlapping second build was stopped;
one interrupted object was restored by the serialized candidate build. No
neighboring modeling failure or deferred acceptance element was found.

Reproduce:

```sh
./build-preview-release/sketch_scale_test
ALLOY_SCALE_EVIDENCE=/absolute/evidence/native ./build-preview-release/sketch_scale_controls_test
python3 tests/sketch_scale_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --native-evidence /absolute/evidence/native --output /absolute/evidence/api
```

Independent Sol read-only review accepted `3569a46` after lifecycle correction
`0c7a457`. The reviewer checked source/cache ownership, dimension/relation
refusals, native hit delivery, history and mesh evidence, and independently
reread both STL volumes. Its first pass found a competing Pattern interaction
that could leave a stale Scale preview. The correction guards Pattern entry and
requires a fresh successful preview before native OK/Enter; the reviewer
accepted the focused 3/3 native and 188-call API lifecycle receipts. The review
did not rerun the app. Exact reference visuals and physical/platform input
remain unqualified.
Private validation editors are closed and user PID 72618 is preserved.


## Independent-review lifecycle correction

The review of candidate `3569a46` found that Pattern start/edit/preview could
create a second interaction during Scale, and native Scale OK/Enter could call
Accept after a failed refresh. Pattern's shared registration now refuses all
its start/edit/preview/accept/cancel entries with `pending_scale` before any
interaction or redraw mutation while a Scale candidate or active controls exist.
This covers rectangular and circular modes. Native Scale acceptance now requires
its fresh preview to succeed, surfaces its refusal and returns immediately on
failure; a failed acceptance is also shown in the panel.

`lifecycle-native/` and `lifecycle-native.log` pass 3/3 Qt functions (20.427 s).
From a valid 1.5 candidate, an actual factor-field Enter with factor 100000
refuses output overflow, keeps the panel open, preserves the exact source/history,
clears the invalid candidate and shows a readable error; a 1.5 retry succeeds.
No test-only widget ranges or session state were injected. Source inspection
confirms Accept is conditional; the Pattern guards remove the reviewed path
that could otherwise retain a stale candidate during a failed refresh.

`lifecycle-api/` passes 188 calls including rectangular/circular starts,
shared edits, both preview modes, accept and cancel attempted during a valid
Scale preview. Each returns `pending_scale` and preserves the candidate, draft,
history and Pattern interaction state. The existing identity/history/reopen and
independent STL checks pass again (3762 and 3510 mm³). No modeling authority or
feature scope changed. Build remains the existing Release directory with -j4;
no push or CI. Private editors are closed and PID 72618 remains preserved.
