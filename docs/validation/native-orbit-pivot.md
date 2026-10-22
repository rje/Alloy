# Native orbit/pivot gate: safe-stop receipt

NAV-009/#428 remains **open and unaccepted**, 2026-10-21. The improved
`tests/navigation_orbit_pivot_controls.cpp` fixture replaces its direct synthetic
context-menu event with a PID-directed macOS right-button event pair, checks the
actual hit receiver, observes genuine Qt/AppKit focus, and stops without focus
repair. Existing shared-command/camera evidence in
[constrained-orbit-pivot.md](constrained-orbit-pivot.md) was not repeated.

The workflow contract remains the externally sourced
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
This work changes validation, not application behavior or the researched policies.

## Bounded run

Accepted cache: `build-preview-release`, Release, `/usr/bin/c++`, default macOS
SDK, private Qt 6.11.2 and OCCT 7.9.3. The focused fixture builds with:

```
cmake --build build-preview-release -j4 --target navigation_orbit_pivot_controls_test
ALLOY_NAV_PIVOT_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/nav009-2026-10-21/native-dpr2-01 build-preview-release/navigation_orbit_pivot_controls_test actualModeAndOffCenterPivot
```

The single DPR2 run used private PID91231, with one initial activation and no
interstep repair. Five recorded checkpoints have all five Qt/AppKit activation
and key-window checks true: setup, visible Free choice, visible left-drag Orbit
and naturally focused Escape, visible Constrained choice, then visible left-drag
Orbit and naturally focused Escape. The constrained camera passes the projected
world-Z level-horizon assertion. These are **Qt-delivered control/drag/key inputs**,
not proof of physical-device or OS key/drag delivery.

The next step attempted a PID-directed OS right-click at the actual-hit blank
canvas coordinate. The popup did not appear within the bounded wait. The final
GPU diagnostics report **zero** right-button presses, context-menu events and
shown context menus. All Qt/AppKit activation/key-window checks were then false.
This establishes a host delivery/focus blocker before any context action, not a
proven application context-menu defect. The exact cause (event routing, host
input permission, or another focus change) is unknown. No global event fallback,
second activation, focus repair, or unchanged retry was attempted.

External evidence is in
`../Alloy-reference-records/validation/nav009-2026-10-21/native-dpr2-01/` and adjacent
`native-dpr2-01.log`. The log reports 2 passed / 1 failed QtTest functions in
8.942 seconds; the workflow function fails at the missing popup. The saved
`native-report.json` records `passed: false`, phase `OS context set`, each passed
checkpoint and final GPU/native state. Its SHA256 is
`65ab92a58ce0aa71c159364f32bba5229a0c007e1129e9031e8f038d9b4fc8c6`.
The pre-input saved model is `orbit-pivot.alloy`, SHA256
`2086982d210c399dfd9e2e9b7fd6f5615cbc797ace0a611dbd6b3fac8c543839`.
No readable final capture or matched visual acceptance was obtained.

After this stop, the fixture gained native popup key-window assertions, an opener
watchdog and endpoint hit assertions. These final harness safeguards are
**build-verified only**; the native run above predates them. The added pivot-pick,
miss/Escape, marker capture, panned left-drag pivot stationarity, actual tab-return,
context Reset and authored/history/selection checks remain unexecuted in the
strict native route. Do not infer their acceptance from source presence.

## Resume boundary

Resume only after concrete foreground/input-delivery evidence changes. Complete
Set, visible surface pick/green marker, pending-pick Escape, Reset, tab restoration,
unchanged authored/history/selection and readable capture. Effective DPR1 was
intentionally not attempted on the unchanged host blocker. The final fixture
bounds popup opening and records failure state; it never repairs native focus.
Keep Qt input, PID-directed OS context input and physical-device results separate.
NAV-008 still owns cumulative mouse/trackpad navigation. Independent review and
all uncompleted #428 acceptance remain required.

The private editor exited. User PID72618 was preserved. No paid CI, tag, push,
publication or user-owned application operation occurred.
