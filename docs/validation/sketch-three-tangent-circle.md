# SKT-108: Three-Tangent Circle

Accepted 2026-10-18 for the bounded three-straight-line workflow. The dated
public input contract and tutorial figure provenance remain outside the
repository at `../Alloy-reference-records/knowledge/specs/skt-095-circles-2026-10-18.md`.
The third line pick commits one analytic circle; there is no radius field or
fourth placement click. The native tool previews the result while hovering the
third line, repeats after completion, and exits with Enter/Escape without
committing unfinished input.

The public rectangle-edge example includes two parallel edges. Alloy therefore
enumerates signed solutions, keeps positive radii with finite contact on all
three selected segments, and accepts exactly one distinct circle. No viable
circle or multiple viable circles refuse transactionally. This is a bounded
Alloy policy; the general source application's side/branch choice is not
established by the public material.

Evidence from Release `build-preview-release`, binary SHA-256
`874b018816f540d691ae171b9fe192b512045cf7ec55c219511dc03816075b12`:

- Focused core and native Qt tests passed 2/2 (8.11 s). Core checks include
  a 30×30 mm three-edge rectangle with a parallel pair, three genuine signed
  Tangent relations, no authored radius dimension, selected-circle/profile
  identity, height 30→40 mm source edit, reversed selection order, no-solution
  and multiple-finite-solution refusals, Undo/Redo and native reopen. Native
  Qt checks the actual nested Create → Circle menu, automatic canvas focus,
  third-line hover preview without draft mutation, third-click commit,
  invalid target, repeat, Enter and Escape without inter-step focus repair.
  Captures are under
  `../Alloy-reference-records/deliverables/three-tangent-circle-2026-10-18-native/`.
- A separate real-app control workflow at
  `../Alloy-reference-records/validation/skt108-three-tangent-circle-candidate2/`
  authored the circle inside three construction edges of a dimensioned
  rectangle and extruded it 5 mm. Changing source height 30→40 mm retained
  circle/body IDs, changed center (15,15)→(10,20) mm and radius 15→20 mm,
  and changed exact volume 1125π→2000π mm³. Undo/Redo, saved reopen, three
  actual app captures and a 33-command transcript passed. The previously
  qualified analytic-circle export route was reused without a redundant STL
  check.
- Actual captures were inspected for the transient circle, highlighted
  defining edges, retained tangent badges, filled region and edited
  source-driven result. The viewport reported Apple M4, OCCT OpenGL on Apple's
  OpenGL 4.1 Metal driver with a 2× framebuffer. Independent source review
  checked finite contact, branch enumeration, shared native/API command and
  transactional solve; no material blocker remained.

Individual authored line picks are supported; window selection and projected
targets are not qualified. Exact general branch behavior and matched modern
styling remain unknown. The Circle Feature Options selector is SKT-109/#533.
Physical hand input and Windows/Linux runtime/GPU remain release gates.
