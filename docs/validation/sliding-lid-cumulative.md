# Sliding-lid cumulative acceptance (#297)

Status: independently accepted by Astra on 2026-10-13. The corrected full
creation run passes; the main movie and clearly labelled focused thumb replay
together supply visible creation evidence. The public-source checklist and provenance are held outside the repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/tut015-cumulative-checklist.md`.
The bounded runner is `tests/sliding_lid_workflow_api.py`. Its earlier numeric
Cut prefix is retained; the extension completes the same document and process.

The final 228-call actual-app run creates Box 50 × 80 × 40 and Shell 4; projects the
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
on Y, with Reverse Direction enabled and limits 0..50. Positive travel then
moves toward the front opening at negative world Y. This direction is inferred
from the actual Box/Lid geometry; the public tutorial establishes the positive
travel limits but not the global axis sign. At the 50 mm preview, Lid Y bounds
are −50..−14.25, entirely clear of the Box's Y=0 front. The runner animates and
stops, sets a 25 mm pose, recomputes a temporary Length edit, restores Length
40 and the closed pose, then saves/reopens the same authored document. It
exports each part separately at rest as binary millimeter STL: Box bounds
0,0,0 to 74,40,100 (44 triangles) and Lid bounds approximately
2.186,0,96.973 to 71.814,35.75,100 (168 triangles). Mesh inspection finds
one connected component each with finite aligned normals. The final run verifies posed Lid Y bounds −25..10.75 at positive position 25, with posed STEP
matching the STL. The two authored/evaluated solids stay separate.

Evidence is at
`../Alloy-reference-records/deliverables/sliding-box-2026-10-13/creation-final/`:
`receipt.json`, `app.log`, the `.alloy` file, three STL files, one STEP file,
and settled native framebuffer captures for the typed preview, dovetail Cut,
Lid Offset closure, model table, resized pair, thumb sketch/Cut, assembled
pair and open Slider. The runner uses Alloy's local API against the actual
application process. Focused corrected core, native and API evidence is under
`../Alloy-reference-records/deliverables/sliding-box-2026-10-13/slider-direction-final/`.
Qt input for individual controls was checked in their
feature receipts; this cumulative gate does not claim physical OS input or
pixel-level parity with an unmatched public frame. The accepted creation movie
and focused supplement are described below.


## Final movie and independent review

`creation-final/source-creation.mp4` is the full 97-second shared-API creation
record, 1120 × 902 at 5 fps, 485 encoded frames, with no capture or encoder
failures. Its 246 repeated frames preserve time spent in synchronous geometry
and framebuffer work; this is not a smooth physical-mouse demonstration.
The actual front-opening pose has Lid Y bounds −50..−14.25 at positive joint
coordinate 50. The posed STL and independently read STEP at coordinate 25
have Y bounds −25..10.75. The reversed green motion cue matches that direction.

The main movie's thumb sketch was above the camera. A separate clearly labelled
21.8-second focused replay at `thumb-visible-final/` restores the actual saved
model's exact pre-thumb feature prefix using feature deletion, then visibly
recreates the midpoint ellipse and Cut with ordinary zoom/pan commands. Its
41-call ledger has no failed commands and verifies the same final Lid volume.
The source model is untouched. `tests/sliding_lid_thumb_recording.py` records
this bounded supplement; no unchanged full workflow was repeated. The future
full runner now centers this stage too. `thumb-visible.png` shows readable
15 × 2 dimensions, the retained midpoint and fully constrained status.

Astra accepted the source-order construction, geometry, bounds, exports, movie
and supplement. This uses the geometrically reflected left-half dovetail
construction, not the source's right-half screen sequence. Controls have their
separate bounded Qt receipts; no physical OS input or pixel-perfect reference
parity is claimed. Box and Lid printable files are separate rest-space models,
not the deliberately posed export. The local demo gallery links both movies,
the editable assembly and both printing files.
