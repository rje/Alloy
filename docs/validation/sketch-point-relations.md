# Contextual sketch point relations

SKT-127/#552 accepted bounded scope, 2026-10-18. Independent Sol review found
no blocking mismatch.
The bounded public-source contract is external at
`../Alloy-reference-records/knowledge/specs/skt-023-sketch-freedom-presentation-split-2026-10-18.md`
(P5/P7); implementation decisions and limitations are recorded in external
`validation/sketch-point-relations-2026-10-18/contract.md`.

With Show Constraints off, selecting an authored curve reveals the existing
renderable relations attached to its entity or owned points. Selecting a point
reveals its attached point relations. Point-point Coincident has no persistent
glyph or unselected point dot; selecting either endpoint reveals a contextual
badge. Point-curve Coincident retains its normal glyph when global display is
on. Clearing selection removes contextual badges. A selected contextual badge
stays visible until deselected and supports existing hover, selection, Backspace
and context Delete behavior. Existing always-visible badge behavior is retained.
`sketchConstraintGlyphs` reports stable relation targets and `contextual` state.

Point context menus expose **Delete Coincident** for attached point-point and
point-curve relations. Each action carries its stable authored relation ID;
`sketch.deleteCoincident {constraintId}` uses the same existing solved deletion
command and draft undo authority. Unrelated-point, wrong-family and stale targets
are refused. There is no new solver representation or save schema. Visibility
and selection do not author edits. The remaining glyph-family inventory belongs
to SKT-128/#553; this change does not add glyphs for unimplemented families.

The single fixture has four open lines, two distinct joined endpoint IDs, one
point-on-line Coincident, a fixed join point and one Horizontal relation. Native
Qt-delivered input toggles Show Constraints, selects the curve and both relevant
points, opens the actual native popup, observes activation, clicks the individual
Delete Coincident action, and checks authored relation removal. Undo/Redo restores
the exact drafts. A contextual Horizontal badge is clicked and removed with
Backspace, then restored. The saved/reopened baseline retains both Coincident
relations and has no permanent point-point glyph. Initial harness activation is
separate; there is no inter-step focus/selection repair. Qt widget input is not
physical OS input. Two Qt top-level-window warnings accompany successful observed
popup activation/closure; the test does not bypass the actual popup hit target.

`sketch_point_relations_controls_test` passed **3/0/0**. The separate
`tests/sketch_point_relations_api.py` launches the real application and exercises
the local server, contextual reveal, stable-ID refusals, deletion, exact draft
Undo/Redo, baseline save/reopen and separately saved deletion/reopen. Its final
receipt and transcript pass. Build: `build-preview-release`, Release, Qt 6.11.2
from the coherent private runtime, OCCT 7.9.3, arm64 AppleClang 17. The accepted
cache has no explicit `CMAKE_OSX_SYSROOT` override.

Evidence lives outside the repository at
`../Alloy-reference-records/validation/sketch-point-relations-2026-10-18/`:

- `native-test.txt`, `native/receipt.json`, and `native/point-relations.alloy`.
- `native/point-point-reveal.png`, `native/point-curve-menu.png`, and the observed
  settled `native/reopened.png` were inspected. Both menus and selected relation
  badges are readable. The reopened capture was taken after deferred dock layout.
- `api/receipt.json`, `api/point-relations-api.alloy`,
  `api/point-relations-deleted.alloy`, and `api/point-relation-api.png`.

Actual GPU state reports **Apple / Apple M4 / OpenGL 4.1 Metal - 90.5**, hardware
acceleration, native viewport 932 × 666 and framebuffer 1864 × 1332 (2× scale) in
the native fixture. The API run records its separate viewport dimensions. These
are application/native framebuffer captures, not mockups. The fixture contains
no solid or closed profile; downstream modeling is outside this presentation and
relation-deletion change. Exact reference pixels, placement, multi-relation menu
organization and current-version timing remain unknown. Windows/Linux runtime
and physical OS input are not qualified. All private validation editors were
closed; the user-owned editor was preserved.

Wrong-family refusal and contextual hover/geometry highlight were source-reviewed;
the retained fixture directly exercises stale/unrelated refusal and contextual
badge selection/Backspace. The native popup log has two Qt top-level-window
warnings, while observed activation/closure and all three checks pass.

Reproduce with the single private GUI lease:

```sh
cmake --build build-preview-release --target alloy sketch_point_relations_controls_test -j 6
ALLOY_POINT_RELATIONS_EVIDENCE=/absolute/external/native build-preview-release/sketch_point_relations_controls_test
python3 tests/sketch_point_relations_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --fixture /absolute/external/native/point-relations.alloy --output /absolute/external/api
```
