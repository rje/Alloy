# Constraint icon interaction

Functional checks passed 2026-10-12 at `bf87e99`, followed by the test-only
identifier lifetime correction in `ab4f7bb`. Astra reviewed implementation and
actual captures. The origin-label correction in `0b74676` subsequently passed
independent visual review and native picking at its shifted location.

The existing visible relation glyphs now expose stable hit targets, selection,
hover, Delete and context Delete. Show Constraints and Show Dimensions operate
independently. They change active-session presentation, not authored geometry or
modeling history. Deletion validates targets before solving and publishes one
accepted draft edit through the existing undo stack.

`sketchConstraintBadgeInteraction` passed against the compiled Qt application.
It checks glyph clicking, native-widget Delete/Backspace versus text editing,
solver freedom, undo/redo, hidden hit targets and selection cleanup, stale-target
atomic refusal, dimension deletion, and hover cleanup when leaving the viewport.
Widget event synthesis is distinct from physical OS input.

A separate running application used API setup for one line, a horizontal
relation and a driving 60 mm length. Native pointer selection and Backspace
deleted the horizontal relation, raising DOF from two to three. Native Cmd-Z
restored the exact authored draft and draft-history index. Native palette toggles
independently hid constraints and dimensions. Actual GPU captures show the result.
Evidence lives under `build-tutorial-polish/evidence/badge-native-*`; the compact
receipt is `badge-native-workflow.json`. The focused Qt log is
`/tmp/alloy-badges-widget.log`.

The initial centered horizontal badge clashed with the origin's Z label.
`0b74676` reserves space around the visible active-frame origin and carries that
offset through subsequent badges on the same anchor. Actual corrected and selected
captures (`badge-origin-corrected.png`, `badge-origin-selected.png`) pass Astra
review. The focused Qt check passed again in `/tmp/alloy-badges-widget-final.log`.
This is bounded origin avoidance, not a general annotation collision algorithm.
Fixed/Coincident glyph geometry is not implemented; public documentation does
not require a permanent icon for every point-to-point Coincident relation. Physical Shift-click multiselection remains unqualified. Highlight
colors and session-only display persistence are explicit Alloy choices while
precise public evidence remains incomplete.

## Reopening populated sketches

`7af56b9` fits evaluated draft geometry on entry and Look At while preserving the
sketch normal; empty sketches retain the origin-centered default. At `0b74676`,
the actual reopened supported bracket sketch shows all five circles, construction
guides, its diameter dimension and historical support stock without cropping.
Every authored point projects within the viewport and Look At leaves the draft
unchanged. `populated-sketch-framing.png` and
`populated-sketch-framing-receipt.json` record the check. Astra accepted the
conservative but usable framing; no exact reference camera-margin claim is made.

## Final hover qualification

On 2026-10-12, the private ExchangeCandidate rendered the existing unselected
relation with API hover active. State reported hovered=true and selected=false,
the actual glyph was amber, and the authored draft remained unchanged. Evidence:
`build-exchange/evidence/closure/constraint-hover.json` and `.png`. This proves
API-driven hover rendering, not physical pointer motion. Together with the
previous widget and native selection/deletion checks, Astra accepted #152 closure.
Broader per-entity styling and remaining relation glyphs stay tracked separately.
