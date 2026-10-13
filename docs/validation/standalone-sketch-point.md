# Standalone sketch Point — candidate (#246)

Status: bounded SKT-056 candidate, 2026-10-13; independent review pending.
Public basis: `plate-countersink-pattern-20261013`, page 3, retrieved 2026-10-13.
That page shows one top-face sketch point, two 10 mm witnesses to adjacent
straight rims, and a preceding centered 75 × 50 × 10 mm plate with four vertical
R5 Fillets. The detailed public provenance, images and unknown gestures remain
external in `knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`
and `independent-visual-acceptance.md`. This implements the Point prerequisite;
Hole, Pattern, the parameterized cumulative lesson and exact physical-input or
pixel equivalence remain separate.

## Compact implementation and command contract

An authored entity `{id, type:"point", pointId}` explicitly references one
canonical `{id, uMm, vMm}` solver point. Entity ID and point UUID are distinct.
Only referenced solver points remain valid; orphan points are still refused.
Point contributes the existing two coordinate variables without a curve,
radius or fake boundary. Its actual OCCT geometry is a vertex. Both legacy
profile derivation and multi-region construction exclude Point entities;
adding a point to a closed rectangle retains that rectangle's profile ID.
A point-only sketch can Finish, edit and reopen with zero selectable profiles.

Native Create → Point activates the `point` tool. Pointer motion previews a
small disposable location cue; one click creates one point and leaves the tool
available for the next point. Escape stops the tool and removes its cue without
removing accepted geometry. `sketch.addPoint {x,y,units?,session?}` uses the same
mutation authority; `sketch.placePoint {x,y}` remains viewport dispatch and now
uses one click for Point. Explicit API coordinates acquire no hidden relation.
An acquired origin/point snap retains explicit Coincident intent to a separate
point UUID, using existing origin/reference machinery; the standalone point is
not secretly fixed. The finite native source fixture needs no snap.

D uses the existing point/straight-edge dimension selection and inline value
editor. Both 10 mm dimensions bind the point UUID to the accepted DOC-020 actual
shortened rim projections; neither edge is replaced by fixed coordinates.
The palette reports zero curves and one standalone point, with neutral
“Standalone points · no profile boundaries” information. It never calls that
point an edge or selectable region.

Finished visible points have their own GPU markers and remain selectable on the
retained face. `selection.pick` returns
`{kind:"sketchPoint", sketchId, entityId, pointId, positionMm}`. `app.state.selection`
retains the same identity through routine UI refresh; an actual body/sketch/
profile selection replaces it through the established selection paths.
`document.inspect.evaluation.sketches[].standalonePoints` exposes the explicit
entity/point relationship and current world position for future Hole validation.
Viewport `standalonePointMarkers` reports visibility and selection separately.
Markers follow authored sketch visibility; active sketches use the existing
point decorations. Selection is transient, while point/dimension UUIDs persist.

## Evidence and result

Isolated worktree: `Alloy-worktrees/standalone-sketch-point`, base `5e98084`.
Evidence: `build/evidence/standalone-sketch-point/`.

- Debug application and affected native/core targets compile with Qt 6.11.2 and
  OCCT 7.9.3. `build.log` records the final application build.
- Focused core passed 3/3 in 0.93 s (`standalone_sketch_point`, `sketch_solver`,
  `sketch_profiles`). The final owned-ID fixture also passed separately, recorded
  in `core-standalone-final.log`; solver/profile production branches were unchanged.
  Coverage proves two placement DOF → zero after the two perpendicular dimensions,
  one vertex/no edges for an ordinary point-only sketch, no fabricated profiles,
  retained explicit entity/point IDs, mixed-profile identity, invalid/orphan
  refusal, source width and radius edits, exact history and native save/reopen.
- `standalone_point_controls` passes the final focused native run in 6.06 s.
  API setup creates the rounded blank separately. Actual Qt input then clicks
  Create → Point, clicks a rough location, presses D, picks the point and each
  actual shortened model edge, places each dimension and types 10/Return.
  There is no focus or selection repair after initial harness activation.
  Each step asserts app-produced focus/selection; two dimensions consume exactly
  the two DOF. Actual Finish button and viewport click select the finished point
  UUID. A fresh edit plus Point/Escape leaves the accepted draft exact.
- The real native executable passes 67 shared-command calls in
  `tests/standalone_sketch_point_api.py`. `api/report.json` and `transcript.json`
  record direct Point creation, two retained 10 mm dimensions, finished selection,
  centered width 75→85 (point x −27.5→−32.5, y15/z10), radius 5→4, exact undo/redo,
  invalid radius/edge-ID atomic refusal and exact save/reopen.
  `api/point-dimensions.alloy` remains a point-only supported sketch above the
  rounded plate, ready for a separate Hole command; it contains no hole.
- Inspected final `native/native-two-edge-point.png`: distinct point, readable
  two 10 witnesses, no circle or filled profile, truthful point/curve counts and
  neutral palette information. `native-finished-selected-point.png` shows the
  retained selected marker after Finish. API original/edited/reopened captures
  corroborate the current geometry and marker. Apple vendor / Apple M4 / OpenGL
  4.1 Metal hardware acceleration is reported with the native framebuffer.
  Window, source scale and annotation placement are unmatched to the public PDF;
  no exact styling claim follows. Native Qt events are not physical OS input.

The final native capture includes the neutral point-only label change. The
passing API run precedes only that label change; its API/document/selection
behavior is unchanged and was not redundantly replayed. Earlier failed attempts
remain qualified: a synchronous InstantPopup harness timed out (fixed by queuing
its actual menu click); a dangling temporary JSON reference in the test pixel
helper crashed after Finish (fixed with an owned QString); and native/API checks
caught selection being cleared by the empty body cue during UI refresh. The
finished-selection defect was fixed before the passing native/API runs.
`native-before-fix.log` and `native-selection-before-fix.log` preserve the latter
failures. Private validation editors exited; no user-owned instance was operated.

Matched within this prerequisite: a real standalone point on the rounded support,
two 10 mm edge associations and native ingredient selection/value workflow.
Unknown: exact public source Point launch keys, pointer timing, marker/label
styling and exact pixel comparison. Full Hole, nine-occurrence Pattern, named
parameter integration, tutorial movie, exports, other-platform runtime and
release qualification are not established here. Existing dimension schemas were
not rewritten; the parallel parameter-binding fields must remain during merge.

## Independent acceptance

Accepted the bounded #246 prerequisite at candidate
`cf8b6cb3aafde766080b33e509da850c5e883a82` after independent source, persisted
artifact, test-log and actual framebuffer review. No additional editor run was
needed. The explicit point entity references its solver point; evaluation emits
a vertex without inventing a curve or profile boundary. The two retained edge
dimensions consume the placement DOF, and the finished selection carries both
entity and point identities through routine refresh. Real body/profile selection
still replaces that selection.

Independent readback confirmed the 67-call API report, exact equality of the
saved authored document and reported final authored state, one standalone point
and no profile in the supported sketch, and unchanged dimension/projection
references across width/radius edits. The final evaluated point is approximately
(−32.5, 15, 10) mm. Focused core evidence and the native input source support the
reported atomic refusals, history and persistence checks. The native sequence
uses initial activation only, followed by actual menu, point/edge, placement and
numeric input; no intermediate focus or selection repair masks the workflow.

The actual two-edge capture has readable 10 mm witnesses, a distinct point and
neutral point-only status. The finished capture visibly selects the same marker.
Accept the author’s stated API-before-label-change evidence boundary and Qt-versus-
OS limitation. This does not accept the later Hole/Pattern/full tutorial gate or
prove matched public-source pixels. Integration must preserve the independently
accepted named-parameter schema and controls; that combined check remains with
the integrator.
