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
`protocol`, `transport`, `endpoint`, legacy `socket`, `pid`, and `status: "ready"`. `--ready-file` writes the same
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
macOS/Linux use `unix-domain-socket`. Windows uses `windows-named-pipe` and a
fresh instance UUID name under the local `\\.\pipe\` namespace; remote names
and Windows `--control-socket` overrides are refused. This default is an Alloy
choice. Qt's user-access option protects the pipe. Registry and ready-file parent
directories must have a protected current-user-only ACL granting inheritable
full access; an existing unqualified policy fails closed rather than being rewritten.
On Windows choose a fresh ready-file parent directory, not a shared temporary root.

Windows Python requests use the packaged QtCore/Network `alloy-local-client.exe`
byte bridge. Resolve it from the installed/source build location or explicitly
configure `--transport-helper PATH` / `ALLOY_LOCAL_CLIENT`; discovery JSON never
selects executable code. The bridge has one absolute connect/write/read deadline,
64 KiB request and 1 MiB response limits. Python owns JSON/correlation validation
and kills/reaps the bridge on outer timeout. Legacy Unix clients retain their
existing socket inactivity timeout semantics. Timeout does not cancel an app command.

Build the helper alongside the app, or configure `tools/local-control` alone for
the Qt-only non-GUI test. The manual-only `local-transport.yml` Windows milestone
qualifies transport and owned-directory ACL handling, not desktop/GPU support.
See [the bounded receipt](validation/local-transport.md) for evidence and pending gates.

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
| `document.parameterAdd` | `name`, `units` (`mm`/`in`), and exactly one of positive numeric `value` or typed user-length `expression`; create one stable-ID user length in `document.inspect.userParameters` |
| `document.parameterUpdate` | `parameterId`, optional `name`, `units`, and at most one of `value` or `expression`; atomically rename or recompute all bound dimensions and extrusions. Formulas retain `referenceIds` by identifier occurrence; `document.inspect.userParameters` adds evaluated `valueMm` without changing authored formulas. |
| `document.modelParameterSet` | `target` copied from `document.inspect.modelParameters[]`, `expression` as an existing user length name or numeric `mm`/`in` literal; atomically rebind an already-authored Box, Shell, driving sketch length, Extrude length/start/end offset or retained open-chain offset. A numeric literal removes its binding even at the same value. |
| `document.parametersDialog` | Open native Change Parameters with separate user Expression/Value columns and component/feature-grouped Model Parameters; double-click an Expression cell to edit it. Unavailable during an active modeling command. |
| `document.tabs` | Active stable tab ID plus open design titles, paths, dirty and sketch-draft flags; also in `app.state.documentTabs` |
| `document.tabNew` | Create and activate an independent blank design; native plus and Cmd/Ctrl+N use this |
| `document.tabActivate` | `tabId`: restore document, accepted sketch draft/journal, selection and camera; pending input refuses |
| `document.tabClose` | `tabId`, optional `discard`; dirty close requires saved state or explicit discard; active pending command refuses |
| `document.new` | `discard`: boolean; dirty replacement refuses unless explicitly acknowledged |
| `document.save` | `path`: absolute `.alloy` path; atomic authored save, clean only after success |
| `document.open` | `path`: absolute `.alloy` path; optional `discard`: boolean, default false |
| `document.saveDialog`, `document.saveAsDialog`, `document.openDialog` | Native chooser wrappers dispatch those same file commands |
| `solid.box` | Required `width`, `depth`, `height`; optional `x`, `y`, `z`, `units`, `name`, `featureId` |
| `solid.cylinder` | Required `radius`, `height`; same optional placement/unit/name/edit fields |
| `primitive.box`, `primitive.cylinder` | Open their actual shared dimension dialogs |
| `solid.extrude` | Required `sketchId`, exclusive `profileId` or distinct `profileIds`, and one signed numeric `distance`, direct user length `distanceParameter`, or arithmetic `distanceExpression` for One Side / Distance. The formula uses stable ordered user-length UUIDs in authored `distanceReferenceIds`, captured `distanceExpressionUnits`, and a separate evaluated `distanceMm`; bare scalar results use entry units. Existing `units` (`mm`/`in`), `name`, `featureId`, operation and target options remain. Invalid formulas/ref changes refuse atomically. |
| `solid.pressPullDialog` | Optional `sketchId`, `profileIds`; Modify/Q opens the shared extrusion session before profile selection |
| `solid.extrudeDialog` | Optional `sketchId`, exclusive `profileId` or `profileIds`; toolbar/E opener for the shared extrusion session |
| `solid.combine` | Required `targetBodyId`, nonempty distinct `toolBodyIds`, `operation` (`join`/`cut`/`intersect`); optional `keepTools` (default false), `name`, existing `featureId`, `operation` (`newBody`/`cut`), one `targetBodyIds` entry for Cut; direct create/edit commit |
| `solid.combineDialog` | Same fields, all optional; open the shared Combine session, default Join with Keep Tools off |
| `solid.combineSelectionMode` | `role`: `target` or `tools`; return to input-body display for shared canvas picking |
| `sketch.create` | Open the docked origin-plane chooser |
| `sketch.begin` | `plane`: `XY`, `XZ` or `YZ`; optional `name`, `units`; begin an authored draft |
| `sketch.edit` | `sketchId`: UUID; re-enter a retained sketch |
| `sketch.line`, `sketch.rectangle` | `x1`, `y1`, `x2`, `y2`; optional `units`, `session`; append stable point/line entities and solve the draft. Rectangles also author four H/V constraints |
| `sketch.twoPointRectangle` | Command-first 2-Point Rectangle; choose plane or face, then first corner and pointer quadrant. In an active sketch, selects Rectangle without changing authored geometry |
| `sketch.twoPointRectanglePreview` / `sketch.twoPointRectangleAccept` / `sketch.twoPointRectangleCancel` | After first corner and pointer quadrant, enter separately labeled `widthText` and `heightText` (optional `session`). Preview is disposable; Accept publishes one origin-anchored rectangle with driving width/height, Cancel clears the typed input |
| `sketch.centerRectangle` / `sketch.centerRectanglePreview` / `sketch.centerRectangleAccept` | Direct center/width/height or typed inline width/height creates driving dimensions. With Center Rectangle selected, a center click then pointer corner click creates H/V edges and construction diagonals with the center retained but width/height free (two size DOF); subsequent D dimensions drive those sizes. Cancel discards typed preview without altering the draft |
| `sketch.centerToCenterSlot` | Required `firstX`, `firstY`, `secondX`, `secondY`, full `width`; optional `units`, `session`, `firstSnapTarget` and `secondSnapTarget` (`{kind:"origin"}` or `{kind:"point",pointId:UUID}`). The two cap centers, not the midpoint, retain independent snap relations. Creates two arcs, two sides and a construction guide without automatic dimensions; native Sketch > Slot uses three clicks for the same authority |
| `sketch.circleDiameterPreview` / `sketch.circleDiameterLock` / `sketch.circleDiameterAccept` | Pending center for Center Diameter Circle or first endpoint plus pointer direction for 2-Point Circle required; `text` or numeric `value`, optional `units`; disposable preview, session-only Tab diameter lock, then one atomic Circle + driving diameter edit on Enter/Accept. Return from the editable field also accepts directly |
| `sketch.twoPointCircle` | Command-first 2-Point Circle on an origin plane or selected supported face. Click the first diameter endpoint, move toward the second to choose direction, then type full diameter and press Enter; a second click can place at pointer separation. Uses the same `sketch.circleDiameterPreview` / `Accept` / `Cancel` inline commands and draft authority |
| `sketch.addTwoPointCircle` | `startX`, `startY`, `endX`, `endY`, positive `diameter`, optional `units`, `session`; `end` supplies direction while the typed diameter sets size. Retains the first endpoint, unit direction, circle radius and driving diameter |
| `sketch.circleDiameterCancel` | Close typed input while retaining the pending center or first diameter endpoint for pointer placement |
| `sketch.lineLengthPreview` / `sketch.lineLengthLock` | With Line and a pending first point, supply `text` or numeric `value` plus optional `units` (`mm`/`in`) and `session`. Preview is disposable; Lock stores a session-only positive length. Move the pointer to choose dominant horizontal/vertical direction, then `sketch.placePoint` commits one line with retained H/V relation and driving length dimension; the next point remains chained |
| `sketch.lineLengthCancel` | Clear the session-only typed Line value while retaining the chain start |
| `solid.extrudeDrag` | Signed `distanceMm`, optional logical `x`/`y` and `phase` (`move`/`end`); same update/preview authority as the native arrow |
| `sketch.addCircle` | `centerX`, `centerY`, positive `diameter`; optional `units`, `session`; append one analytic circle |
| `sketch.text` / `sketch.textDialog` / `sketch.textFonts` | Start Text before or after choosing a sketch face; open the native modeless controls, or list the exact bundled regular/bold font IDs and hashes |
| `sketch.textPreview` / `sketch.textHandle` / `sketch.textAccept` / `sketch.textCancel` | Disposable pending text preview with optional `text`, `fontId`, `heightMm`, `uMm`, `vMm`, `angleDeg`; handle events use `mode` (`move`/`rotate`), `phase` (`begin`/`move`/`end`), `startX`, `startY`, `x`, `y` in viewport logical pixels. Accept publishes one sketch draft edit; Cancel restores the original draft |
| `sketch.textAdd` | `text` (1-16 uppercase A-Z/0-9), `height` (cap height), `x`, `y`, `angleDeg`, optional pinned `fontId`, `units`, `session`; create one retained text object in an otherwise empty active sketch |
| `sketch.textUpdate` | `textId` and optional `text`, pinned `fontId`, `height`, `x`, `y`, `angleDeg`, `units`, `session`; edit the same number of ordered glyph profile slots atomically |
| `sketch.addArc` | `mode`: `center` requires `centerX`, `centerY`, `radius`, `startAngleDeg`, signed `sweepAngleDeg`; `threePoint` requires `startX`, `startY`, `endX`, `endY`, `throughX`, `throughY`; optional `units`, `session` |
| `sketch.tool` | `tool`: `select`, `line`, `rectangle`, `circle`, `centerArc`, `threePointArc` or `dimension`; choose the active drawing/selection tool |
| `sketch.selectionSet` | Optional `pointIds`, `entityIds`, `profileIds`: arrays of authored/evaluated IDs in the active sketch; omitted arrays clear that selection kind |
| `sketch.selectionPick` | `x`, `y`: logical pixels; optional `additive`: boolean. Pick an authored point, line, circle, arc or evaluated profile interior, sharing native selection state |
| `sketch.placePoint` | `x`, `y`: logical pixels; native ray/plane drawing path, or selection when the active tool is `select`/`dimension`. Pointer-created free Line endpoints near horizontal or vertical gain a retained H/V relation. Optional `suppressInference:true` suppresses this axis inference for that point; Control/Command sends it from native input. A Line started on a circle retains its on-circle endpoint; `tangentModifier:true` on its next click projects the free endpoint onto the tangent and retains the relation. An ordinary Line click near one uniquely selected tangent contact on a target circle retains its ending contact and tangent. An acquired endpoint stays exact, and conflicting inference is refused without moving earlier geometry. Direct `sketch.line` remains explicit and unchanged |
| `sketch.pointerMove` | `x`, `y`: logical pixels; disposable drawing preview with origin/point acquisition in `snap`. Pointer Line preview uses the same H/V classifier as commit and displays an H or V cue. Optional `suppressInference:true` hides that cue and leaves axis inference off. `tangentModifier:true` previews a tangent from a captured starting circle; the ordinary target-circle tangent preview uses the same contact resolver as commit and displays Tangent. Neither preview authors geometry |
| `sketch.lineArcDrag` | `phase: begin/move/end/cancel`, with viewport `x,y` for the first three phases. `begin` requires a press within 10 logical pixels of the active Line chain's latest committed endpoint. `move` previews a connected analytic Arc tangent to that Line with no authored change; `end` recomputes from the release point and commits one Arc plus one `arcTangent`, retaining the shared incoming endpoint ID. The Line tool remains active and its next click continues from the Arc end ID. `cancel`, Escape, a click without drag, and an invalid release leave the authored prefix unchanged. The outgoing Line receives no automatic Arc tangency |
| `sketch.lookAt` | View normal to the active sketch plane and fit populated draft geometry; empty sketches use the default origin view |
| `sketch.displayOptions` | Optional `showConstraints`, `showDimensions` booleans; independent session-only visibility, no modeling history entry |
| `sketch.constraintSelect` | `constraintId`, optional `additive`; select retained relation in Select mode |
| `sketch.constraintHover` | Optional `constraintId`; empty/omitted clears hover |
| `sketch.constraintPick` | Viewport-local logical `x`, `y`, optional `additive`; select an actual visible glyph hit target |
| `sketch.deleteSelection` | Optional explicit `constraintIds` and/or `dimensionId`; otherwise selected relations/dimension, then hovered relation; validates and solves one atomic draft edit |
| `sketch.finish`, `sketch.cancel` | Commit or discard the active sketch session |
| `view.projectSketchPoint` | `plane`, `uMm`, `vMm`; actual world-to-viewport projection |
| `sketch.construction` | `construction`: boolean; optional `entityIds` defaults to selected lines; keeps IDs/constraints, changes profile participation |
| `sketch.constructionToggle` | Toggle selected lines; mixed selection becomes construction, all-construction becomes normal; shared X/context/palette command |
| `sketch.constraint` | `type`: supported retained relation, including `symmetry`, `tangent` and `collinear`; optional `pointIds`, `entityIds`, `session`. Omitted targets use active sketch selection. `symmetry` takes three ordered line `entityIds`: first side, second side, construction axis; the solved endpoint correspondence is retained automatically and caller `pointIds` are refused. Explicit `tangent` takes an existing circle and straight line in either order, deriving a retained signed `tangent` relation whose contact lies on the finite line; it also accepts an arc and line sharing one authored endpoint ID, deriving a retained `arcTangent` relation. Caller `pointIds` and `side` are refused. `collinear` takes two distinct authored straight-line IDs in either order; shared endpoint identity and separated lines are supported. Other explicit Tangent pairs remain unavailable |
| `sketch.constraintTool` | `type`: persistent constraint-first tool; repeated picks apply shared retained relations, Escape returns to Select |
| `sketch.dimension` | `kind`: `linear`, `horizontal`, `vertical`, `radius`, `diameter`, `angle`, `arcSweep`, `pointLineDistance`, `parallelLineDistance` or `centerlineDiameter`; optional `pointIds` (two, or one with `pointLineDistance`), `entityId` (one line/curve), `entityIds` (two straight lines for line-pair kinds), `raySigns` (two values ±1 for angle), exactly one of numeric `value` (length), direct user `parameterName`, or local driving-length `expression` such as `d3 + 50`, or `valueDeg` (angle/sweep), plus `units`, `driving`, `annotation`, `session`. An expression captures entry units, binds local dimension UUIDs, and is exclusive with a direct user binding. Omitted targets use selection; `driving` defaults true |
| `sketch.dimensionPreview` | `x`, `y`: logical pixels; optional `kind` (`auto` or a dimension kind above), `driving`, `session`; preview selected targets |
| `sketch.dimensionPlace` | `x`, `y`, optional `session`; fix placement and show the actual inline numeric editor |
| `sketch.dimensionAccept` | Optional `text` or numeric `value` (length)/`valueDeg` (sweep), `units`, `session`; placed length text accepts numeric mm/in, a direct user length name, or a same-sketch expression. Angle text accepts deg/°; formulas exclude angles |
| `sketch.dimensionCancel` | Optional `session`; abandon pending dimension placement/editing or a Driven offer |
| `sketch.dimensionOfferAccept` / `sketch.dimensionOfferCancel` | Required `offerId`, optional `session`; accept or discard a pending redundant new numeric dimension as a read-only Driven measurement. `sketch.dimension` and placed `sketch.dimensionAccept` return `pending: true` with stable `offerId` and `dimensionId` when eligible; `app.state.activeSession.pendingDrivenOffer` exposes the same offer. Accept rechecks the original draft, tab, session, document revision and draft-history position; Cancel changes neither draft nor history. Expressions, direct user bindings, existing-dimension edits and pre-solver duplicate-kind refusals do not offer conversion in this slice |
| `sketch.dimensionEdit` | `dimensionId`, optional `session`; open a driving editor or read-only driven measurement |
| `sketch.annotationSelect` | `dimensionId`; select a retained label |
| `sketch.annotationPick` | `x`, `y`: logical pixels; actual projected/rotated label hit test |
| `sketch.annotationMove` | `dimensionId`, `uMm`, `vMm`, optional `gestureId`, `session`; repeated moves with one gesture identity coalesce into one draft undo entry; move placement only, retaining exact authored points/values |
| `sketch.annotationEndGesture` | `dimensionId`, `gestureId`; end annotation drag coalescing without authoring geometry |
| `sketch.dimensionUpdate` | `dimensionId`, exactly one numeric `value` (length), direct user `parameterName`, local `expression`, or `valueDeg` (angle/sweep); optional `units`, `session`. Numeric or direct-user replacement clears the prior local formula. Reaccepting unchanged formula text and entry units retains its original UUID bindings even if a later name collision exists |
| `sketch.movePoint` | `pointId`, `x`, `y`; optional `units`, `session`; seed a point's coordinates and solve the copied draft |
| `feature.edit` | `featureId`: UUID; `parameters`: partial canonical-mm fields such as `widthMm` |
| `feature.editDialog`, `feature.delete` | `featureId`: UUID; actual primitive/sketch/extrusion/Combine editor or one undoable deletion; invalid downstream dependencies are rejected |
| `feature.visibility` | `featureId`, `visible`: boolean; one authored undoable visibility edit, also used by Browser checkboxes |
| `geometry.inspect` | Evaluated bodies and sketches: stable IDs, validity, geometry summaries and plane frames |
| `history.undo`, `history.redo` | Active sketch: Undo cancels a pending editor/tool first, then traverses accepted draft edits; Redo requires Select. Otherwise traverse committed document history. `activeSession.draftHistory` exposes index/count/canUndo/canRedo |
| `selection.set` | `bodyId`: visible UUID, or empty string to clear |
| `selection.profileSet` | `sketchId`, `profileId`; optional `additive` or `toggle` modifies same-sketch region selection |
| `selection.sketchSet` | `sketchId`; Browser sketch selection, with sole-region resolution when opening Extrude |
| `selection.pick` | `x`, `y`: viewport-local logical pixels; actual body or finished-profile picking; optional `additive`/`toggle`; returns `kind` and retained IDs |
| `view.projectBody` | `bodyId`: UUID; actual projection of bounding-box center to logical pixels |
| `layout.browser`, `layout.timeline`, `layout.navigation` | `visible`: boolean |
| `layout.reset` | Restore those three regions |
| `view.grid`, `view.origin` | `visible`: boolean |
| `view.orientation` | `name`: `isometric`, `rearIsometric`, `top`, `bottom`, `front`, `back`, `right`, or `left`; recenter a named fixed view on the fitted scene |
| `view.zoom` | `factor`: 0.1–10; resulting scale clamps to 0.1–10 |
| `view.pan` | `x`, `y`: relative logical-pixel deltas, each within ±10000 |
| `view.orbit` | `deltaX`, `deltaY`: relative logical-pixel deltas, each within ±10000; orbit the real camera around the active scene/custom pivot in the selected mode |
| `view.orbitMode` | `mode`: `free` or `constrained`; the visible Orbit menu retains the selected mode. Constrained uses world Z up and a level horizon |
| `view.orbitTool` | `active`: boolean; visible Orbit control lets a left canvas drag use the selected mode; Escape exits |
| `view.orbitCenterBegin` / `view.orbitCenterPick` | Start a one-shot visible-surface pivot pick, then provide viewport-local logical `x`, `y`; the result includes the exact body surface hit and camera state. A staged body preview refuses a pivot pick |
| `view.orbitCenterCancel` / `view.orbitCenterReset` | Cancel the one-shot pick or restore the fitted-scene pivot; neither edits the document |
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
`sketchId`, exclusive `profileId` or `profileIds`, signed `distance`, `units` and optional `name`/existing
`featureId`. For creation, Begin reserves `newFeatureId`/`newBodyId` once in
`activeSession`; preview and commit preserve that pair. `source` retains the
sketch/profile IDs. Partial `command.update` changes inputs without an undo entry;
API callers then explicitly invoke `command.preview`. Native field completion
uses these same two commands. A successful preview returns `geometryEvaluated`,
`previewed`, `geometry` (including `volumeMm3`, validity, bounds and source IDs),
`featureId`, `bodyId` and the normalized full `documentDraft`. Camera changes do
not commit or regenerate identities. `previewed: false` invalidates any prior
preview; retained IDs alone do not establish valid current geometry.

Canonical extrusion parameters preserve legacy `{sketchId, profileId, distanceMm}`
or use `{sketchId, profileIds, distanceMm}` with distinct stable IDs. Plural regions
must come from one earlier sketch. New Body requires one connected planar face;
those regions are united before one prism. Cut permits disjoint regions, uses
ephemeral prisms and retains one explicit `targetBodyIds` entry in the existing
body. It authors no cutter bodies. Editing cannot switch between New Body and
Cut in this bounded slice. Missing/disjoint/overlapping references reject before
commit. Body summaries expose `sourceProfileIds` and `sourceAreaMm2`.

Before a distance is entered, ordinary native/API profile picks collect distinct
regions in the active extrusion command. Shift/`additive` adds; platform modifier/
`toggle` toggles. Outside the command an ordinary pick replaces selection.
`activeSession.inputs`, selection and GPU inspection expose actual `profileIds`.
 The
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
Option/Alt-left drag and middle drag use shared `view.pan`; Shift-middle drag
and the checked Orbit control's left drag use `view.orbit` in the selected mode. These gestures
are handled before modeling picks and leave authored state and selection unchanged.
The custom camera direction/up, orbit mode, view center and custom pivot survive
refresh and design-tab switching. Home fits the scene and retains an explicitly
picked pivot; named views recenter on the fitted scene. A blank-canvas context
menu starts an exact visible-body surface pick or resets the pivot. Its green
marker remains separate from model selection. The initial pivot is the fitted
scene center; constrained Z-up, the five-degree pole margin and Home pivot
retention are explicit Alloy policies. Trackpad gestures, configurable up axis,
ViewCube and an automatic orbit-center policy remain separate navigation work. Plain left click uses
`selection.pick` outside sketch editing. Active-sketch clicks use the current
drawing/selection tool; Shift-click adds/toggles sketch selection. Selected bodies
are visibly blue; clearing selection restores normal appearance without changing
the authored document.


For native-input diagnosis, `app.state.focus` reports the logical focus widget,
its object name, active-window state and native focus-window relationship. These
are observations, not proof that an OS key reached the application. In particular,
Qt may report native child-widget focus through its top-level window.

Circle-center midpoint inference is available through the shared pointer flow:
`sketch.pointerMove` reports an acquired midpoint target with stable `entityId`;
first `sketch.placePoint` retains it in `activeSession.firstPoint`; the second
click atomically authors the circle and `midpoint` point-to-line constraint.
`viewport.gpu.sketchSnapCue.marker` is `midpoint-triangle`; persistent midpoint
glyphs include point IDs and target line ID. Hover and first click do not enter
draft history.

### Circle circumference tangent inference

During a circle's second pointer stage, `sketch.pointerMove` returns `snap` with
`target: {kind: "tangent", entityId: lineId}`, snapped contact coordinates and
`radiusMm` when acquired. `sketch.placePoint` uses the same resolver and atomically
retains `{type: "tangent", entityIds: [circleId, lineId], side: 1|-1}` alongside any
center midpoint relation. No hover/first-center authored history entry is added.
GPU diagnostics expose `sketchSnapCue.marker: "tangent-square"` and persistent
constraint glyph identity, contact, bounds and selected state. See
[bounded tangent validation](validation/sketch-tangent.md) for limits.

Pointer-created Lines have a separate bounded circle-contact route. The first
circle click projects onto its circumference and retains `pointOnCurve`. Shift
on the next Line click retains a contact-point `tangent` relation. An ordinary
click near a unique tangent contact on a target circle also retains both
relations. The resolver uses a 10-pixel candidate radius and refuses candidates
whose nearest distances differ by no more than 0.5 pixel; these are Alloy input
policies. The retained contact-point tangent requires an authored Line endpoint
and its matching on-circle relation. Existing tangent records without a contact
point keep their prior finite-line solver behavior. See
[pointer contact validation](validation/sketch-pointer-circle-tangent.md).

### Sketch corner fillets

`sketch.tool {tool:"fillet"}` enters corner picking. One `sketch.placePoint` at a
connected normal-line junction previews a single fillet and focuses radius entry.
Enter accepts it; a second corner click instead collects a common-radius pair.
Adjacent-side picking remains available. `sketch.filletPreview {value,units?}` keeps a
disposable candidate separate from authored state. `sketch.filletAccept` commits
one edit and reports `createdArcIds`/`removedDimensionIds`; `sketch.filletCancel`
discards pending picks/preview. Direct `sketch.fillet {corners:[{entityIds:[a,b]}],
value,units?}` shares the core mutation. Finish rejects pending input, Undo exits
the tool before changing history, and raw pending input blocks tab switches.
See [fillet qualification](validation/sketch-fillet.md), including the open native
accessibility blocker.

Exactly exhausted shared horizontal return lines are supported for coincident
center/contact equal-radius paired arcs. Surviving line IDs persist; consumed
width dimension IDs become driven point-pair reference spans. Preview diagnostics
report `convertedReferenceDimensionIds`, `removedConstraintIds` and
`removedEntityIds` alongside removed dimensions. The visible impact notice
explains conversion/retirement before Enter. Negative remainder or unsupported
unrelated references refuse atomically. See [shared-line qualification](validation/shared-line-fillets.md).



### Positive extrusion end-cap sketches

Body summaries expose `supportFaces` with stable producer/body/face UUIDs and the
evaluated frame. Pass exactly those three IDs as `support` to `selection.faceSet`,
then `sketch.create`; direct face support is also accepted by `sketch.begin`.
`selection.facePick` uses viewport-local logical pixels and refuses unsupported or
occluded faces. `view.projectSketchPoint` accepts exactly one origin `plane` or
`sketchId`; use `sketchId` for supported sketches, including the active draft.
Resolved frames are evaluation/session state, never serialized sketch placement.
This slice supports positive extrusion end caps only. Existing sketch mutation,
history and Finish/Cancel commands remain authoritative.


New supported-face sketches automatically retain available straight-edge
projection references. `activeSession.projectionContext.lines` exposes their
virtual entity/point IDs, current local coordinates and exact source reference.
Body `supportFaces[].edges` exposes semantic edge IDs and source provenance.
`sketch.selectionPick` can return a projected line with `projected`/`source` fields;
constraint commands use those same entity IDs. `projectedLines` and
`unsupportedProjectedEdges` in evaluated sketch summaries distinguish resolved
references from unsupported curves. Saved `parameters.projections` contain IDs
and sources only. References are read-only, fixed solver input, and do not create
profile regions in this bounded implementation.


Supported-cap `projectionContext.points` exposes circular boundary center
candidates with stable ID, source role `curveCenter`, local coordinates and
`retained` state. Pointer acquisition is observable without authored mutation.
Accepting a line endpoint/circle-center snap retains `pointReferences` with only
ID and source, plus the local Coincident relation, in the same transaction.
`availableSupportPoints` includes candidates; evaluated `projectedPoints` includes
retained centers only. These derived points are read-only. Full curve projection
and direct dimensioning of an unretained center are not implemented.
For a face sketch, Circle's first-center pointer snap selects a unique projected
`curveCenter` over the generic sketch origin when both coincide. Hover and
commit use the same candidate rule; distinct overlapping source centers return
`ambiguous_snap`. Other tools and non-face sketches retain their origin priority.


## Inline diameter and five-profile Cut

A circle center followed by typed input uses the shared inline editor with
`inlineEditor.kind: circleDiameter` and `activeSession.pendingCircleDiameter`.
Preview does not change authored geometry. Accept creates the circle, owned
center, applicable snap relation and driving diameter in one draft transaction.
Escape closes the editor and leaves the center available for pointer placement.

Cut sessions accept `operation: cut` and one `targetBodyIds` entry. Creation may
discover a single visible intersecting body; ambiguity refuses rather than
selecting silently. Edits retain captured target identity, including hidden
targets. Earlier supported-sketch edits resolve the history prefix before that
sketch, independent of the final Boolean-modified body's available supports.
For a fresh face-sketch `Cut` with `extent: all`, an absent or empty target array
resolves to the unique visible, identity-placed solid carrying that sketch's
exact retained support face. The resolved body ID is stored as `targetBodyIds`;
explicit target values, including malformed values, follow ordinary validation.
An existing extrusion reopened with `featureId` restores its saved sketch and
ordinary profile IDs unless those source fields are explicitly overridden.
An idle viewport pick does not create a command session when nothing is hit.

At zero distance, `previewHandle` can be true while `previewed` is false and OK
is disabled. An invalid intermediate distance retains the arrow for recovery.
Successful Cut exposes `cutPreviewToolCount`, `cutPreviewSelectedProfileCount`
and `extrusionCue` under viewport GPU diagnostics; `lastArrowDrag` records the
shared command's distance, phase and coordinates. Its `source` is diagnostic
metadata, not independent proof of physical input: API callers can supply
coordinates too. Native input evidence is recorded separately.

See [the five-circle Cut receipt](validation/five-circle-cut.md) for qualified
behavior and remaining tutorial/runtime limits.

Constraint interaction state is inspectable in `activeSession.displayOptions`,
`selection.constraintIds` and `activeSession.hoveredConstraintId`. The viewport's
`gpu.sketchConstraintGlyphs` exposes stable relation and target IDs, visible
logical-pixel bounds, and selected/hovered flags. Hidden glyphs have no hit
targets; hiding a class clears its selection/hover. Display preferences currently
last for the active sketch session and are not serialized as authored CAD data.
See [the constraint interaction receipt](validation/constraint-interaction.md)
for native-input evidence and remaining glyph limitations.

## Tutorial recording

With FFmpeg on PATH, `recording.start` takes an absolute fresh `.mp4` `path` and
optional `fps` (1–30, default 15). `recording.step` takes a caption `text` up to
200 characters. `recording.status` reports active state, paths, frame/repeat
counts and failures. `recording.stop` drains the encoder and returns only after
completion or an explicit error. Existing destination/sidecar files are refused.
Captures combine Qt chrome and the real GPU framebuffer, with caption bands,
observed app-scoped input and command provenance. No global keyboard hook runs.
Editor text is redacted except allowed numeric entries; native popup capture is
not guaranteed. Qt propagation can produce non-spontaneous events in the ledger;
these are not proof of a separate physical gesture. Pointer/ring overlays follow
all delivered Qt mouse events, including automated events, and display a `Qt input`
label. Receipts identify this policy as `pointerOverlay: delivered_qt_events`;
the ledger retains each event's provenance. Recording does not change the
authored document. Keep movies, JSONL logs and metadata outside Git.

The same external-FFmpeg/app-framebuffer path is intended for Windows and Linux;
it avoids OS desktop capture permissions. At each native platform milestone,
install a trusted FFmpeg build with libx264 on PATH, run the short smoke recipe,
and independently verify GPU capture, overlays, duration and completion before
claiming that platform qualified. Only macOS is qualified today. FFmpeg is not
bundled in published Alloy packages. See [video validation](validation/tutorial-video.md).

## Bounded retained Extrude Join

`solid.extrude`, `solid.extrudeDialog` and `solid.pressPullDialog` accept
`operation: join` with one stable `targetBodyIds` entry. Omitting the target on
creation discovers exactly one visible body that can form one connected union
with positive added volume; ambiguity fails instead of choosing another body.
Dialog previews are disposable and share the authored/evaluation authority.
Join keeps target body/producer identity and adds `lastModifierFeatureId`,
`operation: join`, `targetBodyIds`, `addedVolumeMm3` and source-profile summaries.
An exact surviving single positive cap receives Join-feature support provenance
for the next face sketch. Existing features retain their operation while edited.
See [Join contract and bounded evidence](validation/profile-join.md).

`sketch.circle` starts Center Diameter Circle before support selection, with
optional `plane`, `support`, `name` and `units` matching `sketch.create`.
Without support it returns the existing `sketch.plane` session plus
`pendingTool: circle`. `sketch.begin` or native support selection consumes that
pending intent and returns `sketch.edit` with `tool: circle`. In an active
sketch, invoke without arguments to activate Circle. `sketch.cancel` discards
both pending support selection and uncommitted circle drafts.

Overlapping sketch profiles expose one derived UUID per actual atomic bounded
region through existing `document.inspect`/selection commands. Each split
boundary's `edgeProvenance.sourceEntityIds` lists all coincident authored straight
sources; `entityId` is the stable canonical member. Region IDs use retained source
contacts/ordered intersection branches. Changed incident source sets may
intentionally invalidate a profile reference; upstream feature evaluation fails
atomically with `profile_unavailable` instead of selecting another face. No
composite full-circle shortcut is added for an untrimmed crossing sketch.

### Command response deadline

The client waits up to30seconds for a command response; `--timeout` overrides
this bound. Instance identity probes remain2seconds. Loft with downstream Shell
was observed taking5.2seconds for redo and8.2seconds to reopen in the GPU app,
while core evaluation was under1second for the whole focused case. The previous
5second client deadline could abandon a valid in-flight operation. A timeout
does not cancel or roll back a command: inspect the document before retrying.
This client bound is not an interactive performance claim; render/recompute
responsiveness remains subject to future optimization. Workflow transcripts now
record elapsed command time alongside the actual result.

### Local component ownership (bounded ASM-004)

`component.create {name,activate?}` authors a named local definition and its
separate identity-placement root-child occurrence in one CAD transaction.
`activate` defaults to true as an explicit Alloy choice. `component.newDialog`
opens the same Name/Activate UI; `component.activate {definitionId}` selects a
root or child authoring owner for this design without CAD history or revision.
`component.inspect` reports root/active owner and authored definitions/occurrences.
New features belong to the active definition; edits preserve their original owner.
Cross-component modeling dependencies are unsupported and fail explicitly.
Native reopen activates root. `document.inspect` includes active owner and real
evaluated ownership; authored JSON contains only persistent definitions/occurrences.

### Linked component placement and assembly presentation

`component.copyOccurrence {occurrenceId,placement}` creates a linked subtree.
Placement contains `axis:"Z"`, `angleDeg`, and `translationMm:[x,y,z]`, in the
parent's coordinates. The result returns `rootOccurrenceId`, `occurrenceMap`
and, when internal joints are copied, `jointMap`. Source definitions stay shared;
external joints are not duplicated. `component.inspect.localInstances` reports
occurrence-qualified world poses and bounds. Selection of shared geometry needs
its `occurrenceId` as well as `bodyId`/`edgeId`.

For the native clipboard flow, call `component.copySelection {occurrenceId}`
then `component.pasteOccurrenceDialog {}`. The clipboard is local to the original
design/tab; `component.clipboardInspect` resolves its current source. Paste opens
an active `component.copyPlacement` session. Use its returned `id` with
`command.update {session,inputs:{xMm,yMm,zMm,angleDeg}}`, then `command.commit`
or `command.cancel`. The native arrows invoke
`component.copyHandle {field:"xMm"|"yMm"|"zMm",value}`;
`component.copyGestureEnd` ends a pointer gesture without committing. Preview
keeps candidate IDs stable and authored state unchanged. Native Enter commits
once and Escape cancels. A changed document or stale clipboard refuses the
pending operation. Direct `component.pasteOccurrence {placement}` is also
available without the interactive session.

`joint.asBuiltDialog {type?,component1OccurrenceId?,component2OccurrenceId?}`
opens the shared Revolute/Cylindrical command (`type` is `revolute` or `cylindrical`,
default Revolute). `joint.componentRole {slot:"component1"|"component2"}` arms
Browser/viewport picking; `joint.componentPick {occurrenceId,slot?}` fills that
same slot. The active session advances `selectionRole` to `position` when both
are selected. `joint.typeSet {type}` retains the session and participant IDs,
clears Position/coordinates, and revalidates; editing an existing joint does not
permit changing its type or participants. `joint.originPick {position}` accepts
the exact retained reference and Component 2 occurrence shown by
`joint.originInspect`. Generic `command.update/commit/cancel` remain authoritative.
See [shared dialog evidence](validation/as-built-native-selection.md).

`joint.previewAngle {jointId,angleDeg}` stages a bounded retained mechanism pose;
`joint.inspect` reports the solved assembly. `position.revert {}` restores the
retained pose without a history edit, matching the contextual Position/Revert
control. Finish or revert staged motion before copying or exporting.
`joint.originsVisibility {visible}` toggles retained origin glyphs without
changing authored data. See [origin contracts](validation/joint-origin-inference.md)
and [copied mechanism limits](validation/copied-piston.md).

`component.rename {definitionId,name}` updates the shared definition's label;
`component.renameDialog {occurrenceId}` exposes the same transaction. It preserves
occurrence identity and geometry. Browser organization routes through
`component.createFromBodies`, `component.create` and
`component.reparentOccurrence {occurrenceIds,destinationId}`; the latter takes a
parent definition or root document ID. See [native Browser evidence](validation/piston-components.md).

STEP/STL export accepts occurrence subtree scope and preserves placed linked
instances. A shared `bodyId` must be qualified by `occurrenceId` when ambiguous;
whole-visible scope uses `allVisible:true`. Export uses retained poses and refuses
uncommitted assembly motion. Exact format arguments remain discoverable through
command schemas. See [placed output contract](validation/linked-instance-export.md)
for verified scope, neutral-file readback and native-dialog limits.

## Face-positioned Sphere candidate

`solid.sphere` directly creates/edits a retained face-positioned Sphere; the
same schema opens `solid.sphereDialog`. Inputs are `support` containing
`bodyId`/`producerFeatureId`/`faceId`, face-local `uMm`/`vMm`, canonical
`diameterMm`, `operation` (`cut` or `newBody`), optional `targetBodyId` for Cut,
`featureId` for editing, and `name`. Cut defaults to the support body.
A dialog can omit the face and center: `selection.pick` first chooses an actual
supported face, then places the center through a second viewport ray. An explicit
support starts at center placement. `selection.faceSet`/`facePick` also bind the
same support. `view.projectFacePoint {support,uMm,vMm}` returns logical viewport
coordinates using the exact resolved face frame. Session `selectionRole` reports
`face`/`center`/`placed`. `command.update`, `preview`, `commit` and `cancel` reuse
authoritative authored state and disposable real geometry. Preview carries the
evaluated result summary and GPU `previewAppearance` diagnostics expose actual
color/transparency. `solid.repeatSphere {support}` clears center and binds that
new face, preserving last accepted diameter as an explicit Alloy choice.
See [the candidate evidence limits](validation/face-sphere-controls.md).

## Point-to-straight-edge dimensions

`sketch.dimension` supports `kind: pointLineDistance` with one `pointIds` entry
and `entityId` for the straight sketch/projected line, plus ordinary `value`,
`units`, `driving` and `annotation`. Native D/center/edge selection uses the same
IDs. `sketch.dimensionPreview` and `dimensionPlace` expose the separate annotation
placement phase; `dimensionAccept`, `dimensionUpdate`, `dimensionEdit` and
`annotationMove` reuse the existing draft/history authority. Authored `side`
retains the initial driving side of the infinite supporting line. Positive
values drive a real PlaneGCS equation; driven values measure absolute distance.
Curved and degenerate lines and conflicting dimensions are refused atomically.
See [the bounded evidence and explicit semantics](validation/point-edge-dimensions.md).
### Timeline viewport navigation (HIS-005)

`timeline.showLatest` scrolls to the latest active-component feature;
`timeline.showFeature {featureId}` scrolls to one stable feature in that Timeline.
Both are view-only and preserve authored data and CAD history. The visible button
is **Show latest**, not a model-history Move to End command.
`app.state.ui.timeline` exposes actual tile widths, bounds, complete names/types,
tooltips/accessibility names, fully-visible flags and scrollbar position/range.
Fixed glyph targets preserve existing single-click edit and context actions.
See [the bounded input/capture receipt](validation/timeline-overflow.md).

### Centerline and parallel-edge diameter

In a sketch, `sketch.centerline` accepts `entityIds` and boolean `centerline`;
`sketch.centerlineToggle` converts the current selected authored lines. IDs and
relations survive conversion. Centerline is boundary-capable, separately stored
from Construction. Explicit conversion clears the other linetype intent.

`sketch.dimension` supports `kind: "centerlineDiameter"`, two `entityIds` (one
Centerline, one parallel ordinary line), `value`/`units` for driving or
`driving: false` for read-only measurement. Authored refs normalize to axis first
and store retained side. D and dimension preview/place/accept recognize the same
pair and display the full mirrored diameter. It adds one half-distance equation;
independent orientation constraints remain necessary to keep both lines parallel.
Nonparallel, zero, degenerate or conflicting results refuse transactionally.

Revolve native controls share `solid.revolveDialog`, `solid.revolveSelection`
(Profile/Axis role and optional clear), and `selection.pick`. For angular gestures,
`solid.revolveDrag` accepts viewport-local logical x/y plus begin/move/end; angle
comes from the actual projected ring, not an angle argument. Its world/screen
geometry is in `viewport.gpu.revolveCue`, with derived inputs in `angularDrag`.
Full requires extent full and canonical angle360 together. Same-sketch Project
Axis is identity; external axis projection is unsupported. See the
[bounded native receipt](validation/revolve-controls.md).

`app.state.ui.ribbon` reports actual modeling ribbon button IDs, full text,
accessible names, tooltips, enabled/icon/focus state and logical bounds.
`fullyVisible` checks each widget against the ribbon rectangle after layout.
It is inspection only and does not change command routing or authored data.

### Retained Extrude Intersect

`solid.extrude` and `command.begin({command:"solid.extrude",inputs:{...}})` support
`operation:"intersect"`, existing coplanar profile refs, signed distance/units and
one `targetBodyIds` entry. Initial discovery only selects one visible same-owner
eligible body; ambiguity requires explicit selection. Target IDs persist for
recompute. The operation retains body/producer identity, authors one modifier
with no cutter body, and publishes overlap volume. Full containment is valid;
empty/touch/disconnected results refuse atomically. Native operation/target fields
share the same authority. `solid.extrudeDialog` remains selection-oriented; provide
full initial modeling inputs through `command.begin` or update the session. See
[the bounded contract and evidence](validation/extrude-intersect.md).

## Origin Section Analysis

`inspect.sectionDialog` opens the native preview panel; `inspect.section` creates
or edits a retained section using `plane: XY|XZ|YZ`, optional `analysisId`, `name`
and `hatch`. `analysis.inspect` reads document-owned records; `analysis.visibility`
uses `analysisId` and `visible`. Show deactivates the previous section in the same
undo entry. Shared command session update/preview/commit/cancel applies. Analysis
records do not enter the modeling Timeline or change evaluated geometry. Fixed
positive world half-spaces and bounded verification are described in
[the Section Analysis receipt](validation/section-analysis.md).

While `activeSession.command` is `sketch.edit`, `inspect.sectionDialog` accepts
only an existing `analysisId`. The returned edit ID appears separately at
`app.state.sectionEdit`; use that ID for `command.update`, `command.preview`,
`command.commit` or `command.cancel`. The sketch's `activeSession` ID, draft and
selection remain authoritative. This bounded sub-edit permits name, offset,
Flip and hatch changes while keeping the analysis support fixed. Its Cancel
discards the section preview; OK commits one analysis history action and returns
to the same sketch. Other mutation commands wait until the section dock closes.

The tangent-plane candidate adds `construction.tangentPlane` with a retained
`tangent` object: `face: {producerFeatureId, bodyId, faceId}` and
`point: {bodyId, edgeId, endpoint: 0|1}`. It creates ordinary construction-plane
history and returns `planeId`; sketches attach using the existing `planeId` input.
`featureId` edits the source. `command.begin` supports this target with native
preview/Cancel controls. The source must resolve to one current face and an exact
oriented edge endpoint on that trimmed face. Candidate qualification is recorded
in [the receipt](validation/tangent-point-plane.md).

## Straight-line Extend

In an active sketch, `sketch.tool {tool:"extend"}` enables the Modify > Extend
pointer tool. `sketch.extendPreview` and `sketch.extend` both take
`{entityId,uMm,vMm}`: the retained authored source line ID and a sketch-plane
point nearer the endpoint to extend. Preview reports `extendedSegment` through
`extendPreview.segment` without changing the draft or history. The accepted
command moves that same endpoint to the nearest forward intersection with a
finite authored straight line, preserving source line and point IDs. It adds
Coincident for an exact target endpoint or pointOnCurve for a finite target
interior. A tied/overlapping nearest hit, missing intersection, unsupported
curve or incompatible existing constraint refuses atomically. The tool remains
active after a successful click; Enter exits it. Other curve types and general
Extend remain under SKT-011.

## Retained trimmed-face inset

In a fresh sketch on a supported planar face, `sketch.tool {tool:"offset"}` and
an actual support-boundary click enter the shared closed-loop preview. The
same authority is available through `sketch.loopOffsetPreview` with the exact
`outerLoopId`, `kind:"planarInsetV1"`, `chain:true`, and an inward negative
`distance` in session units; `sketch.loopOffsetAccept` commits the valid
preview, and `sketch.offsetCancel` or Escape discards it. This explicit kind
produces one interior profile with retained edge/contact ancestry. A positive
side refuses and clears any previous valid pending preview. Saved loop offsets
without `kind` retain the earlier rounded-annulus meaning. See
[the bounded cap receipt](validation/trimmed-seam-inset.md).
