# Component printing selection

PRT-010/#351 independently accepted, 2026-10-14. Component STL selection includes its visible
bodies. A hidden retained source sheet no longer prevents exporting the visible
solid in the same component. Explicit body selection is unchanged: selecting an
open sheet still refuses strict solid-mesh export. A visible sheet in a component
also refuses rather than silently disappearing from the result.

The native mesh dialog labels the component selection “visible bodies”. The
historical public example and visibility-policy qualification are recorded outside
the repository in `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/component-mesh-visibility.md`.
Current official documentation does not settle hidden-body filtering. Intentional
non-solid mesh export and nested component selection remain separate work.

The application build and actual-app runner pass. Evidence is external at
`../Alloy-reference-records/deliverables/component-surface-mesh-2026-10-14/api-02/`.
The actual dimensioned Loft/Thicken solid moves into its retained sheet component,
survives save/reopen, and exports exactly one visible body. Independent binary STL
inspection finds one closed connected component, 2308 triangles, consistent normals
and positive volume 17505.7868 mm³, within 0.1% of the solid. Explicit-sheet and
visible-sheet component selections refuse with `solid_required`; undo restores the
authored state after sheet visibility changes. Reparent undo/redo was not tested
in this run. The inspected GPU capture shows the correct component and readable
visible-body selection note. These API checks do not establish physical mouse input.

The first run exposed a real component-move prerequisite: sketch constraint
recomputation evaluated an earlier modeling prefix with final Browser ownership
records for a not-yet-created body. Removing those records only from the temporary
prefix matches the existing history evaluator; final ownership validation remains
intact. The corrected run includes reparent, save/reopen and history regression.
Independent source, ledger, mesh and capture review accepts this bounded change.
