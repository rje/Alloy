# Retained finite line intersection (SKT-164)

An ordinary Line endpoint can acquire the finite crossing of two visible,
nonparallel authored straight sketch lines. The square-cross candidate commits
one stable `intersection` relation naming both sources and the authored endpoint.
Source edits solve the endpoint against both lines; finite-span/nonparallel
checks refuse unsupported changes transactionally. The same authoring path
serves native pointer input and explicit API snap targets. The badge is
selectable and deleting it removes the whole retained relationship.

The [compact workflow contract](../research/commands/sketch-line-intersection.md)
links the external dated public evidence. The 10-pixel acquisition radius,
0.5-pixel ambiguity band, square-cross cue and precedence policy are Alloy
choices. No matched public sketch-intersection capture establishes exact glyph
or styling parity. Existing point/origin targets at an identical position win;
three distinct source pairs at one crossing are ambiguous. Batch candidate
geometry builds the point map once and enumerates line pairs; large-sketch
pointer performance has not been measured.

Build: `build-preview-release`, Release, private Qt 6.11.2 runtime, current
macOS 26.5 SDK, targeted `alloy`, `sketch_pointer_intersection_test`,
`sketch_pointer_intersection_controls_test` and
`sketch_pointer_axis_inference_test` builds with `-j4`.

Core validation checks exact analytic crossing, a retained pair, source-seed
following, same/parallel/coincident/outside-span omission, stale atomic refusal,
and the existing H/V inference regression. API validation uses two fixed
construction sources and two fixed plate corners. A pointer-authored triangle
has 400 mm² area and extrudes 5 mm to 2000 mm³. Moving the horizontal source
from y=10 to y=15 drives the endpoint and solid to 500 mm² / 2500 mm³. Moving
the vertical source from x=10 to x=14 drives the same endpoint to (14,15) while
preserving the expected volume. The stable relation is unchanged; finite-span
and parallel source edits refuse with exact authored and undo preservation.
Hover/cancel, hidden construction exclusion, three-line ambiguity, Undo/Redo
and exact save/reopen are separately asserted.

Focused native Qt input activates the harness once, then uses the real viewport
receiver without intermediate focus repair: hover/cancel, start click, crossing
hover/click, Escape, badge hit click, Delete and exact draft Undo. It saves the
native sketch. This is synthesized Qt input, not physical-device qualification.

Actual application evidence and saved models are outside the repository:
`../Alloy-reference-records/validation/sketch-intersection-2026-10-19/`.
Final API evidence is `api-04/receipt.json`; native evidence is
`native-02/native-intersection.alloy` plus candidate/retained captures. GPU
captures are inspected separately from solver/authored evidence. The tested
renderer is Apple M4 / OpenGL 4.1 Metal; Windows/Linux runtime, packaging,
physical mouse input, large sketches, linked projections and curved
intersection families are outside this bounded acceptance. Parent SKT-021/#162
retains its other inference and numeric-input scope.

Validation result: focused core and H/V regression checks passed (2/2); native
Qt check passed (3/3); final actual-app API workflow passed. Candidate and
retained badge captures were visually inspected at their recorded scale.
All private validation windows exited after their runs.

Independent Astra review of the final solver/authoring diff, 84-call API run, native receipt and GPU captures found no blocker in this bounded line-line slice.
