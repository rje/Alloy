# Equal-distance solid Chamfer candidate

SOL-028 (#201), 2026-10-13. Bounded child of SOL-005 (#50).
Public source contract: `tutorial-library-die`; provenance and instructions live
externally in `knowledge/tutorials/warwick/library-die-ledger.md`.
The source asks the learner to compare Chamfer and Fillet, cancel the trials,
then choose one treatment. It specifies selecting every edge with a window.
The 20 mm cube and 1 mm equal distance are Alloy fixture choices, not source defaults.

## Implementation contract

`solid.chamfer` and `solid.chamferDialog` accept retained `targetBodyId`,
`edgeIds`, canonical `distanceMm`, optional `name` and `featureId` for editing.
Only Equal Distance is exposed. Explicit selected seeds go to OCCT's symmetric
distance edge Chamfer builder; no two-distance, angle or tangent-chain options
are represented by inert controls. The dock reports selected edge count, offers
Clear selection and Select all edges, and a Distance field with real disposable
GPU preview, OK/Cancel and Escape. Individual visible edge clicks toggle retained
seeds through the existing authoritative selection command. Select all edges
uses the retained target, or the only body if no target was selected.

The source's drag-window selection is **mismatch**: this candidate supplies an
explicit all-edge control and existing additive individual clicks. It does not
implement the documented window gesture or claim exact interaction compatibility.
Human controls and GPU presentation need independent app inspection. Matched
reference screenshot comparison is unknown because the public lesson provides no
matched parameter-panel/final-model capture.

One Chamfer feature modifies the retained body. Parameter editing evaluates the
prefix source for selection and the full candidate for downstream consistency.
Authored IDs persist; changed/deleted edges refuse rather than rematching by
coordinates. Modified source edge descriptors become explicitly unavailable for
later commands. No generated bevel-edge naming is claimed. Impossible distances
fail before committing document/history. Two-distance and angle variants and
broader chain options remain in SOL-005.

## Focused checks and remaining verification

The isolated Debug macOS build compiles. `solid_chamfer` and `solid_fillet` CTest
checks pass: twelve retained extrusion cube edges, real equal-distance volume
reduction, disposable evaluation, one retained valid closed solid, editable
distance, upstream extrusion height edit, exact undo/redo and save/reopen,
missing-edge diagnostics and impossible-distance atomic refusal.
`tests/solid_chamfer_api.py` is prepared for actual app/API captures and history;
it has not yet run. Independent native controls/visual qualification remain open.
No full die workflow, release, other-platform runtime or issue closure is claimed.
