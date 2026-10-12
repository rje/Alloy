# Local agent control

Alloy's running native application provides a small local control API from its
first shell build. It is an Alloy protocol. Authored document inspection, units,
real box/cylinder and constrained origin-plane sketch creation and editing,
driving/driven dimensions, analytic circles/arcs, finished-profile extrusion,
Combine Join/Cut/Intersect, ordered recompute,
geometry inspection, undo/redo and native save/open use the same authoritative
path as the UI.

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
{"id":"request-2","ok":false,"completion":"failed","error":{"code":"not_implemented","message":"Fillet is not implemented in the UI shell"}}
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
`sketch_solve_failed`, `profile_required`, `unsaved_changes`, `open_failed` and `save_failed`.

## Implemented commands

| Command | Arguments / result |
| --- | --- |
| `commands.list` | Registry with both implemented and unavailable entries |
| `app.state` | Real window/view state, live GPU diagnostics, authored and evaluated document, selection and journal |
| `commands.search`, `commands.close` | Show/dismiss the native search dialog |
| `app.identity` | Stable protocol/process/window/instance identity and capabilities |
| `document.settings` | Open the native shared display-units edit dialog |
| `command.begin` | `command`: `document.units`, `commands.search`, `solid.box`, `solid.cylinder`, `solid.extrude`, `solid.combine` or `sketch.create`; `inputs`: object |
| `command.update` | `session`: UUID; `inputs`: partial object; validates without committing |
| `command.preview` | `session`: UUID; primitives/sketches/extrusions/Combine evaluate disposable geometry and display it without committing |
| `command.panelDock` | `docked`: boolean; float or right-redock the active modeling panel |
| `command.commit`, `command.cancel` | `session`: UUID; explicit completion or refusal |
| `capture.dialog` | Absolute `.png` path; actual active QWidget dialog or modeling dock |
| `document.inspect` | Authored root/feature IDs, ownership, canonical units and journal state |
| `document.units` | `units`: `mm` or `in`; display conversion preserves canonical values |
| `document.new` | `discard`: boolean; dirty replacement refuses unless explicitly acknowledged |
| `document.save` | `path`: absolute `.alloy` path; atomic authored save, clean only after success |
| `document.open` | `path`: absolute `.alloy` path; optional `discard`: boolean, default false |
| `document.saveDialog`, `document.saveAsDialog`, `document.openDialog` | Native chooser wrappers dispatch those same file commands |
| `solid.box` | Required `width`, `depth`, `height`; optional `x`, `y`, `z`, `units`, `name`, `featureId` |
| `solid.cylinder` | Required `radius`, `height`; same optional placement/unit/name/edit fields |
| `primitive.box`, `primitive.cylinder` | Open their actual shared dimension dialogs |
| `solid.extrude` | Required `sketchId`, `profileId`, signed `distance`; optional `units` (`mm`/`in`), `name`, existing `featureId`; direct create/edit commit |
| `solid.extrudeDialog` | Optional `sketchId`, `profileId`; toolbar/E opener for the shared extrusion session |
| `solid.combine` | Required `targetBodyId`, nonempty distinct `toolBodyIds`, `operation` (`join`/`cut`/`intersect`); optional `keepTools` (default false), `name`, existing `featureId`; direct create/edit commit |
| `solid.combineDialog` | Same fields, all optional; open the shared Combine session, default Join with Keep Tools off |
| `solid.combineSelectionMode` | `role`: `target` or `tools`; return to input-body display for shared canvas picking |
| `sketch.create` | Open the docked origin-plane chooser |
| `sketch.begin` | `plane`: `XY`, `XZ` or `YZ`; optional `name`, `units`; begin an authored draft |
| `sketch.edit` | `sketchId`: UUID; re-enter a retained sketch |
| `sketch.line`, `sketch.rectangle` | `x1`, `y1`, `x2`, `y2`; optional `units`, `session`; append stable point/line entities and solve the draft. Rectangles also author four H/V constraints |
| `sketch.addCircle` | `centerX`, `centerY`, positive `diameter`; optional `units`, `session`; append one analytic circle |
| `sketch.addArc` | `mode`: `center` requires `centerX`, `centerY`, `radius`, `startAngleDeg`, signed `sweepAngleDeg`; `threePoint` requires `startX`, `startY`, `endX`, `endY`, `throughX`, `throughY`; optional `units`, `session` |
| `sketch.tool` | `tool`: `select`, `line`, `rectangle`, `circle`, `centerArc`, `threePointArc` or `dimension`; choose the active drawing/selection tool |
| `sketch.selectionSet` | Optional `pointIds`, `entityIds`, `profileIds`: arrays of authored/evaluated IDs in the active sketch; omitted arrays clear that selection kind |
| `sketch.selectionPick` | `x`, `y`: logical pixels; optional `additive`: boolean. Pick an authored point, line, circle, arc or evaluated profile interior, sharing native selection state |
| `sketch.placePoint` | `x`, `y`: logical pixels; native ray/plane drawing path, or selection when the active tool is `select`/`dimension` |
| `sketch.pointerMove` | `x`, `y`: logical pixels; disposable drawing preview with origin/point acquisition in `snap`; no authored change |
| `sketch.lookAt` | View normal to the active sketch plane |
| `sketch.finish`, `sketch.cancel` | Commit or discard the active sketch session |
| `view.projectSketchPoint` | `plane`, `uMm`, `vMm`; actual world-to-viewport projection |
| `sketch.constraint` | `type`: `coincident`, `horizontal`, `vertical`, `parallel`, `perpendicular`, `equal` or `fixed`; optional `pointIds`, `entityIds`, `session`. Omitted targets use active sketch selection |
| `sketch.constraintTool` | `type`: persistent constraint-first tool; repeated picks apply shared retained relations, Escape returns to Select |
| `sketch.dimension` | `kind`: `linear`, `horizontal`, `vertical`, `radius`, `diameter`, `angle` or `arcSweep`; optional `pointIds` (two), `entityId` (one line/curve), `entityIds` (two lines for angle), `raySigns` (two values ±1 selecting the angular sector), `value` (length) or `valueDeg` (sweep), `units`, `driving`, `annotation`, `session`. Omitted targets use selection; `driving` defaults true |
| `sketch.dimensionPreview` | `x`, `y`: logical pixels; optional `kind` (`auto` or a dimension kind above), `driving`, `session`; preview selected targets |
| `sketch.dimensionPlace` | `x`, `y`, optional `session`; fix placement and show the actual inline numeric editor |
| `sketch.dimensionAccept` | Optional `text` or numeric `value` (length)/`valueDeg` (sweep), `units`, `session`; text accepts mm/in for lengths or deg/° for sweep; expressions are unsupported |
| `sketch.dimensionCancel` | Optional `session`; abandon pending dimension placement/editing |
| `sketch.dimensionEdit` | `dimensionId`, optional `session`; open a driving editor or read-only driven measurement |
| `sketch.annotationSelect` | `dimensionId`; select a retained label |
| `sketch.annotationPick` | `x`, `y`: logical pixels; actual projected/rotated label hit test |
| `sketch.annotationMove` | `dimensionId`, `uMm`, `vMm`, optional `gestureId`, `session`; repeated moves with one gesture identity coalesce into one draft undo entry; move placement only, retaining exact authored points/values |
| `sketch.annotationEndGesture` | `dimensionId`, `gestureId`; end annotation drag coalescing without authoring geometry |
| `sketch.dimensionUpdate` | `dimensionId`, positive `value` (length) or `valueDeg` (sweep); optional `units`, `session`; edit a driving dimension by stable UUID |
| `sketch.movePoint` | `pointId`, `x`, `y`; optional `units`, `session`; seed a point's coordinates and solve the copied draft |
| `feature.edit` | `featureId`: UUID; `parameters`: partial canonical-mm fields such as `widthMm` |
| `feature.editDialog`, `feature.delete` | `featureId`: UUID; actual primitive/sketch/extrusion/Combine editor or one undoable deletion; invalid downstream dependencies are rejected |
| `feature.visibility` | `featureId`, `visible`: boolean; one authored undoable visibility edit, also used by Browser checkboxes |
| `geometry.inspect` | Evaluated bodies and sketches: stable IDs, validity, geometry summaries and plane frames |
| `history.undo`, `history.redo` | Active sketch: Undo cancels a pending editor/tool first, then traverses accepted draft edits; Redo requires Select. Otherwise traverse committed document history. `activeSession.draftHistory` exposes index/count/canUndo/canRedo |
| `selection.set` | `bodyId`: visible UUID, or empty string to clear |
| `selection.profileSet` | `sketchId`, `profileId`; select an accepted finished region by retained identity |
| `selection.sketchSet` | `sketchId`; Browser sketch selection, with sole-region resolution when opening Extrude |
| `selection.pick` | `x`, `y`: viewport-local logical pixels; actual body or finished-profile picking; returns `kind` and retained IDs |
| `view.projectBody` | `bodyId`: UUID; actual projection of bounding-box center to logical pixels |
| `layout.browser`, `layout.timeline`, `layout.navigation` | `visible`: boolean |
| `layout.reset` | Restore those three regions |
| `view.grid`, `view.origin` | `visible`: boolean |
| `view.orientation` | `name`: `isometric`, `top`, `front`, or `right` |
| `view.zoom` | `factor`: 0.1–10; resulting scale clamps to 0.1–10 |
| `view.pan` | `x`, `y`: relative logical-pixel deltas, each within ±10000 |
| `view.home` | Fit visible bodies/sketches and a valid extrusion preview in isometric view; reset zoom/pan |
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
`firstPoint`/`secondPoint`/`pointerPoint` describe actual drawing state. `command.preview`
returns the feature as `authoredDraft`, the full root as `documentDraft`, and an
evaluated `sketchGeometry` summary without editing the document. `ui.tab` becomes
`sketch`; the palette is a real docked widget. Finish commits one transaction,
Cancel discards it, and stale revision checks reject outdated candidates. Plane
choice is fixed once editing begins. Evaluated sketch inspection exposes frames,
world-space endpoints, bounds and `solverIntegrated: true`. The `solver` object
reports the accepted solution's diagnostics. GPU state
reports `displayedSketches`, `draftSketchVisible` and `cursorPreviewVisible`.

Sketch selection is independent of body selection. `activeSession.selection`
contains `pointIds`/`entityIds`/`profileIds`; selected points and curves are orange, fixed points
green, and the sketch is black at zero DOF or blue while underconstrained.
This is a whole-sketch classification. Point picking has priority within eight
logical pixels; curve picking uses six. UI clicks and API picking use the same
projected authored geometry and stable identifiers. Region picking follows points
and lines, classifies the actual accepted OCCT face, and exposes its stable profile ID.
`geometry.inspect` and `sketchGeometry.profiles` include face area and edge provenance.
One circle disk or one connected simple closed line/arc loop is supported;
holes and multiple regions are unsupported. Open/unsupported input
retains the sketch with `profileDiagnostics`.

Curve lengths and coordinates use explicit `units` or active sketch units and
normalize to millimeters. Center arcs measure angles in degrees from the sketch
plane's positive U axis; a positive sweep is counterclockwise, a negative sweep
clockwise. Sweep must be nonzero and smaller than 360° in magnitude. Three-point
arcs pass through the named through point; coincident or collinear inputs fail
without replacing the accepted draft. Native tool click order is circle
center → circumference, center arc center → start → end, and three-point arc
start → end → through. C selects Circle; the Arc menu selects its construction.
The active tool offers a separate numeric-entry dialog. Exact OCCT curves remain
distinct from display tessellation; inspection exposes analytic lengths and arc
endpoints alongside stable entity IDs.

Finished visible sketches retain their real profile fill and pick region outside
sketch-edit mode. `selection.pick` returns `kind: "profile"`, `sketchId`, `profileId`
and `profileIds` when a region is selected. Normal body picking takes precedence;
an active extrusion session prioritizes profiles. Hidden sketches have no
pickable ghost fill. Explicit Browser/API sketch selection can still identify a
hidden source for a new command. Extrude retains a selected profile, resolves a
selected sketch's sole accepted region, or auto-selects only the sole visible
eligible profile. Ambiguous input stays unselected and requires actual selection.

`command.begin` with `command: "solid.extrude"` accepts `inputs` containing
`sketchId`, `profileId`, signed `distance`, `units` and optional `name`/existing
`featureId`. For creation, Begin reserves `newFeatureId`/`newBodyId` once in
`activeSession`; preview and commit preserve that pair. `source` retains the
sketch/profile IDs. Partial `command.update` changes inputs without an undo entry;
API callers then explicitly invoke `command.preview`. Native field completion
uses these same two commands. A successful preview returns `geometryEvaluated`,
`previewed`, `geometry` (including `volumeMm3`, validity, bounds and source IDs),
`featureId`, `bodyId` and the normalized full `documentDraft`. Camera changes do
not commit or regenerate identities. `previewed: false` invalidates any prior
preview; retained IDs alone do not establish valid current geometry.

Canonical extrusion parameters are `{sketchId, profileId, distanceMm}`. The
supported extent is one side along the stored origin-plane normal, with signed
magnitude 0.001–100000 mm and New Body only. Negative distance reverses direction;
`units` performs the same mm/in conversion as other modeling inputs. Interactive
Begin starts at zero as an Alloy choice, with OK disabled until a valid preview.
The right Qt dock shows Profile Plane / One Side / Distance / New Body and an
opaque neutral solid with dark edges and a blue direction cue. Its distance
field uses displayed units; typed unit suffixes, a Flip button, interactive
handles, expressions and other extent/operation modes are not supported.

New extrusion commit appends one feature/body and hides the consumed sketch in
one authored candidate and one document undo entry. `feature.visibility` shows
it again; re-editing an extrusion preserves that intentional visibility. Cancel
preserves authored state and restores the original scene. Editing an extrusion
temporarily shows its source and hides the replaced body during preview. Editing
its source sketch temporarily shows the region and hides all direct dependent
extrusion presentations, then recomputes their solids on Finish. This is the
current graph's presentation policy, not general timeline rollback/suppression.
`viewport.gpu.editDisplay` (also `viewport.editDisplay`) reports `sourceSketchId`
and `hiddenBodyIds`. `gpu.finishedProfiles` reports actual profile summaries,
triangle count, visibility and selection; `gpu.extrusionCue` exposes canonical
normal-direction endpoints and distance. Temporary visibility is never saved.
Failed extrusion previews and stale sessions restore the committed presentation.
Missing selection returns `profile_required`; invalid distance or evaluated
references return `evaluation_failed`, preserving the document. Dependent-source
deletion and missing profile references also fail transactionally.

Combine accepts one target and one or more distinct tool bodies, including hidden
bodies explicitly identified by UUID. Join unites them, Cut subtracts the tool
group from the target, and Intersect retains their common volume. The result must
be one valid closed positive-volume solid; empty, disconnected, split or mixed
results fail with structured evaluation diagnostics and preserve the document.
The target body ID survives; `keepTools: false` consumes the tools from evaluated
body output while retaining their authored producer features. Keep Tools retains
the original tool bodies. A successful create/edit is one undo entry and rebuilds
through save/reopen and upstream edits.

For a disposable workflow, begin `solid.combine`, update its `inputs`, explicitly
preview, then commit/cancel using the returned session UUID. Begin reserves a
stable new feature ID; edits use existing `featureId` (also via `feature.editDialog`).
The dock's Target Body/Tool Bodies lists and `selection.pick` share that session.
Choosing target sets the next canvas role to tools; tool clicks toggle membership.
`solid.combineSelectionMode` restores input geometry before picking. Edit inputs
display the history stage before the feature, including previously consumed tools;
valid preview displays the stage through the feature while full downstream
evaluation still validates the candidate. Tools hide temporarily during preview,
even with Keep Tools enabled. `activeSession.presentationStage`, `geometry`,
`finalBodyCount` and `documentDraft`, plus `viewport.gpu.bodyStageKey` and
`displayedBodyIds`, distinguish presentation from authoritative document output.
Cancel and stale sessions restore current document presentation. Direct Boolean
extrusion operations and solid face/edge selection remain unimplemented.

All commits, undo/redo and reopen use the same ordered full evaluation of authored
sketches, fresh profiles and dependent solids. A downstream result resolves its
stable source from the same candidate, rather than a copied old BRep or list
index. Upstream width changes preserve a surviving profile ID and rebuild the
solid; failed dependency/evaluation candidates never publish a partial document.

Coincidence requires two points, fixed requires one point, H/V requires one line,
and parallel/perpendicular requires two lines. Fixed captures the point's current
canonical coordinates as its target. Dimensions use two distinct points or one
line's endpoints. Driving values are positive; omitted `value` captures the current
measurement. `driving: false` stores a derived measurement with no driving value.
Horizontal/vertical values are absolute distances; the solver retains the signed
endpoint branch from the seed. `units` accepts `mm`/`in` and defaults to active
sketch units; authored values and `dimensionValues[].valueMm` remain canonical mm.
Circle/arc Radius and Diameter use one curve entity; D defaults to Diameter on a
circle and Radius on an arc. Explicit `arcSweep` uses one arc and positive
`valueDeg`, preserving the arc's signed sweep direction. Its measured value is
also degrees, unaffected by mm/in display settings. Driven dimensions omit both
value fields. Radius/diameter edits leave unconstrained centers free; sweep edits
do not silently fix rotation or discard endpoint constraints. This Arc Sweep form
does not implement two-line angular dimensions or creation-time numeric locks.

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

The dimension tool previews selected targets, accepts canvas placement, then opens
an adjacent QLineEdit. D, native placement, Enter, Escape, label double-click and
label dragging dispatch the commands above. Authored dimensions optionally store
`annotation: {uMm, vMm}` in canonical sketch-plane coordinates. The editor accepts
finite numbers with optional mm/in suffixes for lengths or deg/° for Arc Sweep;
driven values are read-only. Diameter labels use the supported Ø glyph.
`app.state.inlineEditor` exposes visibility, text, read-only state and bounds.
`viewport.sketchAnnotations` exposes real line/witness/label layout, rotation and
logical-pixel hit bounds; active sessions expose pending placement/editing.
Captures composite the actual inline widget after the GPU frame.
`sketch.movePoint` remains a solved coordinate seed, not a physical point drag.
See the [bounded interaction gaps](research/commands/sketch-controls.md).

`viewport.renderer` is `OCCT OpenGL`; `viewport.gpu` contains live OCCT driver
information and qualification. `msaa` reports requested/capability-approved enabled/
maximum sample counts and texture multisampling support. The current Apple M4
candidate selects 4× with maximum 4; it is a rendering-parameter observation,
not an independent driver framebuffer sample-count proof. A fixed world-space
key and low ambient light distinguish shaded faces while profiles remain unlit.
`vendor`, `renderer`, and `version` come from the
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
use world XY placement and New Body; face-supported placement and direct extrusion
operation modes remain future work. Combine operates on existing bodies.
`feature.edit` uses canonical parameters, independent of
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
python3 tests/workflows/run.py --case sketch-extrude --evidence build/evidence/sketch-extrude
```

The runner launches the built application and records its shared-command results
under `build/evidence/`. The focused extrusion case creates a constrained sketch,
selects its finished face, previews and commits a prism, changes the upstream width,
checks failure preservation and undo/redo, then saves/reopens the chain. Earlier
transport, camera/framebuffer, startup/resize and primitive cases retain separate
evidence; use the cumulative checkpoint when a milestone requires them together.

The [primitive receipt](validation/primitives.md) adds a coherent create/edit/
pick/undo/reopen workflow and separate native keyboard/dialog observations.
Body and active-sketch point/line/profile selection are implemented; solid face/edge
selection is still DOC-006. The [sketch receipt](validation/sketches.md) and
[constraints receipt](validation/sketch-constraints.md) add drawing, solve,
driving/driven values, failed-draft preservation, one-entry Finish, cancellation,
undo and native reopen. Candidate-specific reruns remain identified separately.
The [annotation/profile receipt](validation/sketch-annotations.md) adds the focused
`sketch-annotations` case and separate native typing/drag/Finish checks. The
[extrusion receipt](validation/sketch-extrusion.md) adds `sketch-extrude`: real
finished-region selection, volume 5000 → 6250 mm³ from width 40 → 50, stable IDs,
failed-preview preservation, edit cancellation, paired undo and reopen. Native
E, distance typing, upstream label editing and undo/redo are separate evidence.
The `sketch-extrusion-checkpoint` case reuses accepted representative workflows once.
Full cumulative cases are reserved for material cross-cutting changes or milestones.
Native file dialogs and unsaved-work prompts exist, but observable pending
document decisions and full physical coverage remain DOC-013.

Future modeling work must use the same command/document path and extend state
with document identity, persistent entity references, units/parameters, geometry
summaries, selection, transactions, undo, save/reopen, and command cancellation
when those capabilities actually exist. Add real-app geometry/workflow tests
with each implemented modeling slice; do not fake geometry in automation.

Primitive UI fields invoke shared update/preview on field completion; explicit API
updates retain explicit preview semantics. `ui.activeDialog.presentation` reports
`docked-panel` or `floating-panel` for primitives, sketches and extrusion; `bounds` are logical
main-window-relative
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


For native-input diagnosis, `app.state.focus` reports the logical focus widget,
its object name, active-window state and native focus-window relationship. These
are observations, not proof that an OS key reached the application. In particular,
Qt may report native child-widget focus through its top-level window.
