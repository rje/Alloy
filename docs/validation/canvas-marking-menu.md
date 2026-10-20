# Canvas marking/context menu

SKT-173/#696 bounded candidate, 2026-10-20. Independent compatibility review is
pending. This increment implements the active-sketch Select context; it does
not establish complete menu parity across workspaces or active commands.

## Contract and source boundary

The compact researched contract is external at
`knowledge/specs/skt173-menu-2026-10-20.md`, source IDs SKT173-R1–R4 and
`tutorial-illustrated-bracket-a2`. Original sources, retrieval dates, public
images and broader records remain outside Git. Only permitted public evidence
was used; no reference software was operated.

A canvas right press in an active sketch's Select tool opens a canvas-owned
Qt popup. A later platform context-menu event cannot duplicate that popup.
Normal/Construction and the existing selected-geometry actions retain their
stable IDs and authoritative command routes. The radial header uses actual
accessible buttons, with these supported positions:

| Level | Clockwise from up |
| --- | --- |
| First | Only supported active-sketch slots: Redo east, Sketch south, Undo west, Delete northwest when geometry is selected. Unsupported modeling commands are absent. |
| Sketch | Finish Sketch, 2-Point Rectangle, Fit Point Spline, Project, Line, Offset, Sketch Dimension, Center Diameter Circle. |

Sketch opens the second level; its up arrow returns to the first. Center click,
outside click and Escape dismiss. A held right drag down by 45 logical pixels
opens Sketch around the turn point; a further move of at least 28 pixels chooses
an octant on release. This supports down/down Line and down/upper-left Circle,
plus the other implemented radial directions. Distances and timing are explicit
Alloy policy: the public evidence establishes directions, not thresholds.

The lower list retains the Activity 2 supported Dimension, Normal/Construction,
Collinear, MidPoint, Fix/UnFix, Parallel, Perpendicular, Horizontal/Vertical and
Equal order. Pan and Zoom invoke the existing navigation pointer modes. Other
selection-specific menus (dimensions, constraints, conics, spline-point editing),
non-sketch contexts and active drawing-command menus retain their earlier routes.
There are no unsupported modeling lookalikes.

All radial buttons use `context.radial.*` IDs and accessible names. Their live
IDs, command IDs, arguments, bounds and current level are inspectable at
`state.viewport.gpu.lastContextMenu.radial`; closure records the selected radial
ID in `lastContextMenu.result`. Tool entry invokes `sketch.tool` or the existing
specialized command, and API session/completion/error semantics remain shared.
The menu authors no alternate document model.

## Verification

Accepted build: `build-preview-release`, Release, private Qt 6.11.2, AppleClang
17, OCCT 7.9.3; targets `alloy` and `bracket_midpoint_controls_test`, `-j4`.
The final focused real-application Qt fixture passes 3/3 in 14.8 seconds and checks:

- Actual-hit right press/release opens the menu without test injection of a
  separate context event; additive end-cap selection still converts both caps.
- Line and Center Diameter Circle start the shared tool from actual radial
  button clicks and held-right down/direction gestures; popup closure precedes
  the next key, and natural viewport focus is asserted before every next input.
- Escape, center and outside dismissal preserve authored draft, selection and
  document revision. No interstep focus or selection repairs occur.
- The original A2 midpoint/tangent construction/circle route, exact radius source
  edits, stable relation identities, Undo/Redo and save/reopen still pass.
- Actual popup grabs supplement the whole-app/native GPU capture so popup
  appearance is directly inspected. The final label adjustment avoids elision
  of Rectangle and Sketch Dimension.

External evidence: `validation/skt173-2026-10-20/qt-final/`; `qt2/` is the first
passing candidate before the label adjustment. The original `qt-candidate/`
failed before the first input because AppKit reported the application inactive.
That receipt is preserved. The default compiled Qt fixture now gates Qt focus
and actual hits independently, recording AppKit state; strict mode retains its
AppKit activation assertion. This does not repair or qualify SKT-171/#694.
The passing Qt sequence records inactive AppKit state, so physical OS right-click
and the uninterrupted strict native artist route remain unqualified.

## Compatibility and remaining work

| Checkpoint | Result |
| --- | --- |
| Two selected caps → right-click → construction | Matched bounded Qt route; both stable curves become dashed construction and cease making profiles. Selection retention is Alloy's observed policy. |
| Documented Sketch directions, click and minimum gestures | Matched direction/category for Line and Circle; all eight supported Sketch controls are present. Other direction executions were not separately exercised. |
| Radial plus lower-list structure | Implemented; exact public metrics are unknown. Missing modeling slots reflect current active-sketch capabilities. |
| Label readability | Inspected actual popup; long labels have independent wider targets. |
| Styling/icons/exact layout | Mismatch/unknown: independently authored text buttons, opaque popup background, different row metrics, missing reference-style icons. No pixel identity claim. |
| AppKit/physical OS input | Unknown/failed initial activation; preserved SKT-171/#694 gate. |
| Platform and display scale | Apple M4 DPR2 only; native DPR1, Windows and Linux remain release gates. |

The window is 1500×950 logical, light theme, top XY, Fit then 0.75 zoom;
viewport is 932×616 logical / 1864×1232 native. GPU reports Apple M4 / Apple,
OpenGL 4.1 Metal – 90.5, macOS 26.5.1. Public crop platform/version/scale and
matched viewport dimensions are unknown; compare semantic arrangement and
readability only. No CI, tag, push or release was performed.

Before closing the broad parent acceptance, preserve these proposed Todo splits
as actual linked issues (keys are proposals until root bookkeeping):

- **SKT-174 proposed:** expand the shared radial component to non-sketch canvas
  and active-command contexts, researching context-dependent availability and
  Repeat/PressPull/Hole/Move/Copy routes before exposing them.
- **SKT-175 proposed:** bounded public-style icon, compact metrics and hover/hit
  region refinement, with matched-state popup comparisons and remaining gesture
  direction/keyboard-accessibility verification. Preserve unsupported/unknown
  public context-list membership explicitly.

SKT-171/#694 continues to own strict native activation, physical right-click and
uninterrupted physical artist-route acceptance. This increment does not close it.
