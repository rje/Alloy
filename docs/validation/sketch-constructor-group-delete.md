# SKT-141: retained constructor group Delete

Accepted on 2026-10-18 after independent Astra ownership and evidence review.
The shared `sketch.deleteSelection` command now removes an entire straight Slot,
Arc Slot, Polygon or Chamfer owner when one of its generated curves is selected.
Mirror/Offset and linked/projected owners remain SKT-142–143/#568–569.

The permitted dated public workflow evidence and explicit Alloy policy are in
`../Alloy-reference-records/knowledge/specs/skt102-delete-triage-2026-10-18.md`,
including its SKT-141 addendum. REF documentation establishes select/Delete or
context Delete. It does not establish constructor cascades: whole-owner removal,
cleanup and atomic refusal are Alloy policies. Exact reference cascade and
matched visual parity remain unknown.

Select a member in the canvas or sketch list, then press Delete, or choose
Delete from its canvas context menu. There are no new options/defaults, preview
or confirmation. One successful request publishes one solved draft edit.
Undo/Redo restores exact authored state; saved/reopened documents retain it.

The explicit member maps are Slot `centerlineId/sideIds/arcIds`, Arc Slot
`pathArcId/outerArcId/innerArcId/capIds`, Polygon `entityIds`, and Chamfer
`lineId`. External snap/placement sources never expand deletion targets. The
command removes exclusively used points and references to retired IDs; points
used by surviving entities remain. Polygon retires its owned vertex, anchor and
second-driver relations even when a point survives in unrelated geometry.
Chamfer stores unchanged authored sources and evaluated-only cut lines/contacts;
removing metadata rederives its presentation and restores that source corner.
Adjacent Chamfers survive. Unsupported surviving ownership refuses atomically.
The sketch list and API selection now expose evaluated Chamfer member IDs.

## Verification

The inspected `build-preview-release` cache is Release, source
`/Users/rje/src/cpp/Alloy`, with Qt 6.11.2 from
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
The app and focused tests built successfully. Tested app SHA-256:
`3bffa14c104922220809739786e576b57d3bd3dceb18ab22663347f21aed817c`.
The independent reviewer reran the focused constructor-delete core test and
matched this app hash to both retained API receipts; no blocking ownership
defect was found.

`sketch_constructor_delete_test` passes all four owner schemas, generated
relation/dimension cleanup, external snap preservation, a genuinely shared
Polygon vertex ID, adjacent Chamfer retention and exact final source restoration.
Mixed stale targets, a surviving Chamfer source dependency and unsupported
Mirror ownership refuse without changing the input draft. The focused CTest
selection also passes Polygon anchor/driver and Chamfer contracts (4/4).

`tests/sketch_constructor_delete_api.py` passes one distinct real-entrypoint
fixture per owner with an unrelated fixed circle, external source IDs, exact
Undo/Redo and authored save/reopen. The Polygon has both external placement
drivers and an unrelated snapped line; the Chamfer fixture has two adjacent cuts.
The authored-curve regression passes, including the retained fit-point route,
shared points, ellipse-axis refusal and downstream circle extrusion/reopen.

`widget_interactions_test sketchConstructorGroupDelete` exercises real canvas
click/Delete for straight and Arc Slot, list click/Delete for Polygon, and
Chamfer canvas context Delete plus generated-list-row Delete. It verifies the
stable-ID selection and focus produced before each key. The context route
observes popup activation before its pointer click and closure afterward.
Initial macOS activation is a separate harness step; there is no inter-step
focus or selection repair. This is Qt-delivered native-widget evidence, not
physical hand input. The retained popup window warning does not fail these
checks. Early stopped runs are preserved separately: left presses at two
Chamfer segment positions produced no curve selection because edit handles can
intercept that route. Left-click Chamfer selection is not qualified here.

Evidence, transcripts, native logs, saved models and before/after/reopen GPU
captures are external at
`../Alloy-reference-records/validation/skt141-constructor-delete-2026-10-18/`.
Inspected captures show complete group removal, preserved unrelated geometry,
and a restored Chamfer corner with its adjacent cut retained. The actual native
framebuffer identifies Apple/Apple M4, OpenGL 4.1 Metal, 4x MSAA and a 2x display
scale. Windows/Linux runtime, physical input and exact reference styling remain
separate release gates.
