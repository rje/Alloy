# Multiple support-face loop Offset owners (SKT-153 / #579)

Accepted bounded implementation, 2026-10-18. Independent Astra review reran
the focused core, matched the retained binary hash, compared all four saved
API/native deletion files and inspected the native log and GPU captures; it
found no material blocker. This extends the [single-owner contract](sketch-loop-offset-delete.md)
to independent retained owners within one generated-only face sketch.

Repeat Offset on the same retained support boundary at a distinct distance and
accept each preview. Each invocation appends its own owner; no new defaults or
options are introduced. In Sketch Select, click a generated result region and
press Delete/Backspace. The shared `sketch.deleteSelection` command removes
exactly its owner without confirmation. The surviving rows, sketch/support
object and upstream sources remain exact. Undo/Redo is one draft-history step.

Rows keep the existing schema and UUID-derived profile IDs. Each owner validates
its own boundary lineage against the shared exact producer/body/face reference;
array order and sibling results are never inputs. Evaluation is atomic across
all rows. Updates address the exact `offsetId`; result Delete resolves the
current profile's evaluated `offsetId`. The final deletion removes `loopOffsets`
and leaves an empty supported sketch. Duplicate owner UUIDs, including alternate
UUID spellings, are rejected. Generated edges remain evaluated geometry rather
than authored entity IDs.

The independent profiles can overlap. During sketch editing, native profile
picking chooses the smallest containing generated profile, with stable profile
ID as an equal-area tie breaker. Distinct nested annuli therefore remain
selectable regardless of row order. This preserves the established per-owner
annulus/interior semantics; it does not partition regions or create dependencies
between offsets. Coincident-owner native disambiguation remains unqualified.

Dated permitted public evidence and the pre-implementation contract are external:
`../Alloy-reference-records/knowledge/specs/skt153-multiple-loop-owners-2026-10-18.md`.
Public evidence documents one selection per Offset invocation, a retained
source/result relation and select/Delete. Exact same-sketch result cascade,
overlap hit priority and styling remain unknown; the owner policy is explicit
Alloy behavior. No proprietary reference application was operated.

## Evidence

Evidence directory:
`../Alloy-reference-records/validation/sketch-multiple-loop-offset-delete-2026-10-18/`.

- Build: inspected `build-preview-release/CMakeCache.txt`; Release, Darwin arm64,
  existing Qt 6.11.2 private runtime, OCCT 7.9.3 and Apple Clang 17. The application,
  focused core and native controls targets build successfully.
- Core: `rounded_loop_offset_test` covers two classic annulus owners and two
  `planarInsetV1` owners on the real rounded cap, either deletion order, exact
  surviving row/profile summary, row-reorder invariance, duplicate owner UUIDs
  and an invalid sibling lineage. Existing single-owner support/schema/lineage
  refusals and downstream rounded Join/edit/history/reopen checks also pass.
- Real entrypoint API: `tests/sketch_multiple_loop_offset_delete_api.py` creates
  both classic owners in one sketch, broad annulus first, at -3 and -1.5 mm.
  A third append-preview cancels without changing the draft. Updating the second
  owner preserves the first and Undo restores both exact rows. An R6 to R5.5
  source edit changes both areas while retaining profile/support identities.
  Deleting either owner preserves the other profile summary, support, upstream
  authored features and evaluated bodies exactly. Duplicate/stale/multiple/mixed
  targets refuse with unchanged session/document/evaluation. Exact one-step
  Undo/Redo and save/reopen pass for each deletion order. Deleting a consumed
  profile refuses with stable consumer/profile diagnostics; deleting its sibling
  succeeds and preserves the downstream solid exactly.
- Native Qt: `rounded_loop_offset_controls_test multipleOwnerProfileDelete`
  reuses `multi-native-before.alloy`. For each owner, an actual viewport hit
  selects the expected stable profile and establishes viewport focus, then
  Delete is sent to the current focus widget. Initial harness activation is
  separate; no focus or selection repair occurs between click and Delete.
  Exact owner removal, one-step history, surviving profile/body geometry and
  saved/reopened authored state pass.
- GPU/presentation: API and native before, selected, deleted, undone and reopened
  framebuffer PNGs are retained under `owner-0/` and `owner-1/`. Captures show
  smooth nested rounded boundaries, blue selection, gray retained support,
  the removed generated boundary disappearing, and the sibling remaining.
  Undo restores both boundaries; labels remain readable. GPU is Apple / Apple M4 /
  OpenGL 4.1 Metal - 90.5, hardware acceleration true, framebuffer scale 2.
  API captures use a 1440 × 900 logical window; native captures use 1500 × 950,
  both at scale 2. These sizes are unmatched to public references.
  Exact binary hashes and graphics details are in `receipt.json` and
  `native-receipt.json`. No matched public reference screenshot is available.

The first API fixture attempts exposed two harness assumptions: reopening
renames the document from its path, so atomic comparison now uses a fresh
post-open baseline; draft geometry is returned by commands rather than stored
under a `sketchGeometry` session key. Those were test setup corrections.

Multiple-owner `planarInsetV1` is core-qualified; its multiple-owner real-app and
native workflow remains Todo SKT-158/#584. The existing single-owner inset acceptance
is unchanged. Physical OS input, Windows/Linux runtime, display-scale/resize
changes, arbitrary multiwire supports, offset-of-offset dependencies,
coincident-owner native selection and exact visual parity remain unqualified.
Stale-support recovery guidance remains the separate support-repair scope.

## Reproduce

```sh
cmake --build build-preview-release --target alloy rounded_loop_offset_test rounded_loop_offset_controls_test -j 6
build-preview-release/rounded_loop_offset_test
python3 tests/sketch_multiple_loop_offset_delete_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/sketch-multiple-loop-offset-delete-2026-10-18
ALLOY_MULTI_LOOP_DELETE_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/sketch-multiple-loop-offset-delete-2026-10-18 build-preview-release/rounded_loop_offset_controls_test multipleOwnerProfileDelete
```

One private editor at a time; the API harness and native test close their private
process/window. The user-owned editor PID 33805 was preserved.
