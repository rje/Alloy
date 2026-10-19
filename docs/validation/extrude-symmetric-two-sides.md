# Symmetric and Two Sides solid Extrude

SOL-080/#618 accepted bounded Mac slice, 2026-10-19. Supported closed-profile
New Body, Distance, Profile Plane modes. Independent Sol source, API, GPU and
final native evidence review found no material blocker. [Command contract](../research/commands/extrude-symmetric-two-sides.md).

Implementation preserves authored profile/feature/body IDs and positive cap
lineage. The evaluator and preview normalize Half Length `d` to `[-d,d]`,
Whole Length `d` to `[-d/2,d/2]`, and Two Sides negative `n`/positive `p` to
`[-n,p]`, then use the existing shifted OCCT prism. One Side files/defaults,
signed distances, offset starts and existing cap IDs remain supported.
Native controls use Direction, optional Measurement, Distance or explicitly
labeled +Normal/−Normal inputs, New Body, OK and Cancel. The one-side arrow is
hidden for new modes; numeric entry is the bounded interaction.

## Evidence

External root:
`../Alloy-reference-records/validation/sol080-symmetric-two-sides-2026-10-19/`.

- **Build:** accepted `build-preview-release`, Release AppleClang17 arm64,
  Qt6.11.2, OCCT7.9.3. `cmake --build build-preview-release --target alloy
  extrude_symmetric_two_sides_test extrude_symmetric_two_sides_controls_test
  extrude_offset_start_test -j4`. No CI dispatch/tag/push.
- **Core:** focused `extrude_symmetric_two_sides` and prior
  `extrude_offset_start` pass. Exact rectangle bounds/volumes, same feature/profile
  IDs, independent edits, downstream positive-cap support, invalid zero/negative
  atomicity, inherited crossed-boundary profile guard/source-replacement refusal,
  Undo/Redo and authored save/reopen; `core.log`.
- **Real entrypoint:** `python3 tests/extrude_symmetric_two_sides_api.py --app
  build-preview-release/alloy.app/Contents/MacOS/alloy --output <root>/api`
  passes 125 local-control responses. Disposable preview/Cancel for all modes,
  exact geometry, edits through the same native feature route/shared command,
  invalid zero/negative refusal, Undo/Redo, save/reopen. A retained positive-cap
  circle follows Two Sides edits from −9..16 to −10..18 with unchanged authored
  support. `api/receipt.json` separates command responses and geometry.
- **Interchange:** independent binary STL triangle readback for all three saved
  fixtures checks min/max Z and signed triangle volume against exact OCCT values.
  Saved editable models and STLs are under `api/`.
- **GPU/presentation:** Apple M4, Apple vendor, OpenGL4.1 Metal90.5,
  hardware accelerated, 4× MSAA and native framebuffer details in the API receipt.
  Inspected actual whole-window Two Sides oblique preview and Symmetric Whole
  Length panel captures: readable fields, deliberate translucent preview and
  unclipped OK/Cancel. Captures are in `api/`; public reference build/theme/
  viewport/scale cannot be matched, so pixel parity remains unknown.

The native test uses actual widget hits (including combo openers and numeric line
edit children), initial activation only, and observed popup activation/closure.
The first harness attempt delivered keyboard events to a combo while its
popup remained open; the immediate popup-row click also failed closure. Both
stopped before accepting the route. The simplified final route sends keys to
the observed native popup focus, following the established control fixtures.
Final native run passes all three Qt slots (workflow plus setup/cleanup),
3.697 seconds. `native/controls.log` and `native-symmetric-whole.png` /
`native-two-sides.png` record popup-focus selection, typed Whole20/Cancel,
Two Sides zero refusal and 8/14 creation with native OK, and a retained edit
to negative reach9. No inter-step focus repair. Both current captures were
inspected: fields/hints and OK/Cancel are readable. The native captures use
Top view to inspect panel interaction; separate oblique API captures establish
solid presentation. The private native editor closed after the run.

## Boundaries

The rectangle fixtures use exact authored coordinates and ordinary rectangle
relations; they do not claim a fully constrained sketch. The extrusion issue
requires a supported closed profile.

macOS local runtime evidence only; no Windows/Linux/package/runtime claim.
Physical device input, exact keyboard/pixel parity, native numeric manipulator
parity, taper, thin wall, alternate start/extent/operation combinations and
broader profile families remain outside this child under SOL-010 and release
platform gates. Public evidence establishes the mode/measurement semantics;
Alloy side-to-normal mapping and numeric defaults are explicit local policies.
