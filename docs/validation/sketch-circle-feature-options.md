# SKT-109: Circle Feature Options

Accepted 2026-10-18 for switching Circle creation type in the actual Sketch
Palette. The dated public basis for the Feature Options route remains outside
the repository at
`../Alloy-reference-records/knowledge/specs/skt-095-circles-2026-10-18.md`.
The selector lists Center Diameter, 2-Point, 3-Point, 2-Tangent and 3-Tangent
Circle. It appears only while a Circle mode is active, reflects the shared
`sketch.tool` state and invokes that same command when changed.

Public evidence does not establish whether partial input survives a type
switch. Alloy discards unfinished picks, numeric editors and disposable
previews, preserves committed circles, and focuses the canvas for the first
pick in the new mode. This is an explicit Alloy interaction policy, not a
reference-app parity claim.

Evidence from Release `build-preview-release`, binary SHA-256
`eea64cce1c4bf5e3888875f9566c750407dbe0dca5779063e5026336e0b832a6`:

- Focused native Qt test passed three phases with no failures in 8.54 s. It
  switches with real popup mouse and keyboard input across all five modes,
  including pending center-diameter and two-point editors, two three-point
  picks, a typed 2-Tangent radius and a 3-Tangent third-line preview. Each
  switch leaves the draft unchanged, clears incomplete state, updates the
  authoritative tool/prompt and accepts the next canvas pick without harness
  focus repair. It also verifies one committed circle remains, Enter/Escape,
  draft Undo/Redo and exact native-file reopen.
- A separate real-app entrypoint gate at
  `../Alloy-reference-records/validation/skt109-circle-feature-options-candidate1/`
  selected each mode through `sketch.tool`, inspected active session state,
  captured the selected 3-Tangent palette, then switched to Select and
  captured Feature Options hidden. The four native captures are under
  `../Alloy-reference-records/deliverables/circle-feature-options-2026-10-18-native/`.
- Actual captures were inspected: headings, full mode names, prompts and
  previews are readable in the palette at the tested window size. The app
  reported Apple M4, OCCT OpenGL on Apple's OpenGL 4.1 Metal driver and a
  2× framebuffer. Independent source review checked popup closure before
  shared-command dispatch and automatic canvas focus.

Current reference styling and whether its partial picks persist are unknown.
Physical hand input, other viewport sizes and Windows/Linux runtime remain
unqualified. Individual circle modes retain their own bounded receipts.
