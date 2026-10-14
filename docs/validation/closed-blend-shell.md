# Closed Shell through a complete rounded rectangular prism

GEO-008/#316 independently accepted and integrated, 2026-10-13.

The bounded geometry path recognizes either an authored Box or an XY rectangular
sketch followed by positive New Body Extrude, then exactly one grouped Fillet of
all four original vertical edges at radius R and all eight end/base rims at
radius r. No body modifier may intervene. The evaluated 26 oriented outer
patches must bijectively match a canonical convex rounded prism by analytic
plane/cylinder/torus support, full line/circle trims, quarter-domain pcurves,
shared adjacency and convex corner positions. The original outer faces remain
unchanged. For r <= wall < R in the nondegenerate domain, a rounded-prism core
produces the exact inward cavity; other histories and out-of-domain values
continue through the existing closed Shell evaluator. This path never substitutes a display mesh or sampled
Boolean volume for the analytic boundary certificate.

The corrected public source stock is a centered 93 × 64 mm rectangle extruded
30 mm; its axis assignment is qualified by the source record. With four
vertical R5 edges, eight rim R2.5 edges and a 3.5 mm closed Shell, the actual
cavity volume is 112701.57741982327 mm³ and material volume is
64404.74388444808 mm³. Six inner planar faces and shared inner edge identities
remain selectable. A dependent retained Midplane splits it into two valid
solids of 32202.371942224 mm³ each. Thickness editing, undo/redo, save/reopen,
impossible-thickness atomic refusal and translated/resized Box replay pass the focused
core test. The Box50 × 80 × 40 fixture is an engineering regression, not source
stock.

`tests/closed_blend_shell_api.py` drives the source-shaped stock through the
actual app, preview/Cancel, Shell, Midplane Split, two separately checked mm
STL files and reopen. The focused native Qt test clicks the Shell toolbar,
selects the actual body in Browser, types 3.5 mm and cancels after a valid
preview. External captures, authored receipt and STL inspections are under
`../Alloy-reference-records/deliverables/closed-blend-shell-2026-10-13/api-01`.
The section capture has active blue body selection; its cavity is visible.

The source research and mathematical gate are recorded outside Git under
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/`.
Exact creator asset, physical fit, and broader arbitrary blended-shell offsets
are outside this candidate. TUT-016 owns the later cumulative enclosure gate.

Independent review accepts the final analytic certificate and domain fallthrough.
The reviewer independently reran the focused core and checked both exported STL
parts. Native typing asserts real click-produced focus; Qt input remains distinct
from physical OS input. Main app builds and its focused Shell integration check passes.
