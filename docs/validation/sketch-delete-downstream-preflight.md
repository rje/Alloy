# Historical sketch Delete downstream preflight

SKT-150 / #576 accepted 2026-10-18 after independent Astra review of source,
tests, public-evidence scope, retained logs and native captures. This is the
bounded annular Extrude workflow.

The shared `sketch.deleteSelection` geometry route privately solves the existing
Delete operation, including constructor-owned removals. Before publishing it,
Alloy resolves removed stable entity IDs through committed profile
`sourceEntityIds` and the downstream Extrude's authored `sketchId` and
`profileId` / `profileIds`. Only an intersecting consumer requires candidate
sketch profile derivation; no downstream solid recompute is performed. If that
consumed profile ID disappears, Delete returns `downstream_profile_in_use`.
Diagnostics identify `featureId`, `featureName`, `sketchId`, `profileId`, actual
`removedEntityIds`, all blocked `consumers`, and recovery guidance. There is no
spatial matching, replacement-profile selection, cascade or blanket prohibition
on edits to sketches with consumers.

The refusal preserves the exact active session (including draft, profile
summaries, selection and draft history) and committed document. Native Delete
shows the same actionable message in the wrapped Sketch Palette error area:
keep the curve, or cancel sketch editing and edit the named Extrude to choose a
current profile. The existing feature-edit route remains available after Cancel.

This earlier refusal is **Alloy policy**. Public evidence documents upstream
profile loss, downstream warnings/errors and Edit Feature/reselection recovery;
precise reference timing, text and warning layout for this Delete case remain
unknown. Public provenance and the compact dated workflow specification are
external in
`../Alloy-reference-records/knowledge/specs/skt150-historical-delete-preflight-2026-10-18.md`.
Reference index: S1–S6 in that record; no proprietary runtime was used.

## Verification

Evidence root:
`../Alloy-reference-records/validation/skt150-historical-delete-preflight-2026-10-18/`.
The original retained SKT-147 fixture is the input; the API test adds an unrelated
R3 circle at (35, 0), commits it and saves `api/native-before.alloy`.

- Release build: `build-preview-release`, Qt 6.11.2 private runtime, OCCT 7.9.3.
- Real entrypoint/API: `tests/sketch_delete_downstream_api.py` passes. Owner-generated
  R11 Offset result Delete refuses before Finish with the exact Extrude/profile
  IDs. Association removal then permits testing ordinary authored curve Delete:
  a mixed consumed R15 + unrelated circle request also refuses atomically.
  Full active session and committed document equality are asserted on refusal.
- Safe unrelated Delete adds exactly one draft history step. Undo and Redo restore
  the exact authored draft, Finish succeeds, and save/reopen retain exact authored
  JSON and body summaries. The 5 mm annular solid remains
  1633.6281798666928 mm³ analytically.
- Single API samples recorded 1.2 ms dependent refusal and 45.3 ms safe Delete,
  including transport/application work; these are observations, not a benchmark.
- GPU: Apple M4, Apple vendor, OpenGL 4.1 Metal - 90.5, native framebuffer capture.
  See `api/receipt.json`, `api/transcript.json`, and `api/reopened.png`.
- Native Qt case passes (3 Qt test stages reported, 0 failures, 2.796 s).
  `native/test.log` records Qt 6.11.2 and GPU state. Inspected
  `native/native-refusal.png`: the wrapped red message is fully readable and
  names Extrude; the selected annulus boundary remains. Inspected
  `native/native-safe-delete.png`: the unrelated circle is gone, the annulus
  remains, and the error is cleared. Window size is 1400×900 logical pixels.
  The first attempt failed before workflow input at harness activation;
  the accepted run uses the suite's explicit initial Qt window activation.
- Both private validation processes exited; user-owned PID 33805 was preserved.

Reproduce the API fixture with:

```sh
python3 tests/sketch_delete_downstream_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --fixture ../Alloy-reference-records/validation/skt147-circle-offset-delete-2026-10-18/downstream/before.alloy \
  --output ../Alloy-reference-records/validation/skt150-historical-delete-preflight-2026-10-18/api
```

The focused Qt native case is `widget_interactions_test
sketchDeleteDownstreamPreflight`; set `ALLOY_DELETE_DOWNSTREAM_FIXTURE` to the
absolute `api/native-before.alloy` path and `ALLOY_DELETE_DOWNSTREAM_CAPTURE`
to the absolute `native` evidence directory. It performs canvas selection and
Delete, observes actual selection/focus, reads the visible owner error, then
clicks the unrelated circle and Deletes without intervening focus repair.
It asserts refusal atomicity, safe one-step Undo/Redo, and the same solid after
Finish. This is synthesized Qt input against the compiled application, separate
from physical OS input.

## Bounded limits

This qualifies explicit committed Extrude profile selections in the retained
annular fixture and an unrelated circle. Aggregate group profile selection,
other consumer families, topology repair, cascade/repair UI, arbitrary edit
preflight and missing-reference repair remain broader history work. No exact
reference timing or visual parity claim is made; there is no matched public
warning screenshot. Physical input, Windows/Linux runtime and packaged release
qualification remain separate gates.
