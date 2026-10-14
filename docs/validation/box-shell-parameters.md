# Box and Shell direct named lengths candidate

Status: PAR-004 independently accepted and integrated, 2026-10-13; see acceptance below. The bounded public workflow contract is recorded outside the repository at `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/independent-spec.md`, PAR-004; its source provenance is in that record.

Primitive Box width, depth and height now accept direct names for document user lengths; the source workflow's Width, Length and Height map to those fields respectively. Sharp inward Shell thickness accepts a direct name. Both store stable `*ParameterId` fields alongside canonical millimeter caches. `CadDocument::resolveLengthBindings` refreshes those caches before structure validation, sketch solve and ordered body evaluation. A rename preserves the ID and dependency. A numeric field edit replaces its binding, including a typed literal equal to the current value. Unknown names refuse; native invalid input blocks commit until corrected or canceled. Legacy literal documents remain valid.

The shared API exposes `widthParameter`, `depthParameter`, `heightParameter` on `solid.box` and `thicknessParameter` on `solid.shell`, including create, session updates and timeline edit. The existing Box and Shell native fields accept typed direct names and show them on edit reopen. There is no arithmetic parser or binding for other consumers in this slice. Editing already-created model parameters from the parameter table remains a later requirement. This candidate does not make a primitive Box face selectable for Shell; that topology prerequisite is separate, so the Shell ingredient check uses an accepted extrusion stock fixture.

Focused `named_length_parameters_test` covers bound Box geometry, ID preservation through rename and value update, missing-ID refusal, undo/redo and save/reopen. `solid_shell_test` covers bound thickness, rename/update, impossible-thickness rollback, numeric replacement and existing Shell geometry/history. Both pass. `box_shell_parameters_controls_test` passes actual Qt typing of Box and Shell names, unknown-name attempted commit, same-value numeric replacement and edit reopening (3/3, 4.7 s). Initial window activation is harness setup; demonstrated native field steps do not repair focus or selection. `tests/box_shell_parameters_api.py` passes through the real app entrypoint: bound Box and Shell, a sketch length sharing Thickness, valid update, invalid Shell recompute, undo/redo and authored save/reopen.

The inspected Apple M4 GPU captures show the three Box names in native fields, the Shell name and hollow preview, and resulting Box/Shell bodies. Evidence, saved models, API state/transcript and app log are outside the checkout at `../Alloy-reference-records/deliverables/box-shell-parameters-2026-10-13/`. The native and API runners use independent documents. Their evidence establishes this bounded ingredient; the complete advanced tutorial, actual Box→Shell chain, precise visual matching and Windows/Linux runtime remain separate gates.

## Independent acceptance and integration

Astra independently accepted the bounded ingredient on2026-10-13 after source,
focused core, real-entrypoint ledger and actual GPU capture review. Detailed
review is external at knowledge/tutorials/advanced-audit-2026-10-13/par004-independent-review.md.
The shared main build also passes tests/sliding_box_prefix_api.py: named
50×80×40 Box/Shell4, resize to74×40×100 with Shell3, stable inner back
face/edge IDs, exact history and authored reopen, and inspected hardware GPU
capture. Evidence: external deliverables/sliding-box-2026-10-13/prefix-01.
This is the integrated box prefix; lid, dovetail and motion remain unimplemented.
