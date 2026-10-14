# VIS-004: Solid and Surface navigation candidate

The Design ribbon has real Solid and Surface tab buttons. UI clicks and local
`ui.designTab {"tab":"solid"|"surface"}` share one authority; `app.state.ui.tab`
reports the selected group (or the active Sketch environment). Surface Create
contains Create Sketch, Extrude and the bounded guided Loft. Thicken is in
Solid Create. Construction planes and Inspect remain available on both tabs.
Only implemented controls are exposed; Surface Modify awaits retained Remove
integration. Surface button and menu object names are distinct from Solid.

The evidence contract is external:
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/surface-workspace-navigation.md`.
It documents tab/group layout and command routes. Shortcut variant dispatch and
switching tabs while a dialog is open remain unknown in the public evidence.
Alloy deliberately preserves draft, preview, selection and document when tabs
change. E continues to open Solid Extrude; the Surface tab tooltip identifies
that policy and directs users to explicit Surface Create Extrude. These local
choices do not establish reference parity.

Surface Loft routes to the existing two endpoint-section/Centerline New Component
mode. The Operation selector is locked for this route and shared command updates
refuse mode changes. Solid Loft retains its existing broader entry point. No new
surface geometry, commands or asset extraction is part of this increment.

Validation: isolated macOS build of `alloy` and
`surface_extrude_controls_test` passes. The focused native executable passed
4/4 checks (including setup/cleanup) on 2026-10-14. Qt-delivered clicks activate
Surface, start Extrude, select the fixture line, type 25/30 mm and obtain a
one-face preview. Switching Solid and back preserves the complete session;
Escape then discards it with exact authored-document equality. No inter-step
focus repair occurs; tab buttons preserve dialog focus. A separate check covers
the shared Surface Loft entry point, locked Operation and refused mode update.
The actual capture was inspected for readable controls and visible sheet.
Evidence is external under
`../Alloy-reference-records/deliverables/surface-workspace-2026-10-14/native-01/`.
The source fixture is preserved in the earlier surface Extrude `native-03/`
evidence. Physical OS input and matched screenshot parity remain unqualified.
Independent Astra source review found no blocker; its tooltip polish is applied.
Remove routing remains a linked integration dependency.
