# Future beginner-project workflow gates

Researched 2026-10-11. **Planned recipes only: no gate, application workflow or
geometry check was executed.** These are two original third-party lessons, not
aggregated video summaries. Source URLs, author details and exact section locators
are recorded outside the repository in
`../Alloy-reference-records/tutorial-gate-sources.json`. No account-gated files,
paid guides or reference application were accessed. No video was played, so no
video timestamps or frame observations are claimed.

## Candidate A: first toy block

Source `tutorial-toy-block-2026`: Kevin Kennedy / Product Design Online, original
2026 Day 1 article and complete author transcript. Exact publication date,
application build and platform are unknown. Evidence is documented instruction;
the expected appearance below follows those operations, not an inspected video.

**Documented sequence, millimeters:**

1. On XY, anchor a two-point rectangle at the origin; enter 15.8 × 31.8 using
   inline inputs/Tab. Finish; extrude 9.6. Save.
2. Sketch a diameter-5 circle on top; dimension its center 3.9 from each adjacent
   edge. Extrude 1.7, Join.
3. Pattern that feature: four along the length over extent 24; two across width
   over extent 8. These are total extents, not spacings.
4. Remove the bottom face with an inward shell thickness of 1.49.
5. Sketch on the inside face. Use a construction line between recessed-circle
   centers and its midpoint for a diameter-6.3 circle. Offset inward 1; extrude
   the annulus 8.1. Pattern three features over extent 16 along the length.
6. Apply radius-0.2 fillets to the demonstrated stud, outer block and underside
   tube-base edges. Save the finished object.

Before freezing a runnable fixture, inspect the original public demonstration
to resolve the precise underside snap pair and final edge set. The transcript's
deictic selections are insufficient for exact IDs. Do not silently guess them or
replace the tubes/pattern/shell with a supplied final BRep. Cloud project/folder
organization is explicitly adapted to local Alloy Save/Save As.

**Alloy checkpoint assertions (our derived checks):**

| Checkpoint | Required measurable output |
| --- | --- |
| Base | One valid closed solid; bounds 31.8 × 15.8 × 9.6 up to chosen axis ordering; volume 4823.424 mm³. Dimensioned authored sketch and extrusion remain editable. |
| Stud pattern | Eight joined studs; 8 mm center spacing in each pattern direction. Pre-shell volume equals base volume + 85π mm³. One body, not eight disconnected display copies. |
| Shell and tubes | Open-bottom cavity in a watertight material solid; 1.49 wall setting, three hollow underside tubes and retained feature dependencies. Check actual wall/annular sections and validity rather than guessing an analytic final volume. |
| Finished object | Rounded stud tops/outer edges/tube bases, with feature IDs recording the selected edge roles. Save/reopen preserves all operations; orthographic top/bottom and oblique captures expose both sides. |

An Alloy-only extension changes base thickness 9.6→10, recomputes, undoes/redoes,
then restores 9.6. That is a history assertion, not an extra step attributed to
the author. The full object is not accepted while any demonstrated modeling
operation or required selection remains unavailable.

## Candidate B: library first-project die

Source `tutorial-library-die`: Estes Valley Library's original lesson, section
“Our first project: making a die” through “Now, finish the last four faces,” plus
“Ready to print.” The raw authored Markdown was retrieved and hashed. Publication
date/build are unknown; wording and toolbar arrangement are historical. An
original linked image returned 403 and was not bypassed; visuals below are
expected from instructions, not a matched source capture.

**Documented sequence:** save a game-pieces document; create a named die
component; draw an XY center-point square with inline values/Tab; extrude it to
a cube. Preview Chamfer and Fillet on all edges, cancel to compare, then accept
one treatment. Select planar faces and create spherical cuts for the dots;
use Repeat Sphere and rotate through all six faces. Export the chosen component
as STL without sending it to a print utility. The lesson deliberately lets the
learner choose dimensions and does not prescribe every pip position.

**Deterministic Alloy fixture choices, not source dimensions:** use a centered
20 mm cube; preview a 1 mm equal-distance chamfer then cancel; accept a 1 mm
constant-radius fillet. Use diameter-3 spheres centered on each face plane,
cutting hemispherical pips. Define 1–6 pips on six faces with opposite counts
summing to seven, at a ±5 mm local grid and center as appropriate. Freeze the
actual local face axes and pip table in the eventual fixture.

Checkpoint expectations: initial volume 8000 mm³; cancelled chamfer leaves the
same geometry/history; accepted rounding preserves 20 mm bounds. Twenty-one
nonintersecting hemispherical recesses remove 47.25π mm³ from the filleted cube.
Inspect valid one-solid topology and individual face recess counts, not just
the final silhouette. Export a manifold STL at declared millimeter scale and
check its bounds against the solid. Save/reopen and editing the rounding feature
are Alloy extensions to the lesson. Printing or uploading to the library's
submission service is outside this CAD gate.

## Mapping to the existing backlog

Keep **SKT-015** unchanged: its 40 × 25 × 5 part and upstream width edit remain
the nearest useful sketch/extrude checkpoint. The toy-block base can later be
an additional named prefix fixture, but that prefix must never report full
tutorial completion. Neither full tutorial is ready at SKT-015.

Keep **GATE-020**'s independently specified mounting bracket and cumulative
requirements. These tutorials complement it rather than replacing its through
holes, recompute, export or all-platform checks. A beginner label does not imply
that every command fits GATE-020's present dependencies.

At most two additional validation children are justified; keys below are
proposals for the issue owner, not newly created issues or registered runner cases:

| Proposed child | Required existing capabilities | Additional admission blockers |
| --- | --- | --- |
| `TUT-001`: complete toy block from blank | SKT-015; SKT-005/006/008/009/010/011/013; SOL-002/003/006/009; DOC-006/009/010/012; HIS-002/004; FND-010 | Midpoint relation/snap, point-to-edge dimension and face-reference selection must have explicit implementation coverage. Resolve the two source selection ambiguities above before fixture freeze. |
| `TUT-002`: complete die and STL from blank | SKT-015; SKT-005; SOL-002/003/005; DOC-006/010/012; PRT-001; FND-010; ASM-001 for the demonstrated component step | Center-point rectangle and face-positioned Sphere/Cut with Repeat are not established by DOC-007's box/cylinder scope. Add/identify bounded implementation prerequisites before scheduling this child. |

Do not substitute cylinder holes for spherical pips, duplicate primitives for
feature patterns, or body-only organization for the demonstrated component and
then call the original tutorial complete. If an adapted early exercise is useful,
name its deviations and keep the full child blocked. The full toy block naturally
belongs near later shape breadth; the die also waits on chamfer and its missing
primitive interaction. This avoids delaying SKT-015 with unrelated prerequisites.

For concrete dependency wiring, propose **SOL-014: face-positioned sphere
primitive with Cut and Repeat**, depending on DOC-003/004/006/008/012 and SOL-002.
It must expose center, diameter, support-face identity, target body and operation
through shared UI/API commands, preserve face references and undo, and demonstrate
a hemispherical recess centered on a planar face. Add SOL-014 to TUT-002, and
explicitly extend SKT-005 to cover the center-point rectangle interaction first.
TUT-001 resolves source `tutorial-toy-block-2026`; TUT-002 resolves
`tutorial-library-die`. These are proposals only; no issue, dependency, workflow
registration or CI trigger was changed. Version/manual CI policy is unchanged.

## Execution and completion contract, when dependencies exist

Run each recipe from an empty authored document through the actual Alloy entrypoint
and shared command API. Record every source step as passed, explicitly adapted
(local storage), blocked or failed. An unsupported step makes the whole tutorial
incomplete; a checkpoint screenshot alone is not success. Use native input only
for physical behavior not established by the API: inline Tab entry, face/edge
picking, feature preview/Cancel, repeating a command and a timeline edit. All
required steps must also be available to a human through Alloy's UI.

Retain source/fixture version, executable and library hashes, actual commands,
authored save, geometry summaries and captures at checkpoints. Use fixed semantic
selections and explicit tolerances: propose 1e-6 mm for analytic dimensions and
relative 1e-7 volume agreement before tessellation; mesh bounds use the chosen
export chordal tolerance. These are proposed gate tolerances to review against
kernel policy, not measured pass results.

Include one save/reopen and one upstream edit/undo/redo chain; inspect real shape
validity, count, measurements and restored authored identities. Keep build, API,
geometry, persistence, export, physical input and visual results separate. A local
macOS tutorial pass does not close GATE-020's three-platform package requirements.
When incorporated into a cumulative gate, execute the already accepted capability
ledger once on that candidate; do not multiply unchanged full runs per tutorial.

Final visual comparison must use independently authored Alloy captures and the
permitted source view/scale where available. Until the original demonstration
frames are inspected, claim geometry/workflow correspondence only, not pixel or
exact visual correspondence. Reference assets and third-party lesson text are
not included in application assets or distributed example files.
