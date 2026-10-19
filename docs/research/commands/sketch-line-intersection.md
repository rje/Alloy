# Finite straight-line intersection inference

SKT-164/#610, bounded child of SKT-021/#162. External evidence and the dated
implementation contract are in
`../Alloy-reference-records/knowledge/specs/sketch-intersection-2026-10-19/contract.md`.
The public snap/constraint contract supports retained logical incidence;
exact sketch intersection cue styling and priority are unknown.

While ordinary Line waits for either endpoint, approaching a unique finite
crossing of two visible authored straight lines offers a square-cross candidate.
Acquisition uses the existing 10 logical-pixel radius and 0.5-pixel ambiguity
band, both Alloy choices. Existing authored point/origin targets at the same
position take precedence. Distinct line pairs at an indistinguishable crossing
are ambiguous. Parallel, coincident, collapsed, missing and outside-span pairs
produce no candidate. Construction lines are supported when visible. Linked
projection intersections and other curve families are outside this slice.

Pointer hover is disposable. Click retains one `intersection` constraint with
one endpoint `pointIds` entry and two source-line `entityIds`; two point-on-line
solver equations preserve the relationship through source edits. Finite-span
and nonparallel post-solve checks refuse invalid edits atomically. Escape/tool
switch clears an unfinished gesture. Constraint badge selection and Delete
remove the whole relation; Undo restores it.

`sketch.line` accepts explicit `startSnapTarget`/`endSnapTarget` objects
`{kind: "intersection", entityIds: [lineUUID, lineUUID]}` through the same
geometry authoring path as native pointer placement. Explicit unsnapped
coordinates remain exact. No additional shortcut or intersection command is
introduced. Existing Line length/angle controls remain separate contracts.

Verify a visible candidate and unchanged hover/cancel draft; inspect both stable
source IDs after click; edit each source; inspect exact analytic crossing,
closed profile, downstream Extrude, failed finite/parallel edit, Undo/Redo and
save/reopen. Inspect GPU cue/badge independently of geometry and authored state.
See [the validation receipt](../../validation/sketch-line-intersection.md).
