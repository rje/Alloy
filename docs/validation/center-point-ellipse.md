# Center Point Ellipse candidate (#256, 2026-10-13)

The independent public lesson contract is held outside this repository at
`../Alloy-reference-records/knowledge/tutorials/after-countersunk-plate/ellipse-contract.md`
and `independent-contract.md`. The documented tool picks a center, first-axis
point, then a point on the ellipse contour. The source finishes an origin-centered,
horizontal ellipse with full axes 275 × 100 mm. The lesson omits some constraint
actions, so this candidate makes origin Coincident, Horizontal, and two full-axis
dimensions explicit. It does not claim undocumented initial defaults.

`sketch.centerPointEllipse` and the native Sketch > Create action share one
authority. The authored ellipse has a stable entity ID and two retained full
construction axes. Two Midpoint relations and one Perpendicular relation keep
their center, directions, and lengths live in the existing sketch solver. The
third contour pick determines the initial second semiaxis analytically; it is
not stored as a false fit point. Origin snapping adds a retained Coincident
relation. Native typed entry accepts a labeled full first-axis length, Tab
angle, and signed full second-axis length. These typed values initialize
geometry; they do not silently create driving dimensions. Associative snapping
of the contour pick is not claimed.

OCCT evaluates an analytic ellipse edge and closed selectable profile; equality
of semiaxes uses an analytic circle edge. Crossing first/second axis lengths
canonicalizes the evaluated major axis while preserving authored axis and
ellipse identities. Zero/underdetermined contour input refuses without changing
the draft. Direct dimensions on the ellipse curve are refused; dimensions on
the retained construction axes provide the two driving lengths.

Focused core passed: origin-snapped rough state has three DOF, Horizontal
reduces to two, D275 to one, and D100 to zero. The final profile area is
`6875π mm²`. Editing the first full axis through equality at 100 mm and across
to 80 mm preserves IDs and yields the expected analytic circle/ellipse areas.
Undo/redo and save/reopen preserve exact authored data. A separate oblique
contour calculation and invalid near-axis refusal passed; an open neighboring
line does not erase the ellipse region. `build-ellipse/center_point_ellipse_test`
reported `Center point ellipse core passed` after the final app build.

The focused native Qt test passed in 7.71 s. It used one initial harness
activation, actual tool action, pointer picks, typed fields, Tab/Return,
Horizontal, native D line picks and label placements, and Escape. The typed,
disposable analytic preview, and fully defined captures were inspected at
`build-ellipse/evidence/native-01/`; the final image is smooth and the 275/100
labels and palette state are readable. The private editor exited. The test
performs two short sketches, one typed and canceled and one rough-to-defined;
it does not represent the full multi-page lesson. The native typed convention
is Alloy's explicit full-axis label, not an asserted historical shortcut.

The built macOS application binary SHA-256 is
`d03006f4d74a1480c222dc7afd7f5994f2a4e96a87bc4925ecb552efe942aae6`.
The separate real-app shared-API run passed 35 calls using this same binary.
Its report, transcript, authored save and settled GPU capture are at
`../Alloy-reference-records/deliverables/center-point-ellipse-2026-10-13/api-attempt-01`.
It verifies the 3 → 2 → 1 → 0 DOF source sequence, one analytic profile and
275 × 100 mm bounds, an 80 × 100 mm crossing edit with stable profile/ellipse
IDs, exact undo/redo, and save/reopen. API input is distinct from the native
gesture test. Independent candidate review is pending.
