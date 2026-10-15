# Development preview

Alloy is an open source, modifiable parametric CAD application in active
development. This page describes capabilities with accepted, bounded evidence;
it does not claim complete professional-CAD coverage or exact compatibility with
any reference product.

## Current capability

| Area | Demonstrated capability | Evidence and boundary |
| --- | --- | --- |
| Sketches and solids | Constrained line, arc and circle sketches; dimensions and formulas; retained references; selectable profiles; editable extrusion, Join/Cut/Intersect, Shell, Fillet, Revolve, Sweep and Loft workflows | [Primitive workflow](validation/primitives.md), [support tutorial](validation/support-tutorial-independent.md), [sketch expression tutorial](validation/sketch-expression-tutorial.md), [surface Loft and Thicken](validation/surface-thicken.md). Individual receipts cover specific routes and fixtures. |
| History and documents | Feature editing, undo/redo, native save/reopen and parameter-driven recompute are accepted in bounded workflows | See the receipts above and [sliding-lid cumulative workflow](validation/sliding-lid-cumulative.md). [Committed-state crash recovery](validation/document-recovery.md) now passes forced-interruption recovery of named and unnamed documents; active drafts are excluded. |
| Assemblies | Components, nested organization, linked copies, retained Revolute/Cylindrical/Slider motion and selected-instance export are demonstrated on bounded piston and box assemblies | [Piston native workflow](validation/piston-native-workflow.md) and [sliding-lid cumulative workflow](validation/sliding-lid-cumulative.md). These do not establish unrestricted assembly or collision coverage. |
| Surfaces | Surface workspace navigation, Extrude, guided Loft, paired-sketch and planar crossing-sheet Trim, Remove and Thicken are available; a bounded Loft/Thicken chain creates an editable solid | [Surface workspace](validation/surface-workspace-navigation.md), [sketch-tool Trim](validation/surface-trim.md), [planar sheet-tool Trim](validation/surface-planar-sheet-trim.md), [surface Thicken](validation/surface-thicken.md). The complete surface-iron tutorial remains unfinished. |
| Navigation | Free and constrained orbit, Pan, Zoom, Zoom Window, Fit, Look At, projection modes, orientation cube, saved Home and named views have bounded Qt/API/GPU receipts | [Navigation receipts](initial-build-readiness.md#navigation-checklist). Physical trackpad and broader OS input qualification remain open. |
| Import and export | Native Alloy documents and bounded STEP/STL workflows are demonstrated, including millimeter STL readback and selected-component export | [STEP exchange](validation/step-exchange.md), [bracket exchange](validation/bracket-exchange.md), [linked-instance export](validation/linked-instance-export.md). Format and workflow coverage is not exhaustive. |
| Local automation | The local control API discovers and executes Alloy commands, reports application/document state and captures screenshots. UI and API use the same application command and document state. | [Agent control API and client](agent-control.md). Geometry evaluation is synchronous and cannot be cancelled; [busy feedback receipt](validation/geometry-busy-feedback.md) records this boundary. |

## Package and start

The currently qualified installable package is **macOS arm64 on the development
Mac only**. Open the [local preview archive](../../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-candidate-01/Alloy-macos-arm64-preview.zip)
and compare it with the [package receipt](validation/macos-relocatable-preview.md)
(SHA-256 `c5eedaabacf504c845cf42973f536afa0af21c6bf97e909b5cc2130122ca2fa9`).
This archive contains accepted sources through `a355c50`; later source changes,
including planar sheet-tool Trim and crash recovery, require a newer build and are not in that ZIP.
Extract the ZIP, then launch `Alloy.app` from the extracted folder. For terminal
launch with a control endpoint ready file:

```sh
"/path/to/Alloy.app/Contents/MacOS/alloy" --ready-file /tmp/alloy-ready.json
```

The package receipt records bundled Qt/OCCT libraries, a successful extracted
launch with development SDK/plugin overrides removed, Apple M4 hardware
acceleration, and a bounded API save/STL workflow on the preceding candidate.
The navigation refresh adds accepted navigation sources and its own extracted
launch/API/save/STL check. This is a host-local ad-hoc signed preview, not a
public release or clean-host qualification. Windows and Linux package, desktop
and GPU testing await external testers. Windows/Linux CI compilation or local
transport evidence does not establish a runnable GUI package.

The package includes the Python client and Qt transport helper. With the app
running, discover its instance and commands, inspect state, and capture a
screenshot:

```sh
APP="/path/to/Alloy.app/Contents/MacOS"
python3 "$APP/alloyctl.py" instances
python3 "$APP/alloyctl.py" --ready-file /tmp/alloy-ready.json commands
python3 "$APP/alloyctl.py" --ready-file /tmp/alloy-ready.json state
python3 "$APP/alloyctl.py" --ready-file /tmp/alloy-ready.json screenshot /tmp/alloy.png
```

The API is local to the machine by default. It exposes structured command
results and errors; a client with access can operate the instance as the current
user. See [protocol, discovery and command details](agent-control.md).

## Example models and recordings

The repository includes small starter documents: [gauge block](../examples/gauge-block.alloy),
[rectangle sketch](../examples/rectangle-sketch.alloy),
[dimensioned rectangle](../examples/dimensioned-rectangle.alloy) and
[sketch extrusion](../examples/sketch-extrusion.alloy). The external
[demo gallery](../../Alloy-reference-records/demos/2026-10-13/index.html) links
accepted recordings, editable models and checked exports, including the bracket,
nested cubes, egg cup, toy block, die, ornament, sliding-lid box, piston and
bounded enclosure checkpoints. Their evidence varies by workflow: some recordings
use the shared API, while feature-specific Qt input receipts and GPU captures
are tracked separately. The gallery and models are local review artifacts, not
included in the application archive.

## Known limits and evidence types

| Evidence route | What the current receipts establish | What remains unknown or open |
| --- | --- | --- |
| Source/core | Selected geometry, authored-data, evaluation and storage contracts | Not proof that every UI route or platform works. |
| Local API | Real application commands, state, geometry summaries, save/reopen and selected exports on recorded fixtures | Does not prove ordinary pointer/keyboard interaction, OS dialogs or physical input. |
| Native Qt input | The exact controls, focus, selection and event sequence named by each receipt | Does not establish physical mouse/trackpad parity or all desktop environments. |
| GPU/framebuffer | Receipts that explicitly identify the actual renderer, vendor/device, version and capture | Current production viewport is OCCT AIS/V3d over hardware-accelerated OpenGL. Native Metal on macOS and Vulkan on Windows/Linux are future renderer destinations; the isolated Metal investigation did not migrate production. |
| Packaged runtime | The extracted macOS arm64 archive described above on one development host | Intel Mac, clean host, Windows, Linux, broader display scaling/resize, signing/notarization and public distribution remain unqualified. |

[Measured piston dragging](validation/assembly-native-drag-profile.md) remains
above the 100 ms development-preview target: the optimized Release run measured
129–150 ms per move. This bounded fixture does not establish general frame rate;
responsiveness work remains open.

The full enclosure tutorial is unfinished. Accepted stages and the final-stage
recording do not establish the complete cumulative tutorial: a clearance check
found two small connector intersections, and whole-reference clearance and
physical board fit remain unresolved. The complete surface shoehorn and surface
iron tutorial workflows also remain unfinished. No advanced tutorial should be
read as fully accepted beyond the workflow and evidence named by its receipt.

Alloy does not yet claim comprehensive import/export coverage, unrestricted
surface/assembly workflows, asynchronous or cancellable geometry evaluation,
draft-interaction recovery, physical-input parity, or feature-for-feature replacement
coverage. Continue with the [initial-build readiness queue](initial-build-readiness.md)
and the [release gates](releases.md) for current work and qualification status.
