# Primitive Box topology candidate

Status: core and real-app shared-API candidate, 2026-10-13. Physical native-input
and independent visual acceptance remain separate. The external source specification is stored in
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/independent-spec.md`.

The bounded source prefix is a 50 × 80 × 40 mm primitive Box from the XY origin,
followed by inward Shell 4 mm on its top face. Box now publishes six actual OCCT
planar support faces and twelve actual straight edges. Their UUIDs derive from
the authored Box feature and semantic axes/corners, so dimension edits and
save/reopen preserve their identities. A topology with missing, split or
ambiguous primitive faces/edges refuses evaluation rather than assigning a
nearby replacement. The top face resolves through the same support path used
by Sketch and Shell; solid edges use the shared Fillet/picking path.

Shell maps each Box face's straight offset boundaries only through unique OCCT
offset history. The five inner planar faces include four straight support edges
each, with current endpoints and retained source IDs. This supports same-face
projection on the inner back wall at y = 76 mm and gives a stable face for later
To Object. The inner bottom at z = 4 mm is a different face.
Projection of remote or perpendicular edges onto the front face remains a
separate workflow requirement; this candidate does not claim that interaction.

The `primitive_topology` core test exercises the actual Box → top-face Shell4
shape and volume, outer and inner face/edge counts, inner back projection,
upstream width and Shell thickness edits, stable identities, undo/redo,
save/reopen and atomic refusal of a missing face. Existing `solid_shell` and
`geometry_kernel` focused tests pass. `tests/box_topology_api.py` passed through
the real app entrypoint in 28 shared-API calls. It selected the actual top face
through viewport picking, checked preview/Cancel, committed Shell, captured
GPU frames, edited Box to 54 × 90 × 40 mm, and checked history and reopen.
The actual inner back face remained at y = 86 mm with four stable edges after
the resize. Captures of the box, Shell panel and settled open box were inspected:
the walls, opening, back and bottom are readable with no apparent missing
faces. Evidence is external under
`../Alloy-reference-records/validation/box-topology-2026-10-13/attempt-02`.
The API runner closes its private editor. This does not establish physical
mouse input or a matched public screenshot comparison.
