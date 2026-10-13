# Rough pointer Center Rectangle candidate (#237)

The independent `wall-outlet-sketch-20261013` contract, section D, calls for an
origin-centered rough outer rectangle before separately dimensioning its top
and right edges to 70 × 114 mm. The source does not specify the rough seed
size. Its dated URLs, illustrations, and uncertainty ledger remain outside the
repository at `../Alloy-reference-records/knowledge/tutorials/wall-outlet-audit/independent-contract.md`.

Pointer center/corner placement now authors the existing four H/V outline
edges, two construction diagonals, retained center and center relation, with
no width/height dimensions. Width and height are the only two size degrees of
freedom. The pointer's rough coordinates are initial guesses, not hidden fixed
constraints. Native D on the top and right edges subsequently authors the
only two driving size dimensions. Direct `sketch.centerRectangle` and the
typed/locked inline width/height path still create their two driving dimensions;
old authored files are not rewritten or stripped of dimensions.

The focused core test passed: rough origin-centered placement has six edges,
zero dimensions and 2 DOF; adding horizontal 70 then vertical 114 gives 1 then
0 DOF and bounds x±35, y±57. It checks the analytic profile, undo/redo and exact
reopen, alongside unchanged legacy driving-size save data. The existing focused
typed-input widget test passed 3/3 in 2.027 s and retained both driving
dimensions; [its log](../../../Alloy-reference-records/deliverables/rough-center-rectangle-2026-10-13/typed-control-test.log)
is external.

The [native pointer/D log](../../../Alloy-reference-records/deliverables/rough-center-rectangle-2026-10-13/native-test.log)
passed 3/3 in 6.023 s. One uninterrupted Qt widget sequence selected Center
Rectangle, clicked the origin and rough corner, then used D on the top and
right edges with typed 70/114. Captured [rough](../../../Alloy-reference-records/deliverables/rough-center-rectangle-2026-10-13/rough-center-rectangle.png)
and [defined](../../../Alloy-reference-records/deliverables/rough-center-rectangle-2026-10-13/defined-center-rectangle.png)
GPU views show the blue two-DOF outline and black constrained outline with
70/114 annotations and legible construction diagonals. Draft undo/redo,
finish, save and exact authored reopen passed. The fixture deliberately used
65% zoom to keep the rough corner and dimension labels on-screen; its actual
rough corners were approximately (±33.949, ±55.099) mm. The final model,
end-state JSON and screenshots are in the same external deliverables directory.
OCCT OpenGL reported Apple M4, Apple vendor, GL `4.1 Metal - 90.5`, hardware
acceleration and a 1464 × 1142 framebuffer.

This matches the documented rough-to-dimension sequence for the bounded outer
rectangle. Exact rough seed pixels and the rest of the wall-outlet sketch gate
remain unverified here; no physical OS input or full tutorial claim is made.
