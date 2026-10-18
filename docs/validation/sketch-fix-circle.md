# Authored circle Fix/UnFix

SKT-134 / #559 accepted bounded Center Circle scope, 2026-10-18.
Independent Sol review found no remaining blocker.
The dated permitted public contract is external
`knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`, P2/P3.
It documents geometry size/location locking, green fixed geometry and repeat
Fix/UnFix click removal. Exact reference pixels and cursor artwork are unknown.

The existing Constraints menu and Sketch Palette **Fix/UnFix** tool now accepts
an independent authored circle by its circumference. Each successful click
clears selection and leaves the tool active. Clicking it again removes only
that circle's explicit ownership. Escape returns to Select. There is no preview,
numeric tool input or permanent Fix badge. Existing dimensions and other
relations remain authoritative. The center point remains a separate point pick.

The native tool, `sketch.fixToggle` and `sketch.constraint` with `type: fixed`
share one command authority. One retained `fixedCircle` relation has `id`,
`type`, one `entityIds` circle, its matching one `pointIds` center, `uMm`,
`vMm` and `radiusMm`. Three solver equations and independent residual checks
lock both center coordinates and radius. The schema validates identity/type,
matching center and finite supported retained coordinates/radius. Removing
exactly that stable relation ID is also supported by `sketch.deleteSelection`.

Existing circle radius dimensions can introduce dependent equations. As with
fixed lines, acceptance requires an accepted baseline with all explicit
fixed-curve relations removed, no combined conflicts, successful combined
solve and independently passing residuals for all retained relations. A
conflicting radius edit refuses atomically; after UnFix the retained radius
dimension can change the circle. One-step Undo/Redo applies after leaving the
tool (Undo first cancels an active tool).

## Bounded verification

Build directory: `build-preview-release`, verified Release/private Qt 6.11.2,
AppleClang 17 and OCCT 7.9.3. Evidence root:
`../Alloy-reference-records/validation/sketch-fix-circle-2026-10-18/`.

`tests/sketch_fix_circle_controls.cpp` uses the actual Palette button and
circumference clicks after one harness activation, with no subsequent focus or
selection repair. It checks dimensioned and undimensioned circles, repeat-click
UnFix, retained ownership, Escape then one-step history, radius conflict/recovery,
exact authored save/reopen and native GPU captures. Its focused core slot checks
two-point diameter ownership, both source/result circle Offset ownership,
schema-valid retained construction-plane projected-circle target refusal and malformed
retained-relation rejection. The synthetic projection record does not resolve an actual
model edge; it proves the target is refused because its ID is absent from authored
entities, after separately validating the projection schema.

`tests/sketch_fix_circle_api.py` runs through the real application entrypoint.
It separately checks both circle centers and radii, retained radius identity,
conflicting dimension update and addition, shared toggle paths, stale/mixed
refusal without history mutation, stable-ID deletion, exact history and reopen.
The existing point/line fixtures retain their original scope; their unsupported
entity assertion now uses a point entity rather than a newly supported circle.

The final circle native run passes 4/4 (5.360 s), including core ownership/schema
refusals. The final real-entrypoint circle API run passes 67 calls. The original
point/line native regression passes 3/3 (5.533 s); its real-entrypoint API
regression passes 70 calls. Native creation/reopen and API
creation/reopen captures were inspected: green fixed circles/centers, readable retained
R10 then R12, active Fix/UnFix prompt and no permanent Fix badge. Captures composite
QWidget chrome with the native OCCT GPU framebuffer. Hardware reports Apple M4,
Apple vendor, OpenGL `4.1 Metal - 90.5`, 4× MSAA and 2× display scale. Native
viewport is 932 × 666 logical / 1864 × 1332 framebuffer; API is 872 × 566 /
1744 × 1132. These are actual 100% Alloy views, not matched public pixels.

Independent visual inspection found a duplicate `Fixed · Fixed` curve-row label.
The shared curve list now shows Fixed once while retaining every other relation
label. Focused native assertions cover both circles and a fixed line with its
Horizontal label preserved. Settled native/API captures were regenerated and
inspected; the circle row reads `Circle 1 · P1 · R12 · Fixed`. Pre-correction
captures and receipts remain under `failed-attempt/`.

The initial native run found a genuine line-only selection guard, corrected
before the final passing run. Its failed log remains in `failed-attempt/`.
The first core fixture assumed Circle Mirror support, which is not implemented;
it was replaced with actual retained Circle Offset source/result. A point/line
regression invoked without its required evidence environment reached an unsaved
close decision and was stopped; its proper evidence-enabled run passes. The new
circle harness uses a temporary output directory when no evidence path is supplied;
the registered CTest path also passes without an evidence environment.
The user-owned editor is preserved and each private editor exits before the next.
API receipts retain full command results, authored identities and GPU state;
native receipts retain the actual native workflow state and saved fixture.

## Explicit remainder

This receipt qualifies independent center circles with and without a retained
radius dimension. Two-Point Circle's `diameterStartPointId` and retained direction
own placement and explicitly refuse. Circles mentioned by retained groups,
including Offset source/result, also refuse. Two-Point Circle remains SKT-139/#564;
retained Circle Offset source/result remains SKT-138/#563. Remaining authored
constructor combinations are SKT-140/#565 before claiming general authored circle
coverage. Projected geometry remains read-only. Three-Point and tangent
constructors author ordinary circles plus source constraints; their unqualified
combinations remain SKT-140/#565. Arc/ellipse/conic/spline
Fix remains SKT-135–137/#560–562. Physical OS input, exact matched pixels,
other-platform GPU runtime and broad exact-match behavior remain unknown.
