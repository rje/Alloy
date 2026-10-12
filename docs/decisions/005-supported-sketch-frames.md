# Decision 005: supported sketch frames

Accepted design for the first face-supported sketch slice, 2026-10-12.
The bounded implementation passed core, real-app and native modeling checks; see
[the receipt](../validation/face-supported-sketches.md). This extends decision 002;
it does not establish general face naming.

A sketch retains local 2D coordinates and an authored support reference containing
producer feature, body and semantic face UUIDs. Its resolved world frame belongs
to evaluation/session state, not a stale serialized placement. Origin-plane
sketches retain their existing format and default frames.

The first supported face is the real end cap of a positive-distance extrusion.
Its semantic UUID derives from the producing feature UUID and end-cap role, never
an OCCT enumeration index, pointer, centroid or nearest-face search. Resolution
requires the actual unique planar face on the current available producer result.
Unsupported Boolean modifications, consumed bodies and missing/ambiguous references
fail explicitly; there is no fallback to an origin plane.

Evaluate the supporting producer before the dependent sketch. The referenced body
must belong to that exact producer. Inherit source U/V axes and transport the
source origin by extrusion depth, so width/radius edits preserve the origin and
depth edits transport it. Initially reject negative-depth support until its
right-handed frame convention is implemented and verified.

Pass the same resolved frame explicitly through analytic edges, profile extraction,
extrusion direction, drawing, picking/ray projection, snapping, annotations and
Look At. The local constraint solver remains independent of world placement.

The acceptance workflow is the rounded bracket's end face → new sketch → retained
local geometry, followed by parent width/radius/depth edits, undo and reopen.
Depth 50 → 60 must move world placement while preserving authored local numbers.
General planar faces, repair UI and projected model-edge relationships remain
separate work; the cap-only slice must be described accurately.
