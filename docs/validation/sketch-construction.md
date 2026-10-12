# Construction line checkpoint

SKT-016 (#144) partial, 2026-10-12. Line conversion only; curve construction,
inference, sketch fillets and region extraction remain separate tutorial blockers.

`construction` is an optional typed line property (absent means normal). Conversion
retains IDs, points, dimensions and geometric relations; the same solver continues
to evaluate it. Profile derivation excludes construction entities before graph and
provenance generation. Draft/finished displays retain dashed selectable geometry.
X, context menu, palette and API invoke shared conversion commands.

## Evidence

- `build-tutorial/evidence/construction-core.txt`: focused sketch_curves regression
  passes conversion, retained constraints/IDs, width edit, profile restoration,
  JSON persistence and invalid mixed selection refusal.
- `build-tutorial/evidence/sketch-construction-ledger/report.json`: real-app coherent
  rectangle/diagonal workflow passes selected conversion, exact dashed draft IDs,
  profile exclusion, dimension edit25, draft Undo/Redo, Finish one history entry,
  committed history and save/reopen.
- First runner attempt reached storage reset then exposed a missing required
  discard argument in the test; corrected final run passed.
- Astra source review found the selected overlay masked dashes; final presentation
  correction passed independent capture review: selected cap stays orange dashed.
- Native physical cap pick plus X converted exactly that ID and retained its
  dimensions/constraints. `construction-native-selected.json/.png` records it.
- Native right-click, Down, Return converted an API-seeded selected line;
  `context-menu-sequence.json` proves the same ID became construction. An AX
  inspection between opening and selecting dismissed the popup; batched input
  works. Viewport input counters distinguish routing from popup observation.
- Accessible Perpendicular toggle now invokes the authoritative tool command;
  `constraint-accessible-active.json` verifies it.

## Limits

No exact Activity2 pass. Native multi-selection and full source visual comparison
remain separate. No CI/release. Full SKT-016 stays open for its remaining scope.
