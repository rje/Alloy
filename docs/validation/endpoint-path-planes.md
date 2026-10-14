# Retained endpoint Plane Along Path candidate

GEO-003 adds a Plane Along Path command for one earlier, visible fit-point
spline and proportional Start (0) or End (1). A construction-plane feature
retains the source sketch/entity IDs and endpoint, sharing the existing plane
evaluation, Browser, sketch-support, history and storage authority. The plane
origin and normal come from the **evaluated** spline edge at the selected
ordered fit endpoint. Its in-plane U axis follows the source sketch normal;
V completes a right-handed frame. That axis roll and signed-normal convention
are Alloy choices because the public source does not specify them. Chains,
interior/physical/To Object extents, model edges and Resize remain outside this
child. The public source and source-versus-inference record are external at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/endpoint-path-plane-contract.md`.

The app and native target build. The focused core `path_endpoint_plane` test
passes a 130 mm YZ fit-spline spine, two endpoint frames, an attached end
sketch, undo/redo, save/reopen and atomic unavailable-source refusal. It also
edits a driving span from 50 to 65 mm without rewriting authored fit-point
coordinates, then checks the plane origin and normal against the actual solved
OCCT edge and the dependent sketch frame.

The 38-call real-app API ledger in
`../Alloy-reference-records/deliverables/endpoint-path-planes-2026-10-13/api-02/`
covers Start/End creation, frame orthogonality, source point edit, dependent
sketch edit, plane edit, undo/redo, atomic stale reference and save/reopen.
The final Qt-native check in
`../Alloy-reference-records/deliverables/endpoint-path-planes-2026-10-13/native-05/`
uses the real Construct command button, clicks the finished spline in the
canvas, opens the position combo and chooses End with delivered keys, then
previews, cancels and accepts through the native dock. The inspected isometric
capture shows the spline, filled perpendicular preview plane and readable
controls. API source edits were run before the solved-edge refinement; the
dimension-constrained core assertion covers that refinement. Native input is
Qt-delivered, with no claim of OS-level pointer parity or complete surface
tutorial acceptance.
