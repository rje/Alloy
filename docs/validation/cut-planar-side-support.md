# Planar side support from a straight Cut

A straight-profile Cut can now publish a planar prism side as a retained
support when the Boolean result has one unique current planar descendant and
all four straight boundary edges have unique current tool-edge descendants.
The face and edge IDs derive from the Cut feature and authored source
boundaries. The support frame takes its normal from the actual result face,
which can reverse the Cut tool face orientation. Ambiguous or incomplete
history leaves the side unavailable.

The focused core test constructs a closed pocket, verifies its two cavity
normal directions and four boundary IDs, sketches on one side, refuses an
invented support, and checks stable IDs after a dimensional edit, undo and
reopen. The existing open card-rim test also passes. The same core test opens
the accepted enclosure Cut and verifies its Y17 side can drive a retained
0.5 mm offset plane at Y17.5 and a sketch.

A real-app API run opened that Case, selected the exact Y17 support, created
the offset plane, projected three actual side boundary edges into a sketch,
then edited the plane, undid the edit, saved and reopened. Its command ledger,
authored model and capture are outside the repository in
`../Alloy-reference-records/deliverables/enclosure-snap-2026-10-13/geo013-api-01`.
The screenshot shows the projected-edge count; the small, partly occluded
lines are assessed by the authored projection IDs and current geometry, not
by visual position alone. Selecting the side face through a physical canvas
click and authoring the later stepped cantilever remain separate checks.
