# SKT-156: multi-boundary face Project and selected-edge Delete

Candidate verified locally, 2026-10-18; independent compatibility review pending.
One explicit Project Face action now retains `faceProjectionGroups`: a stable
group UUID, exact producer/body/face source and ordered projected result IDs.
Selecting a result retains the existing **selected-edge Delete** policy. Delete
removes only that result and its memberships; sibling IDs, group source, sketch
support and upstream bodies remain. Empty groups retire. The group records the
face action; it does not imply a whole-face cascade.

This is Alloy policy. External evidence record
`../Alloy-reference-records/knowledge/specs/skt156-face-projection-delete-2026-10-18.md`
contains S1–S3, creator/source URLs, retrieval dates and the contract decided
before implementation. A published face Project exercise documents selecting a
face and confirming its projected geometry. Available Delete documentation is
for lost-link repair; live face-result cascade and a matched Delete screenshot
remain unknown. No proprietary application was operated or inspected.

## Ownership and shared command

Repeated face selection refreshes the same deterministic group without changing
existing result IDs or reordering other groups. Current implicit support rows
can participate in an explicit face action. Two source faces may share one
projected result; selected-edge Delete removes its membership from both while
retaining their surviving results and identities. Legacy per-edge projections
remain per-edge records. No proximity-based owner inference is added.

Validation checks group shape, UUID uniqueness, distinct existing member IDs
and matching source body. Kernel projection resolution checks the retained face
and that each surviving result is still a boundary of that face. Group metadata
is retained in authored data and excluded from solver expansion, then restored
after solving. Project preview/cancel remains private; failed final group solve
or evaluation restores the previous preview.

The existing native Delete/Backspace/context and `sketch.deleteSelection` routes
still share `SketchGeometry::deleteEntities`. Their private candidate updates
only explicit memberships before normal dependency checks, solve and downstream
selected-profile preflight. A surviving authored virtual-point consumer or
retained Offset dependency refuses atomically. Attached annotation cleanup
retains SKT-143 policy. No new option, default, preview or confirmation is added.

## Verification and evidence

Inspected `build-preview-release/CMakeCache.txt`: native Release/arm64,
private Qt 6.11.2 and OCCT 7.9.3. Built `alloy`,
`sketch_linked_projection_delete_test`,
`linked_projection_delete_controls_test` and
`sketch_constructor_delete_test`. Focused projection core and the constructor
Delete regression pass. Core includes shared membership, exact sibling/source
retention, empty membership retirement, malformed/stale member refusal and a
grouped virtual-endpoint dependency refusal.

`tests/sketch_linked_projection_delete_api.py` passes through the real app.
Two actual planar faces produce two four-boundary groups. It verifies stable
duplicate selection, selected-edge membership changes, exact sibling/source and
support preservation, empty-group retirement, one-step history and exact
Undo/Redo/reopen. A retained rectangle Offset prevents deleting its face source
edge before draft/history/selection/document changes. A committed Extrude of
the 300 mm² projected profile similarly refuses Delete with named consumer and
profile; deleting an unrelated point preserves every committed body on Finish
and reopen. Existing individual line/circle/point Delete routes remain covered.

`linked_projection_delete_controls_test` passes Qt-delivered native canvas
click/Delete on a face boundary, circle context/Delete and point list/Backspace.
It observes actual stable selection and focus, popup readiness/closure, exact
group membership and history/reopen. Initial harness activation is separate;
focus/selection is not repaired between demonstrated input steps. The existing
nonfatal popup top-level warning remains. This is native widget evidence, not
physical hand input.

`tests/sketch_face_projection_shared_api.py` passes a separate small real-app
box fixture. Its top and side face actions share one stable result; the side
also produces projected points. Repeating the top action preserves the draft
and owner identities; Cancel restores it. Deleting the shared edge updates both
groups, preserves all other authored parameters and source bodies, and passes
exact one-step Undo/Redo and save/reopen.

Evidence root:
`../Alloy-reference-records/validation/sketch-face-projection-delete-2026-10-18/`.
`api/`, `native/` and `shared/` retain receipts, exact authored fixtures and GPU
captures; API directories include command transcripts. `qualification.json`
records binary hashes and capture dimensions. Application SHA-256:
`ffb8067ef236553761dfff6cb36c5c55100d55a4c5454d94b54935622ab65668`.

GPU receipts report Apple vendor, Apple M4, OCCT OpenGL,
`4.1 Metal - 90.5`, hardware acceleration enabled. Inspected the native selected
and deleted face states and the isolated shared-result capture: projected
boundaries retain purple presentation, the requested result disappears,
unrelated boundaries/source remain and palette text is readable. In the shared
fixture another distinct result overlaps the removed edge, so its visible
profile can remain closed; authored IDs/membership establish the removal.
The primary fixture uses broad framing including an imported source assembly;
small circular geometry is correspondingly small. No matched public Delete
capture, exact styling parity or resize/display-scale qualification is claimed.
Private validation editors closed; user-owned editor preserved.

## Remaining scope

The existing straight-boundary Project Face restriction remains. Unsupported
curves, independent copies, source repair/relink/Break Link and broader
downstream consumer families stay with their separate owners. Whole-face
Delete is not the chosen policy. Physical hand input and Windows/Linux
runtime/GPU qualification remain release gates.
