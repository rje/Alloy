# Native Measure selection and controls (DOC-040)

Status: bounded macOS Qt-delivered native qualification accepted after independent
Sol review and a corrected result-label layout. Physical OS input, Windows/Linux
runtime and packaged-release qualification are separate. No full visual-parity
claim follows from this receipt.

The public command contract remains external at
`knowledge/specs/doc039-measure-minimum-2026-10-19.md` (DOC-039, retrieved
2026-10-19). Analytic evaluator acceptance is recorded in [measure.md](measure.md)
and source integrity in [measure-references.md](measure-references.md).

## Change and native workflow

Closing Measure used to leave no keyboard focus receiver after Escape. The
panel's finished handler now activates its owner and focuses the canvas when no
modeling session owns focus. Native Escape followed immediately by `I`, native
Close and ordinary face selection now pass without harness focus repair.

`tests/measure_controls.cpp` uses one private editor. API fixture setup authors a
20 × 10 × 5 mm Box, radius-5 × 12 mm Cylinder, 3–4–5 sketch line, a point 6 mm
from that finite line and two connected perpendicular lines. Initial macOS
harness activation is explicitly separate from the demonstrated steps. Native
Inspect > Measure checks the actual toolbar hit, active native popup and actual
menu-action hit, then observes popup closure. The mouse press opens the popup;
the menu receives the action click/release, avoiding release onto a toolbar
button that command refresh may delete.

The same sequence checks the default Edge filter, 5 mm sketch-edge length,
Point/Edge native combo selection, a two-source 6 mm point/line distance,
clicking its rendered value to copy `6.0000 mm`, Clear Selection, a 90° pair,
planar 200 mm² area, 20 mm solid-edge length and cylindrical 120π mm² area.
Every pointer delivery checks the actual receiving widget. Combo popups use
their observed native keyboard receiver and must close before the next step.
Camera setup uses shared view commands; Measure selections use actual native
viewport hits and the shared evaluator. Escape goes to the receiver actually
produced by the prior steps; after native closure the canvas is asserted as the
receiver for `I`. Close returns to the same canvas receiver and a cylinder-side
click produces ordinary face selection rather than another Measure pick.

## Evidence and reproduction

The accepted cache is `build-preview-release`: Release, Qt 6.11.2, OCCT 7.9.3,
Apple Clang 17, arm64. Build `alloy` and `measure_controls_test` with `-j4`.

```
cmake --build build-preview-release --target alloy measure_controls_test -j4
ALLOY_MEASURE_EVIDENCE=/absolute/output/native-2x build-preview-release/measure_controls_test
QT_SCALE_FACTOR=0.5 ALLOY_MEASURE_EVIDENCE=/absolute/output/native-1x build-preview-release/measure_controls_test
python3 tests/measure_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output /absolute/output/api
```

Both native runs pass (one test sequence, 37 shared command observations each).
They preserve complete `document.inspect` state, including authored geometry,
history and undo state, and produce byte-identical before/after authored saves.
The fresh real-executable API fixture separately passes 52 calls, including
active-inch values, exact identities, analytic geometry and authored invariance.
Qt test windows are native application components, while that API fixture uses
the real application entrypoint; these are distinct evidence categories.

GPU diagnostics report Apple / Apple M4 / OpenGL 4.1 Metal - 90.5 and hardware
acceleration. The same 1400 × 900 logical window produces actual framebuffer
ratios **2** (2296 × 1216 viewport pixels) and **1** (1148 × 608), with native
viewport picking passing at both. `QT_SCALE_FACTOR=0.5` obtains the 1× backing
ratio on this Retina desktop; this is not a physical monitor-switch test.
`point-line.png`, `right-angle.png`, `planar.png`, `body-edge.png` and
`cylindrical.png` combine native QWidget chrome and the OCCT GPU framebuffer.
The independent review found `right-angle.png` clipped its second result at
both scales despite passing text assertions. The result label now reserves
height for each displayed line and the dialog layout honors that minimum.
The focused native sequence was rerun at 1× and 2×; its updated label-height
assertion passes, and both refreshed `layout-fix/native-*/right-angle.png`
captures visibly show `Angle: 90.0000 deg` above Clear Selection. Other
inspected captures have readable names/results and visible selected geometry;
1×/2× use the same logical fixture. No post-build resize, physical display move
or other-platform GPU evidence is claimed.

External receipts, logs, transcript, saves and captures are under
`validation/doc040-measure-native-2026-10-19/` in the reference records directory.
The original failing runs remain there: native combo click closure, an overly
specific focus expectation, demonstrated missing focus after Escape, and an
ordinary face-versus-body selection expectation. The product fix is supported
by the failing focus-return run and final passing sequence.

## Compatibility boundary

Matched: documented command/shortcut, selection/result grouping, Clear Selection,
click-to-copy and supported result values, through shared native/API authority.
Unknown: pixel-matched panel styling, because the accessible public source image
has no panel and supplies no comparable viewport size/scale/theme. The inspected
public distance image shows two cubes with in-canvas dimension graphics; this
fixture uses independent analytic geometry. Those graphics remain a known
breadth mismatch owned by DOC-041, alongside precision/secondary units, extra
fields and wider geometry pairs. The external visual provenance/check is
`knowledge/visuals/doc040-measure-2026-10-19/provenance.md`.

All private editor/test processes exit between runs. No CI, tag, publication or
other-platform runtime was performed. This accepts the bounded native control
gate; cumulative candidate-package qualification remains open.
