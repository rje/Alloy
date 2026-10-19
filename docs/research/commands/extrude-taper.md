# Polygon solid Extrude taper

SOL-081/#619 bounded implementation contract, 2026-10-19. External evidence
`knowledge/specs/sol081-taper-extrude-2026-10-19.md` records public sources,
retrieval date and the documented/inferred/unknown distinction.

- **Input/default:** `solid.extrude` and `solid.extrudeDialog` accept signed
  `taperAngleDeg` in degrees, default zero. Negative narrows the far end;
  positive widens it. Finite domain strictly −90° < angle < +90°; geometry
  collapse may refuse any otherwise in-domain value. No silent geometry clamp.
- **Nonzero scope:** one `profileId`, straight polygon without holes, positive
  Distance, One Side, Profile Plane, New Body. Convex rectangle and concave L
  fixtures qualify the bounded slice. Other combinations refuse atomically.
  Zero uses the previous prism path, including existing operations/directions.
- **Selection/presentation:** ordinary finished profile/sketch selection; native
  panel orders Profile, Start, Direction, Extent, Distance, Taper Angle, Operation.
  Degree suffix, signed value and translucent GPU preview expose taper direction.
  No taper manipulator is claimed. Existing distance arrow remains available.
- **Preview/commit/cancel:** shared evaluator uses OCCT draft helper for preview
  and commit. OK requires successful geometry; Cancel and existing Escape discard
  the session. A failed edit preserves committed authored data and geometry.
- **History/edit/storage:** retain signed angle on the feature, same feature/body/
  profile IDs, normal Undo/Redo and save/reopen. Omitted angle on direct edit
  preserves the retained value. A zero edit restores a prism.
- **References:** qualify the mapped actual far planar cap and its boundary
  descriptors only; taper edits preserve its feature-derived face identity and
  support frame. Side-face and solid-edge descriptors are intentionally absent.
  Do not infer full topology lineage from cap support.

Public evidence documents labels, signed semantics and numeric entry/OK, but
exact keys, focus, cancel, field spacing, theme and maximum angle are unknown.
Alloy's domain, preview/history/refusal and selection policy are explicit local
contracts. No pixel-parity or physical-device-input claim.

Verification: focused kernel/document checks, real-entrypoint convex ±5°/zero,
concave L −5°, downstream far-cap sketch and solid through an edit, atomic
collapse/domain refusal, history/reopen and independent STL bounds/volume.
Native actual-hit angle entry, Cancel, OK and retained edit are separate evidence.
