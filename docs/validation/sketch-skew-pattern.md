# Nonorthogonal rectangular sketch Pattern candidate

SKT-183/#706, 2026-10-20. Extends the accepted retained rectangular
[Pattern core](sketch-rectangular-pattern.md) and
[canvas interaction](sketch-pattern-canvas.md) to two distinct nonparallel
construction Lines. Circular Pattern behavior and retained storage are unchanged.
Root owns backlog, inventory and continuation-context updates.

The permitted public contract is external in
`../Alloy-reference-records/knowledge/specs/skt097-sketch-patterns-2026-10-20.md`
(R1/R3) and `skt097-canvas-picking-followup-2026-10-20.md` (P2/P3), retrieved
2026-10-20. P2 expressly describes nonperpendicular directions and a diamond
array; P3 uses construction Lines as direction references. This establishes the
supported direction family, not exact hover/handle styling, tolerance or spacing
sign behavior. No proprietary application was operated.

## Retained contract and policy

Two distinct authored construction Lines define unit directions `a` and `b`,
each following its retained start→end orientation. Line location sets direction,
not the pattern origin. Source center `c`, positive spacings `s1`, `s2` and
integer indices produce exact centers `c + i*s1*a + j*s2*b`. Reversing a Line
reverses that direction's progression. Negative spacing is refused; Spacing / One
Direction and total Quantity including the seed remain explicit Alloy policies.
The command dock labels the start→end and positive-distance policies.

The previous perpendicular-only check becomes a normalized cross-product check:
`abs(a.x*b.y - a.y*b.x) > 1e-7`. Equal line IDs, parallel/antiparallel directions,
lengths below 0.001 mm, and touching/overlapping output circles refuse before
publishing authored intent. Existing coordinate, count and spacing bounds remain.
The Direction 2 prompt requests a distinct nonparallel construction Line. Pick
roles, repicking, hover, handles and numeric controls use the existing authority.
D2's offset text now has a thin leader to its grip; art, offset and snap behavior
remain Alloy policies with detailed reference parity unknown.

No derivation or identity algorithm changed. Solving removes generated rows,
solves source intent once, then evaluates vector-sum outputs. Surviving `(i,j)`
Circle/center IDs persist through count, spacing and direction edits; regrown
indices recover their deterministic UUIDs. Generated mutation, constraint and
source/direction Delete ownership guards remain. Explicit downstream profile
sets preserve surviving IDs; count reduction removing a selected Cut profile
refuses before draft/history publication.

## Evidence

Build: `build-preview-release`, Release/private Qt 6.11.2, OCCT 7.9.3, Apple
Clang, `-j4`. Evidence:
`../Alloy-reference-records/validation/skt183-2026-10-20/`.

- `sketch_pattern_test`, `core-final.log`: orthogonal foundation plus exact skew centers,
  unchanged source-only DOF, all surviving indexed identities through count/spacing
  edits, deterministic regrowth, source edits, reverse Line sign, owner guards,
  parallel/antiparallel/degenerate/colliding skew direction-edit atomic refusals,
  exact authored history/save/reopen. Circular core regression also passes.
- `sketch_pattern_canvas_controls_test`, `native-final.log`: 4/4 functions/data
  rows in 25.030 s. Both orthogonal and skew fixtures perform actual Create-menu
  popup action hits, source/two-axis hover and picks, parallel pick refusal,
  all four grips, numeric synchronization, overlap rollback, Cancel/Escape/OK,
  glyph edit/Cancel, Undo/Redo, six-profile Cut, save/reopen and STL. Each uses a
  single private editor, real Qt hit widgets and no interstep focus/selection repair.
  The skew axis is (24,32), so native centers are (8,7) + i*(16,0) + j*(7.2,9.6).
- The initial native log preserves an incorrect fixture assumption: an
  integer-pixel skew drag snapped to 12.1 mm while the test demanded 12.0 mm.
  The final fixture independently calculates the expected 0.1 mm snap from the
  actual pressed/released UV positions, verifies it, then uses real native numeric
  entry when exact fixture spacing is needed. It does not search for special
  pixels or repair focus/selection. Failed and corrected evidence remain separate.
- `tests/sketch_skew_pattern_canvas_api.py`, `api-final/`: the real executable
  opens the exact `native-final/skew/native-picked-grid.alloy`; independent Python
  recomputes every source/instance center and ten UUIDv5 identity fields. Source
  radius 1.5→2 mm, center (8,7)→(9,8), Direction 2 endpoint (24,32)→(32,24), and
  Direction 1 endpoint (40,0)→(36,12) recompute the existing six-profile Cut with
  identical profile IDs/group/instance definitions. Parallel, antiparallel and
  degenerate endpoint edits preserve exact draft/history. Count reduction removing
  selected profiles refuses. Document Undo/Redo and authored save/reopen are exact.
- The edited 52×40×5 plate has exact volume 10023.008881569227 mm³. Independent
  STL readback finds 3492 triangles, one consistently wound watertight component,
  Euler characteristic −10, bounds (0,0,0)..(52,40,5), and mesh volume
  10023.128489068933 mm³ (within 0.01%). Each of the six independently calculated
  radius-2 holes has 864 wall-vertex samples spanning z=0..5. Native STL independently
  has the same manifold/component/Euler properties. Screenshots, exact geometry,
  authored storage and mesh assertions are separate evidence.
- Native skew handle and edited solid captures were inspected at native size for
  legible labels, diagonal direction, six distinct circles and six open holes.
  Final D2 leader capture is `leader-clearance/handles.png`. Hardware diagnostics
  identify Apple M4 / Apple, OpenGL 4.1 Metal, MSAA4 and DPR2 framebuffers.
  Circular canvas native regression passes 3/3 in 10.925 s and is separately
  logged. The D2 leader is a display-only follow-up; its reopened real-app
  capture verifies the final placement without repeating geometry tests.

## Limits

This closes the perpendicular-only supported-family limit within one ordinary
Circle, two nonparallel construction Lines, Spacing / One Direction and retained
bounds. Extent, Symmetric, arbitrary multi-entity seeds and suppression remain
ADV-005. Physical AppKit pointer/trackpad, DPR1 and packaged Windows/Linux runtime
remain separate release gates. Public screenshots are unmatched in scale/state;
exact visual parity is unknown. No CI, tag, release or push was run. Private
fixture editors closed; user-owned PID 72618 preserved. Independent acceptance
is pending.
