# Repeated constraint checkpoint

2026-10-12 local macOS candidate, `build-tutorial/alloy.app`.
This is a bounded implementation checkpoint for the tutorial delivery sequence,
not a completed source tutorial or a closed SKT-007/008/018 issue.

Implemented persistent constraint-first selection, automatic Horizontal/Vertical
orientation, Equal line lengths and circle/arc radii. Successful picks clear the
pending pair while keeping the tool active; Select/Escape exits. Failed attempts
preserve authored geometry and refresh the active tool/prompt. Renderer-authored
badges identify retained horizontal, vertical, parallel, perpendicular and Equal
relations, including their actual target IDs and viewport bounds.

## Evidence

- Debug build: Alloy, sketch_curves_test and widget_interactions_test passed.
- `build-tutorial/evidence/curves.txt`: focused solver tests include Equal lengths,
  five-circle propagation, contradictory dimensions and invalid references.
- `build-tutorial/evidence/widgets.txt`: existing synthetic Qt input regression,
  4 passed; covers sketch entry/dimension editing and Combine interaction.
- `build-tutorial/evidence/constraint-tools/constraint-tools-api.json`: private
  real-app GPU workflow passed. Five circles, four repeated Equal relations,
  one driving diameter, rejected relation recovery, H/V and perpendicular,
  relation-to-glyph targets, Finish, undo/redo, save/reopen and subsequent edit.
- `equal-circles.png` and `line-relations.png` in that directory capture the actual
  OCCT framebuffer and Qt chrome. Fixture coordinates are centered for visibility.

Reproduce with:

```sh
python3 tests/constraint_tools_api.py \
  --app build-tutorial/alloy.app/Contents/MacOS/alloy \
  --output build-tutorial/evidence/constraint-tools
```

## Remaining acceptance

This fixture seeds sketch geometry through authoritative commands; it does not
prove native inferred placement or exact source tutorial actions. Native repeated
constraint button/edge picking and Escape require a dedicated observation. The
existing widget regression does not establish those new gestures. Full source
matched visual acceptance, badge hit/hover, show/hide, deletion and per-entity
constraint colors remain open. Counts/target IDs cannot prove badge readability
or correct visual interaction. No full tutorial, platform release, or CI run is
claimed. The user's previous app instance was not used for these tests.

Astra independently reviewed final source, the 93-entry API transcript and both
centered captures. Accepted the bounded API/geometry/storage and initial badge
presentation: targets visible, active-tool feedback readable, no blocking
clipping/collision in these fixtures. This is not source-matched visual fidelity
or native interaction acceptance.
