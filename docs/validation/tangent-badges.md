# Tangent badge visual fix (#234)

The retained tangent and arc-tangent badges now draw an original curved trace
touching a straight line. Their anchor calculation, origin avoidance, 12-pixel
pick bounds, colors, relation IDs and solver path are unchanged. The symbol is a
bounded readability fix; exact visual parity with any reference is unknown.

The `alloy` app target compiled on macOS arm64. In the real app, the saved
`paper-clip.alloy` model was opened and its path sketch re-entered. The GPU
capture is outside the repository at
`../Alloy-reference-records/deliverables/paper-clip-2026-10-13/tangent-badges-234.png`.
It is 2880 × 1800 physical pixels at 2× scale; the viewport framebuffer is
1744 × 1142. Diagnostics reported OCCT OpenGL on Apple M4, version
`4.1 Metal - 90.5`. At normal zoom, the arc-tangent badges visibly show curved
traces touching their lines instead of empty boxes.

The shared control API reported ten constraint glyphs in sketch edit. Hovering
retained arc-tangent relation `a220f2cc-1cab-4181-bebd-17e47d68bbd0` set
`hoveredConstraintId` and the glyph's `hovered` flag. Picking viewport-local
`(396, 130)` in its unchanged `(390, 124, 12, 12)` bounds returned and
selected the same relation ID; its glyph reported `selected: true`. The sketch
edit was canceled and the private app exited without saving the source model.
The full tutorial workflow was not replayed for this visual change.

## Independent acceptance

Accepted candidate `acb2dd0` on 2026-10-13. Independent source review confirms
that only the tangent/arc-tangent stroke construction changes; anchors, pick
bounds, relation identity and selection/highlight routing are untouched. The
reported real-app hover/pick result above supplies the bounded interaction
check; it is shared API input, not a physical pointer claim.

The actual GPU capture was inspected both as a complete frame and as an
original-resolution crop. The badges now visibly depict a curved trace touching
a straight line. Their small normal-scale presentation is distinguishable from
the previous empty outline boxes; the crop confirms actual strokes rather than
missing font glyphs. This resolves the concrete paper-clip badge defect. Exact
reference symbol parity and broader constraint-icon styling are not asserted.
No tutorial replay or additional editor was needed for this isolated visual fix.
