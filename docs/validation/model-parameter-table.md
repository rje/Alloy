# Model parameter table candidate (#276)

Status: independently accepted and integrated, 2026-10-13. The bounded public
workflow contract and source links are outside the repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/model-parameter-table-contract.md`.

Solid Modify → Change Parameters now shows User Parameters and grouped Model
Parameters in the same dialog. Model rows are derived from authored feature
data and identified by stable feature IDs plus a field, dimension ID, or
open-chain Offset group ID. The Expression cell accepts an existing user
length name or a numeric mm/in literal; Value is the resolved read-only
length. A direct name stores its source UUID, so renaming the user parameter
preserves the binding. A numeric replacement removes the binding even at the
same value. Invalid expressions and downstream evaluation errors leave the
previous authored document unchanged.

Rows cover Box width/depth/height, Shell thickness, driving sketch length
dimensions, Extrude distance and authored start offset, To Object end offset,
and retained open-chain sketch Offset distance. To Object has no invented
driving distance row. The open-chain binding retains the authored side sign
while its positive Clearance source changes. Native Expression editing and
`document.modelParameterSet` use the same document commit authority; API
callers take the exact `modelParameters[].target` from `document.inspect`.

The focused headless `model_parameter_table` test constructs a literal
Box → Shell plus slanted length dimension, then rebinds after authoring and
checks stable IDs, rename propagation, Box/Shell recompute, impossible Shell
rollback, same-value numeric unbind, undo/redo and save/reopen. It also creates
an actual retained five-line Offset, binds Clearance through the table,
checks its inward side and recomputation, and replaces the binding with a
literal. The neighboring `named_length_parameters`, `extrude_to_object`, and
`open_chain_offset` focused tests pass. The real-app API fixture passes the
seven-row Box/Shell/Line/To Object/open-chain Offset sequence, including
74 × 40 × 100 Box resizing, Clearance 0.25 → 0.5 with the inward Offset sign
retained, undo/redo and reopen. The Qt native test double-clicks the Expression
cell and types a user name, an unavailable name, and a same-value numeric
replacement; focus and authored binding states are asserted after each step.
Initial window activation is harness setup, with no focus or selection repair
between the demonstrated input steps. The settled seven-row dialog capture is
`../Alloy-reference-records/deliverables/model-parameter-table-2026-10-13/api-seven-final/bound-model-table.png`;
the adjacent `result.json` records authored/evaluated states and command
transcript. The native bound-row capture is `bound-table.png` in the parent
evidence directory. Actual Qt event injection qualifies the native route;
physical keyboard/mouse hardware is not claimed.

General arithmetic expressions, user-parameter deletion, model-name-driven
formulas, and broader sketch/solid consumers remain outside this bounded
ingredient. The full sliding-lid compatibility gate is separate.
