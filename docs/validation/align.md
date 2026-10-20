# Retained planar-face Align — SOL-084

Bounded core/API candidate, 2026-10-19. Native controls and visual acceptance remain SOL-091/#653; no native activation or exact reference parity claim.

## Command contract

`solid.align` takes exact `sourceFace` and `targetFace` objects containing
`producerFeatureId`, `bodyId`, and `faceId`, optional `flip` (Alloy default false),
`name`, and `featureId` for editing. Both bodies must be distinct active root-owned
closed single solids, and each face must resolve uniquely to its actual retained
planar OCCT face. Components/imported occurrences and arbitrary generated faces
without exact retained frame identity refuse.

Alloy's explicit `retained-face-uv-v1` rule maps the source retained frame origin,
U axis and normal onto the corresponding target frame. This aligns the complete
frame, including its declared in-plane roll; no geometric proximity or edge-order
heuristic chooses a twist. Flip rotates 180 degrees about target U, reversing
normal and V. This rule/default is an Alloy policy; public parity is unknown.
The rule uses the retained semantic frame, which is not necessarily the face
center. Coincident selected planes do not imply identical face outlines.

The evaluated source summary exposes `align.sourceFrame`, `targetFrame`, `flip`,
`frameRule` and row-major homogeneous 4×4 `transform` (millimeter translation).
Retained face and edge IDs are transported rigidly with the source. The target
is fixed and the source keeps its body ID. References remain authored and the
transform is reevaluated from the current earlier source/target geometry.

`command.begin` with `command: solid.align` starts an API session, including with
empty inputs. `command.update` accepts From/To/Flip inputs; `command.preview`
stages disposable evaluated bodies and reports frames/matrix. Cancel clears the
stage with no authored or undo mutation. Commit reevaluates the same candidate
and creates/edits one retained Align history feature. Editing is available via
`solid.align` or an API session with `featureId`; native Timeline edit is pending.
Unknown fields, invalid frames, lost exact references, same-body references,
non-root bodies, and invalid solids refuse atomically.

## Evidence and remaining scope

Core fixture `align_document_test` verifies exact origin/U/normal mapping, flip,
source identity, stationary target, retained face/edge lineage, upstream dimension
recompute, Undo/Redo, persistence, and atomic same-body/missing-face refusal.
Real application fixture `tests/align_api.py` covers the shared session lifecycle,
Cancel, commit/edit, downstream exact-face Offset Face, undo/reopen and refusal.
The 38-call real-app fixture passes and saves its complete transcript/model under
`../../../Alloy-reference-records/validation/align-2026-10-19/api/`.
The local Release build uses Qt 6.11.2 and OCCT 7.9.3 on arm64 macOS.

Native command-first From/To picks, Modify entry, flip control, visible target
frame cue, Escape/focus, Timeline double-click and 1×/2× GPU comparisons are
explicitly unimplemented/unqualified under SOL-091/#653. Broader Align geometry,
Angle increments, components, offsets and persistent assembly relations remain
under SOL-008. No Windows/Linux runtime or packaging claim follows.

Public evidence: external dated SOL084 contract
`../../../Alloy-reference-records/knowledge/specs/sol084-align-2026-10-19.md`.
It separates documented editable Align/From/To/Flip from inferred live preview
and unknown normal/roll defaults. Reference names/assets remain external.

Reproduce:

```sh
cmake --build build-preview-release --target alloy align_document_test -j4
ctest --test-dir build-preview-release -R '^align_document$' --output-on-failure
python3 tests/align_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/align-2026-10-19/api
```
