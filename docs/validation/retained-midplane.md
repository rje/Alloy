# GEO-005: retained Midplane candidate

`construction.midplane` creates or edits one construction plane from two ordered,
exact planar body-face references. It stores both producer/body/face identities,
not a frozen offset. The plane sits halfway between the current parallel
support planes; its sketch frame follows the first selected face. The visible
datum border fits the selected faces without changing that geometric frame.
Coincident, nonparallel, missing and consumed supports fail atomically.

The native Midplane dock takes first and second canvas clicks, shows both
selected faces and a live preview, enables OK only for a valid pair, and
supports Cancel and Escape. The local API uses the same face-selection session
and authored command; direct creation and edit accept `faces`, optional `name`,
and optional `featureId`. Edit preserves the plane ID. The existing Browser,
Timeline, construction-plane sketch and document history use the same feature.

Focused `midplane` core test passes a Box 50×80×40 top/bottom pair, opposed
normals, a construction-plane sketch, source-height edit moving both plane and
sketch from z20 to z30, undo/redo, save/reopen and invalid-pair rollback.
Focused `midplane_controls` test passes two real Qt canvas face clicks, preview,
Cancel, commit and edit reopen. The real-entrypoint
`tests/midplane_api.py` ledger passed 33 calls with exact authored references,
dependent sketch/source edit, rollback, history and reopen. The final native
capture was inspected for readable selections/status and a fitted midpoint
border. API evidence preceded the display-only fit refinement; the final core
and native tests passed after that refinement. The private editor is closed.

Evidence and the independently authored public command contract are outside
Git at `../Alloy-reference-records/deliverables/retained-midplane-2026-10-13/`
and `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/midplane-contract.md`.
The broader construction geometry parent and enclosure Split Body remain
separate work. The source's precise datum shade, extent controls and normal
orientation are not established by the public evidence; the first-face
orientation is an Alloy convention.


Independent Astra review accepted the final source and readable fitted preview.
Integrated as b2136f2; main application build and focused midplane core pass.
GEO-005 is closed.
