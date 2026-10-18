# Bracket STEP and STL delivery

Recorded 2026-10-12 on the merged exchange/tutorial candidate in
`../Alloy-worktrees/exchange-finish`. The source is the saved, complete
illustrated bracket from the real-app tutorial API chain. This delivery does
not change its authored dimensions or history. The source, full-size STEP and
STL, scaled print sample STL, and machine-readable verification are outside
the repository at
`../Alloy-reference-records/deliverables/illustrated-bracket-2026-10-12/`.
`tests/bracket_exchange_api.py` reproduces the exports and checks.

The app reopened the saved design and reported one valid, closed OCCT solid
with five Cut source profiles, bounds 918.198 × 438.909 × 50 mm and volume
4,694,718.448 mm³. It exported one explicitly selected body in millimeters
through the shared `document.exportStep` and `document.exportStl` commands;
the inspected authored document was identical before and after export.
An independent OCCT STEP reader found one valid solid, file unit millimetre,
the same bounds within 0.0001 mm and volume within 0.000001 mm³.

The binary STL contains 3,636 triangles. A separate stdlib reader checked
triangle count, nondegenerate facets, finite unit normals aligned with triangle
winding, paired oppositely directed edges, positive signed volume and bounds.
Its volume is 4,694,743.175 mm³, about 0.00053% above the analytic solid at
0.05 mm surface deviation and 5° normal deviation. The exact 0.2-scale
derivative preserves the topology and normals; its bounds are
183.640 × 87.782 × 10.000 mm and signed volume 37,557.946 mm³, 0.008 of
the full-size STL within floating-point output precision. STL carries no unit
metadata, so the filenames and accompanying deliverable note explicitly say
to import both STL files as millimeters.

The sample's 183.640 mm span exceeds a strict 180 mm bed before brim, skirt or
clearance. Printer fit and actual slice/G-code remain dependent on a chosen
printer, slicer and material profile; no such profile or slicer was available
in the local environment. The STL is the slicer-ready mesh artifact, while the
full-size `.alloy`, STEP and STL retain the authored size.

The focused `step_exchange` native test, real-app independent STEP import and
export test, and real-app STL unit/refinement/manifold test pass on this
candidate. The prior native STEP export sheet and millimeter save passed
independent readback. The earlier native default STL Save As Mesh sheet/save also passed;
`exchange-finish/build-step/evidence/stl-native-receipt.json` records its
12-triangle mesh, bounds and normals. A later [native-sheet check](native-exchange-sheet.md)
qualifies STEP File Open, inch selection, nondefault mesh settings and actual
Save/Cancel, with independently checked outputs. Export sheet activation in
that run used Alloy's local control; physical File/Browser menu entry remains
under DOC-035/#516. The shared command paths and format outputs above are verified.
The exact physical tutorial and multi-platform product gates remain open.

The integrated main candidate a840d36 built successfully and repeated the
saved-bracket export/readback check in the external `integrated-check/` folder.
Its STEP/STL bounds and mesh checks pass. No full geometry rerun was needed for
the UI-only changes. Remaining native file-dialog checks are tracked in #167.

## User slicer verification, 2026-10-12

The user loaded the delivered STL sample into their slicer and provided a
screenshot showing the intact five-hole bracket on a bed labeled Original Prusa
MK3.5. They then confirmed: "yep, slice looks great". This records successful
user-observed import and slicing/layer-preview review. Slicer version, material
and slicing settings were not supplied; physical printing is not yet reported.
The screenshot is retained with the external deliverables, outside Git.
