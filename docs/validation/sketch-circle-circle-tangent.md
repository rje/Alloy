# Existing-circle pair Tangent — SKT-116

Accepted 2026-10-18 for explicit Tangent between two authored circles through the shared sketch command and solver. The relation retains the nearest external or internal branch from current solved geometry. Equidistant branch choices and coincident centers refuse; internal contact keeps the originally outer circle as outer through edits. These branch rules are declared Alloy policies because public sources do not settle them.

The real-app workflow passed 75 API calls. External contact kept two separate selectable circle regions while radius changed 5→7 mm and a center offset changed 3→6 mm; a separate internal fixture kept an outer radius 15 mm and inner radius 5→7 mm as center distance changed 10→8 mm. Reversing the outer radius order refused atomically. The relation ID, authored Undo/Redo and saved reopen remained stable. Focused core checks cover external/internal solve and refusal. Native Qt input passed 3 tests for both pick orders and selectable/deletable contact badge without focus repair.

Inspected captures show the contact badge and edited circles on a hardware-accelerated Apple M4 OCCT OpenGL 4.1 Metal viewport. Native input is Qt-delivered, not physical OS qualification. Exact reference layout/glyph parity and other-platform runtime remain open.

Saved fixtures, logs, captures and the structured real-app result are at `../Alloy-reference-records/deliverables/sketch-circle-circle-tangent-2026-10-18/` (`api-final`, `native-01`, `core-final.log`). The external dated family contract is `../Alloy-reference-records/knowledge/specs/skt-022-remaining-analytic-constraints-2026-10-18.md`. Arc-line interior and other curve pairings remain in linked Todo issues under SKT-022.
