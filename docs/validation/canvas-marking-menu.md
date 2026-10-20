# Canvas marking/context menu

SKT-173/#696 bounded increment accepted 2026-10-20 after independent Astra
source/evidence review. This increment implements the active-sketch Select context; it does
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

The original broad menu acceptance was explicitly split before closure:

- **SKT-174/#697 Todo:** expand the shared radial component to non-sketch canvas
  and active-command contexts, researching context-dependent availability and
  Repeat/PressPull/Hole/Move/Copy routes before exposing them.
- **SKT-175/#698 Todo:** bounded public-style icon, compact metrics and hover/hit
  region refinement, with matched-state popup comparisons and remaining gesture
  direction/keyboard-accessibility verification. Preserve unsupported/unknown
  public context-list membership explicitly.

SKT-171/#694 continues to own strict native activation, physical right-click and
uninterrupted physical artist-route acceptance. This increment does not close it.
The reviewer accepted the bounded Qt active-sketch component, shared-command
lifetime and focused evidence. It confirmed that gesture events are Qt-delivered,
keyboard and assistive-technology operation are unverified, and the captured
AppKit state was inactive; no physical-input parity is inferred.

## 2026-10-20 SKT-175 bounded visual/access acceptance

This bounded refinement adds independently authored geometric icons to supported
radial and context-list actions, a transparent radial field, compact 24px radial
targets with full text, 11px compact list rows, and blue focus/hover/gesture states.
Targets are sized from the live font metrics and do not overlap. Popup and
held-gesture relocation clamp the complete menu to the available screen. The
context list remains normal authoritative QMenu actions; no modeling capability
or unsupported slot is added. External researched contract:
`knowledge/specs/skt175-menu-visual-access-2026-10-20.md`, reusing source IDs
SKT173-R1/R2 and `tutorial-illustrated-bracket-a2` with archived provenance.

Tab/Shift-Tab traverses visible enabled radial controls and context rows in a
cycle. Radial arrows follow clockwise/counterclockwise order; Enter or Space
invokes the focused button. Keyboard submenu entry puts focus at Finish Sketch,
and Return to first level is a real accessible button. Escape dismisses from
both areas. Names, button roles, action interfaces, shared commands/arguments and
stable IDs remain available; menu diagnostics now include global position and
DPR. Screen-reader narration is not qualified by interface inspection alone.

Accepted build directory remains `build-preview-release`, Release/private Qt
6.11.2/OCCT 7.9.3, targets `alloy` and `bracket_midpoint_controls_test`, `-j4`.
External final evidence: `validation/skt175-2026-10-20/final2/`; build log is one
level above. The focused test passes **4/4 in 19.1 seconds**:

- The isolated component fixture uses actual-hit clicks on all eleven command
  buttons, plus Sketch, Back and dismissal. It separately verifies all eight
  clockwise Sketch gesture octants, exact command IDs/tool arguments, real Qt
  accessibility names/button roles/action interfaces, disjoint target bounds,
  full keyboard traversal/wrap/reverse execution and four screen-corner bounds.
  Its command callback records dispatch; it does not claim document outcomes.
- The actual Alloy fixture starts Line by uninterrupted keyboard traversal and
  actual-hit clicks start Line, Circle, Fit Point Spline, Offset and Dimension.
  Line/Circle gestures and Escape/center/outside cancellation still pass without
  interstep focus or selection repairs. Natural viewport focus is asserted before
  the next input. The selected-cap Normal/Construction route, midpoint/tangent
  geometry, radius edits, stable IDs, Undo/Redo and exact save/reopen still pass.
- Actual transparent popup grabs, whole-app GPU composition and keyboard-focus
  capture were inspected. Labels remain readable and the focused Line has the
  blue state. The QTest mouse-move-only capture has no visible hover highlight;
  physical pointer hover is unqualified, alongside AppKit/OS input SKT-171/#694.

This fixture remains 1500×950 logical, XY/light/Fit then 0.75 zoom; viewport
932×616 logical, 1864×1232 framebuffer, Apple M4/Apple/OpenGL 4.1 Metal – 90.5,
macOS 26.5.1, DPR2. AppKit application/key-window flags remain false. Qt logs
retain `QWidgetWindow ... must be a top level window` warnings during captures;
API/geometry/input assertions pass. DPR1, Windows/Linux runtime, physical input
and assistive-technology operation remain unqualified.

| Checkpoint | Bounded result |
| --- | --- |
| Original icons, transparent field, readable compact radial targets | Implemented and actual app captures inspected. Symbols are independently authored; no reference asset shipped. |
| Supported click/gesture routes, keyboard and Qt accessibility interfaces | Matched bounded command dispatch; representative real-app outcomes and original bracket route pass. |
| Exact public metrics/theme/hover | Unknown: archive does not identify platform/build/DPR. Physical hover was not observed. |
| Lower-list silhouette | Mismatch: QMenu rows still occupy the full radial width, whereas the archived image has a narrower centered list. Proposed Todo follow-up must retain actual row hits, keyboard/accessibility and cancellation. |

Independent Sol review accepted the bounded result after the Backtab correction.
The narrower centered context-list silhouette remains SKT-176/#699 Todo; its
actual row hit/accessibility and edge placement must still be qualified.
No CI, tag, push or release was performed. All private validation windows closed.

### Independent-review Backtab correction

The first candidate's reverse cycle handled Shift-modified Tab but omitted Qt's
separate `Key_Backtab` encoding. Independent review rejected that keyboard gap.
The narrow correction handles both encodings through the same reverse traversal.
A direct `Qt::Key_Backtab` fixture now wraps the first radial control to the last
list row, traverses list rows, then crosses the first row back to the last radial
control, delivering each next key to the naturally focused widget without repair.
The affected `bracket_midpoint_controls_test` target rebuilt with `-j4`; the full
focused fixture passes **4/4 in 19.3 seconds**, preserving real-app input and
bracket geometry/history/reopen coverage. Latest external receipt/captures/log:
`validation/skt175-2026-10-20/backtab/`; build log: `backtab-build.log`.
Independent re-review accepted the corrected path. The lower-list-width mismatch and physical
input/accessibility/platform limits above are unchanged. Private editor closed.
