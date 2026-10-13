# Native Sketch Text controls candidate (#232)

Status: implementation candidate, 2026-10-13; independent compatibility review
is pending. Public source contract: `warwick-initials-keyring-20261013`, page
5 steps 19–20, recorded outside the repository in
`../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.
The source demonstrates a Text dialog, editable content/font/size and on-canvas
move and rotation. Its example font and style are examples, not verified defaults.

The Sketch Create menu and solid Create menu expose Sketch Text. The command can
start before choosing an origin plane or supported planar face, or within an
active sketch. A modeless Qt dialog edits uppercase A–Z/0–9 content, cap height,
font, X/Y baseline and angle. The selector offers the exact unmodified DejaVu
Sans 2.37 regular and bold font assets; the latter has SHA-256
`e6476c1b80502924294eed40894c5b18e06c181444ca953e5334262df9c27724`
and shares the release license in `third_party/dejavu/LICENSE`. Each preview
evaluates the pinned outline into OCCT geometry, shows two in-plane GPU handles,
and leaves the authored sketch and draft history unchanged. The center handle
moves the baseline; the outer handle rotates about it. Invalid content, changed
character count on edit, unsupported or missing font, and failed outline
evaluation leave OK disabled with an error. Cancel restores the prior draft.
OK publishes one sketch draft edit through the existing sketch authority.

The supported identity contract is one retained text UUID and one persistent
profile slot per character position. Content edits preserve downstream profile
references only when the character count stays the same. Font, size, placement
and rotation edits preserve these slots subject to normal downstream solid
evaluation. No general host font picker, arbitrary Unicode shaping, kerning,
multiline text, mixed analytic/text sketch, persistent individual glyph-edge
identity, or exact reference-dialog styling is claimed.

Focused evidence: `ctest -R '^sketch_text_controls$'` passed on macOS with the
actual Qt dialog and GPU view. Its injected user sequence typed content,
changed the native font and height controls, dragged both view handles,
canceled a first attempt, accepted the second without interim authored changes,
then reopened Edit. Test activation occurs once at startup; no focus/selection
repair occurs between demonstrated steps. `ctest -R '^sketch_text$'` passed
the unchanged D/B/8 outline and OCCT extrusion fixture. The real-app
`tests/sketch_text_controls_api.py` passed command-first supported-face entry,
dialog/preview/Cancel, bold/height/placement, both handle phases, OK, D/B holes,
and one-body Extrude Join. The preview, dialog, joined solid, report and
transcript are in
`../Alloy-reference-records/validation/sketch-text-controls-2026-10-13/`.
The inspected OpenGL capture shows readable bold DSB glyphs and visible move
and rotation handles; the joined capture shows the rotated raised letters.
This is a bounded #232 candidate, not a full keyring-tutorial acceptance.
