# SKT-102: authored ellipse, conic and spline Delete

Accepted for the bounded authored-curve scope on 2026-10-18 after independent
Sol review. This extends the accepted [ordinary geometry Delete](sketch-geometry-delete.md)
with whole-entity deletion of authored `ellipse`, `conic`, `fitSpline` and
`controlSpline` entities through the existing shared `sketch.deleteSelection`
command. Actual grouped/generated/linked removal remains SKT-141–143/#567–569.

The permitted public evidence, retrieval dates, actionable workflow and unknowns
are external at
`../Alloy-reference-records/knowledge/specs/skt102-delete-triage-2026-10-18.md`.
Select a curve, press Delete or choose Delete from its canvas context menu.
There are no new options or defaults, preview or confirmation. One successful
request makes one solved draft-history edit; Undo/Redo restores the authored
state. The existing selected fit-point Delete route removes that fit point
rather than the whole fit spline. Whole-curve and explicit point targets remain
distinct.

Alloy's transactional policy removes only selected curves, exclusively used
points, and constraints/dimensions referencing retired IDs. An ellipse also
retires its two authored construction axes and their exclusively used endpoints.
A selected axis alone refuses while its ellipse survives. Surviving shared
points retain their stable identities and coordinates. Any surviving entity or
retained group/derived metadata depending on retired geometry refuses before
publication and directs the user to its owning operation. This cleanup policy
is an Alloy design; exact reference cascade and visual parity are unknown.

## Build and command evidence

Inspected `build-preview-release/CMakeCache.txt` before building: Release,
source `/Users/rje/src/cpp/Alloy`, Qt from
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
`cmake --build build-preview-release --target alloy widget_interactions_test -j 4`
passed. The tested app is
`build-preview-release/alloy.app/Contents/MacOS/alloy`, SHA-256
`794cb6064fee6dcf61d9f8862f7df272d3b371f96f851c4a01c335fab0b28e3b`.

`widget_interactions_test sketchAuthoredCurveDelete` passed 3/3 QtTest rows
(including init/cleanup), 2.102 s on the final focused rerun. The test establishes initial macOS process
activation before demonstrated input. Then actual canvas clicks select a fit
spline between fit points; Delete removes the whole spline. A separate canvas
context event on the ellipse contour selects the ellipse; a pointer click on
the popup Delete action removes its axes, points and relations. The final test
observes native popup activation before that click and closure afterward. A sketch-list
row click selects the surviving spline and the focused list's Delete key removes
it. Each step checks the produced stable-ID selection and focus before the next
key. There is no inter-step focus or selection repair. Undo/Redo is exact.
This is Qt-delivered native-widget evidence, not physical hand input. One Qt
window warning is retained in the log; the test's focus and state checks pass.
The first run stopped at initial window activation; no user input sequence was
accepted from that run. The explicit initial activation harness resolved it.
The final popup check is retained in `native-qt-popup-activation.log` alongside
the earlier `native-qt.log`.

`python3 tests/sketch_authored_curve_delete_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/skt102-authored-delete-2026-10-18`
passed through the actual local-server application entrypoint. Its 122-call
transcript checks:

- Separate whole-curve deletion of both spline kinds, conic and ellipse, exactly
  one history increment each, unchanged surviving point coordinates/IDs,
  retired-reference cleanup, and exact draft Undo/Redo.
- A conic and fit spline share an authored point identity: deleting either
  curve retains the other's shared point. A surviving line's distinct coincident
  endpoint and relation also persist. Selected fit-point Delete removes one
  point and retains the spline, with exact Undo.
- Ellipse deletion removes its full-axis dimension and internal relations.
  Independent axis deletion refuses while the ellipse survives.
- Deleting all authored target curves leaves the independent 30 mm circle.
  Its profile extrudes 8 mm into a valid closed solid, 5654.866776461628 mm³
  (independently calculated expected volume `pi * 15^2 * 8`). Save/reopen preserves
  the complete authored document and evaluated body identity.
- A retained straight-slot member refuses with `slotGroups` owning-operation
  guidance. Draft, draft history and selection are identical before/after the
  refusal. This qualifies safe refusal, not slot/group deletion.

## Geometry, GPU and presentation evidence

Evidence is external under
`../Alloy-reference-records/validation/skt102-authored-delete-2026-10-18/`:
`receipt.json`, `transcript.json`, `native-qt.log`, selected/deleted captures for
all four curve types, `owner-refusal.png`, `reopened-solid.png`, and the saved
`authored-curves-deleted-solid.alloy` model.

The actual app reports Apple / Apple M4, hardware acceleration true,
OpenGL `4.1 Metal - 90.5`, native framebuffer 2376×1216. Inspected fit-spline and
ellipse selected/post-delete captures and the reopened isometric solid capture:
selected curve/handles are visible, deleted geometry disappears, retained
geometry and labels remain legible, and the downstream cylinder is smooth.
The mixed fixture intentionally contains open curves and overlapping regions;
its profile warnings describe that state. No matched public screenshot exists
for this fixture, so screenshot parity remains unknown. Resize/display-scale
behavior was not requalified for this command-only change.

The private validation editors exited after evidence collection. User-owned
PID 33805 was preserved. Windows/Linux runtime, physical hand input, actual
group/linked removal, broader cross-feature deletion policies and exact public
cascade/visual parity remain unqualified.
