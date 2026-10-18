# SKT-107: Two-Tangent Circle

Accepted 2026-10-18 for the bounded two-straight-line authoring workflow.
The dated public command contract and provenance remain outside the repository
at `../Alloy-reference-records/knowledge/specs/skt-095-circles-2026-10-18.md`.
After selecting two authored straight sketch lines, the native tool previews
the circle, accepts a pointer-derived or typed radius, and commits on a final
placement click. A typed value followed by Return does not commit early. The
tool repeats and exits with Enter/Escape; Escape discards incomplete input.

The public source does not specify side/solution choice. Alloy uses the
placement point to choose each directed line's signed side and projects the
pointer onto that angle bisector to infer an untyped radius. A typed radius
fixes size while placement still chooses the sides. Contact must stay on each
finite line segment. Parallel/near-parallel lines and line/arc/circle mixes are
outside this bounded mode. This policy is an Alloy decision, not a measured
reference-app behavior.

Evidence from Release `build-preview-release`, binary SHA-256
`7e82c57acb28eeb6dce53312afe713b5e65ca4d90b8744771af5f10228c63829`:

- Focused core and native Qt tests passed 2/2 (about 5 s). Core checks cover
  pointer and typed radius, two genuine signed Tangent relations, finite
  contact/profile, a 30→40 mm source-width edit, 10→8 mm radius edit, stable
  circle ID, incompatible edit and invalid/ambiguous/unsupported inputs,
  Undo/Redo and native-file reopen. Native Qt checks the actual nested Create
  → Circle menu, automatic canvas focus, two line picks, pointer and typed
  previews, Return without early commit, final click, repeat, Enter, Escape
  and transactional failed placement without inter-step focus repair. Native
  captures are under
  `../Alloy-reference-records/deliverables/two-tangent-circle-2026-10-18-native/`.
- A separate real-app control gate at
  `../Alloy-reference-records/validation/skt107-two-tangent-circle-candidate1/`
  created a circle tangent to two construction edges of a dimensioned 30×30 mm
  rectangle, extruded its circular profile 5 mm, then widened the source to
  40 mm and changed radius from 10 to 8 mm. The same circle and body IDs
  remained. Its center moved (20,10)→(30,10)→(32,8) mm and exact solid volume
  changed from 500π to 320π mm³. Undo/Redo, saved reopen, three app captures
  and the structured command transcript passed. The existing analytic-circle
  extrusion/export route was already independently qualified under SKT-095;
  this gate did not repeat STL export.
- Actual captures were inspected for readable pointer/typed radius previews,
  line highlights, the R8 dimension and tangent geometry after source edit.
  The Apple M4 viewport reported OCCT OpenGL on Apple's OpenGL 4.1 Metal
  driver with a 2× framebuffer. Independent source review checked the shared
  native/API command route, signed constraints, finite-contact check and
  transactional solve; no blocker remained.

Exact public branch choice, current preview style, window selection and
physical hand input remain unqualified. 3-Tangent Circle is SKT-108/#532;
the Circle Feature Options selector is SKT-109/#533. Windows/Linux runtime
and GPU presentation remain release gates.
