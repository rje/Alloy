# Straight-line Extend checkpoint

SKT-090 adds a bounded Modify > Extend tool for an authored straight sketch
line. Hover previews a green segment from its nearer endpoint to the nearest
forward intersection with another finite authored straight line; click
commits one solved sketch edit. The source line and moving point IDs remain
unchanged. An exact target endpoint gains a Coincident relation; a supported
target interior gains pointOnCurve. Ties, overlaps winning the nearest hit,
missing intersections, unsupported sources and incompatible constraints refuse
without changing the draft. These endpoint/reference choices and tolerances
are Alloy policy; the public behavior and unknowns are recorded externally in
`../Alloy-reference-records/knowledge/specs/sketch-line-extend-child-2026-10-14.md`.

Focused core `sketch_extend` passes a 2 mm gap at a 20 × 10 mm corner. It checks
the unchanged draft on a no-hit attempt, a farther collinear segment behind
the unique nearest corner, source/endpoint/target IDs, the retained Coincident,
one exact 200 mm² profile, whole-sketch document undo/redo and save/reopen.

The production-theme native Qt gate `sketch_extend_controls` passes with API
fixture and Extend-tool setup, actual hit-owned viewport hover/click at the
right line, a visible green preview, unchanged draft during preview, Extend
remaining active after the click, and focused Enter exiting the tool. Shared
API finishes the sketch, extrudes its one profile 5 mm, verifies a closed
valid 1000 mm³ body, then
checks Extrude undo/redo and exact reopen. Draft-level Extend undo uses the
existing shared `publishSketchSolve` journal path and was source-reviewed,
not independently exercised in this gate. The focused native log, three GPU
captures and saved model are in
`../Alloy-reference-records/deliverables/sketch-line-extend-2026-10-14/native-03/`.
This is Qt-delivered native control evidence, not a physical OS input or a
from-blank tutorial recording. Only the stated line case is accepted;
SKT-011 retains other Extend, Break and curve-modification scope.
