# Native piston continuation harness — WIP

The `piston_native_workflow_test` executable is a resumable validation harness,
not accepted cumulative workflow evidence. It builds on the accepted supplied
four-solid fixture and invokes native Qt controls for organization, joint
selection and origins, motion/Revert, and linked Paste. It is deliberately not
registered as automatic CTest: a private editor requires the sole GUI lease.

## Current blocker (2026-10-14)

The latest full attempt stops after **Create Components from Bodies > OK**.
The owned conversion dialog becomes natively active before input, closes after
OK, but the main window remains inactive with no focused widget/native window
through a one-second observation-only wait. The harness does not repair focus
or activation between steps. This is not yet attributed to product behavior
versus Qt event delivery/window-system environment. A separate root-owned OS
check could not acquire the app window, so it did not resolve the distinction.
No cumulative success, completed film, or physical OS-input parity is claimed.

An isolated organized-checkpoint joint-origin sequence passed: actual Browser
child clicks, focused Type-popup keyboard choice, exact retained circular-edge
hover, modifier lock, center click and OK. It does not establish the full chain.
Detailed attempt logs, captures, and the supplied fixture remain external under
`deliverables/piston-native-workflow-2026-10-14/` (latest blocker `native-10`,
isolated diagnostic `native-07`). No reference images are shipped here.

## Resume contract

Set `ALLOY_PISTON_NATIVE_SOURCE` to the external supplied four-native-solid
fixture and `ALLOY_PISTON_NATIVE_EVIDENCE` to a new external output directory.
The only activation setters are initial `raise()`/`activateWindow()`; later
checks observe actual focus and activation. Original occurrence rows receive
clicks on their embedded label widget; copied plain rows receive viewport
clicks. Popup keys go to its actual focused widget. Direct setup, inspection,
projection, capture, save and reopen calls use `executeApi` provenance.

`ALLOY_PISTON_NATIVE_ISOLATE_ORIGIN=1` accepts an organized, unjointed checkpoint
for the bounded first-joint diagnostic. It is not cumulative acceptance.
After a full smoke passes, `ALLOY_PISTON_NATIVE_RECORD=1` enables the paced film;
`ALLOY_PISTON_NATIVE_HINTS` must point to that smoke's `hover-hints.json` so final
hover uses proven geometric samples while still verifying actual exact hits.
Camera orientation, Browser expansion/scroll, supplied-file loading, exact
numeric −180 mm supplement, and local save/reopen are declared adaptations.
The event stream is delivered Qt input, not physical mouse/keyboard evidence.

The planned final save check reopens once and compares complete authored JSON
and all inspected occurrence poses. Recording captions dwell one second;
mechanism and Paste arrow drags deliver five pointer positions with 140 ms
intervals. Neither final path has run. Cursor alignment/readability and the
actual seven-row coupled motion still require inspection after a passing film.
