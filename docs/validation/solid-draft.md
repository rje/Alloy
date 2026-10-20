# Retained one-face solid Draft — SOL-086

Bounded core/API candidate, 2026-10-20. Native controls, pointer/focus behavior,
Timeline interaction and GPU visual acceptance remain SOL-093. No native input,
matched screenshot, Windows/Linux runtime or broad Draft parity claim follows.

## Command contract

`solid.draft` requires `targetBodyId`, one exact `face` object containing
`producerFeatureId`, `bodyId`, `faceId`, `neutralPlaneId`, explicit signed
`angleDeg` and explicit `flipPullDirection`. Optional `name` and `featureId`
create or edit one retained `draft` feature. Partial edits preserve omitted
parameters. No initial angle or pull default is inferred from public evidence.

The admitted input is one valid closed local orthogonal rectangular prism:
six planar faces, twelve straight edges and eight vertices. Box and rectangular
New Body Extrude are exercised. The selected retained planar side must be
perpendicular to the neutral plane. The earlier retained construction plane
must coincide with a cap. Its normal defines pull; Flip reverses that normal.
No occurrence, curved body, tangent-face propagation, parting-line, multiple
selected faces, alternate plane reference family or topology-changing Draft is
accepted by this slice. The explicit Alloy numeric policy requires a nonzero
magnitude of at least 0.001° and strictly below 89.9°; geometric refusal can occur
well inside that interval.

Positive angle removes material on the pull side of the neutral plane; negative
adds it there. For outward side normal `n`, pull `d`, signed radians `a`, the
result normal must equal `cos(a)n + sin(a)d`. The source is deep copied before
OCCT `BRepOffsetAPI_DraftAngle`; Add/Build, tangent propagation, B-rep validity,
closedness, complete unchanged topology counts, and signed analytic wedge volume
are checked before publishing a result. OCCT 7.9.3 requires Build before querying
ConnectedFaces in this fixture.

Copy and Draft kernel histories must provide a unique result-member descendant
for every source face, with one owner per result face. No nearest-face matching
is used. Unchanged faces retain IDs and frames. The drafted side receives a
stable Draft-produced ID and its frame rotates about the exact neutral-plane
hinge. A supported sketch/Cut follows this frame through Draft edits. Boundary
and solid-edge references are explicitly unavailable across this first Draft
slice (`modified_by_draft_unqualified`); there is no invented edge lineage.

`command.begin` with `command: solid.draft` accepts empty inputs, then shared
`command.update` and `command.preview` stage disposable evaluated geometry.
`command.cancel` leaves authored geometry, dirty state and undo index untouched.
Commit reevaluates the same authority and creates/edits one history feature.
`feature.editDialog` is an API edit route restoring retained inputs; it does not
supply a native dialog in this slice. Invalid preview clears its disposable stage,
reports structured error, preserves the committed model and leaves the session
usable for a valid retry. Invalid direct edits/commits preserve authored history
and downstream geometry atomically. Edit preview shows the Draft history prefix
while validating the complete downstream result before accepting the candidate.

## Evidence

Local Release build: arm64 macOS, AppleClang 17, Qt 6.11.2, OCCT 7.9.3, accepted
`build-preview-release` cache. `solid_draft` and prior `solid_face_offset` core
regression pass (2/2).

`tests/solid_draft.cpp` checks all four signed-angle/pull combinations; analytic
volume; fixed hinge and signed top-corner coordinates; unchanged face IDs and
frames; unchanged source B-rep; disposable candidate; a drafted-face circle and
2 mm blind Cut; Draft 5°→3° recompute; exact downstream Cut volume; atomic 80°
refusal; Undo/Redo, save/reopen and upstream prism-height recompute.

`tests/draft_api.py` passes 70 real-entrypoint/control responses, including
begin/update/preview/cancel, ±5° and pull flip, direct commit, exact drafted-face
circle/Cut, API edit restoration/preview/commit, separate edit/Cut Undo/Redo,
direct/session 80° refusal plus valid retry, saved authored/evaluated equality,
and rectangular Extrude as the second source family. The transcript, editable
model, independent STL readback and app log are preserved in
[the external API receipt](../../../Alloy-reference-records/validation/solid-draft-2026-10-20/api/receipt.json).

The 40×20×20 mm source is 16,000 mm³. At +5°, the front top edge shifts inward
`20 tan(5°) = 1.7497733 mm`; the wedge removes 699.9093082 mm³, leaving
15,300.0906918 mm³. At +3° with the 4 mm diameter, 2 mm blind Cut, analytic B-rep
volume is 15,555.6050245 mm³. Independent binary STL readback gives
15,555.6130079 mm³, one connected watertight component, 588 triangles, outward
winding, Euler characteristic 2 and bounds [0,0,0]–[40,20,20] mm. The reader uses
stdlib triangle parsing/integration independently of OCCT geometry summaries.

No native controls or GPU captures were qualified because the Mac console is
locked. SOL-093 retains native Modify/selection, angle/flip controls, preview,
Cancel/OK, Timeline edit and 1×/2× visual gates. Broader Draft families remain
SOL-013. Public behavior/default/visual uncertainties and source provenance are
in the external SOL086 contract
`../../../Alloy-reference-records/knowledge/specs/sol086-one-face-draft-2026-10-20.md`.

Reproduce:

```sh
cmake --build build-preview-release --target alloy solid_draft_test solid_face_offset_test -j4
ctest --test-dir build-preview-release -R '^(solid_draft|solid_face_offset)$' --output-on-failure
python3 tests/draft_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/solid-draft-2026-10-20/api
```
