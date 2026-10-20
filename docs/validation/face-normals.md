# Selected-face normal inspection — SRF-033 / #636

Alloy now offers **Show Face Normals** in Inspect for the current selected
planar or cylindrical face. It evaluates one strictly interior world-space
B-rep sample and draws a non-pickable GPU arrow along the oriented unit normal.
The arrow targets 48 logical pixels where possible and is capped at 35% of
the face's bounding-box diagonal. This is a transient Alloy inspection
proposal: public evidence establishes evaluated normals but does not establish
a matching native vector-overlay command or its styling.

## Evidence

- Core `a3b33c3` built the Release app and `face_normals_test` with `-j4`;
  the focused test passes forward/reversed, transformed, cylindrical,
  trimmed-hole, unsupported and stale-selection cases.
- Independent `tests/face_normals_api.py` passed 86 shared real-app calls.
  Planar +Z and cylindrical radial normals agree with analytic expectations
  and a separate probe of reopened OCCT B-reps at the reported UV samples.
  Extra samples confirm varying normals across the same cylinder face.
- Toggling, changing selection, orbiting and resizing leave the authored
  document, revision, dirty state and Undo history unchanged. Source edits
  and Undo/Redo recompute the arrow; hiding a body clears it. New/Open reset
  the session-only display off, and the final saved document is byte-identical
  to the original authored save.
- Inspected neutral/light Apple M4 OpenGL 4.1 captures at DPR2 show a readable
  gold arrow on the selected blue face, an attached tail marker, outward
  cylinder direction and removal when disabled. The OCCT framebuffer changed
  from 2056×1216 to 1696×938 on resize; projected arrow length was about
  45–51 logical pixels. The private validation editor was closed.

The independent review, app transcript, saved model, reopened-B-rep probe and
captures live outside git under
`../Alloy-reference-records/validation/face-normals-2026-10-20/`.
The dated public research and explicit proposal/unknowns live under
`../Alloy-reference-records/knowledge/specs/srf033-face-normals-2026-10-20.md`.
No proprietary runtime or CI was used.

## Remaining acceptance

The Mac console is locked. Physical actual-hit face selection, Inspect toggle
and 1×/2× native visual qualification remain SRF-049/#672. Exact public
vector-overlay visual and control parity is unknown. Wider glyph controls,
multi-sample fields, zebra and continuity analysis remain SRF-008; packaged
Windows/Linux GPU runtime remains a release gate.
