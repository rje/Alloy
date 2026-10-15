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

## Finite-cap investigation, 2026-10-15

Independent saved-result inspection found cap pcurve/surface points depart
shared 3D edges by up to 0.355 mm, within inflated stored tolerances near
0.36 mm. Current edge usages oppose their neighboring faces; reversing a
wire alone breaks that relationship. Of 26 sampled points inside each sampled
UV polygon, five lie outside the original valid solid, up to 0.390 mm away.
Polygon membership is sampled, not a formal finite-domain proof.

Reconstructing a face from the same surface and wire cannot establish exact
boundary agreement or material preservation. No reconstruction, new Fillet run,
product fallback or geometry acceptance followed. This diagnoses the inferred
Alloy fixture and failed kernel output, not the public lesson's exact geometry.
The external `shoehorn-workflow-2026-10-15/geo019-finite-cap-01/` directory
retains the probe, report and manifest. Root reviewed the probe and Astra's
stopping decision. GEO-019 returns to Todo pending a materially different
kernel/input strategy; generated support and full tutorial finishing remain open.


## Independent-strip candidate, 2026-10-15

A materially different saved-input construction now produces one valid closed
solid of volume 17,033.6912694748 mm³ with all four R12/R16 corner supports.
Each adjacent pair of exactly extended ruled sidewalls generates a rolling-ball
strip independently. Disposable corner cutters close outside the source material;
a topology copy of the source shares the identical extended wall supports while
retaining its original trims. This avoids the failed terminal-cap construction
and a separately demonstrated coincident-support Boolean misclassification.

Saved-result inspection finds all four round supports, no auxiliary cap supports,
no material outside the normalized input and eight tangent sidewall contacts.
Sampled radius error is below 5.11e-7 mm and contact normal mismatch below
2.534e-7 radians. These numerical checks qualify the geometric experiment;
retained feature identity, edits, UI and downstream acceptance remain pending.
External `shoehorn-workflow-2026-10-15/geo019-strategy-01/` preserves the candidate,
construction rationale, failed alternatives and certification probes.
GEO-019 is active again for product integration of this bounded strategy.

The following exact R1.5 perimeter operation has also been preflighted once on
that candidate. Original/offset broad support identities identify two boundary
chains of nine and eleven edges. Both stripes report Ok, but the grouped build
fails with one faulty contour and no partial result. The saved boundaries include
small trim fragments; causality is not established. Evidence is external under
`geo018-perimeter-01/`. An independent top/bottom construction is being evaluated;
no radius reduction or generic healing is accepted.
