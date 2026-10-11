# Local agent control

Alloy's running native application provides a small local control API from its
first shell build. It is an Alloy protocol, not the reference CAD application API. No geometry or
CAD document commands are implemented yet.

## Launch and discover

```sh
./build/alloy.app/Contents/MacOS/alloy --ready-file /tmp/alloy-ready.json
```

On Linux, use `./build/alloy`. The app prints one startup JSON line containing
`protocol`, `socket`, `pid`, and `status: "ready"`. `--ready-file` writes the same
object atomically after the window is shown. Each launch defaults to a fresh
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
means command state has been applied synchronously. It is not a CAD evaluation
receipt or an OS compositor fence. `app.quit` returns success before scheduling
exit. `capture.screenshot` completes only after the PNG is committed to disk.
The client correlates request IDs and exits 0 for success, 1 for an application
error, or 2 for a client/transport error.

`commands.list` supplies stable IDs, titles, availability, descriptions, and
required parameter type/enum/range schemas. Unknown/missing/incorrect arguments
fail without applying the command. Error codes include `invalid_request`,
`request_too_large`, `unknown_command`, `not_implemented`, `invalid_arguments`,
and `capture_failed`.

## Implemented commands

| Command | Arguments / result |
| --- | --- |
| `commands.list` | Registry with both implemented and unavailable entries |
| `app.state` | Real shell/window/view state, stage, kernel discovery, zero geometry counts |
| `commands.search`, `commands.close` | Show/dismiss the native search dialog |
| `layout.browser`, `layout.timeline`, `layout.navigation` | `visible`: boolean |
| `layout.reset` | Restore those three regions |
| `view.grid`, `view.origin` | `visible`: boolean |
| `view.orientation` | `name`: `isometric`, `top`, `front`, or `right` |
| `view.zoom` | `factor`: 0.1–10; resulting scale clamps to 0.1–10 |
| `view.pan` | `x`, `y`: relative logical-pixel deltas, each within ±10000 |
| `view.home` | Reset orientation, scale, and pan |
| `capture.screenshot` | Absolute `.png` path; returns dimensions, device scale, source |
| `app.quit` | Exit the app after replying |

State includes Browser/Timeline/navigation/search visibility, logical window and
viewport sizes, device scale, reference-grid orientation, zoom, pan, grid/axis
visibility, and an applied-command revision. Revision advances for successful
view/layout/search commands, including a repeated no-op command; it is not a
persistent document revision or a tracker for every OS resize/interaction.

The screenshot is `QWidget::grab()` of the actual main window, including the
reference view and app UI. It excludes native OS decorations, menus, and separate
dialogs; close search before a clean capture. It is real widget rendering,
not a desktop screen capture or geometry/GPU validation. The shell uses QPainter;
replace or adapt this capture path when integrating an OCCT viewport that needs
its own framebuffer capture.

## Verification and extension contract

```sh
ctest --test-dir build --output-on-failure
```

The test launches the built app, discovers commands, changes view and layout,
opens/closes search, checks structured errors and malformed JSON, verifies state,
captures two distinct real-window PNGs, restores the initial presentation, and
quits. Evidence is written to `build/evidence/`.

Future modeling work must use the same command/document path and extend state
with document identity, persistent entity references, units/parameters, geometry
summaries, selection, transactions, undo, save/reopen, and command cancellation
when those capabilities actually exist. Add real-app geometry/workflow tests
with each implemented modeling slice; do not fake geometry in automation.
