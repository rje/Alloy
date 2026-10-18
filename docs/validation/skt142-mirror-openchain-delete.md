# SKT-142: Mirror and open-chain Offset owner Delete

Accepted 2026-10-18 after independent Astra source and evidence review.
The existing `sketch.deleteSelection` / `SketchGeometry::deleteEntities` authority
now removes a whole retained line/arc Mirror or `openChainOffsets` owner when a
generated curve is selected. No additional command model or UI was introduced.

The dated permitted public evidence is external at
`../Alloy-reference-records/knowledge/specs/skt142-mirror-offset-delete-triage-2026-10-18.md`.
Its S1 documents geometry selection followed by Delete or context Delete. Exact
Mirror/Offset result-deletion cascades and matched visual parity remain unknown.
Whole-owner removal, shared-ID retention and atomic refusal are explicit Alloy
policies, rather than a reference compatibility claim.

Select one generated member in the canvas or sketch list and press Delete, or
use canvas context Delete. Select mode is required. There are no new options,
defaults, preview or confirmation. A successful request publishes one solved
draft edit; Undo/Redo restores exact authored state and save/reopen retains it.

Mirror member expansion uses only `entityPairs.resultEntityId`; candidate owned
points use `pointPairs.resultPointId`. Every source in the removed group must be
an authored line or arc. Source and axis entities survive, including source point
IDs reused by on-axis results. Valid constructor-owned line/arc inputs use the
same schema and are allowed; that additional input ownership was not separately
exercised in this fixture. Owners containing fit splines refuse atomically.
Open-chain expansion uses `generatedEntityIds`; `generatedPointIds` and optional
`topIntersectionPointIds` are candidates for retirement. Source lines, the
optional top source and linked user parameters survive. Points still referenced
by surviving geometry remain, including genuine shared closure endpoint IDs.
Relations and measurements referencing retired IDs are removed by the existing
cleanup. Surviving entity/owner dependencies, source-only selection, mixed
source/result requests and stale requests refuse without publishing partial work.
Other Offset schemas remain SKT-144–147/#570–573; projected ownership remains
SKT-143/#569. This receipt does not qualify downstream feature-dependent deletion.

## Verification

The inspected `build-preview-release` cache uses Release, this repository's
source directory, and Qt 6.11.2 from
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
The application, focused core and Qt widget targets built successfully. Tested
app SHA-256:
`ebf02bd2a3f4840865256efeeb48041ff82efa0ead8b872f97498bb8c65f4e9e`.
The reviewer independently reran the focused constructor-delete core test,
matched the application hash to the API receipt, inspected both saved models,
native/API logs and all six captures, and found no blocking ownership defect.

`sketch_constructor_delete_test` covers mixed line/arc Mirror cleanup, an on-axis
shared source point, read-only result measurements, a surviving Chamfer dependency,
fit-spline Mirror refusal, complete five-line Offset cleanup, a separate closure
with shared derived IDs, and source/mixed/stale refusal. Existing Slot, Arc Slot,
Polygon and Chamfer cases still pass. The focused CTest selection also covers the
existing Mirror and open-chain Offset contracts.

`tests/sketch_mirror_openchain_delete_api.py` passes separate real-entrypoint
Mirror and Offset fixtures with an unrelated fixed circle. The Mirror has one
line, one arc and an on-axis shared source point. The Offset has five source
lines, an optional top source, a direct user-length binding and a separate closure
using genuine shared top-intersection points. Both verify one-step history,
exact Undo/Redo, source/point identity preservation, atomic source/mixed/stale
refusal and authored save/reopen; the Offset also verifies parameter preservation.

`widget_interactions_test sketchMirrorOpenChainDelete` passes real canvas
click/Delete for a Mirror line result, sketch-list click/Delete for an Offset
member, and canvas context Delete for an Offset member. Stable selection IDs and
the actual focus produced by clicks are checked before keys. Popup activation and
closure are observed. Initial macOS activation is a separate harness step; no
focus or selection repair occurs between demonstrated steps. This is Qt-delivered
native-widget evidence, not physical hand input. The existing popup window warning
is retained and does not fail the route.

Evidence is external at
`../Alloy-reference-records/validation/skt142-mirror-openchain-delete-2026-10-18/`:
`api/receipt.json`, `api/transcript.json`, six selected/deleted/reopened captures,
two saved models, `core.log`, `native.log` and `build.log`. Inspected captures show
both Mirror outputs gone while sources/axis/circle remain, and all five Offset
outputs gone while source geometry, circle and the independent closure remain.
The source and closure overlap along part of the top boundary; authored IDs were
inspected separately. Reopened captures have a wider viewport after leaving
Sketch, so they establish visible persistence rather than pixel-identical layout.
The 1440 × 900 logical window uses 2× display scale; captures combine Qt chrome
with native OCCT framebuffer pixels. GPU receipt: Apple / Apple M4, OpenGL
`4.1 Metal - 90.5`, hardware accelerated. Windows/Linux runtime, physical input,
matched public-reference styling and downstream solids remain separate gates.

Stopped fixture runs are retained separately: initial native Offset setup omitted
required tool activation; initial API Offset supplied mutually exclusive numeric
and parameter inputs; the first broader CTest invocation named an unbuilt Offset
target. These were harness/setup corrections, not accepted runtime evidence.
Only the user-owned Alloy instance was left running; private validation windows
were closed before handing the GUI lease back.
