# SOL-033 native Sweep candidate evidence

2026-10-13, isolated `sweep-controls`, baseline b500ef9 (accepted SOL-032).
PrivateQt6.11.2, OCCT7.9.3, AppleClang17, macOS arm64. App and native test build
pass; focused unchanged Sweep core1/1 passes. Source ID:
sweep-paper-clip-warwick-2017-20261013. Exact references remain external.

`tests/sweep_controls.cpp` passes3/3 in14.46s through the actual app. Analytic
circle/path sketches are seeded via shared API, explicitly bypassing source
line-lock and paired-fillet authoring for this controls gate. The test clicks
actual Create popup > Sweep, picks circular Profile and ordinary path edges,
checks unsupported circle-as-Path error with disabled OK, expands the complete
10-edge chain, toggles Chain Selection with an ordinary checkbox Space event,
selects three individual sections, and returns to the full chain. Actual
Cancel preserves authored/history state and clears overlays/temporary visibility;
actual OK commits the analytic586.9634808526 mm³ body. Timeline tile click enters
edit, exposes both hidden source sketches, clears/reselects Profile/Path and
Cancels unchanged. Undo/redo and exact save/reopen pass. A separate short
negative-Y path native preview proves the parallel normal sign; the main path
uses the opposite sign. Neither is merely a mocked geometry/control.

The first native run stopped after successful commit because its Timeline lookup
compared QVariant to QJsonValue; string comparison corrected the test. The final
complete run passes. No application defect or input-oracle weakening is implied.
These are injected Qt events delivered to real controls and viewport, not physical
OS input or native sketch-construction proof. Native controls include actual
readable fixed Full1.00/Perpendicular/New Body fields; unsupported modes are absent.

`tests/sweep_controls_api.py` also passes. It exercises the same profile/path
pixel selectors, checked/unchecked policy, stable reservations, Cancel, component
ownership, accepted body, hidden-source edit/reselection, undo and exact reopen.
It does not substitute for the native run. The preexisting foundation API runner's
native-controls predicate is updated for the newly available dock; its earlier
kernel/history evidence is reused, not replayed wholesale.

Artifacts: `build-sweep-controls/evidence/native.log`, native PNG/state JSON,
`native-sweep.alloy`; API report/transcript/app.log, PNGs and
`sweep-controls.alloy` under `evidence/api`. GPU is AppleM4/OpenGL4.1 Metal atDPR2.
Actual inspected captures show gray solid with blue profile/orange selected chain,
readable selector counts, fixed fields, Cancel/OK and fully visible red error.
Geometry fits the viewport. Browser long fixture name elides normally; no controls
or error text are clipped. All private editors closed and slot released.

Compatibility: public field labels/role order and final Single Path/checked Chain
Selection/Full1.00/Perpendicular/New Body state are matched in this bounded slice.
Exact chain propagation and global defaults are unknown; Alloy's checked default
and connected unbranched-component policy are explicitly documented. Screenshot
OS/build/DPR/viewport are unmatched; exact pixel/spacing/color parity is unknown.
No broad #57 or complete #225 tutorial acceptance is claimed. Source ingredient
input gates and any Circle key-focus follow-up remain separate.

Reproduce with one private editor at a time:

```sh
cmake --build build-sweep-controls --target alloy sweep_controls_test solid_sweep_test
ctest --test-dir build-sweep-controls -R '^solid_sweep$' --output-on-failure
ALLOY_SWEEP_CONTROLS_EVIDENCE="$PWD/build-sweep-controls/evidence/native" build-sweep-controls/sweep_controls_test
python3 tests/sweep_controls_api.py --app build-sweep-controls/alloy.app/Contents/MacOS/alloy --output build-sweep-controls/evidence/api
```

Independent Astra review pending; root owns integration/acceptance. Shared
[command and selection contract](../research/solid-sweep.md).
