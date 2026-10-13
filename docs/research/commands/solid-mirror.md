# Bounded body Mirror contract

The permitted nested-cube tutorial source and page-by-page observations are in
`../Alloy-reference-records/knowledge/tutorials/warwick/nested-cubes-ledger.json`.
The source is independently authored public teaching material, retrieved
2026-10-12. This record describes a bounded Alloy workflow, not general parity.

Observed tutorial step: **Create → Mirror**, select **Bodies**, select the lower
open cup, activate **Mirror Plane**, select its remaining top rim at z = 12.5 mm,
then accept. The mirrored upper cup remains a separate body until a later
**Combine Join**. The initial top half occupies z = 12.5–25 mm, has the same
3,182 mm³ volume as the lower cup, and does not alter the lower cup. Exact
keyboard shortcuts and defaults outside the shown step remain unknown.

Alloy's `solid.mirror` authors `{sourceBodyId, mirrorPlane}` where `mirrorPlane`
is a stable `{producerFeatureId, bodyId, faceId}` planar face. The feature owns a
new body UUID; `featureId` edits the same feature and keeps its body UUID.
`solid.mirrorDialog` and `command.begin` use the same feature command. The dialog
has explicit Objects and Mirror Plane selection roles, preview, OK and Cancel.
Cancellation creates no feature or undo entry. A stale or ambiguous reference
returns a structured failure and preserves the document. Origin and offset
construction planes, component Mirror, and other Pattern Types remain separate.

The result is a true OCCT reflection, validated as one closed solid with equal
volume. Mirrored faces and edges get distinct deterministic identities from the
Mirror feature ID and exact kernel descendants. Their frames transform into a
right-handed basis. The source body and its references stay unchanged. A parent
sketch/extrusion edit rebuilds both bodies and preserves Mirror identities.

Verification uses a plain extruded rectangle and, separately, the 25 × 25 ×
12.5 mm Shell-2 cup. The latter must yield two distinct 3,182 mm³ bodies, one
occupying z = 0–12.5 and the other z = 12.5–25. Check frame handedness,
source and new IDs, edit/undo/redo, upstream depth edit, invalid plane refusal,
save/reopen, disposable preview/cancel and native GPU presentation separately.
