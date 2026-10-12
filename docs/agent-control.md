# Local agent control

Alloy's running native application provides a small local control API from its
first shell build. It is an Alloy protocol. Authored document inspection, units,
real box/cylinder and constrained origin-plane sketch creation and editing,
driving/driven dimensions, geometry inspection, undo/redo and native
save/open use the same authoritative path as the UI.

## Launch and discover

```sh
./build/alloy.app/Contents/MacOS/alloy --ready-file /tmp/alloy-ready.json
```

On Linux, use `./build/alloy`. The app prints one startup JSON line containing
`protocol`, `socket`, `pid`, and `status: "ready"`. `--ready-file` writes the same
object atomically after the window is shown. It now also includes a unique
`instanceId`, native `windowId` and capability families, matching `app.identity`.
Each launch registers a private record in `~/.alloy/instances/` (override with
`--instance-dir`) and removes its own record on orderly quit. Each launch defaults to a fresh
private temporary directory containing its socket. Keep the app running while
using the client in another terminal:

```sh
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json commands
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json state
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json exec view.orientation --args '{"name":"top"}'
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json exec view.grid --args '{"visible":false}'
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json exec layout.reset
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json screenshot /tmp/alloy.png
python3 tools/alloyctl.py --ready-file /tmp/alloy-ready.json exec app.quit
```

`python3 tools/alloyctl.py instances` verifies live identities and reports stale
records. With one live registered instance, commands can omit endpoint flags.
With several, pass `--instance UUID`; the client refuses an ambiguous choice.
Ready-file selection verifies the live protocol/PID/window/instance UUID before
sending an operation. Stale records never cause socket deletion or endpoint
takeover. Explicit `--socket` remains a caller-selected direct endpoint.

Alternatively pass `--socket PATH` to the client or set `ALLOY_SOCKET`. For a
chosen app endpoint, `--control-socket /absolute/private-directory/control.sock`
requires an existing directory with no group/other permissions, normally mode
0700. The app will not remove an existing socket to steal another instance's
endpoint. On macOS keep socket paths short (the operating system limits them).
The client currently supports Unix-domain sockets on macOS/Linux. Windows client
transport remains unimplemented.

## Protocol and completion

Transport: Qt local socket, UTF-8, one JSON object per line, maximum incoming
buffer 64 KiB, idle connection timeout 10 seconds. No TCP port, cloud service,
authentication account, or arbitrary-code command is exposed. Socket access is
restricted by its private parent directory and Qt's user-access option. A client
with access can operate this Alloy instance and write captures as the current
user; this is not an isolation boundary against other processes of the same user.

Request:

```json
{"id":"request-1","command":"view.orientation","args":{"name":"top"}}
```

Successful response:

```json
{"id":"request-1","ok":true,"completion":"completed","command":"view.orientation","revision":1,"result":{}}
```

Error response:

```json
{"id":"request-2","ok":false,"completion":"failed","error":{"code":"not_implemented","message":"Extrude is not implemented in the UI shell"}}
```

Commands run on the UI thread through `AlloyWindow::execute`, also called by UI
buttons, menu actions, named-view selection, and mouse navigation. A success
means command state has been applied synchronously. Modeling success includes
validated OCCT evaluation before the authored transaction is committed. It is
not an OS compositor fence. `app.quit` returns success before scheduling
exit. `capture.screenshot` completes only after the PNG is committed to disk.
The client correlates request IDs and exits 0 for success, 1 for an application
error, or 2 for a client/transport error.

`commands.list` supplies stable IDs, titles, availability, descriptions, and
required parameter type/enum/range schemas. Unknown/missing/incorrect arguments
fail without applying the command. Error codes include `invalid_request`,
`request_too_large`, `unknown_command`, `not_implemented`, `invalid_arguments`,
`capture_failed`, `evaluation_failed`, `kernel_failure`, `stale_entity`,
`stale_session`, `no_active_sketch`, `invalid_session`, `invalid_sketch`,
`sketch_solve_failed`, `unsaved_changes`, `open_failed` and `save_failed`.

## Implemented commands

| Command | Arguments / result |
| --- | --- |
| `commands.list` | Registry with both implemented and unavailable entries |
| `app.state` | Real window/view state, live GPU diagnostics, authored and evaluated document, selection and journal |
| `commands.search`, `commands.close` | Show/dismiss the native search dialog |
| `app.identity` | Stable protocol/process/window/instance identity and capabilities |
| `document.settings` | Open the native shared display-units edit dialog |
| `command.begin` | `command`: `document.units`, `commands.search`, `solid.box`, `solid.cylinder` or `sketch.create`; `inputs`: object |
| `command.update` | `session`: UUID; `inputs`: partial object; validates without committing |
| `command.preview` | `session`: UUID; primitives/sketches evaluate disposable geometry and display it without committing |
| `command.panelDock` | `docked`: boolean; float or right-redock the active primitive panel |
| `command.commit`, `command.cancel` | `session`: UUID; explicit completion or refusal |
| `capture.dialog` | Absolute `.png` path; actual active QWidget dialog |
| `document.inspect` | Authored root/feature IDs, ownership, canonical units and journal state |
| `document.units` | `units`: `mm` or `in`; display conversion preserves canonical values |
| `document.new` | `discard`: boolean; dirty replacement refuses unless explicitly acknowledged |
| `document.save` | `path`: absolute `.alloy` path; atomic authored save, clean only after success |
| `document.open` | `path`: absolute `.alloy` path; optional `discard`: boolean, default false |
| `document.saveDialog`, `document.saveAsDialog`, `document.openDialog` | Native chooser wrappers dispatch those same file commands |
| `solid.box` | Required `width`, `depth`, `height`; optional `x`, `y`, `z`, `units`, `name`, `featureId` |
| `solid.cylinder` | Required `radius`, `height`; same optional placement/unit/name/edit fields |
| `primitive.box`, `primitive.cylinder` | Open their actual shared dimension dialogs |
| `sketch.create` | Open the docked origin-plane chooser |
| `sketch.begin` | `plane`: `XY`, `XZ` or `YZ`; optional `name`, `units`; begin an authored draft |
| `sketch.edit` | `sketchId`: UUID; re-enter a retained sketch |
| `sketch.line`, `sketch.rectangle` | `x1`, `y1`, `x2`, `y2`; optional `units`, `session`; append stable point/line entities and solve the draft. Rectangles also author four H/V constraints |
| `sketch.tool` | `tool`: `select`, `line`, `rectangle` or `dimension`; choose the active drawing/selection tool |
| `sketch.selectionSet` | Optional `pointIds`, `entityIds`: arrays of authored IDs in the active sketch; omitted arrays clear that selection kind |
| `sketch.selectionPick` | `x`, `y`: logical pixels; optional `additive`: boolean. Pick an authored point or line, sharing native selection state |
| `sketch.placePoint` | `x`, `y`: logical pixels; native ray/plane two-click drawing path, or selection when the active tool is `select`/`dimension` |
| `sketch.pointerMove` | `x`, `y`: logical pixels; disposable pending-line/rectangle cursor preview |
| `sketch.lookAt` | View normal to the active sketch plane |
| `sketch.finish`, `sketch.cancel` | Commit or discard the active sketch session |
| `view.projectSketchPoint` | `plane`, `uMm`, `vMm`; actual world-to-viewport projection |
| `sketch.constraint` | `type`: `coincident`, `horizontal`, `vertical`, `parallel`, `perpendicular` or `fixed`; optional `pointIds`, `entityIds`, `session`. Omitted targets use active sketch selection |
| `sketch.dimension` | `kind`: `linear`, `horizontal` or `vertical`; optional `pointIds` (two), `entityId` (one line), `value`, `units`, `driving`, `session`. Omitted targets use selection; `driving` defaults true |
| `sketch.dimensionUpdate` | `dimensionId`, positive `value`; optional `units`, `session`; edit a driving dimension by stable UUID |
| `sketch.movePoint` | `pointId`, `x`, `y`; optional `units`, `session`; seed a point's coordinates and solve the copied draft |
| `feature.edit` | `featureId`: UUID; `parameters`: partial canonical-mm fields such as `widthMm` |
| `feature.editDialog`, `feature.delete` | `featureId`: UUID; actual edit dialog or one undoable deletion |
| `geometry.inspect` | Evaluated bodies and sketches: stable IDs, validity, geometry summaries and plane frames |
| `history.undo`, `history.redo` | Sole document journal; restore authored values and evaluated shapes together |
| `selection.set` | `bodyId`: visible UUID, or empty string to clear |
| `selection.pick` | `x`, `y`: viewport-local logical pixels; actual OCCT body picking |
| `view.projectBody` | `bodyId`: UUID; actual projection of bounding-box center to logical pixels |
| `layout.browser`, `layout.timeline`, `layout.navigation` | `visible`: boolean |
| `layout.reset` | Restore those three regions |
| `view.grid`, `view.origin` | `visible`: boolean |
| `view.orientation` | `name`: `isometric`, `top`, `front`, or `right` |
| `view.zoom` | `factor`: 0.1–10; resulting scale clamps to 0.1–10 |
| `view.pan` | `x`, `y`: relative logical-pixel deltas, each within ±10000 |
| `view.home` | Fit visible bodies and sketches in the isometric view; reset zoom/pan |
| `window.resize` | Logical width 1040–4096 and height 640–4096 |
| `capture.screenshot` | Absolute `.png` path; returns dimensions, device scale, source |
| `app.quit` | Optional `discard`: boolean, default false; dirty refusal unless acknowledged, then exit after reply |

State includes Browser/Timeline/navigation/search visibility, logical window and
viewport sizes, device scale, reference-grid orientation, zoom, pan, grid/axis
visibility, and an applied-command revision. Revision advances for successful
view/layout/search/window-resize commands, including a repeated no-op command; it is not a
persistent document revision or a tracker for every OS resize/interaction.
`document.revision` separately advances when authored state is replaced (including
undo/redo). `document.authored` is authoritative; root ownership and UUIDs are
independent of Browser text. Dirty reflects the sole document journal checkpoint.

In sketch editing, `activeSession.command` is `sketch.edit` and `draftSketch`
contains the retained candidate feature (with no `bodyId`). Inputs identify its
plane and display units; coordinates are stored in millimeters. `tool` and pending
`firstPoint`/`pointerPoint` describe actual drawing state. `command.preview`
returns the feature as `authoredDraft`, the full root as `documentDraft`, and an
evaluated `sketchGeometry` summary without editing the document. `ui.tab` becomes
`sketch`; the palette is a real docked widget. Finish commits one transaction,
Cancel discards it, and stale revision checks reject outdated candidates. Plane
choice is fixed once editing begins. Evaluated sketch inspection exposes frames,
world-space endpoints, bounds and `solverIntegrated: true`. The `solver` object
reports the accepted solution's diagnostics. GPU state
reports `displayedSketches`, `draftSketchVisible` and `cursorPreviewVisible`.

Sketch selection is independent of body selection. `activeSession.selection`
contains `pointIds`/`entityIds`; selected points and lines are orange, fixed points
green, and the sketch is black at zero DOF or blue while underconstrained.
This is a whole-sketch classification. Point picking has priority within eight
logical pixels; line picking uses six. UI clicks and API picking use the same
projected authored geometry and stable identifiers.

Coincidence requires two points, fixed requires one point, H/V requires one line,
and parallel/perpendicular requires two lines. Fixed captures the point's current
canonical coordinates as its target. Dimensions use two distinct points or one
line's endpoints. Driving values are positive; omitted `value` captures the current
measurement. `driving: false` stores a derived measurement with no driving value.
Horizontal/vertical values are absolute distances; the solver retains the signed
endpoint branch from the seed. `units` accepts `mm`/`in` and defaults to active
sketch units; authored values and `dimensionValues[].valueMm` remain canonical mm.

Accepted mutations return `draftSketch`, `sketchGeometry`, `diagnostics` and
`authoredCommitted: false`. `activeSession.solverDiagnostics` contains the last
accepted result: `solver`, `status`, `dof`, `constraintCount`, `maxResidual`,
`lengthToleranceMm`, `angularTolerance`, `conflictingConstraintIds`,
`redundantConstraintIds` and `dimensionValues` (ID, kind, driving flag and value).
Accepted statuses are `underconstrained` and `fully_constrained`. A rejected solve
returns `sketch_solve_failed` with `error.diagnostics`, also observed as
`activeSession.lastSolveFailure`; it preserves the usable draft, accepted geometry
and journal. Finish commits the normalized points and authored relations once;
save/reopen reconstructs the same solution from them. Older files without the
optional arrays remain valid without inferred constraints.

The dimension tool currently selects targets for palette numeric entry. GPU labels
show measured values (driven values in parentheses), but canvas placement, inline
typing, double-click label editing and native point dragging remain unimplemented.
`sketch.movePoint` is a solved coordinate seed, not a physical drag gesture.
See the [bounded interaction gaps](research/commands/sketch-controls.md).

`viewport.renderer` is `OCCT OpenGL`; `viewport.gpu` contains live OCCT driver
information and qualification. `vendor`, `renderer`, and `version` come from the
actual context. Known software fallback is labeled `software-rejected`;
unrecognized hardware is `unverified`. `hardwareAccelerated` requires an
identified hardware vendor/device and a GL version, and the smoke test asserts it.
`details` preserves raw OCCT diagnostics, including physical GL viewport size.
State inspection synchronizes and redraws the native drawable before reading
diagnostics, and capture restores native rendering after its offscreen dump.
`nativeWidth/Height` use OCCT native-window units (logical points on macOS);
`framebufferWidth/Height` use physical pixels. `kernel.visualizationIntegrated`
and `kernel.integrated` are true for the implemented authored primitives.
`camera` reports actual OCCT `eye`, `center`, `direction`, `up` (world-space
three-element arrays), `scale`, and `aspect`. Input/projection/pan use native
window dimensions to translate logical coordinates; framebuffer DPR is separate.
Camera synchronization defers rendering until the completed state is ready.

Primitive lengths/positions are interpreted in `units` (or current display units)
and stored in millimeters. Canonical dimensions must be 0.001–100000 mm, with
position magnitude at most 100000 mm. This second-stage geometry policy can
reject an input that satisfies the outer command schema. The initial primitives
use world XY placement and New Body; plane picking and Boolean operation modes
remain future work. `feature.edit` uses canonical parameters, independent of
display units. Save/open persists feature intent and stable IDs, rebuilds shapes,
and starts a clean empty session undo journal after reopening.

The screenshot combines `QWidget::grab()` of main-window chrome with the actual
OCCT scene rendered by `V3d_View::ToPixMap` into a GPU framebuffer at the current
viewport size/scale. It excludes OS decorations, menus, and separate dialogs;
close search before a clean capture. It is not an OS-compositor screenshot.
Failure to capture the GPU view returns `capture_failed`; a blank raster substitute
is never reported as success. Capture results add `viewportPixelWidth`,
`viewportPixelHeight`, and `viewportSha256` (raw RGB32 framebuffer bytes) so camera
changes can be checked independently of status labels and shell layout changes.
No screenshot establishes authored geometry or CAD workflow acceptance.

`activeSession` exposes actual dialog inputs/schema, the base document revision,
status and session UUID. `ui.activeDialog` separately inspects widget visibility
and displayed inputs. Draft updates and previews create no undo entries; commit
calls the same registered document command as direct UI/API execution. Cancelled
or superseded IDs cannot commit. `document.lastTransactionId` identifies the last
committed/undone/redone journal entry. These are synchronous operations; no async
job family is exposed until a real long operation needs it. Failed primitive
previews and stale sessions clear their disposable presentation; rejected sketch
solve mutations retain the last accepted draft. A main-window capture
excludes separate dialogs; `capture.dialog` captures those real 2D widgets.

## Verification and extension contract

```sh
ctest --test-dir build -R '^(document_model|geometry_kernel|sketch_document|sketch_solver|client_selection)$' --output-on-failure
python3 tests/workflows/run.py --case constraint-regression --evidence build/evidence/constraint-regression
```

The test launches the built app, discovers commands, changes view and layout,
opens/closes search, checks structured errors and malformed JSON, verifies state,
asserts a live hardware OpenGL context, changes the real camera and compares
framebuffer hashes, resizes the native window at its device scale, checks GPU
viewport dimensions before any startup/resized offscreen capture (to avoid
masking a native sizing bug), checks capture dimensions, restores presentation,
and quits. Evidence is written to `build/evidence/`.

The [primitive receipt](validation/primitives.md) adds a coherent create/edit/
pick/undo/reopen workflow and separate native keyboard/dialog observations.
Body and active-sketch point/line selection are implemented; solid face/edge
selection is still DOC-006. The [sketch receipt](validation/sketches.md) and
[constraints receipt](validation/sketch-constraints.md) add drawing, solve,
driving/driven values, failed-draft preservation, one-entry Finish, cancellation,
undo and native reopen. Candidate-specific reruns remain identified separately.
Native file dialogs and unsaved-work prompts exist, but observable pending
document decisions and full physical coverage remain DOC-013.

Future modeling work must use the same command/document path and extend state
with document identity, persistent entity references, units/parameters, geometry
summaries, selection, transactions, undo, save/reopen, and command cancellation
when those capabilities actually exist. Add real-app geometry/workflow tests
with each implemented modeling slice; do not fake geometry in automation.

Primitive UI fields invoke shared update/preview on field completion; explicit API
updates retain explicit preview semantics. `ui.activeDialog.presentation` reports
`docked-panel` or `floating-panel`; `bounds` are logical main-window-relative
coordinates. `viewport.x/y/width/height` describe the actual canvas in that same
space. `viewport.gpu.previewReplacedBodyId` identifies the temporarily hidden
original presentation, without changing authored state. Panel Close, Escape and
Cancel route through the shared cancellation command.

Viewport picking retains fractional logical coordinates until native rounding.
Option/Alt-left drag and middle drag use shared `view.pan`; plain left click uses
`selection.pick` outside sketch editing. Active-sketch clicks use the current
drawing/selection tool; Shift-click adds/toggles sketch selection. Selected bodies
are visibly blue; clearing selection restores normal appearance without changing
the authored document.
