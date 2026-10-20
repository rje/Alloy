# Uniform retained body Scale — SOL-087

Bounded core/API candidate, 2026-10-20. Native controls, Body/Origin selection,
Timeline interaction and GPU visual acceptance remain SOL-094/#656. No native
input, matched screenshot, Windows/Linux runtime or broad Scale parity claim
follows from this receipt.

## Command and geometry contract

`solid.scale` takes one `targetBodyId`, explicit `pivot: documentOrigin`, and
finite strictly positive dimensionless `factor` (Alloy default 1). Optional
`name` and `featureId` create or edit a retained `scale` timeline feature.
Partial direct edits preserve omitted retained parameters. There is no implicit
centroid, custom pivot, multiple-body, copy-first or nonuniform mode.

The target must be one active, valid closed root-owned solid, without an
occurrence identity. The feature preserves the body and original producer
identities. Evaluation uses `gp_Trsf::SetScale` about `(0,0,0)` and
`BRepBuilderAPI_Transform` with geometry copying and without mesh copying.
Every available retained face and edge must have one unique exact OCCT history descendant
present in the result. Already unavailable solid edges remain unavailable.
There is no proximity-based remapping. Face, boundary
edge and solid edge IDs are unchanged. Frame origins and positional provenance
scale; normalized face U/V/normal axes remain unchanged for positive uniform
scale. The result must be one valid closed solid with factor-cubed volume.
Authored feature parameters contain no evaluated B-rep or tessellation.

`command.begin`, `command.update`, `command.preview`, `command.commit` and
`command.cancel` use the same candidate/evaluator as direct execution.
`feature.editDialog` restores retained inputs through the API; this slice has
no native Scale dialog. A preview validates the complete downstream graph,
then displays the Scale feature's history prefix for edit inspection. Invalid
preview discards its stage and leaves the session available. Cancel preserves
authoring, dirty state and undo index. Invalid direct/session commit preserves
the entire committed authoring and evaluated result atomically.

A later retained Offset Face follows the original exact scaled top-face
reference. Its fixed millimeter offset is reevaluated after Scale, rather than
being multiplied by Scale. Unsupported downstream geometry refuses the whole
Scale edit. This is Alloy's tested history policy, not a documented reference
product parity claim about attached dimensions.

## Evidence

Local Release build: arm64 macOS, AppleClang 17, Qt 6.11.2, OCCT 7.9.3, accepted
`build-preview-release` cache. Focused `scale_document` plus `align_document`
and `solid_draft` regression checks pass (3/3). Core checks cover exact
body/face/edge identity, normalized frame axes, multiplied positional provenance,
volume, disposable preview, edit, Undo/Redo, save/reopen, nonpositive/nonfinite
factor and missing target/pivot refusals, and explicit root-owner/occurrence
refusal.

The real-entrypoint fixture `tests/scale_api.py` passes 65 control responses. It
exercises an off-origin
20×20×12 mm prism, initial bounds `[10,-8,4]`–`[30,12,16]` mm and volume
4,800 mm³. At factor 1.5 its volume is 16,200 mm³; at factor 2 its bounds are
`[20,-16,8]`–`[60,24,32]` mm and volume 38,400 mm³. A downstream 1 mm top-face
Offset follows a factor edit while retaining its authored reference. A later
inward Offset that would consume a sufficiently shrunken prism refuses the
upstream Scale edit before authoring, geometry or undo mutation.

External transcript/model/mesh evidence is stored in
[the API receipt](../../../Alloy-reference-records/validation/scale-2026-10-20/api/receipt.json).
The factor-2 STL has 12 triangles, one watertight outward-oriented component,
Euler characteristic 2, the exact requested bounds, and signed volume
38,400.00000000001 mm³. The saved/reopened downstream Offset result is
40,000 mm³. Independent binary STL parsing/integration uses the stdlib reader in
`tests/stl_export_api.py`, independently of the OCCT summary.

The Mac console is locked, so this receipt contains no native input or GPU
visual qualification. SOL-094/#656 retains Modify placement, Body/Origin
selection, factor editing, preview/Cancel/OK, Timeline input and 1×/2× captures.
Public workflow sources, unknown focus/default/visual behavior and provenance
remain in the external SOL087 contract
`../../../Alloy-reference-records/knowledge/specs/sol087-uniform-scale-2026-10-20.md`.

Reproduce:

```sh
cmake --build build-preview-release --target alloy scale_document_test align_document_test solid_draft_test -j4
ctest --test-dir build-preview-release -R '^(scale_document|align_document|solid_draft)$' --output-on-failure
python3 tests/scale_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/scale-2026-10-20/api
```
