# Illustrated first-activity modeling checkpoint

2026-10-12: the bounded modeling sequence passes. This is not full TUT-003
acceptance: new-document tabs and a complete native activity run remain pending.
The public source contract is `tutorial-illustrated-bracket`; detailed evidence
and images remain in the external research library.

The real application creates the connected six-edge origin outline, applies three
Perpendicular pairs, two Horizontal relations and two Equal pairs through retained
constraint-first sessions, then places dimensions of 500 mm, 45 degrees and aligned 100 mm.
The resulting closed profile is fully constrained. Authored relations, glyph target
identities, actual endpoint lengths, analytic area, Finish, undo/redo and reopening
are checked. Constraint-stage and endpoint screenshots accompany the receipt.

Changes required by this actual workflow:

- A nearby placement on one oblique selected edge now chooses aligned length,
  even outside its axis-aligned bounds. Explicit kinds and two-point dimensions
  retain existing behavior. The 96-logical-pixel band is Alloy policy, not verified
  reference-product tolerance.
- PlaneGCS uses its supported least-norm DogLeg step. The default particular step
  collapsed a still-free cap when adding 45 degrees. This changes solver trajectories
  without inventing extra authored constraints; independent residual and collapse
  rejection remain. It does not promise to preserve every unconstrained length.
- Constraint badge scale samples a 100-mm projected basis rather than 1 mm, avoiding
  integer-pixel rounding that hid symbols on the cap at 10% zoom.

Evidence outside git, under `build-tutorial/evidence/`:

- `activity1-combined-build.txt` and `activity1-core-regressions.txt`: build and five
  focused solver/sketch/profile/extrusion/curve regressions pass.
- `bracket-activity1-combined/report.json`: registered 109-command workflow passes;
  its run directory contains stage images, authored receipt and saved part.
- `aligned-native-receipt.json`, `aligned-native-pending.json`,
  `aligned-native-100.png`: API-seeded oblique line; actual D, edge click, off-edge
  placement, typing 100, Enter and Escape choose linear and produce actual 100 mm.
  This native check preceded the combined solver/grid build; its UI policy is
  unchanged in the combined candidate.

Astra independently reviewed source, receipts and stage captures. The final
500/45/100 silhouette and labels match the source workflow in bounded geometry and
presentation. Source display scale/build are unknown, so pixel equality is not
claimed. Cap symbols remain readable but stack near the dimension witnesses,
whereas the reference associates corner marks with corners. Near-zero solved
point coordinates need display formatting polish. Neither is full visual parity.

Adaptive grid coverage was corrected after the first capture exposed OCCT's
negative-origin convention and half-extent semantics. `grid-coverage-build.txt`
and `grid-coverage/grid-coverage.json` record the final focused check. Captures at
10% zoom and after pan/zoom/resize show the grid spanning the drawable; Astra
independently confirmed both. No geometry rerun was needed for this final isolated
presentation correction. Earlier failures and diagnostics remain outside git.
