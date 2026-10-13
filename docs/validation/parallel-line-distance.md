# Parallel-line D candidate (#239)

Source contract: external `knowledge/tutorials/wall-outlet-audit/independent-contract.md`, section C, and issue #239. The source demonstrates separate D picks of parallel cutout edges and a construction axis, placed labels, and typed 11 and 28 mm. Alloy's ordered first-line/second-line signed side is an explicit implementation choice. The complete outlet drawing is a later cumulative gate.

The candidate adds `parallelLineDistance` to the existing dimension command and shared native D route. Two authored straight line UUIDs are retained, with signed side, a positive perpendicular distance and a separate label annotation. The GCS relation removes one separation degree of freedom. Both endpoints are checked after solve for parallelism; orientation must come from existing authored sketch relations, never an implicit Parallel relation. Centerline Diameter retains its precedence for its eligible pair, while nonparallel ordinary lines route to Angle. A repeated D placement for the same pair opens the existing dimension ID; direct duplicate creation refuses with an edit instruction. Incompatible pairs and solver conflicts leave the draft unchanged.

Validation: `parallel_line_distance` passed: independent 11 and 28 mm distances, two DOF removed, edits to 14 and 30, ordered UUID/side retention, reversed duplicate and nonparallel atomic refusals, an edit refusal when initially parallel but unrestrained lines lose orientation, undo/redo and exact authored reopen. Existing `sketch_centerline` and `point_edge_dimensions` core regressions passed. `parallel_line_controls` passed: native D, two actual line picks, separate placement and focused typing for 11 and 28, two retained labels with two witnesses each. The native capture shows readable 11 and 28 labels and perpendicular witnesses in Alloy's GPU viewport. Its three open lines deliberately have no closed profile, so the palette's open-loop warning is fixture-specific. Core test and native run are separate evidence; no full outlet replay or exact source-pixel match is claimed.

App and both test targets compile on macOS AppleClang/Qt 6.11.2/OCCT 7.9.3. Final native log: external `validation/parallel-line-distance-2026-10-13/native-settled-ctest.log` (1/1 passed, 7.00 s); capture: `parallel-line-d-11-28-settled.png` beside it. The GUI test canceled its draft and closed its window. The prior fixture timeout was its unsaved-close confirmation dialog, confirmed by a native stack sample and corrected in the fixture.

The original immediate `parallel-line-d-11-28.png` remains as unsettled evidence: its palette profile warning overlaps the info text. The final fixture processes queued Qt events, waits 80 ms, then measures and captures the same final state. At the actual 274 px label width, info is y=47..92 and profile moves from y=68 before settlement to y=98 after, leaving a 6 px gap; height-for-width and actual heights agree at 45/60 px. `parallel-line-palette-layout.json` retains those measurements. The settled capture shows both palette blocks and dimension labels readable, so no production layout change was needed for this candidate.

## Independent acceptance

Accepted `439065b` atop `f711bae` on 2026-10-13 after independent source,
focused test and actual capture review. The relation retains the ordered line
UUIDs and signed side, contributes one perpendicular-distance equation, and
checks both endpoint offsets after solving. It does not silently add Parallel.
Existing Centerline Diameter routing retains precedence; nonparallel ordinary
pairs retain Angle routing. Source inspection and the focused core evidence
cover edits, duplicate/nonparallel refusals, loss-of-orientation refusal,
unchanged authored state on failure and exact history/reopen. Broader tilted
line input and physical OS events are not claimed from the horizontal fixture.

The native fixture starts from shared-command-authored lines and constraints,
then sets initial viewport focus before the demonstrated D sequence. Actual Qt
line picks, separate label placement, typed 11/28 and Return use command-produced
editor focus without intervening focus repair. The settled follow-up passes its
single CTest in 7.00 seconds. This is Qt-dispatched widget evidence, separate
from the reported three focused core/regression passes and the future full
outlet gate.

The first screenshot's palette overlap was real capture timing, not an expected
consequence of open geometry. The inspected settled image now has readable
11/28 dimensions, two witnesses per label, DOF feedback and the fixture's
open-profile warning. Persisted metrics show the profile label moving from y68
to y98 after queued layout, below the info label's y47+45 extent. The correction
only synchronizes capture and adds the layout assertion; no production layout
fix or focus manipulation is attributed to it. Original evidence is retained.
No extra editor run or full tutorial replay was needed for this review.
