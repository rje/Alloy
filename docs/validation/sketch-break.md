# Analytic sketch Break candidate (SKT-169 / #617)

Date: 2026-10-20 UTC. Candidate pending independent review and integration.

Public source contract: stable reference SKT-169, external
`../Alloy-reference-records/knowledge/specs/skt169-break-2026-10-20.md`.
That dated record contains source provenance and separates documented behavior,
screenshot observations, inference and unknowns. No proprietary application was
operated. Exact screenshot parity is unknown; references are unmatched.

## Bounded command contract

Enter an editable sketch, choose **Modify > Break**, hover an authored analytic
curve and click that curve to split. Red X markers preview the exact crossing
locations while the full original curve remains visible. The tool stays active
for more picks; Enter ends it. Escape clears a pending preview and exits without
committing that pick. Both entry routes use `SketchGeometry::breakCurve`:

- `sketch.breakPreview {entityId,uMm,vMm}`: transient markers/diagnostics;
- `sketch.break {entityId,uMm,vMm}`: one atomic accepted draft edit;
- `sketch.tool {tool:"break"}`, pointer hover/click and native Modify controls
  invoke those same commands and authored document state.

A line or finite arc requires exactly one unique transverse interior crossing.
A circle requires exactly two distinct transverse crossings with one other
analytic curve. It becomes two counterclockwise analytic arcs whose sweeps sum
to 360°, preserving the complete locus. This coverage is an Alloy geometry
contract inferred from splitting, not a quoted reference guarantee. Circles
with zero/one/more crossings, tangencies, existing-endpoint intersections,
overlap and coincident crossing ownership refuse atomically. Grouped, derived
and linked owners refuse; sketches with linked curve boundaries or unsupported
nonanalytic boundaries refuse because this slice cannot prove a unique complete
intersection set. Construction lines/arcs retain their construction flag.

All unaffected entity/point IDs persist. A line/arc keeps its original ID on
the parameter-start piece; its new piece receives a new UUID. The two pieces
share a new split point. A circle keeps its original ID on the arc containing
its old zero-angle seam and assigns a new UUID to the other arc. The arcs share
the existing center; their two endpoint pairs have separate UUIDs at identical
intersection coordinates. Sharing both endpoint IDs makes the solver's radial
rules redundant, so the pairs remain independent. Break adds no intersection
constraints to the crossing boundary. Subsequent boundary edits therefore do
not retain a live split-intersection dependency.

The conservative annotation policy retires every constraint/dimension that
references the split entity, reporting explicit removed IDs and reasons.
Point-only and unrelated annotations remain, with explicit retained IDs.
No constraint fan-out or ambiguous remapping is attempted. A rejected solve
returns the original draft; a successful solve must preserve the candidate's
geometry. Preview generates disposable candidate UUIDs; committed UUIDs persist
through Undo/Redo and save/reopen. API results expose `breakDiagnostics` with
source/retained/created entity IDs, cut coordinates and annotation dispositions.

## Evidence

Accepted build directory: `build-preview-release`, CMake Release, private Qt
6.11.2 from `../Alloy-dependencies/qt-private-runtime/install`, native arm64.
Build uses `-j4`. No paid CI, tag, push or issue closure was requested or run.

External evidence root:
`../Alloy-reference-records/validation/sketch-break-2026-10-20/`.

- Focused `sketch_break` covers line/finite arc/circle analytic geometry, full
  circle sweep, original-ID preservation, shared line split point, explicit
  relation retirement, unchanged fixed-endpoint relation retention, absent crossing, tangent, endpoint and multiple-crossing
  atomic refusal. Adjacent `sketch_trim` and `sketch_extend` regressions pass.
- `tests/sketch_break_api.py` runs through the actual application/local server.
  It checks transient preview/history, explicit diameter retirement, two radius
  10 arcs with total sweep 360°, unchanged crossing line, atomic refused repeat,
  draft/document Undo/Redo, an independent secant-endpoint edit with unchanged
  arcs/relations, total profile area 100π mm² and authored save/reopen.
  `api/report.json`, `api/transcript.json`, captures and `circle-break.alloy`
  retain the actual fixture/results.
- `sketch_break_controls_test` delivers Qt input to actual-hit native controls.
  It observes Modify popup activation/closure, circle hover markers, Escape before
  commit, circle click, repeated line and finite arc hover/click, Enter finish,
  Undo/Redo and reopen. Initial harness activation is separate; no focus or
  selection repair occurs between modeled user steps. Native captures show
  correct circular/arc crossing markers and complete source-curve presentation.
  Evidence lives in `native/`, including `circle-break.alloy`.
- Hardware evidence is distinct: actual app reports Apple / Apple M4,
  OpenGL `4.1 Metal - 90.5`, hardware acceleration true, framebuffer
  2376×1216 with `ResolutionRatio=2`. Native window is requested at 1500×950
  logical units, light theme; framebuffer captures are inspected. This does not
  qualify resize, other display scales, physical OS pointer/keyboard input or
  Windows/Linux runtime.

The bounded workflow matches documented Modify entry, intersection preview,
click/repeat/Enter and two-arc circle splitting. Constraint/identity policy,
precise Escape sequencing and exact visual parity remain Alloy policies or
reference unknowns. Broader nearest-intersection/multi-piece behavior, splines,
linked/grouped owners and annotation remapping remain outside this slice.
