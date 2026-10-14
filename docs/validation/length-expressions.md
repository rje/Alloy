# User length expressions (PAR-006)

User parameters accept bounded typed arithmetic in the existing Change Parameters
table and local commands. An authored formula stores its text and the stable UUID
of each identifier occurrence. Its evaluated millimeter value is derived for
model bindings, dialog values and inspection; literal-only saved files retain
their original records. Renaming a source rewrites whole identifier tokens while
preserving those UUIDs. Invalid units, arithmetic, cycles or dependent geometry
refuse an atomic document change.

The supported grammar is decimal numbers, optional `mm`/`in` suffixes, user
length names, parentheses, unary signs and `+ - * /`. Bare numbers are scalars
during arithmetic; a final scalar uses the parameter row's unit. For example,
`Width*2` and `Width+5 mm` work, while `Width+5` refuses. Changing the row unit
changes a scalar-only formula's physical value; an explicit-length result keeps
its physical value. Functions, angle expressions, anonymous formulas in model
fields and broader unit algebra remain in PAR-001. Cycle diagnostics identify
the cycle condition without displaying its full dependency path.

Focused core checks cover formula evaluation through an existing bound Box,
repeated references, rename, incompatible arithmetic, cycle/depth refusal,
undo and reopen. The real-app API fixture creates an 80×40 rectangle driven by
`Length=Width*2`, extrudes it 5 mm, then edits Width 40→45: Length becomes 90
and volume changes 16000→20250 mm³. Rename, refusal, undo/redo, save/reopen and
actual GPU/table captures pass. A separate Qt-delivered native double-click and
typed edit of the user Expression cell passes with actual editor focus checked.
This does not claim OS-delivered typing or a direct anonymous model-field formula.

Detailed evidence and the dated public-source contract are outside the repository
under `../Alloy-reference-records/deliverables/length-expressions-2026-10-14/`
and `../Alloy-reference-records/knowledge/specs/par001-length-expression-slice-2026-10-14.md`.
