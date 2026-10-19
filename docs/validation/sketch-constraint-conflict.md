# Direct sketch relation conflict candidate

SKT-100 / #519, bounded candidate, 2026-10-19. Native input acceptance is
pending an unlocked desktop. This is an Alloy diagnostic contract; exact
reference presentation is unknown. The public evidence and its provenance are
external at `../Alloy-reference-records/knowledge/specs/skt-100-overconstraint-diagnosis-2026-10-18.md`.

## Supported command and repair contract

For one authored straight line with a positive driving Euclidean endpoint length,
adding Vertical to an existing Horizontal (or the reverse) returns
`constraint_conflict`. The direct certificate is that both orientation equations
would collapse the endpoints, contradicting that positive length. This is not a
general minimal conflict-set extraction. A valid baseline solve is required;
unsupported cases retain generic solver failure rather than invented culprits.

The same `sketch.constraint` authority serves the native relation tool and API.
The failure contains `accepted: false`, the candidate-only `attemptedRelation`
record and `attemptedRelationAuthored: false`, the accepted `conflictingRelations`
and IDs, shared line and endpoint IDs, and the positive `supportingDimensions`.
The proposed relation is never authored; its ID is inspectable in the failure
record. All accepted relation/entity/dimension IDs resolve in the normal draft.
`postFailureSolver`, `documentRevision`, `draftHistory`, and `sketchId` report the
last valid session separately from the failed attempt. The authored draft,
coordinates, solver status, document revision and undo/redo history are preserved.

The native Palette retains readable conflict text outside its scroll area,
naming the attempted and existing orientation and the driving length. The
**Select Existing Relation** button invokes `sketch.conflictSelect`, returns to
Select, reveals constraints and selects that accepted badge. Normal canvas badge
selection and Delete remain the removal authority. A successful solve/Delete or
history traversal clears the diagnostic; selection alone preserves it. No Driven
alternative is offered for a geometric relation.

After deleting Horizontal, Vertical solves the anchored 40 mm line to `(0, 40)`.
An exactly perpendicular initial seed can make the length equation singular;
only after the normal solve fails, the orientation path retries an aligned
endpoint seed through the same solver and every retained equation. No equations
are dropped and an incompatible fixed endpoint still refuses. The zero-axis tie
chooses the positive target direction; broader branch-choice parity is unknown.

## Verification and continuation

Build directory: `build-preview-release`, Release, AppleClang 17, private Qt
6.11.2 SDK and OCCT 7.9.3; build with `-j4`.

- `sketch_conflict_test` checks the exact certificate, immutable failed draft,
  Vertical repair, inverse Horizontal switch preserving unrelated geometry,
  fixed-endpoint refusal and generic fallback without positive length proof.
- `sketch_solver_test` checks existing solver constraints, dimensions, conflicts,
  redundancy and copy isolation.
- `tests/sketch_conflict_api.py` uses the real application entrypoint for failure,
  unchanged draft/history/status/revision, shared selection/Delete, undo/redo,
  Vertical repair, 40→50 mm source edit, exact save/reopen and 60 mm post-reopen
  edit. It saves the model, transcript, diagnostic and app/GPU captures externally.
- `sketch_conflict_controls_test` is prepared and compiled for native Qt input:
  initial activation, readable error and repair button, actual canvas badge hit,
  Delete, undo/redo and Vertical retry, with no inter-step focus repair. It must
  not be accepted until run on an unlocked desktop. The current locked macOS
  console blocks that gate; no repeated activation is attempted.

Separate remainder: rejected edits of an already accepted driving dimension with
certified culprit IDs and dimension-only explicit Driven recovery; general
coupled/sketch conflict diagnosis; broader branch choice; matched public pixels;
Windows/Linux native/GPU and packaged-runtime qualification. Existing generic
failed-dimension atomicity and existing Driven mechanisms do not establish those
additional diagnosis/recovery contracts.

## Candidate evidence receipt

The final build and focused core run passed: 2/2 `sketch_conflict` and
`sketch_solver` CTest cases. The prepared native Qt test target compiled; it was
not executed because the console is locked. Native button/badge/Delete behavior
therefore remains unqualified.

The final real-entrypoint API run passed 63 calls. Evidence is external at
`../Alloy-reference-records/validation/sketch-constraint-conflict-2026-10-19/api/`:
`receipt.json`, `orientation-repair.alloy`, `orientation-repair-edited.alloy`,
`conflict.png`, `selected-existing.png`, `repaired-50mm.png` and
`reopened-source-edit-60mm.png`. The failure and selected-badge captures were
inspected for readable persistent text and selection; repaired captures show the
complete vertical line and 50/60 mm annotations with no stale error. The final
replay adds Look At before the 50 mm capture after the initial horizontal camera
clipped its upper endpoint. This changes evidence framing, not modeling behavior.

Actual graphics reports Apple M4 / Apple, hardware accelerated, OpenGL
`4.1 Metal - 90.5`, 2× framebuffer; captures explicitly report
`QWidget chrome + OCCT OpenGL framebuffer` and viewport SHA-256. The private
application exited and no validation editor remains. These are actual-app API
and GPU screenshot results, separate from pending native/physical input and
matched-public-reference visual parity.
