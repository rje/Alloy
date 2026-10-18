# Sketch Fix/UnFix

SKT-133 / #558 accepted bounded point-and-line scope, 2026-10-18;
independent Sol review found no blocking defect.
The permitted public contract is external
`knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`, P2/P3, reviewed
2026-10-18. It documents locking size and location, green fixed geometry and
repeat-tool/click removal. Exact reference pixels and cursor artwork are unknown.

## Workflow contract

Sketch Constraints and Sketch Palette expose **Fix/UnFix**. With the tool active,
click one authored straight line or sketch point to fix it; click the same target
again to unfix it. Each successful click clears the pending selection and leaves
the tool active for another target. Escape returns to Select. A line is targeted
by its curve; an endpoint is a point target. There is no preview or numeric input
and no permanent Fix badge. Fixed curves and point markers use the existing green
category. Other relations, including Horizontal, retain their normal badges.

`sketch.fixToggle` and `sketch.constraint` with `type: fixed` share the native
command authority. Supply exactly one `entityIds` line target or `pointIds` target;
with neither field supplied, the current selection is used. Native preselection
also works. Stale, unsupported or mixed targets refuse without changing authored
geometry or draft history. The origin remains fixed and cannot be toggled.

A point retains the existing `fixed` schema (`id`, `type`, one `pointIds`, `uMm`,
`vMm`). A line owns one `fixedLine` relation with its stable line ID, ordered
endpoint IDs and both endpoint coordinates. Four coordinate equations lock
translation, orientation and length under that one relation ID. Unfix removes
only that target's explicit Fix ownership; other constraints continue to apply.
Existing explicit point locks are preserved. If another relation keeps geometry
immovable after UnFix, that relation remains authoritative.

Fix accepts dependent equations from existing line drivers only if removing all
`fixedLine` relations leaves an accepted baseline solve, the combined solve has
no conflicts and every retained relation passes independent residual checks.
Conflicting length edits refuse atomically. After UnFix, the line returns to its
independently free role and retained length drivers can change its size. After
Escape exits the active tool, each toggle and stable-ID relation deletion is one
Undo/Redo step (Undo with any active tool first cancels that tool), and save/reopen
retains exact authored identities and coordinates.

## Evidence

Build: `build-preview-release`, Release, AppleClang 17, Qt 6.11.2 private SDK,
OCCT 7.9.3. External evidence root:
`../Alloy-reference-records/validation/sketch-fix-unfix-2026-10-18/`.

- `tests/sketch_fix_unfix_api.py` runs the real application entrypoint and local
  API. It checks a line with Horizontal and retained length drivers, point schema,
  shared toggling paths, conflicting length refusal, stale/unsupported/mixed
  refusal, exact history, restored freedom/length edits, refix and exact reopen.
- `tests/sketch_fix_unfix_controls.cpp` drives the actual Palette button and
  viewport picks with native Qt input after one initial activation. It checks
  repeat clicks, focus produced by each pick, one-step history, both endpoint
  locks, origin/owned-axis refusal, stable-ID deletion, Escape and exact save/reopen. No
  inter-step focus or selection repair is used.
- `api/receipt.json` and `native/receipt.json` keep solved roles, authored data,
  target IDs, native workflow state and GPU metadata distinct from captures.
  Hardware reports Apple M4, Apple vendor, OpenGL `4.1 Metal - 90.5`.

Final native Qt run passed 3/3 (5.82 s), and the real-entrypoint API run passed
70 calls. The native creation/final/reopen and API creation/reopen captures were
inspected: green fixed line and point, blue ordinary circle, a retained Horizontal
mark, readable Fixed labels and no permanent Fix badge. The native reopened
Palette was captured only after its queued layout settled. Captures composite
QWidget chrome and the native OCCT GPU
framebuffer. Native viewport is 932 × 666 logical / 1864 × 1332 framebuffer; API
is 872 × 566 / 1744 × 1132. Both report 4× MSAA and 2× display scale. The saved
fixtures are viewed at 100%; they are not matched reference-pixel comparisons.
The user-owned editor is preserved; each private fixture editor closes before
the next opens.

## Explicit limits

This candidate accepts sketch points and authored straight lines. Circle, arc,
ellipse, conic and spline Fix/UnFix, plus generated/grouped geometry ownership,
remain linked Todo issues SKT-134–138/#559–563. Projected geometry remains
read-only. This receipt does not establish physical input, matched reference
pixels, cursor icon parity, other-platform runtime or broad exact-match behavior.
