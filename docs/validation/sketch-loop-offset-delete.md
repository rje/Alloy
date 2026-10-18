# Support-face loop Offset Delete (SKT-145 / #571)

Accepted bounded slice, 2026-10-18. Independent Sol compatibility review
found no correctness blocker after rerunning focused core, inspecting the
current app/transcript and native captures, and checking the input route.

The shared `sketch.deleteSelection` command accepts `profileIds` only for a
generated-only face sketch containing a retained `loopOffsets` owner. In Sketch
Select, click its output region and press Delete/Backspace. This removes the
whole local owner without confirmation; the face sketch, support object and
upstream geometry survive. No new Offset defaults or controls are introduced.
Generated BRep edges are not represented as authored entity IDs. Ordinary sketch
profile deletion remains unsupported. Undo/Redo is one draft-history step.

The row is validated and its exact support and boundary lineage reevaluated
before the selected stable profile is resolved to its owner. Stale, malformed,
duplicate, wrong-profile and mixed target requests refuse before publication.
A downstream Extrude consuming a disappearing evaluated profile refuses with
stable consumer/profile IDs and recovery guidance, including profiles with no
authored source entities. Candidate profile evaluation is skipped when no
committed consumers exist.

The current schema permits exactly one owner row per generated-only sketch.
The fixture proves that an independent second sketch on the same support retains
its owner and IDs. Multiple owners inside a single sketch remain unqualified;
this implementation does not expand that schema.

## Evidence

External evidence directory:
`../Alloy-reference-records/validation/sketch-loop-offset-delete-2026-10-18/`.
Dated public evidence and bounded contract:
`../Alloy-reference-records/knowledge/specs/skt145-loop-offset-delete-2026-10-18.md`.
The public select/Delete gesture is documented. Whole-owner deletion is an Alloy
policy; reference cascade, native profile-region hit priority and exact styling
are unknown. No proprietary reference software was operated.

- Build: `build-preview-release`, Release, Qt 6.11.2, OCCT 7.9.3, Darwin arm64.
- Focused core: `rounded_loop_offset_test` passes both classic annulus and
  `planarInsetV1` ownership removal on a real rounded support face, exact
  surviving sketch/support parameters, malformed/stale/duplicate refusals,
  and missing support/changed boundary lineage refusals.
- Actual application API: `tests/sketch_loop_offset_delete_api.py` qualifies
  classic annulus only. A retained R6 to R5.5 support edit keeps profile/support
  identity; Delete preserves upstream authored features, evaluated body and
  independent sibling sketch. Exact one-step draft Undo/Redo and save/reopen
  pass. A real downstream Extrude consuming that generated profile refuses
  with unchanged session, authored state and evaluated geometry.
- Regression: `tests/sketch_delete_downstream_api.py` passes the existing
  consumed circle Offset fixture, including safe unrelated curve removal.
- Native Qt: `rounded_loop_offset_controls_test generatedProfileDelete`, with
  `ALLOY_LOOP_DELETE_EVIDENCE` pointing at that external directory. The test
  activates once, clicks the actual profile region, verifies profile selection
  and viewport focus, then sends Delete to the actual focus widget without
  inter-step focus/selection repair. Undo/Redo and reopened authored state pass.
- GPU: Apple / Apple M4 / OpenGL 4.1 Metal - 90.5, hardware acceleration true,
  framebuffer scale 2. Native/API before, selected, deleted, undone and reopened
  PNGs are retained. The support face remains visible; the result highlight and
  generated inset boundary disappear and return on Undo. No matched reference
  screenshot is available, so styling parity is unknown.

`planarInsetV1` native/API and its more complex source fixture remain unqualified;
core coverage does not establish that workflow (SKT-152/#578). Multiple owners
within one sketch remain SKT-153/#579. Stale support/lineage errors are atomic
but retain terse evaluator wording; their recovery guidance is not qualified.
Physical OS input, Windows/Linux runtime, resize/display-scale changes and
exact visual parity remain release gates.

## Reproduce

```sh
cmake --build build-preview-release --target alloy rounded_loop_offset_test rounded_loop_offset_controls_test -j 6
build-preview-release/rounded_loop_offset_test
python3 tests/sketch_loop_offset_delete_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/sketch-loop-offset-delete-2026-10-18
ALLOY_LOOP_DELETE_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/sketch-loop-offset-delete-2026-10-18 build-preview-release/rounded_loop_offset_controls_test generatedProfileDelete
```

Only one private validation editor was open at a time; each exited after evidence.
