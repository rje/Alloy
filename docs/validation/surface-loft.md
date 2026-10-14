# Guided open-surface Loft candidate

SRF-009 adds one retained open arc, one retained open line, and one fit-spline
centerline to the shared Loft command. New Component creates a root child
component containing one valid open sheet face, with four stable builder-derived
boundary IDs. The centerline and sections are resolved from evaluated sketch
entities. A source edit rebuilds the sheet under the same body, face, edge and
component identities. The existing circular solid Loft remains available.

The bounded source and orientation policy are recorded outside the repository
in `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/thicken-loft-design.md`.
The chosen centerline transport is corrected Frenet with midpoint placement
and endpoint-normal section planes. The public material does not establish
those exact construction defaults. This increment supports the shoehorn-sized
arc/line sections and open fit spline; broader section, guide and rail modes
remain outside SRF-009. A curved Loft face is available for exact face picking
and later Thicken, while planar sketch attachment correctly refuses it.

`surface_loft` and the existing `solid_loft` core tests pass. The former checks
one valid face, four retained boundaries, zero solids, New Component ownership,
construction-curve refusal, source edit with stable identity, undo/redo,
save/reopen, and atomic missing-guide refusal. The 36-call real-app API ledger
and inspected preview/commit/reopen captures are in
`../Alloy-reference-records/deliverables/shoehorn-surface-2026-10-14/app-01/`.
It previews and cancels without authoring, then commits the sheet; editing the
middle fit point changes area from 5600.469 to 5701.722 mm² while preserving
the retained IDs. The Qt-native test log and capture are in
`../Alloy-reference-records/deliverables/shoehorn-surface-2026-10-14/native-01/`.
It API-selects New Component to open the dialog, uses actual Qt mouse clicks
for arc, line and centerline selection, inspects the preview, and cancels. A
separate programmatic combo change checks its handler; the mouse operation
chooser is not claimed as verified native input. Qt-delivered clicks are not
physical pointer evidence.

SRF-010 uses this sheet as a distinct retained surface source and
creates a separate closed solid while hiding the original sheet. Open-surface STL/STEP export is not claimed. The
completed shoehorn workflow and physical shape remain cumulative gates.

Astra independently accepted `f268d92` + `a089fb8` on2026-10-14, integrated
as `53043e4` + `0870d69`. Native operation chooser remains unverified.
