# Exact shoehorn corner diagnostic

2026-10-14. The exact retained R12/R16 grouped Fillet remains refused. The
current positive-material input is valid, closed and one solid, with volume
17,519.59912009738 mm³. No authored spline, radius, seed or tolerance changed.

The existing manual console target evaluated the saved Thicken fixture once.
Each of four retained corners forms one contour, and every endpoint has three
incident kernel edges. OCCT reports Build complete, all four stripes Ok and
no faulty contours or vertices, but returns an invalid closed solid. Two faces
report `BRepCheck_BadOrientationOfSubshape`. Thus a reported walking/start
failure or four-edge endpoint condition does not explain this result.

A saved-result inspection found those faces each contain one wire. Their face
wire-orientation checks fail; all wire closure, 2D closure, orientation and
self-intersection checks pass. The other ten faces pass the exact analyzer.
This does not establish finite patch containment or intended material.

One explicitly bounded `ShapeFix_Face::FixOrientation` experiment on the saved
output reversed both wires and retained their support surfaces and all 28 edge
identities. The whole result still fails exact validation. Its reported area
and bounds change, including minimum Z from approximately −6.007 to −25.732 mm.
Bounds of invalid geometry are not physical measurements; this result cannot
establish preserved finite patches or material. Own-status errors clear, but
this experiment did not record contextual statuses, so the precise remaining
contextual failure is unknown. No generic healer, shell-repair cascade or
production fallback was added.

Build and console evidence are separate from application acceptance. Existing
target builds passed; the exact operation and rejected repair returned exit 2.
The saved inspection returned 0 to indicate inspection completion, not valid
geometry. No editor or GPU workflow was needed for these failed candidates.

Evidence is external under
`deliverables/shoehorn-workflow-2026-10-14/geo019-grouped-diagnostic-01/`,
`geo019-saved-inspect-01/` and `geo019-orientation-only-01/`: exact source and
hashes, saved inputs/results, topology reports and failure receipts. The
original input hash and public-source uncertainty remain in the external
`shoehorn-fillet-radius-diagnostic.md` contract.

GEO-019 remains open. The next investigation must explain the generated finite
patches and shared-shell orientation from saved evidence, or qualify the
inferred source curvature against additional public evidence. Another radius,
mode or repair sweep is not justified. GEO-018 generated references and the
later perimeter round remain blocked; the complete tutorial is not accepted.
