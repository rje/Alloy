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

## 2026-10-20 SKT-176 centered-list candidate

SKT-176/#699 has a local candidate; independent review is pending. The compact
external contract is `knowledge/specs/skt176-centered-menu-2026-10-20.md`, reusing
SKT173-R1/R2 and the archived Activity 2 image/provenance. No reference application
was operated and no reference asset is shipped.

The radial panel is now an ordinary child of the same canvas-owned QMenu.
Native QMenu measures original icons, live font/labels and right-aligned tab
shortcuts; symmetric content margins center those native rows below the panel.
The list's background/border follows the actual action geometry. Complete-popup
screen clamping includes the radial panel and list, including gesture relocation.
`lastContextMenu.radial.contextList` exposes live list/row bounds for inspection.
No action hit geometry is replaced by a painted approximation or manual trigger.

Selected Activity 2 list: **180×261** logical action union inside a **356×436**
popup. Unselected list: **165×67** inside **355×242**. The radial panel remains
350×166. Ordinary and narrow windows are 1500×950 and 1050×750 logical, with
light theme, top XY, Fit then 0.75 zoom. Apple M4 / Apple / OpenGL 4.1 Metal –
90.5, macOS 26.5.1, DPR2; exact source platform/font/build/DPR remains unknown.

The accepted `build-preview-release` Release/private Qt 6.11.2/OCCT 7.9.3 build
uses targets `alloy` and `bracket_midpoint_controls_test`, `-j4`. The focused
fixture passes **4/4 in 21.6 seconds**. Final external evidence:
`validation/skt176-2026-10-20/final2/`; build logs:
`validation/skt176-build.log` and `validation/skt176-test-build.log`.

- Component checks retain every radial actual-hit command/eight gesture octants,
  keyboard forward/reverse/direct Backtab, names/roles/action interfaces and
  cancellation. Each screen corner now also checks real native row bounds,
  narrow centered width, no action in transparent flanks, row accessibility and
  an actual shortcut-end click that dispatches once and dismisses.
- Actual Alloy checks retain representative shared Line/Circle/Spline/Offset/
  Dimension input, the selected-cap construction conversion, exact two-circle
  midpoint/tangent relations, source radius edits, Undo/Redo and save/reopen.
  New selected/unselected narrow-window checks open at all four canvas corners,
  inspect complete on-screen popups, unclipped rows and real widget hit receivers,
  then dismiss by actual-hit center clicks. Draft/revision/selection remain exact;
  there is no interstep focus or selection repair.
- Selected/unselected popup and whole-app captures were inspected at DPR2. The
  native popup grabs show every row even when a popup extends outside the main
  application window; whole-app captures use the upper corner for context.
  Native keyboard-focus, original icon, full label and shortcut readability are
  preserved in ordinary-size captures.

| Checkpoint | Bounded result |
| --- | --- |
| Narrower solid list centered below radial controls | Matched semantic silhouette in selected-cap public crop; actual Alloy list is readable with aligned icon/label/shortcut columns. |
| Real row bounds, native accessibility interfaces, one-shot commands | Matched focused Qt checks and real construction route. VoiceOver operation remains unknown. |
| Complete popup at screen/canvas edges, narrow window | Passed focused bounds and actual receiver checks; no row clipping in native popup grabs. |
| Exact public dimensions, font, scale, complete context membership | Unknown; public crop has no matched metrics. Broader membership remains SKT-174/#697. |
| Physical OS input, AppKit activation, other platforms/DPR1 | Unqualified; existing SKT-171/#694 and release gates remain. |

Preserved failed probes: `candidate/` caught deferred QMenu layout clipping a
radial target, corrected by centering the panel from post-popup dimensions.
`narrow/` and `narrow-focus/` record an unsuccessful Escape-to-current-focus
probe after harness window resize; the popup did not own keyboard focus.
The successful narrow bound/hit sequence uses actual center dismissal without
focus repair. This does not qualify resized-window natural Escape or establish
an ordinary physical-input defect; the existing strict native gate remains.
The earlier corrected ordinary fixture passed at `final/`, and the full narrow
fixture passed at `qualified/` before moving whole-app captures to the upper
corner. Qt capture warnings and inactive AppKit flags remain as in SKT-175.
No new broad exact-match claim is made. No CI, tag, push, release, issue closure
or board update was performed. All private validation windows closed.
