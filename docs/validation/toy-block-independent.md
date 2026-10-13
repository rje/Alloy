# Toy block independent review

Reviewed 2026-10-13. Accept the complete adapted API/artifact gate for #146 and
bounded Pattern #191, inner-roof references #196, Bottom view #198, display
meshing #199, and pending numeric input teardown #202. This does not establish
an uninterrupted OS-operated tutorial or exact interaction/visual parity.

The external TUT-001 full source ledger covers the 31.8 × 15.8 × 9.6 base,
Ø5 stud centered 3.9 from adjacent edges, Join 1.7, 4-by-2 feature pattern over
24/8 extents, underside Shell 1.49, and roof construction diagonal joining
first-row first and second-row second recessed centers. Its midpoint locates
Ø6.3, offset inward 1 to Ø4.3, annular Join 8.1, then three tubes over extent
16. The final radius 0.2 fillet includes exactly 19 edges: eight stud top rims,
four outer top edges, four outside vertical corners, and three tube free-end
outer rims. Inner tube rims and the outer bottom perimeter are excluded.
Source URLs, retrieval provenance and frames remain in the external tutorial
archive; the diagonal is resolved by frame 1011 and final edge sets by frames
1128–1152.

One adaptation remains explicit: the first stud's two 3.9 driving dimensions
use retained projected-corner coordinate components. They produce the required
model and update behavior, but are not the demonstrated center-to-model-edge
dimension gesture. That human interaction remains SKT-017/#150; do not describe
this gate as literal completion of every demonstrated native gesture.

The final actual-app ledger has 113 successful commands and ten saved features.
It checks eight studs, three hollow tubes, the 19-edge selection, completed-chain
stud-height edit 1.7→1.9, undo/redo/restoration and exact authored reopen.
Final BRep volume is 2452.356117 mm³. Independent strict STL readback found one
closed consistently oriented component, 36,644 triangles, finite unit normals
aligned with winding, bounds approximately 31.8 × 15.8 × 11.3 mm, and mesh volume
2451.548940 mm³. Export checks retain their original manifold criteria.

The verifier-authored Qt case `patternTimelineAxesAndRoofCenterSnaps` passes
(3/3 including fixture setup/cleanup, 4.211 seconds). API authors the initial
base/stud; actual timeline and viewport clicks select the feature and two axes.
Keyboard quantity/distance input with Enter produces eight instances, exact
Cancel and OK. Actual Browser Bottom click, roof-face click and Create Sketch
button expose eight distinct recessed-center candidates. Line, Construction,
and Circle menu actions plus viewport mouse events retain the two correct
center references and the diagonal midpoint; inline keyboard input creates a
nonconstruction Ø6.3 circle. These are Qt-dispatched events, not OS mouse input.
macOS did not grant test-window activation, so global L/C/X shortcuts were not
verified; menu actions were used instead. Synthetic Tab did not commit Pattern
distance in that inactive-window attempt; Enter succeeded.

The same test leaves edited Pattern numeric text pending at window destruction.
Before #202 it reproduced a crash: spinbox hide interpreted the text after
window members had been destroyed. The destructor signal guard now exits
normally. This is one focused teardown regression, not a broad shutdown audit.

Actual Pattern controls are readable at 1280 × 900 with blue selected axes and
neutral solid preview. Roof capture shows eight purple center marks, retained
Coincident endpoints and the Midpoint circle. Final top and underside captures
show smooth rounded studs, shaded roof/walls and three visible annuli. Compared
with the earlier top capture, the display coefficient 0.0005/angular deviation
0.10 retains acceptable silhouettes and highlights at this recorded scale.
No universal subpixel or all-zoom smoothness claim is made. The final runner
reports fillet 5.760 seconds and reopen 5.415 seconds; the prior 38-second stall
is removed for this fixture, while general latency remains #83.

The 48-second complete movie was sampled across creation, roof construction,
tube Join, final fillet and underside inspection. It labels the API commands;
it is complete creation evidence, not a physical mouse walkthrough.

Evidence: `build-tutorial-polish/evidence/toy-block-final/` contains the full
ledger, editable model, STL and six captures. Independent event log and captures
are in `build-tutorial-polish/evidence/toy-independent/` (`qt-input.txt`,
`qt-pattern.png`, `qt-roof-midpoint.png`, `movie-contact.png`). The movie is
external at `demos/2026-10-13/toy-block/toy-block-complete.mp4`.

## Updated retained edge-dimension gate

Independently accepted 2026-10-13 after SKT-034/#205 and HIS-005/#207. This new
run supersedes the first-stud coordinate-component adaptation for the current
deliverable; the original evidence and the historical qualification above remain
unchanged. The new evidence is `build-tutorial-polish/evidence/toy-block-point-edge/`
and the external movie is
`demos/2026-10-13/toy-block/toy-block-with-edge-dimensions.mp4`.

The reviewer inspected the saved ten-feature model. Stud contains diameter5
and two driving `pointLineDistance` dimensions of3.9, each retaining the same
circle-center ID and a projected straight-edge ID with an oriented side. The
center is (3.9,11.9), radius2.5; no coordinate-component replacement remains.
The actual Top capture `first-stud-edge-dimensions.png` shows both readable3.9
witnesses, Ø5 and Fully constrained. Separate independently accepted #205 Qt
evidence covers the direct D/center/edge/placement/Enter gesture; this full run
uses the same authoritative commands through the API.

The row pattern moves inward in world −Y by8, preserving the same eight stud
locations. Stored signed extents depend on each retained edge's direction and
must not be read as global coordinates. The roof construction endpoints remain
world (3.9,3.9,8.11) and (11.9,11.9,8.11), matching the public Bottom-view
first-row first to second-row second diagonal. Their distinct occurrence-aware
center references, coincident endpoints, midpoint relation and retained inward
circle offset remain in the file. Changing the stud seed did not require
reversing that diagonal. The sampled Bottom movie frame confirms its screen
orientation and subsequent three-tube result.

The new complete ledger passes128commands, completed-chain stud-height edit,
undo/redo/restoration and exact reopen. The60.8second movie reports608frames
and zero capture failures. Independent samples span base creation, the new
edge dimensions, roof diagonal, midpoint/offset, tube pattern and finished
underside. Captions identify API execution; this is not an uninterrupted OS
mouse walkthrough. The small overall model framing remains a presentation
limitation, without obscuring the dedicated dimension capture.

Independent strict readback of the new STL confirms one closed consistently
oriented component,36644triangles, finite unit normals aligned with winding,
31.8×15.8×11.3mm bounds and signed volume2451.5489396895mm³. The existing exact
19-edge fillet and full-source modeling scope remain accepted. No new GUI replay,
physical-input claim, exact pixel parity or broad parent-feature closure is
inferred from this update.
