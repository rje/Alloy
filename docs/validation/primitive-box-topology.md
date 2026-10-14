# Primitive Box topology candidate

Status: focused core candidate, 2026-10-13. Independent real-app API and native
visual acceptance remain pending. The external source specification is stored in
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
projection on the inner back wall and gives a stable face for later To Object.
Projection of remote or perpendicular edges onto the front face remains a
separate workflow requirement; this candidate does not claim that interaction.

The `primitive_topology` core test exercises the actual Box → top-face Shell4
shape and volume, outer and inner face/edge counts, inner back projection,
upstream width and Shell thickness edits, stable identities, undo/redo,
save/reopen and atomic refusal of a missing face. Existing `solid_shell` and
`geometry_kernel` focused tests pass. `tests/box_topology_api.py` is prepared
for the sole editor lease: it selects the actual top face through viewport
picking, checks preview/Cancel, commits Shell, captures GPU frames, edits Box,
and checks history and reopen. It has not yet been run.
