# Bounded analytic Measure candidate (DOC-039)

Status: implementation candidate; this does not close DOC-039 or establish full
workflow/visual compatibility. Public inputs and observed limitations are in the
external `knowledge/specs/doc039-measure-minimum-2026-10-19.md` contract. Source
index DOC-039 was retrieved 2026-10-19; bulk evidence remains external.

## Command contract

Inspect > Measure and `I` open a modeless native Measure panel. Edge, Face and
Point filters dispatch canvas selections through `selection.pick` and the same
pure evaluator used by `inspect.measureResult`. Selection 1/2 show source type and authored name, with exact identifiers in
tooltips and structured API output. Clear Selection clears the transient picks; Close/Escape
ends inspection. No feature, parameter, annotation or undo transaction is created.

`inspect.measureFilter` selects Edge/Face/Point; `inspect.measureClear` resets
the same native picks. `inspect.measureResult` accepts optional `sources` (otherwise the panel picks):

- `{kind: edge, bodyId, edgeId, occurrenceId?}` identifies an exact current edge.
- `{kind: face, bodyId, faceId, producerFeatureId?, occurrenceId?}` identifies one
  current named planar, cylindrical or toroidal face.
- `{kind: sketchEdge, sketchId, entityId}` identifies solved sketch topology.
- `{kind: sketchPoint, sketchId, pointId}` identifies a standalone solved point.

One edge returns exact OCCT Length; one qualified face returns Area. A standalone
point/straight finite edge returns shortest Distance. Two straight edges return
Distance and Angle only for an unambiguous 90-degree pair; other angle conventions
are explicitly refused. Source IDs are resolved again on every query; input
coordinates are never trusted. Hidden, missing and ambiguous identities, empty or
overfull selections and unsupported pairs return structured errors. Linked body
instances require unambiguous occurrence identity. Numbers include canonical
mm/mm²/deg and unrounded active-unit values. The panel displays four decimals,
with clickable copy values. Panel sources refresh after model/unit changes and
close on document switch or another active modeling session.

## Bounded remaining work

- Native actual-hit selection, activation/readability and control parity require
  the recorded execution result below; a successful API query alone is insufficient.
- Measurement-hover-specific dimension graphics, precision/secondary-unit
  controls, loop length/radius/diameter readouts, general angle branches, arbitrary
  geometry pairs and unnamed curved faces are outside this candidate.
- Standalone sketch points are supported; general topological vertices and
  source endpoint picks remain a distinct extension.
- Cylindrical area is the bounded curved-face acceptance fixture; toroidal
  evaluation exists but needs a retained Fillet fixture before claiming that path.
- Native Windows/Linux, display-scale qualification and matched public visual
  comparisons remain unqualified.

Proposed follow-up: keep DOC-039 open and split a child for native Measure
presentation/hover/vertex completion and a child for extended analytic results
and angle conventions. Preserve these requirements rather than shrinking the
parent's acceptance.

## Execution evidence

- Release build of `alloy` and `measure_controls_test` passed with the existing
  `build-preview-release` CMake cache and `-j4` on macOS/Apple Clang 17.
- `tests/measure_api.py` passed through the actual application executable: 52
  transcript calls cover 5 mm edge length, 6 mm point/finite-line distance, 90°
  line pair, 200 mm² planar area, 120π mm² cylindrical lateral area, inch
  conversion, exact source IDs and structured refusals. Every inspection group
  preserves the complete `document.inspect` response; inspection also preserves
  byte-identical authored saves. The native filter/picker authority was exercised
  by API viewport coordinates for edge, face and point/line states.
- Inspected `api/measure-inch.png`, `measure-planar-inch.png` and
  `measure-point-line-inch.png`: human-readable selections, four-decimal
  active-unit results, and a panel positioned at the upper right. IDs remain
  accessible through tooltips/API. These are semantic Alloy layout checks, with
  no matched public screenshot or exact visual-parity claim.
- Actual framebuffer/GPU evidence identifies Apple / Apple M4 / OpenGL 4.1
  Metal - 90.5, hardware acceleration, 2× native resolution ratio.
- Native `measure_controls_test` cannot establish initial application activation
  on this desktop (`w.isActiveWindow()` stays false), so it stops before the
  demonstrated toolbar/geometry input sequence. No inter-step focus or selection
  repair is used. Earlier probes exposed a real centered-panel obstruction,
  corrected by the upper-right placement, and an overly strict assumption that
  the modeless dialog itself must become active; both failed logs are retained.
  Physical/native selection and control acceptance remain unqualified.
- Private validation application/test processes exited; no CI, publishing or
  other-platform runtime was performed. Independent compatibility review remains
  with the integrator.

External output: `validation/doc039-measure-2026-10-19/` under the
reference-records directory, including build/native logs, API transcript,
framebuffer captures and both saved fixtures. This is a bounded candidate, not
DOC-039 closure.
