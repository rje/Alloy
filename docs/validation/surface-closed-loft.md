# Three closed sections, no-rail surface Loft

SRF-019/#451 adds an explicit `guideType: "none"` retained Loft with ordered
section references. The document stores sketch and entity IDs, not a BRep or
display mesh. The current evaluator admits exactly three distinct root sketches:
two ordinary fit splines closing the first section, one full ellipse, and two
ordinary fit splines closing the last. It creates one valid open two-face sheet
in a new component, without caps or a solid. Other section arrangements refuse
explicitly. Absent `guideType` keeps the earlier Centerline behavior; `rail`
keeps the earlier rail behavior. The new schema is structurally ordered for
future expansion, but this receipt does not claim broader geometry admission.

The focused core test passes on the independently authored three-sketch fixture.
It verifies source visibility can be turned off after Loft, then edits only the
**middle fit point of the first section's first spline** from 45 to 48 mm.
Area changes from 9041.5497 to 9059.2325 mm² while the two face IDs and six
edge IDs remain stable. Undo/redo and exact save/reopen retain those
IDs and the edited source. An invalid retained last-section entity refuses
atomically; unknown guide types are rejected by source-reviewed validation. The separate
external OCCT probe edited a larger first-section bulge; its area result is not
the result of this retained fixture edit.

The real application API run passed 36/36 calls with no failed command, from
document open through selected-input preview, Cancel invariance, commit,
hidden-source edit, history, reopen and Loft edit-dialog restoration. The
selected inputs and camera setup in this run were supplied through the shared
API. Its preview capture shows the open sheet and 2/1/2 section counts. The
largest recorded API call was about 635 ms. Source and API evidence were
independently reviewed before the native test.

The focused Qt run passed 1/1 in 3.60 seconds on the normal application code
at a **1040×640 logical-pixel** window. API setup opened the authored fixture
and Loft dialog; an actual focused popup keyboard route chose `None`. Actual
hit-owned Section controls and viewport clicks selected all five curves.
The native run then exercised actual Cancel, reopened a preset valid session
through the API, and clicked the actual OK button to commit and save. The
minimum-size captures show a readable two-face GPU preview, 2/1/2 selections,
and fixed OK/Cancel footer. The lower guidance capture used programmatic
scrollbar positioning as presentation setup; it is not evidence for physical
wheel scrolling. The dock measured 294×349 logical pixels within the 1040×640
window, with a 280-pixel scroll area and a visible fixed footer.

Initial native attempts remain preserved. A direct combo row click hit Qt's
popup-release guard; naturally focused keyboard choice resolved it. The first
picker route needed the queued mode-specific dock rebuild to finish before
using its controls. An ellipse point coincident with a construction axis was
replaced by an off-axis point on the same displayed ellipse. A combined width/
height assertion then rejected the correctly sized 294-pixel dock because the
test expected at least 300 pixels. The old unscrolled form visibly clipped its
footer; the bounded scroll-form/fixed-footer correction and separate geometry
assertions produced the final passing run. No failed route is counted as native
acceptance.

Lineage admission first matches original OCCT curve handles, locations and
finite parameter ranges through wire assembly. Where the kernel replaces an
output curve, unique correspondence uses length plus nine samples in each
direction; 33 samples per section edge check output agreement. These are
bounded sampled checks, not a whole-curve identity proof. Seam IDs use the
authored first-spline endpoint ordering rather than wire traversal order.
Full source-exact surface-iron Trim-boundary contact, exterior quality, and
the cumulative TUT-018 workflow remain in SRF-015/SRF-017 and
their tutorial gate. This ingredient does not establish those results.

Reproducible fixtures, source probe, exact logs, screenshots and saved models
are outside the repository in
`../Alloy-reference-records/deliverables/surface-iron-handle-loft-2026-10-14/`:
`core-01/`, `api-01/`, and final `native-07/ctest.log` with
`minimum-three-section-preview.png`, `minimum-guidance-footer.png`, and
`native-three-section-sheet.alloy`. The earlier `native-01` through `native-06`
directories preserve failed-input and layout-diagnostic evidence.
