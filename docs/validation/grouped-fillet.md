# Grouped solid Fillet candidate

Status: isolated SOL-046 candidate, 2026-10-13. The public command contract and
source provenance are external at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/grouped-fillet-contract.md`.

One retained Fillet feature can hold independent constant-radius selection sets.
Each set stores its own stable ID, authored seed edge IDs, radius and Tangent
Chain On. The evaluator builds all sets on the same pre-feature solid, expands
tangent contours from current topology and refuses seeds assigned to different
sets when their contours overlap. It refuses missing or ambiguously mapped
edges rather than picking replacement geometry. Existing single-radius Fillets
remain readable and editable through the same native dialog.

The native Fillet dock shows the sets together with edge counts and radii. The
selected row receives canvas edge picks and its radius field; plus adds a set,
the remove control deletes the active set, and Clear empties only that set.
Constant Radius, Tangent (G1), Rolling Ball and Tangent Chain On are the
supported controls in this slice. The same `solid.fillet`, `solid.filletDialog`,
`command.update`, preview, commit and cancel paths serve local automation and
the native controls. The exact public default radius and historic row styling
remain unknown.

The focused core fixture uses an actual 50 × 80 × 40 mm primitive Box: its four
vertical edges receive R5 and eight top/bottom edges receive R2.5 in one
Fillet. It checks valid closed geometry, actual removed volume, one-step
undo/redo, edit of one group without changing the other, upstream width edit,
atomic duplicate/impossible-radius refusal and exact authored save/reopen. A
second fixture rounds vertical corners first, then proves one retained top-rim
seed expands into a multi-edge tangent contour, including after upstream width
recompute. `solid_fillet` and `grouped_fillet` pass headlessly.

The 31-call real-entrypoint API run passes draft preview/Cancel, one grouped
Fillet, valid closed geometry (158476.576 mm³), group edit, Box resize to
60 mm, atomic impossible-radius refusal, undo/redo, exact authored reopen and
one-component millimeter STL readback (4412 triangles, bounds 0–60 × 0–80 ×
0–40 mm). A focused Qt native check passes real viewport edge clicks into
different sets, numeric R5/R2.5 entry, plus/remove, preview/Cancel, grouped
OK and edit-dialog row reload. The settled native capture shows both rows,
their radii and the selected original edges; the API capture shows the
committed rounding. These checks use actual Alloy controls and GPU frames;
they do not establish OS-level physical input. Evidence and the API ledger are
external under `../Alloy-reference-records/validation/grouped-fillet-2026-10-13/`.
All private editor processes were closed after the checks.

Independent review is pending. This ingredient qualifies grouped Fillet on Box
stock; retained edges after future surface Thicken remain a separate tutorial
dependency.
