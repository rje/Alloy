# SKT-147: retained Circle Offset result Delete

Accepted 2026-10-18 after independent Sol review of source, focused checks,
actual-app captures, and retained transcripts. The existing
`sketch.deleteSelection` / `SketchGeometry::deleteEntities` authority now treats
`circleOffset.entityIds=[source,result]` as an explicit single-result owner.
Select its result circle and press Delete/Backspace, or select it and use canvas
context Delete. The result, unused generated center, Offset relation and its
exact center-pair Coincident relation are removed. Source geometry, source
annotations and unrelated geometry/relations keep their authored identities.
A center used by surviving geometry stays. Source-only or mixed source/result
requests refuse atomically with “Delete the Offset result first, separately
from its source geometry.” Stale requests and surviving retained metadata
references also refuse without publishing a partial draft.

Public Delete gestures and Offset association removal are documented by
S1–S3 in external
`../Alloy-reference-records/knowledge/specs/skt147-circle-offset-delete-triage-2026-10-18.md`.
The exact result-geometry cascade is unknown: the single-result policy above is
Alloy policy. There are no new options, defaults, preview or confirmation.
Selecting the Offset glyph and pressing Delete removes only its `circleOffset`
relation: both circles and their center Coincident relation remain. The result
radius then accepts an independent driving dimension. Matched reference
styling, precise glyph hit preference and broad visual parity remain unknown.

## Verification

Inspected `build-preview-release/CMakeCache.txt`: Release, current repository,
Qt 6.11.2 at
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
The application, focused Circle Offset core, constructor Delete regression and
Qt widget targets build successfully. Final application hash is recorded in
`api/receipt.json` below. The Circle Offset test also corrects an existing
temporary `QJsonValueRef` lifetime error in its retained dimension ID.

`sketch_circle_offset_test` passes retained creation/edit/profile/persistence
checks and new result Delete, source/mixed/stale atomic refusals, exact surviving
parameters with a dimensioned source, source center lock and unrelated fixed
circle. A shared-center fixture keeps a surviving circle's generated center
while removing the Offset's exact center coincidence. Two valid disk profiles
remain after the primary result deletion. `sketch_constructor_delete_test`
passes the adjacent constructor/Mirror/open-chain Offset regression contract.

`tests/sketch_circle_offset_delete_api.py` passes through the real application
entrypoint with a dimensioned R15 source, retained inward R11 result, source
center lock and unrelated fixed circle. It checks exact atomic refusal state
and draft history, association-only removal plus an independent R9 result edit,
exact successful source parameters, one Delete history step, exact Undo/Redo and
authored save/reopen. Selected/deleted/reopened captures were inspected: the
inner circle disappears and the dimensioned source and unrelated circle remain.
The reopened viewport is wider after leaving Sketch and demonstrates visible
persistence, not identical framing. This fixture does not claim a downstream
solid. The projected-source variant was inspected in code, not runtime-qualified:
center lookup expands projection context and no projection record is retired
by the result target; broader linked projection deletion remains separate work.

`widget_interactions_test sketchCircleOffsetDelete` passes Qt-delivered result
canvas click/Delete, actual Offset badge click/Delete, and selected result
canvas context Delete. Stable selected entity/constraint IDs and resulting focus
are observed before Delete. Popup activation and closure are observed for the
context route. Initial macOS harness activation is separate; there is no focus
or selection repair between demonstrated input steps. Each route checks its
single history step and exact Undo/Redo. This is native-widget evidence, not
physical hand input. The existing popup window warning remains nonfatal.

## Downstream dependency boundary

One genuine annular-profile Extrude consumer was probed separately. Result
Delete succeeds in the historical sketch draft. Finish then fails atomically
with `evaluation_failed` / `profile_unavailable`, identifying the owning Extrude
feature and missing profile. The committed authored document and complete
evaluated body array remain exactly unchanged, and Sketch Edit stays active.
Thus the existing document evaluator prevents a dangling committed model, but
shared Delete does not yet refuse the dependency before changing the draft.
This earlier actionable dependency guard is SKT-150/#576; this result does not
claim it.

Evidence is external at
`../Alloy-reference-records/validation/skt147-circle-offset-delete-2026-10-18/`:
`build.log`, `core.log`, `constructor-regression.log`, `api.log`, `native.log`,
`api/receipt.json`, `api/transcript.json`, `api/selected.png`, `api/deleted.png`,
`api/reopened.png`, `api/circle-offset-deleted.alloy`; downstream probe has
`downstream/receipt.json`, `transcript.json`, `before.alloy` and `after.png`.
GPU receipt: Apple / Apple M4, hardware accelerated OpenGL `4.1 Metal - 90.5`;
captures include Qt chrome and the native OCCT framebuffer. Private validation
editors are closed; user-owned PID 33805 is preserved. Windows/Linux runtime,
physical input, matched reference styling, projected-source runtime and early
downstream Delete refusal remain unqualified.
