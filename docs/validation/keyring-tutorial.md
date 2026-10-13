# Initials keyring independent acceptance (#233)

Accepted on 2026-10-13. This review covers the successful complete shared-API
run using main application code through `bd25380` and runner `a50a000`, plus
accepted native ingredient evidence from #227–232, #234 and #236. Public source
ID: `warwick-initials-keyring-20261013`. Creator, URLs, six page images and exact
source uncertainties remain in the external reference records.

Independent review inspected the source contract and final runner changes,
203-command successful report, authored model, stage captures, representative
beginning/middle/end movie frames, and a separate strict STL readback. No full
application replay was needed. Earlier failed attempts are retained externally
and are not passed workflow evidence; the final run alone supplies this gate.

## Demonstrated modeling sequence

The run starts blank with command-first two-point Rectangle, XY origin and
60×30 typed fields. Four successive adjacent-side fillets create actual R6
quarter arcs while preserving the original four line IDs and visible overall
(60)/(30) reference dimensions. Their conversion from driving dimensions and
release of the corner-origin anchor are disclosed by the accepted #228 path;
the source's post-fillet driving semantics remain unknown. Driving overall-size
and origin preservation remain #235, not a hidden claim of this lesson.

The rounded profile extrudes 2 mm. A new sketch on its actual named z=2 cap
retains the full boundary and checked-chain inward offset −1.5 mm, producing
one annulus with inner dimensions 57×27 and R4.5 corners. That annulus joins
1.5 mm above the floor. Explicit face-sketch creation resolves a transition
left implicit in the public lesson without substituting geometry.

The two-point Circle uses first diameter endpoint (5,−15), +X direction and
typed Ø4; its center is (7,−15), subject to documented pointer quantization.
The written Ø4 takes precedence over a later apparent Ø5 illustration. Exact
placement/direction and the anchored-endpoint representation are explicit
Alloy choices. A −10 mm Cut passes through the floor. Selection of the retained
upper circular rim and R1 solid Fillet preserves the original broad-floor
support for text; all intervening support checks use the exact retained triple.

Text uses the permitted personalized DSB example with pinned DejaVu Sans Bold,
14 mm cap height. Shared move and rotation handle phases move the baseline near
(55,−5) and rotate 179.3684 degrees. This explicitly differs from an exact 180
degree numeric entry. Evaluated text bounds x=11.9732..53.3117 and
y=−19.1013..−4.5728 mm clear the rounded bore and lie inside the floor strip.
D/B holes survive; all three text profile slots join 1.5 mm and the document
is saved. The fixed bundled font is an allowed example choice, not exact
reference-font or default-font parity.

## Geometry, presentation and persistence

The authored save exactly equals the successful report's source state and both
recorded model/STL SHA-256 values independently match. Nine retained features
produce one valid closed solid with bounds (0,−30,0)..(60,0,3.5) mm. Analytic
rounded area is 1769.0973355292 mm², annulus area 247.4800842940 mm², and the
rimmed body volume 3909.4147974995 mm³. The bore removes 8π mm³; Fillet reduces
volume from 3884.2820562708 to 3881.2841045021 mm³. The final text Join gives
4391.1521015717 mm³, agreeing with the summed glyph-area addition and its
independently checked clearance.

Independent unchanged strict-reader inspection found 26,398 finite, consistently
oriented facets, unit aligned normals and one closed connected component.
STL bounds are exact to the stated fixture; volume is 4390.9369218335 mm³,
about 0.00490% below CAD volume. The same-slot DSB→DS8 edit preserves profile
and body identities; exact undo/redo and original save/reopen pass in the
post-recording integrity tail. These are actual retained-model/export checks,
not conclusions drawn only from pixels.

Actual captures show the continuous raised rim, rounded open bore, separated
raised letters and their holes. The completed solid is fully framed. Overall
reference/radius annotations and the corrected tangent symbols are readable.
The integrated Circle capture has separated palette summary/profile text,
closing the earlier timing-sensitive overlap concern in this full workflow.
The text dialog and move/rotation presentation are visible; no new blocking
visual defect was found in the inspected states.

## Evidence and limits

External deliverables:
`../Alloy-reference-records/deliverables/initials-keyring-2026-10-13/`
contains `keyring-tutorial-api.json`, `initials-keyring.alloy`,
`initials-keyring-mm.stl` and stage PNG/JSON evidence. The movie is
`../Alloy-reference-records/demos/2026-10-13/initials-keyring/initials-keyring-full.mp4`.
Independent media inspection confirms 1120×902, 638 frames, 63.8 seconds;
the report records zero capture failures. Representative frames cover rectangle,
fillets, rim/circle, hole Fillet, text handles and the completed body. Repeated
frames and recorded holds are not a frame-rate or response-latency benchmark.

The movie is shared API operation, not uninterrupted physical keyboard/mouse
input. Native child tests establish their separately documented Qt-dispatched
entry, focus, selection, preview and cancellation actions; their scope is not
expanded by the recording. Earlier projection/framing/startup runner failures
were corrected before this successful full run, rather than counted as partial
passes. Exact reference styling, unspecified source coordinates/solver semantics,
general fonts/offsets and cross-platform runtime remain outside this acceptance.
Repository privacy does not change the external clean-room reference boundary.
