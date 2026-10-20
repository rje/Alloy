# Bracket Activity 2 visual checkpoints

SKT-172/#695 bounded increment accepted 2026-10-20. Independent Sol
source/evidence review found no blocker for the recorded scope.
This refines the existing [A2 geometry/input fixture](bracket-midpoint.md).
Full public marking-menu layout remains a mismatch under SKT-173/#696 before
the original visual requirement can be considered closed.
Strict native activation/physical input remains SKT-171/#694.

The compact before/after specification is external:
`knowledge/specs/skt172-bracket-visual-2026-10-20.md`. Source IDs remain
`tutorial-illustrated-bracket-a2`; original source URLs, creator and retrieval
dates are in that record and its SKT-170/source-index links. No reference
software was operated. New controls invoke existing authoritative commands.

## Candidate changes

- The native geometry popup groups Delete separately, then Sketch Dimension
  and Normal/Construction with separate D/X shortcut columns, then the observed
  supported Collinear, MidPoint, Fix/UnFix, Parallel, Perpendicular,
  Horizontal/Vertical and Equal order. Existing action IDs remain stable.
- Authored selected sketch curves and their attached relation marks use blue.
  Additive selection, selected-relation identities, construction dashes,
  red Chamfer targets, point selection and transient hover behavior retain
  their separate existing semantics.
- Construction-line midpoint and circle/line tangent badges stay near their
  own midpoint/contact instead of joining unrelated vertical relation stacks.
  Origin-label collisions move these local badges left rather than farther
  above the caps. The triangle points upward in top view. Rendered strokes,
  diagnostic anchors and selectable bounds use the same shifted coordinates.

## Verification

Accepted build directory: `build-preview-release`, Release/private Qt 6.11.2,
AppleClang 17, OCCT 7.9.3. Build targets `alloy`,
`bracket_midpoint_controls_test`, `sketch_collinear_test`, `arc_midpoint_test`
with `-j4`.

The existing native Qt fixture adds actual menu order/shortcut checks and
retained glyph-anchor/hit-bound agreement. It retains the original actual-hit
A2 selection, construction conversion, triangle/square inference, exact
50-to-60 mm radius edits, stable relations, Undo/Redo and save/reopen checks.
It uses Qt event delivery with initial harness activation, not physical OS
input. The original explicit QContextMenuEvent delivery remains; this does
not qualify physical right-click dispatch.

Final Qt route passes 3/3 (17.4 s); focused core Collinear and Circle/Line
Tangent pass, and Arc Midpoint passes after correcting a pre-existing test-only
dangling temporary JSON lookup to an owned `.value("id")`. The initial Arc
Midpoint run crashed at that invalid lookup; no product geometry code changed.
The final framebuffer reports Apple M4 / Apple, OpenGL 4.1 Metal - 90.5,
1864×1232 native pixels for a 932×616 logical viewport. Captures are inspected;
all private validation windows exited, with no private editor left running.

External captures/receipts: `validation/skt172-2026-10-20/qt-final2/`.
The earlier candidate is `qt-final/`; it passed the fixture but visual inspection
found the lower origin-label avoidance still displaced badges upward, prompting
the final lateral correction. The external `comparison.html` links the public,
prior and candidate captures without shipping any reference assets.

## Per-checkpoint compatibility

| Checkpoint | Result | Evidence and remaining limits |
|---|---|---|
| Construction menu supported sketch order | Matched | Dimension, construction and the supported relation order match the observed linear block; actual action hit still converts both caps. |
| Full context/marking layout | Mismatch | The radial shell, broader navigation/modeling commands, icons and compact row metrics are absent/different; SKT-173/#696 owns the separate bounded implementation. |
| Selected end caps | Matched category | Blue selected caps replace orange; IDs and additive selection remain exact. Exact color/line-width parity is unknown. |
| Construction | Matched category | Dashed source caps remain separate from solid profile edges and excluded from profile detection. Exact dash metrics are unknown. |
| Inference cues | Matched category | Existing midpoint triangle and tangent square remain at actual inferred inputs; the final relations are inspected independently. |
| Retained badges | Improved; bounded mismatch | Midpoints/contact marks stay near caps and clear the 100/500/45 dimension labels in this state. Other equality/perpendicular marks and origin decoration differ from public placement. |
| Two-circle outcome/summary | Matched bounded geometry | Two end circles, dashed caps and three pale warm regions remain; the public activity summary contains later workflow content outside this A2 slice. |
| Native scale | 2x captured; 1x unknown | Connected Apple M4 screen reports DPR2; no native DPR1 screen qualified. |

Alloy before/after use a 1500×950 logical light-theme window, top XY view,
Fit then 0.75 zoom, unchanged fixture and camera. Public images are cropped
canvases with unspecified version, platform, DPR and viewport scale. The
comparison establishes categories/order/readability, never pixel-exact parity.
No CI, tag, push or release was performed.

The independent review inspected both archived public checkpoints and actual
final Apple GPU captures, checked the shared command routes and test-only JSON
ownership fix, and accepted the supported order, color category and badge
placement. It did not qualify physical native right-click, DPR1, other-platform
runtime or exact pixel identity. Those remaining requirements stay in
SKT-171/#694, SKT-173/#696 and the release gates.
