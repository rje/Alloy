# Origin and endpoint snapping checkpoint

Verified 2026-10-12 on the isolated macOS hardware-accelerated tutorial candidate.
This is bounded SKT-005 evidence, not acceptance of the full illustrated tutorial.

Pointer-authored lines and rectangles acquire the origin or existing sketch points,
show a transient cue, and retain real coincidence constraints. The origin has a
stable fixed point identity. Chained line segments retain connected endpoints;
explicit-coordinate API creation remains unchanged. Hover and rejected solves do
not author geometry. The 10-logical-pixel radius and ambiguity threshold are Alloy
implementation choices, not verified source-product constants.

Evidence outside git, under `build-tutorial/evidence/`:

- `snap-core-final.txt`: focused solver tests, including transactional failure and
  retained relationships. Test JSON references now own their values.
- `sketch-snaps/sketch-snaps-api.json`: real application pointer preview and cue
  acquire/release, constrained 30 x 20 profile, 5-mm extrusion, width edit to40,
  committed history and save/reopen. Captures accompany the receipt.
- `snap-native-outline.png`, `snap-native-state.json`, `snap-native-document.json`:
  native L, five physical clicks and Escape create four lines with one fixed origin
  and five coincidences. Initial empty XY sketch was opened through the API.
  Native Finish Sketch succeeds. Geometry remains six-DOF, intentionally without
  automatic H/V relations; this is not a dimensionally exact rectangle exercise.
- `snap-native-history.json`: API undo/redo and save/reopen preserve the native
  authored result. `snap-native.alloy` is the saved document.
- `snap-angular-regression/report.json`: existing angular/extrusion workflow passes
  through the registered ledger case against this candidate.

Astra independently reviewed source, API receipts, and captures: no material
blocker. The small origin cue overlaps the axis marker and remains a visual polish
item. Separate native acquisition of an existing non-origin endpoint after Escape,
a second display scale, and ambiguous-candidate interaction were not qualified.
Undo of individual accepted draft edits remains SKT-020 (#156); only committed
sketch history is claimed here. Human build-input was untouched. No CI/release ran.
