# External package test checklist

Planned gate for Windows/Linux development previews. No package is qualified by
this document alone. The release owner supplies a candidate archive, its checksum,
a matching editable sample and control-client instructions before requesting a
run. Use the packaged app, without adding developer libraries to make it launch.

## Record once

- Candidate version, source revision and archive SHA-256.
- OS version; Linux distribution and X11/Wayland session.
- GPU model/driver, monitor resolution and display scaling.
- Whether Alloy has been installed before and whether any prerequisite was needed.

Do not change system security settings or install unexpected dependencies to
work around a failure. Report the failure and its visible message instead.

## Try in order

1. Extract to an ordinary writable folder and launch. Capture the window and any
   warning. Resize it; move it between monitors if available. Labels, panels and
   the viewport should stay readable and correctly sized.
2. Open the supplied sample. Orbit, pan, zoom, Fit and return Home using the
   candidate's documented controls. Select a body and a face; selection should be
   visible and camera movement should remain smooth and controllable. Report
   mouse versus trackpad and any missing middle button.
3. Create an XY sketch, draw a circle, add a diameter and finish. Extrude it into
   a solid. Follow the supplied candidate-specific short steps without clicking
   elsewhere to repair focus. Note any key, popup or preview that needs a workaround.
4. Edit the diameter or extrusion, undo and redo. Save As to a new file, close,
   reopen and confirm the final shape and dimensions. Try cancelling one unsaved
   close: the document should remain intact.
5. Export that part to STL and open it in a slicer if available. Check physical
   dimensions and shape; printing is not required. Retain the model and export.
6. Reopen the supplied advanced sample, make its specified small parameter edit,
   then undo. Inspect for missing geometry, long stalls or broken references.
7. Run the supplied local control smoke: discovery, state, GPU diagnostics and
   screenshot capture. Retain output and logs. This does not replace the manual
   input observations above.
8. Quit normally and relaunch the saved model. Report crashes, lost settings or
   unexpected recovery prompts. Recovery testing uses a disposable copy and a
   separate supplied procedure; do not forcibly terminate personal work.

## Return

Send the record above, pass/fail/not-tried for each step, a final screenshot,
control/GPU logs and the saved model/export where relevant. For a failure, include
its earliest reproducible step and exact message. A short recording is useful
for navigation/focus problems; capture only the app and avoid personal content.

The release owner checks results against the exact archive and records supported
configurations and limitations. Missing results remain pending. An untested
architecture, distribution or GPU is not covered by another machine's success.
