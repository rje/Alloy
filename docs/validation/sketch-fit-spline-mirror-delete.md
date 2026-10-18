# SKT-148: retained fit-spline Mirror result Delete

Accepted 2026-10-18 after independent Sol source and evidence review.
The existing `sketch.deleteSelection` / `SketchGeometry::deleteEntities` authority
now accepts retained open fit-spline Mirror owners, including a mixed owner
containing a line, arc and fit spline. Selecting any generated entity removes the
whole selected Mirror owner. No new Mirror creation mode or Offset family is added.

The permitted public evidence is the external dated
`../Alloy-reference-records/knowledge/specs/skt142-mirror-offset-delete-triage-2026-10-18.md`.
S1 documents selected geometry Delete and context Delete; S2/S3 document Mirror
creation. Exact result cascade, matched styling and visual parity remain unknown.
Whole-owner deletion is Alloy policy, rather than an exact compatibility claim.

In Select mode, select a generated result in the canvas or sketch list and press
Delete, or select it and use canvas context Delete. No new options, defaults,
preview or confirmation are introduced. A retained Mirror spline now reaches the
shared geometry context menu instead of the source-only Insert Fit Point menu.
The source spline's existing authoring menu is unchanged. Right-click retargeting
from a previously selected source spline is not qualified: that existing source
menu still takes precedence, so this receipt requires selecting the result first.

The owner expands only through `entityPairs.resultEntityId`. Paired generated
points and removed entities' fit-point arrays are candidates for cleanup; points
used by surviving geometry remain. Source entities, source point IDs, tangent
handles, axis and on-axis source/result shared identities survive. Surviving
entity/retained-owner dependencies refuse atomically; source, axis, mixed
source/result and stale requests do not publish partial edits. Existing relation
and measurement cleanup applies to retired IDs. One successful request adds one
draft history step; exact Undo/Redo and authored save/reopen preserve the result.
This does not qualify downstream feature-dependent deletion.

## Verification

Inspected `build-preview-release/CMakeCache.txt`: Release, this repository's
source directory, Qt 6.11.2 at
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
The app, focused constructor-delete core and Qt widget targets built successfully.
The exact final app SHA-256 is in `api/receipt.json` below.
The reviewer reran the focused core contract, matched the final app hash to the
API receipt, checked the transcript and saved model, and inspected all three
captures. No blocking defect was found.

`sketch_constructor_delete_test` passes mixed line/arc/fit-spline owner deletion
from either a spline or line result, exact baseline parameters including an
on-axis fit point, surviving Chamfer dependency refusal, and atomic
source/axis/mixed/stale refusals. Existing constructor and line/arc Mirror and
open-chain Offset cases still pass.

`tests/sketch_fit_spline_mirror_delete_api.py` passes the real application entrypoint
with a mixed line/arc/fit-spline owner, a retained endpoint tangent, an actual
on-axis shared spline point, construction axis and unrelated fixed circle.
It checks exact source entities/points, whole-owner removal, one history step,
atomic refusals, exact Undo/Redo and authored save/reopen. Captures show all three
results removed while the original spline, line, arc, axis and circle remain.
Reopened layout is wider after leaving Sketch; it demonstrates visible
persistence rather than pixel-identical framing. Geometry here remains open;
the unrelated circle supplies the valid closed region, and no downstream solid
is claimed.

`widget_interactions_test sketchFitSplineMirrorDelete` passes Qt-delivered native
canvas click/Delete, sketch-list click/Delete, and selected canvas result context
Delete. Actual selection IDs/focus are observed before keys; popup activation and
closure are observed. Initial macOS activation is separate; no focus/selection
repair occurs between demonstrated user steps. The context fixture first clicks
the result, then right-clicks it. This is native-widget evidence, not physical
hand input. The existing popup window warning remains nonfatal.

Evidence is external at
`../Alloy-reference-records/validation/skt148-fit-spline-mirror-delete-2026-10-18/`:
`build.log`, `core.log`, `native.log`, `api.log`, `api/receipt.json`,
`api/transcript.json`, `api/selected.png`, `api/deleted.png`, `api/reopened.png`
and `api/fit-spline-mirror-deleted.alloy`.
GPU receipt: Apple / Apple M4, hardware accelerated OpenGL `4.1 Metal - 90.5`;
captures combine Qt chrome with the native OCCT framebuffer. Initial stopped Qt
runs are retained separately: a fit-point click selected a point, and the
unselected-result context attempts entered the existing source authoring menu.
The final fixture uses a between-fit-points curve location and the explicit
result selection described above. Only the user-owned PID 33805 remains; private
validation windows were closed. Windows/Linux runtime, physical input, matched
reference styling, broader context targeting and downstream solids remain open.
