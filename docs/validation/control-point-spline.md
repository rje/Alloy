# Control point spline and curvature comb candidate

2026-10-13, isolated `feat/control-point-spline`, based on the accepted fit spline
and shared validation correction. Source contract:
[control-point-spline.md](../research/control-point-spline.md). The independent verdict below qualifies this implementation and evidence receipt.

The candidate app builds with Qt 6.11.2 and OCCT on arm64 macOS. Targeted control
and existing fit geometry fixtures pass. Control checks independently derive
quadratic positions, signed curvature and outward display tips; actual OCCT
closed-region area is 1600 mm² and becomes 6400/3 mm² after the middle-pole edit.
They also check retained UUIDs, mixed fit/control DOF, optional display setting
retention, zero curvature, undefined stationary sample, reversal invariance,
atomic invalid arguments and exact document undo/redo/save/reopen.

Native Qt fixture `control_point_spline_controls_test` passed 3/3 in 3680 ms.
Tool activation uses `QAction::trigger`; subsequent three picks, Check, Escape,
pole selection, drag preview/cancel/late release/commit, contextual comb toggles
and Density/Scale edits are actual Qt mouse/key events. Only initial harness
activation sets window/focus. Delivered steps do not repair focus or selection.
A first preserved run stopped at a fixture click on checkbox whitespace; retry
clicks the actual indicator. No production fix was required for that failure.

The actual capture was inspected: the complete three-pole/two-leg brown cage,
both evaluated spline curves and blue combs, selected fit tangent grips and
readable contextual controls fit inside the viewport. Native framebuffer state
reports Apple M4, OpenGL 4.1 Metal, GLSL 4.10 and resolution ratio 2. Numeric
comb defaults/gain and public visual matching remain qualified by the contract;
this is no broad styling parity claim. Windows/Linux runtime remains unverified.

External evidence: `validation/control-point-spline/core.log` and
`native/native-attempt-01.log`, `native/native-attempt-02.log`,
`native/native-control-comb.png`, `native/native-control-state.json` under the
external reference-records directory. No validation editor remains open.

Candidate app SHA256:
`a7c7ce488284722ecf41706b6c8056e3d40a32660e8c4523b08872e37b27fd11`.
Native fixture SHA256:
`c4988889d100cf1cf5138917620958ed4f92ca6adda9d0c4768a7fb33d61b20e`.
Root owns the distinct real-app API acceptance script
`tests/control_point_spline_api.py` on main. Its second-attempt report records
`api-pass` across 32 calls on the same app binary, checking mixed source
curves, independent control positions/curvature/tips, presentation-only changes,
actual closed areas and exact authored history/reopen. Evidence remains external
at `deliverables/control-point-spline-2026-10-13/api-attempt-02`. The initial attempt is retained: its expected tip used the inward normal,
contrary to the reviewed outward convention. Only the test oracle changed;
the passing attempt uses the same product binary.

## Independent acceptance

Accepted candidate `1c592a52193d0f14a96a74667f46d22f5b2f7db2` after source,
focused test logs, actual native/API images, report and saved-model review.
No additional editor or unchanged workflow replay was needed. Three retained
poles evaluate as a genuine quadratic under either maximum degree, and cage
legs remain presentation rather than additional solver entities. The shared
fit/control solver preserves separate fit-handle DOF. Comb settings retain
authored geometry and DOF while using the same exact OCCT curve derivatives.

The signed-curvature and outward-tip equations are consistent, including
reversal; the independent quadratic position/curvature and closed-area core
checks are meaningful. Zero-speed samples break the envelope, zero curvature
does not invent teeth, and clipping remains inspectable. The corrected
32-call API report uses the candidate binary listed above; its saved document
exactly matches `sourceState.authored`. Core/API history and reopen assertions
complement native pole commit/cancel/late-release and actual comb controls.

The inspected native image contains the complete three-pole cage, both combs,
all selected fit grips and readable controls without clipping or label overlap.
The API image is a smaller overview with readable controls. Native evidence
is Qt-dispatched input after initial harness activation, with QAction tool
activation; it does not establish physical OS input or a mouse-open menu path.
Knot policy, comb gain/defaults/clipping and arbitrary fixture coordinates are
explicit Alloy choices. This accepts the bounded #254 ingredient, not exact
reference curvature-display parity, other-platform runtime or full lesson #257.
