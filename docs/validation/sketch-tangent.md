# Persistent circle-to-line tangent placement

2026-10-12: a bounded Activity 2 circle-placement slice is implemented and verified.
The public-source contract is tutorial-illustrated-bracket-a2; detailed evidence
and images remain in the external knowledge library.

Circle-center placement can retain a line midpoint. At the circumference stage,
a nearby finite line contact shows a square cue; accepting it creates a fresh
circle, midpoint relation and signed circle/line tangent relation atomically.
The radius remains solved from geometry. Stable line/circle IDs and signed side
survive subsequent parent edits. Hover and the initial center add no authored edit.

The 10 logical pixel acquisition radius, 0.5 pixel ambiguity band and finite-contact
policy are Alloy choices. Endpoint tolerance is 1e-6 mm. Exact source inference
priority, target pairings and modifier overrides remain unknown. Each placement
retains one chosen tangent, not an inferred pair of redundant relations. Only
circle/line tangency is supported here; arcs and general curves remain ahead.

## Evidence

- `build-tutorial/evidence/tangent-build.txt`, `tangent-endpoint-build.txt`:
  application builds pass. `tangent-core.txt`: focused sketch_curves test passes.
- `build-tutorial/evidence/sketch-tangent-ledger/report.json`: actual app workflow
  prepares the constrained six-edge outline through explicit API commands, converts
  both ends to construction, then uses pointer midpoint/tangent inference for both
  end circles. Radius 50 and DOF0, parent width 100→120 changes both radii to60,
  exact relation IDs, undo/redo and save/reopen pass. Captures record hover squares
  and retained midpoint/tangent glyphs. Explicit setup is distinct from inference.
- `build-tutorial/evidence/tangent-native-{setup,center,created}.json` and
  `tangent-native-created.png`: API-opened first-activity part and construction/view
  setup, followed by actual C, midpoint click, circumference click and Escape.
  The left circle has radius50, one midpoint and one tangent, DOF0. Native repeat
  at the upper end and the complete second activity were not exercised.

Astra accepted source, real-app receipt, cue/glyph captures and bounded native
placement. Small pale glyphs and origin clustering remain presentation polish;
no exact visual-parity claim. Mixed line/circle region extraction, sketch fillets
and multi-region Press Pull remain separate blockers. No private app remains open.
