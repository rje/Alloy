# Sketch Text foundation candidate (#231)

Status: bounded implementation candidate, 2026-10-13. Independent compatibility
review and cumulative keyring tutorial acceptance remain open. Public workflow
source: `warwick-initials-keyring-20261013`; source details and uncertainties are
outside the repository in `../Alloy-reference-records/knowledge/`.

Alloy retains one text object (UUID, uppercase Latin content, fixed regular
font asset ID and SHA-256, cap height in mm, baseline position and angle) in a
text-only sketch on a named planar support. The bundled unmodified DejaVu Sans
2.37 font has full notices in `third_party/dejavu/LICENSE`; see
`THIRD_PARTY_NOTICES.md` for exact provenance and hashes. The font must be
present with matching bytes. Qt `QRawFont` supplies the finite glyph outlines;
straight segments and cubic segments become exact OCCT lines and Bezier curves.
Each glyph becomes one planar face; D, B and 8 retain their inner wires as holes.
No outline flattening tolerance or bitmap is involved. The current layout is
direct glyph advance, without kerning or general shaping.

The text UUID owns one ordered profile slot per character index. Profile IDs
derive from that UUID and slot index, remain stable through text/height/position/
angle edits with the same character count, and can be selected together for the
existing Extrude/Join command. The supported edit command rejects a character
count change. An invalid new outline or changed font identity fails before draft
publication; a downstream recompute failure prevents document commit. Individual
glyph boundary edges have no persistent downstream naming contract. Text cannot
be mixed with analytic authored sketch entities in this bounded foundation.
Font choice, bold, multiline/Unicode shaping, native text dialog and handles are
later scopes.

Evidence: `ctest -R '^sketch_text$'` passes the pinned DSB and DS8 fixture,
closed D/B/8 holes, actual 1.5 mm OCCT extrusion/positive volume, exact authored
JSON roundtrip and invalid font identity refusal. The real Alloy API fixture
`tests/sketch_text_api.py` passed on macOS/Apple M4 with Qt 6.11.2 and OCCT
7.9.3: a 60×30×2 mm floor, face-supported DSB, 1.5 mm Join, edit to DS8 at
13 mm, profile ID stability, changed valid solid volume, undo/redo, and exact
save/reopen. Initial and edited one-body volumes were 3904.6735437 and
3853.1725650 mm³. The native 2880×1800 capture is a QWidget frame plus OCCT
OpenGL framebuffer at 2× scale. The viewport reported Apple M4 / Apple / OpenGL
4.1 Metal - 90.5 and a 2376×1226 framebuffer. The capture was inspected: smooth
raised glyphs, visible holes, and a clear 1.5 mm join to the floor. Evidence
files (model, capture, transcript, report) are in
`../Alloy-reference-records/validation/sketch-text-2026-10-13/`.

This verifies the bounded API and visual geometry fixture. Physical native text
creation and placement, exact reference styling and the complete keyring lesson
are unknown here; the dialog and cumulative lesson are separate work.

## Independent acceptance

Accepted candidate `aea9628` for the bounded foundation on 2026-10-13.
Independent review covered the helper, schema/evaluation integration, shared API
publication path, focused test source and persisted passing core log, all 28
successful API responses, authored save data, and the actual raised-initials
capture. No additional editor or unchanged test replay was needed.

The bundled font and complete license are byte-identical to their members in
the declared 2.37 release archive; independently computed archive and font
SHA-256 values match the notices. The embedded-resource/hash check avoids system
font fallback. Retained text parameters remain distinct from generated geometry.
The finite direct-advance layout and exact line/cubic conversion preserve the
observed D/B/8 holes; the actual joined DS8 capture shows readable raised letters
and cavity depth, without a missing-glyph or visible geometry defect.

The final saved document exactly equals the transcript's final authored state.
The reviewed API run proves one-body Join, same-slot replacement, undo/redo and
exact reopen. Slot identity means character **position**, intentionally allowing
B to become 8 while retaining that slot's profile ID; it does not promise glyph
identity across insertions or arbitrary topology naming. Count changes refuse
before publication. Invalid font identity is exercised by the core test; other
validation/publication refusal branches were source-inspected, not claimed as
additional live negative tests. Geometry is evaluated before draft/history
publication, and normal document commit retains downstream atomic validation.

This accepts #231 only. Native text controls/placement, alternate fonts/styles,
general shaping, mixed analytic/text sketches, persistent glyph-edge references,
cross-platform runtime equivalence, and the cumulative source lesson remain
outside this evidence. The source's optional font and rotation choices are not
represented as exact visual parity by this regular-font foundation fixture.
