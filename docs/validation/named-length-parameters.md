# Named length parameters candidate (#244)

Status: isolated implementation candidate, 2026-10-13; independent review pending.
The public exercise contract is indexed outside the repository at
`../Alloy-reference-records/knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`
and its bounded implementation mapping at `implementation-reference.md` in the
same directory. The source calls for named Length, Width and Thickness values,
two sketch dimensions that use names, and an Extrude distance that uses a name.

The document now owns an optional `userParameters` table. Each row has a stable
UUID, unique direct-reference name, `mm` or `in` display unit, and canonical
`valueMm`. A driving length dimension keeps `parameterId` alongside its solved
`valueMm`; Extrude keeps `distanceParameterId` alongside `distanceMm`. Values
are refreshed by ID before structure validation, sketch solving and ordered
solid evaluation. Names resolve only when authored, so a rename preserves
dependencies. A numeric dimension or extrusion edit deliberately replaces its
binding. Legacy numeric documents remain valid. Invalid names, units, values,
missing references and failed downstream geometry leave the prior authored
document, evaluation and revision intact.

`document.parameterAdd` and `document.parameterUpdate` are shared local API
commands. `document.inspect` reports rows and retained bindings in `authored`.
Sketch dimension creation/update accepts `parameterName` in place of a numeric
value; the existing inline dimension editor accepts the same direct name.
Extrude accepts `distanceParameter` in place of `distance`. Its existing native
Distance control accepts either a signed literal or a direct name, shows the
name on reopen, refuses an unknown name, and preserves the binding on an
untouched Return. Changing the field to a numeric literal removes the binding
even if the evaluated number is unchanged. Solid Modify → Change Parameters
opens a native row table and Add/Update controls. No expression parser,
arithmetic, dependency graph, deletion, or parameter binding on other feature
properties is claimed.

Focused `named_length_parameters` core test passes Length 75, Width 50,
Thickness 10 through a constrained rectangle and real OCCT extrusion; edits
resize sketch and solid. It checks `in` conversion, stable ID after rename,
numeric cache, undo/redo, save/reopen, duplicate/negative/missing-reference
refusal, and atomic rollback when a narrowed bound width makes a downstream
fillet impossible. The `named_length_parameters_controls` Qt test exercises
actual native table typing, a rough Center Rectangle followed by D, side
pick, label placement and direct Length entry, inline dimension-name editing,
Distance-name typing, unknown-name refusal, same-value literal replacement,
bound edit Return, and GPU preview/solid captures. Initial window activation
is harness setup; no focus or selection repair occurs between demonstrated
native steps. The fresh D dimension uses actual Qt key and pointer events;
the separate edit was opened through shared `sketch.dimensionEdit`. The D
capture was taken after queued layout settled; its dimension, status and
profile labels are readable without overlap.

`tests/named_length_parameters_api.py` passed through the actual Alloy app
entrypoint, exercising the same commands, geometry, update, undo/redo,
duplicate-name refusal, native save/reopen and screenshot. Evidence is at
`../Alloy-reference-records/deliverables/named-length-parameters-2026-10-13/`:
native `named-length-native-create.png`, `named-length-extrude-preview.png`, `named-length-solid.png`,
`named-length-state.json`; API `api/named-length-api.png`,
`api/named-lengths.alloy`, `api/api-state.json`, `api/api-transcript.json`,
and `api/app.log`. The inspected captures show readable native controls and
solid/profile presentation on Apple M4 OCCT OpenGL 4.1 Metal. This is a
bounded parameter slice, not the complete countersink tutorial or a
cross-platform runtime qualification.

## Independent acceptance

Accepted `56c4619d09d2660ecf07a9800a234c60f598b96d` for bounded PAR-002/#244
following source, test, actual capture and persisted API artifact review. Stable
parameter IDs survive name changes; resolution precedes dependent evaluation,
and failed evaluation does not publish a partial document. The final core log
passes in 0.46 s, including invalid downstream Fillet rollback. General
expression arithmetic, deletion and other feature-property bindings remain
outside this candidate.

The corrected ordinary Distance control accepts a name or literal. Untouched
bound Return preserves the ID; an explicitly typed same-value literal plus Tab
removes it. Explicit numeric edit overrides no longer rehydrate the old binding.
Unknown-name input disables acceptance instead of silently committing the prior
preview. The native test source and reported successful run cover those paths.

The separate new-dimension case uses shared parameter/sketch/tool setup, then
actual Qt rough-rectangle clicks, D, side pick, pointer preview, separate label
placement and Length/Return. It asserts the retained parameter ID and 75 mm
value. Only initial activation is forced; subsequent focus is asserted rather
than repaired. The other named dimension edit deliberately uses shared
`sketch.dimensionEdit` activation and is not described as native selection.
The inspected settled D capture clearly shows 75 and one remaining DOF with
nonoverlapping palette text; the Distance capture clearly shows Thickness in
its ordinary field. The finished sketch annotation displays the evaluated
number, not the source figure's fx marker; no exact annotation styling claim
is made. Parameter-table typing is exercised by Qt, but these inspected images
do not establish pixel fidelity for that dialog.

Independent readback of the 27-call actual-app transcript, saved model and
`api-state.json` confirms retained Length/Width dimension bindings and Thickness
extrusion binding, values 85/50/10 after the edit, volume
42499.999999999985 mm3, and saved authored equality. History/reopen and the
intentional duplicate-name refusal are recorded in that script/transcript.
Core, Qt input, actual-process API and GPU images are distinct evidence; the
complete parameterized countersink workflow remains #249. No extra GUI run or
unchanged test replay was needed for this review.
