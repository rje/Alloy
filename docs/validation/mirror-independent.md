# Body Mirror independent review

Reviewed 2026-10-13 on c6ee735. Accept bounded #188: one retained source body,
a named planar face as mirror plane, and one distinct mirrored body.

The verifier-authored `mirrorBodyRimCancelCommit` case uses the shared commands
only to seed a 25 × 25 × 12.5 stock and inward Shell 2. Actual widget input opens
Mirror, chooses Objects, clicks the body, chooses Mirror Plane, and clicks the
rim. Preview volume is 3,182 mm³; Cancel restores exact authored state. A second
pass commits two valid closed bodies of 3,182 mm³ each. The latest named run
passed in `build-tutorial-polish/evidence/loft-independent/qt-input-final.txt`.

The real-app review independently used the prior Shell fixture, committed Mirror,
verified reflected z bounds 12.5..25 and two valid closed solids, and saved the
result. The completed #172 chain subsequently Joined these halves and retained
supported faces through three cuts, fillet, offset plane, and inner sketch.

Inspected final `mirror-independent/qt-dialog.png` and `qt-preview.png`: both
selection counts are fully readable at 1180 × 760; the selected source is blue
and the new reflected preview is neutral gray. Object and plane selection roles
are explicit. Earlier empty-session mutation and clipped labels were caught by
this review and corrected before acceptance. Exact historic layout parity is not
claimed; the bounded source body selection cue and input flow are satisfied.

Evidence: `build-tutorial-polish/evidence/mirror-independent/`, particularly
`mirrored-shell.alloy` and corrected Qt captures; nested-chain evidence is in
`nested-cubes-independent.md`. Qt-dispatched widget events are not physical OS
mouse input. No private editor remained after verification.
