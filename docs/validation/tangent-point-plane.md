# Tangent plane at retained endpoint candidate

GEO-004/#286 API/core foundation independently accepted, 2026-10-14.
Native input and full source endpoint acceptance remain open.
Full source-sized rounded-handle acceptance also requires GEO-018/#350: generated
corner-round endpoint identities through the separate perimeter Fillet.

Reference `surface-shoehorn` (creator tutorial transcript/public video, retrieved
2026-10-14) selects the curved top face then a back-corner Fillet endpoint. The
external finishing contract and 740–750 second frames remain under
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/`.
The public evidence does not establish signed plane axes, edit behavior or defaults.
This candidate uses independently chosen deterministic projected global-X axes,
falling back to global Y when X is parallel to the normal.

## Command contract

`construction.tangentPlane` authors an ordinary `constructionPlane`, with
`parameters.source.tangent` containing `face: {producerFeatureId, bodyId, faceId}`
and `point: {bodyId, edgeId, endpoint: 0|1}`. Face and point must belong to the same
current body. Both must resolve to exact current named kernel topology. Endpoint
roles use current oriented edge metadata and must uniquely match its vertices.
Projection must place the endpoint on the trimmed face within tolerance; an
unavailable, ambiguous, off-face or singular source refuses recompute atomically.
The kernel evaluates the surface derivatives at that point, respecting face
orientation; it never substitutes a face center.

Native Construct controls expose Face, Point edge and Endpoint, plus disposable
plane preview, OK and Cancel/Escape. Canvas selection selects the face first,
then an actual boundary vertex within ten logical pixels; equivalent shared
vertex references use stable edge-ID ordering. Distinct overlapping vertices
are refused. The feature editor reuses the same command. The local control API
supports direct create/edit and command.begin/update/preview/commit/cancel.
Dependent sketches reuse the existing `planeId` support and ordered evaluation.

## Recorded evidence

- Debug macOS application and `tangent_point_plane_test` build passed.
- Focused core test passed on the actual guided curved Loft foundation, using
  its retained end-boundary endpoint: endpoint origin, orthonormal frame, upstream
  spline-bend normal update, attached 5 mm circle, undo/redo, save/reopen and atomic
  missing-endpoint refusal.
- `git diff --check` passed.

- Real-entrypoint local-server API passed preview/Cancel, direct create, attached
  circle, undo/redo, feature-dialog name edit, atomic missing-edge refusal,
  save/reopen and captures. After reopen the harness explicitly activates the
  source component; cross-component authored modeling is intentionally refused.
- Inspected actual controls show readable face/edge/endpoint labels, status and
  OK/Cancel. The reopened viewport shows the smooth curved sheet and retained
  planes/sketches. GPU diagnostics report Apple / Apple M4 / OpenGL 4.1 Metal
  90.5, GLSL 4.10 and display ratio 2. Evidence remains external at
  `../Alloy-reference-records/validation/tangent-point-plane-2026-10-14/api/`.
- Native Qt input remains **unverified**. The focused test could not establish
  initial viewport focus even with separate initial harness activation; it
  stopped before demonstrated Escape/selection. The logs are preserved in the
  adjacent `native/` directory. No focus repair occurred between demonstrated
  user steps. An unproven deferred command-focus change was discarded.

Astra source/core review accepted the bounded foundation with state/Escape
follow-ups fixed in e57bd32. Final runtime review accepted the bounded API/core and GPU evidence. No matched
visual-parity claim is made. The tests use a current Loft endpoint, not the final
back-corner Fillet endpoint; the cumulative tutorial must exercise the latter
before GEO-004 is closed.
