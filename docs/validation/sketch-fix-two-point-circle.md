# Two-Point Circle Fix/UnFix

SKT-139 / #564 accepted bounded scope, 2026-10-18. Independent Astra review
found no blocking mismatch.
The permitted public contract is external
`knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`, P2/P3 (retrieved
2026-10-18): selecting geometry with Fix/UnFix locks size/location and turns it
green; a repeat tool click removes Fix. Exact reference pixels are unknown.

## Command and retained ownership

Use Constraints or Sketch Palette **Fix/UnFix**, then click an independent
Two-Point Circle circumference. The tool commits immediately, clears selection
and stays active. A repeat circumference click removes only its explicit Fix
relation. Escape returns to Select. There is no numeric input, preview or
permanent Fix badge. Clicking the center/source point remains a point pick.

Native input, `sketch.fixToggle` and `sketch.constraint` with `type: fixed`
share the existing command authority. The existing `fixedCircle` relation keeps
one stable ID and entity ID. For Two-Point Circle its ordered `pointIds` are
center and retained `diameterStartPointId`; it adds `startUMm`, `startVMm`,
`diameterDirectionU` and `diameterDirectionV` to the ordinary center/radius
`uMm`, `vMm`, `radiusMm` fields. Ordinary Center Circle's schema is unchanged.
The opposite diameter endpoint is derived, not a newly authored point.

Five solver equations retain center, radius and source endpoint; the existing
Two-Point Circle radius/direction rules still participate. While fixed the source
is an explicit solver variable locked to its retained coordinates, rather than
an unprotected evaluator seed. Schema validation retains matching identities and
direction. Independent residuals separately check source, center, radius and
opposite diameter endpoint. Dependent diameter equations use the accepted
fixed-geometry baseline policy: no combined conflicts, successful solve, clean
baseline without explicit curve Fix and passing geometric residuals.

A conflicting diameter edit or source/center move refuses atomically in the
qualified fixture. After UnFix, the same diameter dimension and source point
remain editable. The original direction and all unrelated constraints survive.
Stable-ID `sketch.deleteSelection` removes only that relation. After leaving an
active tool, Undo/Redo each restore one authored change exactly.

## Bounded verification

Build: `build-preview-release`, Release, AppleClang 17, private Qt 6.11.2 and
OCCT 7.9.3. Evidence:
`../Alloy-reference-records/validation/sketch-fix-two-point-circle-2026-10-18/`.

`tests/sketch_fix_two_point_circle_controls.cpp` operates the actual Palette
button and circumference after one initial activation, without later focus or
selection repair. One oblique Two-Point Circle and one ordinary fixed circle
exercise native Fix/repeat UnFix, cleared selection, Escape, exact one-step
history, diameter refusal/recovery, source translation after UnFix, unchanged
other Fix, one Fixed row label, no badge and exact authored save/reopen. Its
core slot rejects malformed source identity, missing fields, altered direction,
incoherent source coordinates and invalid radius; it also checks a fixed source's
freedom report with unrelated free geometry.

`tests/sketch_fix_two_point_circle_api.py` uses the real app entrypoint. It checks
source, center, radius, direction and both diameter endpoints independently;
atomic diameter/source/center/stale/mixed refusal including unchanged history;
shared constraint/toggle paths; retained dimension identity; unrelated ownership;
stable-ID removal; exact history and save/reopen. API receipt: 71 calls passed. Final focused CTest passes all four native targets
in 17.42 s: Two-Point Circle 4.34 s, Center Circle 4.30 s, point/line
4.33 s and arc 4.45 s.

Native and API creation/reopen screenshots composite actual QWidget chrome with
the native OCCT GPU framebuffer. Hardware reports Apple M4, Apple vendor,
OpenGL `4.1 Metal - 90.5`, 4x MSAA and 2x display scale. Native viewport is
932 x 666 logical / 1864 x 1332 framebuffer; API is 872 x 566 /
1744 x 1132. Inspected native capture shows the edited green circle, green
source/center, readable diameter 24 and no permanent Fix badge. These are
unmatched Alloy views, not a measured public pixel comparison.

The first API run expected source/center movement to succeed as a no-op;
the app correctly refused the conflicting move, so the fixture now verifies
atomic refusal. The first core run compared absent optional `constraints` with
an explicit empty array; its expected normalized empty relation list was
corrected. Logs remain in `failed-attempt/`. These were fixture corrections;
the native workflow itself passed before the core expectation correction.

## Limits

Retained Circle Offset/group targets and projections still refuse through the
existing authority. The center-circle regression preserves both source/result
Offset refusal and schema-valid projected-target refusal. General constructor
combinations remain SKT-140/#565; retained Circle Offset remains SKT-138/#563.
This candidate does not establish matched pixels, physical OS input, other
platform runtime or broad exact compatibility. Each private validation editor
exits before the next; user-owned PID 33805 is preserved.
