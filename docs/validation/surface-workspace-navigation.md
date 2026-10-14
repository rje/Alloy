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
`surface_extrude_controls_test` passes. Focused native/API and capture evidence
is pending the sole private validation-editor lease; no completion claim yet.
