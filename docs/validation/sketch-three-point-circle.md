# SKT-095: Three-Point Circle

Accepted 2026-10-18 for the bounded Three-Point Circle workflow. The dated
public input contract and provenance remain outside the repository at
`../Alloy-reference-records/knowledge/specs/skt-095-circles-2026-10-18.md`.
Three ordered circumference picks create one analytic circle. A snapped origin
or authored point becomes a retained point-on-circle relation; free picks
create the ordinary free circle without invented fixed dimensions. The native
tool previews the third pick, repeats after completion, and exits with Enter
or Escape without committing incomplete input.

Evidence from Release `build-preview-release`, binary SHA-256
`c196fb5baf7f36fb49163fb15520e1e84468502abf2d1d8236c88a2c5c89a11a`:

- Focused geometry and native Qt tests passed, as did the existing sketch-curve
  regression after sharing its circumcircle calculation. The core check covers
  free-circle degrees of freedom, editable ordinary diameter, retained origin
  and source points, source edit, profile/extrusion, transactional invalid
  triples, conflicting driven diameter, history and native reopen. The native
  check uses the actual menu, three picks, live preview, repeat, Enter/Escape
  and collinear refusal without inter-step focus repair. Its 3-check run took
  4.295 s. Captures are in
  `../Alloy-reference-records/deliverables/three-point-circle-2026-10-18/native/`.
- An independent real-app control run at
  `../Alloy-reference-records/validation/skt095-three-point-circle-candidate1/`
  created a three-source circle on a dimensioned 20×20 mm construction
  rectangle, then extruded its circular profile 5 mm. Moving the source
  height to 30 mm retained circle and body IDs, changed radius from √200 to
  √325 mm, and changed exact body volume from 1000π to 1625π mm³. A collinear
  command refused with the draft unchanged. Undo/Redo and saved reopen passed.
  Independent binary STL readback found one closed component, 356 triangles,
  180 vertices, Euler characteristic 2, and 5100.94 mm³ mesh volume versus
  5105.09 mm³ exact body volume. The model, STL, three app captures,
  38-command transcript and structured receipt are beside the gate.
- Actual captures show a readable preview, snapped-point markers and a filled
  analytic disk, then the source-edited sketch and shaded solid. The viewport
  reported Apple M4, OCCT OpenGL, Apple vendor, OpenGL 4.1 Metal driver and a
  2× framebuffer. Independent source review checked the shared command route,
  snap validation, retained relations, transactional solve and capture/result
  agreement.

Circle Feature Options in the Sketch Palette remains SKT-109/#533. The
distinct 2-Tangent and 3-Tangent Circle modes remain SKT-107/#531 and
SKT-108/#532. Exact modern preview/glyph styling, physical hand input and
Windows/Linux runtime/GPU presentation remain unqualified. The public visual
examples do not match this capture's theme/size, so no pixel-level parity
claim is made.
