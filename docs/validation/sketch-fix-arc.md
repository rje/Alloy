# Authored arc Fix/UnFix

SKT-135 / #560 accepted bounded scope, 2026-10-18. Independent Astra review
found no source or geometry blocker.
Permitted public contract: external `knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`, P2/P3 (reviewed 2026-10-18). Documented geometry size/location locking, green fixed geometry and repeat Fix/UnFix click removal guide this implementation. Exact reference pixels and cursor artwork remain unknown.

The existing Constraints menu and Sketch Palette **Fix/UnFix** tool accepts an independent authored finite arc by its curve. Each successful click clears selection and leaves the tool active. Clicking again removes only that arc's explicit ownership. Escape returns to Select. There is no preview, numeric tool input or permanent Fix badge. Its center remains a separate point pick. Existing dimensions and Tangent/Coincident relations remain authoritative. Undo first cancels an active tool; one-step authored history applies after tool exit.

The native tool, `sketch.fixToggle` and `sketch.constraint` with `type: fixed` share one authority. One retained `fixedArc` relation stores `id`, `type`, one `entityIds` target, ordered `pointIds` (center/start/end), `uMm`, `vMm`, `radiusMm`, `startAngleDeg` and signed `sweepAngleDeg`. Five equations lock center coordinates, radius and both angular parameters; existing arc rules determine endpoint positions. Independent geometric residuals check these locks, while strict schema validation checks target identity, normalized finite angular bounds, supported coordinates/radius and retained-group independence. Stable-ID `sketch.deleteSelection` removal is supported.

As with fixed lines/circles, dependent driver equations are accepted only with a clean baseline after removing explicit fixed-curve relations, no combined conflicts, a successful combined solve, and passing independent residuals for every retained relation. Conflicting source edits refuse atomically. UnFix preserves all other authored authority and allows valid radius/sweep dimension changes.

## Bounded verification

Accepted build directory: `build-preview-release` (Release, private Qt 6.11.2, AppleClang 17 and OCCT 7.9.3). Evidence root: `../Alloy-reference-records/validation/sketch-fix-arc-2026-10-18/`.

`tests/sketch_fix_arc_controls.cpp` exercises actual Palette button and arc curve clicks after one harness activation, with no later focus/selection repair. It uses positive and negative semicircular sweeps, dimensioned and undimensioned arcs, repeat-click UnFix, Escape and exact one-step Undo/Redo, conflicting R10→R12 then UnFix recovery, sweep160°→150° conflict/recovery, exact save/reopen and native captures. Core checks cover malformed retained lock records and real retained Slot arc ownership refusal.

`tests/sketch_fix_arc_api.py` extends the accepted rounded plate fixture with retained distinct-ID arc/line Tangent and explicit point Coincident. It checks radius/sweep conflicts, shared toggle paths, stale/mixed target refusal, stable-ID deletion and history; an existing extrusion then refuses a fixed source edit, accepts UnFix R5→R7 and width60→70, and preserves the relation and point identities through history and exact save/reopen. Volume is checked independently against the rounded-rectangle formula.

The final native run passes 4/4 (4.718 s); the real-entrypoint API run passes 99 calls. The expected initial and edited solid volumes are independently checked as 11973.1747704 and 13947.4225500 mm³. Shared native circle and point/line regressions pass 4/4 (5.009 s) and 3/3 (5.410 s). `git diff --check` passes.

Inspected captures show green fixed arcs/centers/endpoints, readable separated R12/150° labels, retained Tangent marks with the R7 rounded profile, and the resulting smooth rounded solid. Native and API framebuffer captures composite QWidget chrome with the actual OCCT GPU framebuffer. Hardware identifies Apple M4, Apple vendor, OpenGL `4.1 Metal - 90.5`, with 2× display scale. Native viewport is 932 × 666 logical / 1864 × 1332 framebuffer; API is 872 × 566 / 1744 × 1132. These are 100% Alloy views, without a matched public pixel comparison.

Pre-correction native captures overlapped radius and sweep labels; fixture annotation positions were separated before the final passing run. Failed API setup evidence preserves an unsupported `addPoint.snapTarget` assumption (corrected to an explicit Coincident command), two overly strict assumptions about constrained-drag return status (the solver can refuse or accept an unchanged solution; drag behavior was removed from this bounded fixture), and an assumption that a historical source-edit draft is byte-identical to the pre-Finish draft (atomic refusal now compares the immediate historical draft). The strict radius/sweep refusal, native exact draft history, authored persistence and downstream geometry assertions all pass. These fixture corrections are retained under `failed-attempt/`.

The user-owned PID33805 was preserved; every private validation editor exited. Build, API, native input, authored geometry/history and screenshot evidence remain separate.

## Explicit limits

Acceptance is bounded to independent ordinary center-mode finite arcs, with both sweep directions in native checks and a positive quarter arc in the downstream fixture. Other constructor combinations have common retained arc output but are not separately qualified here. Retained groups (including Slots, Mirror and Offset ownership), derived geometry and projected targets continue to refuse; SKT-138/#563 owns broader geometry ownership. Ellipse/conic/spline Fix remains SKT-136–137/#561–562. Physical OS input, exact matched public pixels and Windows/Linux GPU runtime remain unqualified.
