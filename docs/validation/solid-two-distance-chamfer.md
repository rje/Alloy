# Two Distance solid Chamfer — SOL-088 / #626

Bounded candidate: one retained straight extrusion `endRim` edge with two
incident perpendicular planar faces. Tangent propagation is refused; multiple
edges, arbitrary B-rep topology and generated-edge naming remain SOL-005.
Native actual-hit selection/typing and 1×/2× GPU acceptance remain SOL-095/#657.

## Retained command contract

`solid.chamfer` and `solid.chamferDialog` accept `mode: "twoDistance"`, one
`edgeIds` identity, `targetBodyId`, independent canonical millimeter
`distanceOneMm` and `distanceTwoMm`, `flip`, and exact
`referenceFace: {bodyId, producerFeatureId, faceId}`. Distance one is measured
on that incident face; distance two on the other plane. Flip swaps these
assignments without modifying either authored number or reference identity.
The kernel uses OCCT's explicit `Add(d1,d2,edge,face)` overload and checks that
its contour contains only the selected edge. Both lengths must be finite and
positive, and the result must be one changed valid closed solid.

The native dock offers Equal Distance / Two Distance, both values, Flip and
both current face roles/IDs. Its initial Two Distance values are 1/1 mm (Alloy
policy). If no reference face was supplied, initial preview chooses the
lexically smaller incident retained face ID and stores it in session inputs;
changing edge/body selection clears the previous reference before resolving a
new assignment. Explicit API reference faces must resolve to the selected
edge's actual incident face. These ordering/default choices are Alloy policies.
Legacy Equal Distance files retain their three-field parameter shape.

Shared previews evaluate disposable candidates; Cancel leaves authored state
unchanged. Edits use pre-feature geometry and validate downstream features
before commit. Invalid drafts clear preview eligibility and cannot commit;
valid correction/retry is supported. Existing history and storage authority
handles undo/redo and reopen.

## Evidence

- Release `build-preview-release`, Qt 6.11.2, OCCT 7.9.3, AppleClang 17 arm64.
  Local `-j4` application and focused-test build; no CI dispatched.
- `solid_two_distance_chamfer` proves exact 30×20×10 stock, 2/1 mm top/front
  setbacks, 5970 mm³ volume, exact vertex positions before/after Flip, explicit
  nonincident-face refusal, disposable source preservation, independent edit,
  upstream Extrude height edit, atomic zero/negative/impossible refusal,
  undo/redo and retained reopen.
- Existing unchanged `solid_chamfer` regression passes all-edge Equal Distance
  preview/edit/upstream/history/storage and invalid-distance refusal. Its
  unchanged real-app API fixture also passes; capture calls completing do not
  qualify locked-console native interaction or presentation.
- The 89-call real-app API fixture passes preview/Flip/Cancel/commit, both-value
  edit, exact volumes, unchanged back-face sketch/2 mm-diameter blind Cut,
  nonincident reference clearing/reassignment after edge selection changes,
  zero/negative inputs in both fields and impossible positive input,
  preview/commit refusal with unchanged authored/undo state, valid retry,
  upstream 10→12 mm height edit, Undo/Redo and authored/evaluation reopen.
- Independently read binary STL: one closed component, 592 triangles, Euler
  characteristic 2, bounds [0,0,0]..[30,20,12] mm to floating point tolerance.
  Signed mesh volume 7106.859404 mm³ versus exact 7106.858407 mm³
  (`7110 − π`), with finite unit normals agreeing with winding.

The actual-app fixture is `tests/solid_two_distance_chamfer_api.py`; evidence is
external under `../Alloy-reference-records/validation/solid-two-distance-chamfer-2026-10-20/`.
The permitted public-workflow specification is external knowledge record
`specs/sol088-two-distance-solid-chamfer-2026-10-20.md`. Exact factory defaults,
side-order convention and matched live preview styling remain unknown from
public evidence. The research's 30→36 mm length edit is represented here by an
upstream height change 10→12 mm, exercising retained edge/support recompute.
No proprietary reference runtime was operated or captured.

## Remaining acceptance

The Mac console was locked: no native actual-hit input, physical OS input,
matched reference comparison, or 1×/2× GPU presentation is claimed. #657 keeps
those gates explicit. Windows/Linux native/runtime and broader topology remain
separate. The implemented dock has compile/API evidence only at this stage.
