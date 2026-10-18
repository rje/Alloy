# SKT-155: overlapping authored and projected curve selection

Accepted bounded slice, 2026-10-18. Independent Sol review reran the retained
native workflow (3/3 in 16.3 s), matched the actual-app binary hash and inspected
the saved selection/deletion captures; it found no material blocker. In Sketch
Edit / Select, hold unmodified left mouse on overlapping visible curves for
500 ms to open Select Other. Its rows show Authored/Projected, curve type and
a short stable ID. Choosing a row uses `sketch.selectionSet`; the Sketch
Palette retains its role and full stable ID. A quick click keeps the ordinary
nearest-hit policy. Escape dismisses the chooser without changing the prior
selection or draft. Moving beyond the pointer drag threshold cancels the hold.
There are no modeling options, preview, confirmation or storage changes.

Context Delete now preserves a deliberately selected candidate beneath the
pointer instead of retargeting to another coincident curve. Delete and
Backspace continue through the existing `sketch.deleteSelection` authority.
Projected circles also receive the same thicker purple selected overlay as
projected lines; authored curves retain their orange selection overlay.
`sketch.selectionCandidates(x,y)` inspects the same visible curve hit candidates,
including full ID, type, projected flag, pixel distance and projected source.
Selection and candidate inspection author no modeling history.

Public evidence and the compact workflow contract are outside the repository:
`../Alloy-reference-records/knowledge/specs/skt155-overlapping-sketch-selection-2026-10-18.md`
(S1–S3, retrieved 2026-10-18). The hold gesture is documented for overlapping
sketch/projected geometry. The explicit role/ID cue is Alloy policy. Ordinary
Delete semantics remain the accepted SKT-143 policy. No proprietary application
was operated, inspected or captured.

## Evidence

Reproduce the bounded runs (paths are relative to the repository):

```sh
ALLOY_OVERLAP_DESIGN="$PWD/../Alloy-reference-records/validation/sketch-linked-projection-delete-2026-10-18/api/native-before.alloy" ALLOY_OVERLAP_EVIDENCE="$PWD/../Alloy-reference-records/validation/sketch-overlap-selection-2026-10-18/native-final" ./build-preview-release/sketch_overlap_selection_controls_test
python3 tests/sketch_overlap_selection_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --fixture ../Alloy-reference-records/validation/sketch-overlap-selection-2026-10-18/native-final/coincident-before.alloy --output ../Alloy-reference-records/validation/sketch-overlap-selection-2026-10-18/api
```

The accepted `build-preview-release` cache points to this checkout, Release,
private Qt 6.11.2 runtime, and Homebrew OCCT. Built `alloy` and
`sketch_overlap_selection_controls_test` with the native Apple Clang toolchain.

`sketch_overlap_selection_controls_test` passes all three Qt test entries,
including its native-input workflow. It opens the accepted SKT-143 fixture and
creates independent authored copies at the current projected line endpoints and
circle center/radius. For each authored and projected target of each shape:

- Native mouse press/hold opens the chooser; the test observes active popup
  identity, reads the exact target row ID and role, and clicks that row.
- It observes popup closure, exact authoritative selected ID, full visible
  palette cue and natural viewport focus before the next input. No focus or
  selection repair occurs between demonstrated steps.
- Authored targets use native canvas context Delete; projected targets use the
  Delete key. Exact surviving parameter JSON preserves the coincident
  counterpart, unrelated geometry and all retained support/source identities.
- Deletion adds exactly one draft history step; Undo and Redo restore exact
  expected drafts and resolved projection context. Finish and save/reopen
  preserve authored data exactly and every upstream body summary remains exact.

Quick-click selection and chooser Escape preserve modeling history. Hidden
projected geometry disappears from candidate inspection. Close camera views are
staged before each independent workflow; cursor positions are projected only
after the camera settles. Native tests retain their known nonfatal Qt popup
window warning. This is Qt-delivered native widget input, not physical OS input.

`tests/sketch_overlap_selection_api.py` also passes through the actual `alloy.app`
entrypoint. Both pairs expose independently selectable IDs and projected sources;
all four shared selection/Delete routes have one-step exact Undo/Redo and exact
projection-context restoration. Cancelling the sketch leaves the complete
committed inspection unchanged. Its receipt retains the tested binary hash.

The API and native runs report hardware OCCT OpenGL, Apple / Apple M4,
`4.1 Metal - 90.5`. Actual app captures show readable role/full-ID cues and
local selected geometry; chooser captures show both independent rows. The
projected circle has a thicker purple highlight after the capture-found
presentation correction. Build, shared API, native input, authored persistence,
upstream geometry and GPU evidence remain separate.

Evidence and saved models are external under
`../Alloy-reference-records/validation/sketch-overlap-selection-2026-10-18/`:
`native-final/` is the accepted native run and `api/` is the actual-entrypoint
run. Earlier `native/` attempts are superseded fixture/assertion/camera setup
evidence, not acceptance. Private editors were closed; the user editor was
preserved.

## Compatibility and retained remainder

Matched: documented approximately half-second hold-to-choose gesture; independent
selection of overlapping authored/projected lines and circles. Alloy role/ID
labels are deliberate source cues. Full popup styling remains unqualified under
unmatched public screenshot size/state. Mismatch: the general documented chooser
has Depth/Parents organization and hover cross-highlighting; this bounded chooser
does not implement those. SKT-157/#583 owns that Todo remainder; its external
bounded contract is `skt155-select-other-remainder-proposal-2026-10-18.md`.
Broader target families, exact hierarchy behavior, physical
OS pointer delivery and other-platform native/GPU runtime remain unqualified.

Existing SKT-143 refusal/cascade policies remain authoritative; no new geometry
Delete policy or source-repair mode is claimed here. The fixture's coincident
circles deliberately produce an unavailable-profile diagnostic until repaired;
that diagnostic does not change the independently selectable local identities.
