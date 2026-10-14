# Sliding-lid cumulative runner candidate (#297)

Status: source-order real-app API and geometry gate passed, 2026-10-13. The
public-source checklist and provenance are held outside the repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/tut015-cumulative-checklist.md`.
The bounded runner is `tests/sliding_lid_workflow_api.py`. Its earlier numeric
Cut prefix is retained; the extension completes the same document and process.

The 206-call actual-app run creates Box 50 × 80 × 40 and Shell 4; projects the
front and a normal inner edge as a point; types Line 4/55 with its horizontal
helper, Coincident, midpoint axis, connector and Mirror; then Cuts the actual
dovetail to the inner back face. A root-sibling Lid sketch projects the cut
front, offsets the five-edge lower chain inward 0.25, and uses a separate Line
through the derived top intersections to close a selectable profile. Its
NewBody extrusion uses the same retained inner back face with end clearance
0.25. Both bodies and both constrained sketches evaluate after parameter edits.

Only after those two solids exist, the runner adds Length, Width, Height,
Thickness and Clearance. It binds seven required existing model fields by
stable target ID and leaves the Box Cut's editable zero end offset literal.
Length 40, Width 74 and Height 100 recompute the source cuts and projections.
The Lid ends at Y 35.75 behind the inner back face at Y 36; its inward sketch
Offset stays negative 0.25 while Clearance is positive 0.25. A front Lid
sketch projects the retained base-cap top edge, snaps a 15 × 2 ellipse center
to its midpoint at X 37/Z 100, and Cuts from Start Offset −10 over Distance
5. Its removed volume is 58.904862 mm³, the expected half-ellipse volume.

The actual Box top-back ledge positions a root Slider of Lid relative to Box
on Y, with limits 0..50. The runner previews the open position, animates and
stops, sets a 25 mm pose, recomputes a temporary Length edit, restores Length
40 and the closed pose, then saves/reopens the same authored document. It
exports each part separately at rest as binary millimeter STL: Box bounds
0,0,0 to 74,40,100 (44 triangles) and Lid bounds approximately
2.186,0,96.973 to 71.814,35.75,100 (168 triangles). Mesh inspection finds
one connected component each with finite aligned normals. A posed Lid STL
has Y bounds 25..60.75; the posed STEP reads back as one valid solid with
matching bounds. The two authored/evaluated solids stay separate.

Evidence is at
`../Alloy-reference-records/deliverables/sliding-box-2026-10-13/cumulative-final/`:
`receipt.json`, `app.log`, the `.alloy` file, three STL files, one STEP file,
and settled native framebuffer captures for the typed preview, dovetail Cut,
Lid Offset closure, model table, resized pair, thumb sketch/Cut, assembled
pair and open Slider. The runner uses Alloy's local API against the actual
application process. Qt input for individual controls was checked in their
feature receipts; this cumulative gate does not claim physical OS input or
pixel-level parity with an unmatched public frame. A paced creation movie is
still required for the complete tutorial deliverable and is tracked
separately from this API/geometry gate.
