# Sketch relation glyph inventory

SKT-128/#553 accepted bounded scope, 2026-10-18. This gate records the existing ten rendered
relation families and the fixed-point partial state. Independent review found
the Vertical mark overlapped its target line. The renderer now offsets Vertical
12 logical pixels laterally, carrying the same offset into its stroke, anchor
and hit bounds. Independent Sol review accepted the corrected captures.
The missing geometry Fix/UnFix and repeat-tool unfix workflow is explicitly
preserved in Todo SKT-133/#558 under SKT-023/#164.

The permitted public contract and provenance are external:
`../Alloy-reference-records/knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`,
refs P1–P6, retrieved 2026-10-18. These document semantics and interactions;
there is no matched reference screenshot or measured icon-pixel comparison.

## Compact workflow contract

Create a sketch on XY, enter Select, and leave Show Constraints on (the default).
Each retained visible badge reports its relation ID, semantic kind, affected
entity/point IDs, viewport-local bounds and selection state through
`sketchConstraintGlyphs`. Clicking a badge selects that relation and highlights
its affected geometry. The shared edit action is Delete/Backspace, followed by
recreation if necessary; these relations have no numeric per-badge edit field.
Selection changes presentation without authoring geometry. Deletion is one
undoable draft edit; Finish commits, Cancel discards the session, and saved
models retain the authored relation IDs and targets.

| Family | Fixture and result |
|---|---|
| Point-on-curve Coincident | Standalone point on a finite line; square Coincident mark and both targets matched. Native Backspace/Undo passed. |
| Horizontal/Vertical | Separate lines plus a two-point Horizontal relation; distinct H/V marks and line/point targets matched; Vertical now clears its line. Native Backspace/Undo passed for both Horizontal paths. |
| Parallel | Two oblique lines; paired slashes on each line, same relation ID and both targets matched. |
| Perpendicular | Two separate perpendicular lines; corner marks and both targets matched. |
| Equal | Two separate equal-length lines; paired bars and both targets matched. |
| Tangent | Circle touching a finite line; contact-anchored tangent mark and circle/line targets matched. Native Backspace/Undo passed. |
| Concentric | Two circles seeded with distinct center points; one center badge and both circle IDs matched. |
| Midpoint | Standalone point at a line midpoint; triangle mark, point and line IDs matched. |
| Collinear | Separate segments on one oblique support line; shared-line mark and both targets matched. |
| Symmetric | Mirrored lines and construction axis; axis-anchored mark, all three entities and retained endpoint targets matched. Native Backspace/Undo passed. |
| Fixed point, partial | Stored fixed relation and green standalone point matched; stable-ID API deletion and exact Undo/Redo passed. No permanent Fix badge is expected. Geometry targeting/native unfix mismatch remains #558. |
| Point-point Coincident exception | Retained distinct point IDs have no permanent badge or unselected dot. Accepted contextual reveal/Delete Coincident remains [SKT-127](sketch-point-relations.md). |

“Matched” above is bounded to documented family semantics and observed Alloy
state/presentation. Every rendered relation in this fixture was selected using
its actual Qt viewport hit target, then inspected in a capture. Precise public
icon strokes, highlight colors, placement, current layout, physical OS input
and cross-platform GPU behavior remain unknown. Tangent variants reuse their
accepted ingredient receipts; this gate uses one circle/line pair.

## Verification evidence

`tests/sketch_glyph_inventory_controls.cpp` creates one mixed sketch with 26
entities, 14 retained constraints and 15 visible badges. Its 12 inventory
relations cover 11 semantic kinds (Horizontal/Vertical is one family), including
the separate two-point Horizontal path. It verifies semantic identity, target
IDs, positive bounds, native pick identity and selection without authoring an
edit. The Vertical hit bounds must also clear its line, including after reopen. It asserts no redundant constraints. The sketch remains intentionally
underconstrained at 65 DOF; it is a relation inventory rather than a closed-part
modeling lesson.

Initial harness activation precedes native input. No focus or selection repair
occurs between badge clicks and keys. Each click must focus the viewport before
Backspace. Five representative deletions cover point/curve, single-line,
point-pair, tangent and symmetry targets; Undo restores the exact authored draft.
Broader common controls, hover, context Delete and stale/hidden-target refusal
reuse the accepted [SKT-008 badge path](constraint-interaction.md). The fixture
also checks the fixed-point stable deletion and Undo, then exact authored
save/reopen, IDs and glyph availability. The corrected Qt run passed 3/3 in 12.6 seconds.

`tests/sketch_glyph_inventory_api.py` opens that same saved fixture through the
real application entrypoint. Its 65-call run independently checks all glyph
identities/targets, nonauthoring selection, fixed-point deletion/Undo/Redo, exact
authored Finish/save/reopen and a hardware framebuffer capture. Native input is
the separate Qt evidence, not a claim about Python pointer commands.

External evidence root:
`../Alloy-reference-records/validation/sketch-glyph-inventory-2026-10-18/`.
`native/receipt.json`, `native/test.log`, the saved
`native/mixed-glyph-inventory.alloy`, all `native/selected-*.png`, and
`api/receipt.json` preserve the evidence. The corrected unselected and selected Vertical captures
were inspected again; its mark is now clearly separate from the line at native
creation, selected state and reopen. The remaining selected-family captures
were inspected in the initial run for distinct readable marks, affected blue geometry highlights,
construction-axis styling and the green fixed point. The clean final
`api/reopened-glyph-inventory.png` confirms the retained presentation. Native
`reopened.png` was taken before the palette completed its layout; use the API
capture for settled reopen presentation, not that intermediate palette spacing.
The `before-vertical-fix/` evidence predates the reviewer-found Vertical overlap
correction and is superseded by the current `native/` and `api/` captures.
The earlier `native-first-fixture/` is also superseded: coincident initial circle
centers shared a point and made Concentric redundant. The corrected accepted
fixture seeds separate centers and asserts no redundancy.

Build: `build-preview-release`, Release, AppleClang 17, Qt 6.11.2 private SDK,
OCCT 7.9.3. Both runs identify hardware Apple M4, Apple vendor, OpenGL
`4.1 Metal - 90.5`, 4x MSAA and 2x display scale. Native viewport 932 × 666
logical / 1864 × 1332 framebuffer; API viewport 872 × 566 / 1744 × 1132.
Capture source is QWidget chrome plus the OCCT GPU framebuffer. Native creation
uses 140% zoom; reopen uses the normal 100% fit, so these are not pixel-matched
comparisons. Resize/display-scale qualification remains the accepted viewport
foundation, not a new test in this inventory. Private editors exited; the
user-owned editor was preserved.

Reproduce sequentially with an absolute external evidence directory:

```sh
cmake --build build-preview-release --target alloy sketch_glyph_inventory_controls_test -j 6
ALLOY_GLYPH_INVENTORY_EVIDENCE=/absolute/external/native build-preview-release/sketch_glyph_inventory_controls_test
python3 tests/sketch_glyph_inventory_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --fixture /absolute/external/native/mixed-glyph-inventory.alloy --output /absolute/external/api
```
