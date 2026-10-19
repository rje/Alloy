# Transient history rollback — HIS-012

2026-10-19 candidate on macOS arm64. Bounded read-only prefix inspection;
assembly rollback and broader timeline authoring remain HIS-003 work.

## Command/workflow contract

The Timeline feature action **Roll History Marker Here** invokes
`timeline.rollbackBefore {featureId}`. The named feature is the first inactive
feature. Its left border marks the boundary and future authored tiles remain
identifiable with a gray background. **Move to End**, also exposed as
`timeline.returnToLatest {}`, restores the complete accepted evaluation.
**Show latest** continues to scroll only. No new shortcut, drag or playback
behavior is introduced.

`document.inspect.rollback` and `app.state.ui.timeline.rollback` report active,
beforeFeatureId, evaluated/authored feature counts, readOnly and the explicit
Undo/save policies. Timeline tiles report `evaluated`. Authored features, IDs,
revision, dirty state and Undo stack do not change when moving the marker.
A missing feature or failed prefix refuses atomically as `rollback_unavailable`.
No marker appears in the authored schema or saved file.

The document retains its complete accepted evaluation separately from the
transient prefix. Geometry inspection and viewport presentation use the same
active evaluation. Return to latest reuses the complete cache. Save writes the
complete authored history while leaving the inspection marker active; reopen
starts at latest. Undo/Redo returns to latest and traverses ordinary document
history. A successful document application also clears the marker.

Modeling and authored metadata changes are refused as
`history_rollback_read_only` until Move to End. Camera navigation, inspection,
capture, document decisions/cancel, local file actions and tab lifecycle remain
available. There is no feature insertion/editing at the marker in this child.
Existing command sessions must finish or cancel before rollback begins.

Documents containing joints or an assemblyPin feature refuse rollback before
changing state. Assembly presentation currently consumes top-level joint/Pin
constraints outside the evaluated feature loop; truncating geometry alone
could silently retain future constraints. Qualifying that constraint-prefix
behavior remains the explicit assembly-history remainder under HIS-003.

## Evidence and compatibility

Public source IDs `timeline` and `timeline-rollback-menu` support history-marker
navigation and the named Timeline action. Retrieved 2026-10-19. The external
research/compact specification is
`../Alloy-reference-records/knowledge/specs/his012-transient-rollback-2026-10-19.md`.
The precise current menu side/inclusivity, native marker dimensions, keyboard
behavior and pixel styling remain unknown. Choosing an explicit before-feature
API and read-only policy are bounded Alloy decisions. No matched screenshot or
exact reference-workflow claim is made.

The focused core test creates two authored Boxes and checks one/empty prefix,
world-body presentation, unchanged authored/revision/history, atomic invalid
marker, exact full-cache restoration, Save while rolled/reopen latest, and
Undo/Redo. The real-entrypoint fixture uses a 20 × 15 × 10 mm Box followed by
top Offset Face +5: the same body ID changes from 3000 to 4500 mm³ and height
10 to 15 mm. It checks actual prefix/latest geometry, retained feature IDs,
unchanged authoring/history, read-only refusal, camera navigation, lifecycle
inspection/answer routing, Save/reopen and Undo/Redo. No export path changes.

```sh
cmake --build build-preview-release --target alloy history_rollback_test -j4
build-preview-release/history_rollback_test
python3 tests/history_rollback_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/history-rollback-2026-10-19
```

Cache: Ninja/Release, Qt 6.11.2 from the accepted private runtime,
OCCT 7.9.3, AppleClang 17 arm64. Core checks passed. Actual-app API checks
passed; native framebuffer captures identify Apple M4, Apple driver,
OpenGL 4.1 Metal, 2× display scale. Prefix/full/reopen captures and structured
transcript/report are in the external output directory above. The first
capture exposed a stale later-feature selected-face overlay; marker movement
now explicitly clears that renderer selection before publishing the prefix.

Final API replay passed with a clean inspected prefix capture: no future face
highlight remains, and the GPU inspection reports an empty selectedSupportFace.
The external report retains the final executable/model/capture hashes and call
count. Independent review is recorded separately by the integrating reviewer.

One bounded Qt actual-hit driver attempt is preserved externally as
`history_rollback_controls.cpp`, `native-controls.log` and
`native-controls-lldb.log` in that output directory. It failed with exit 139;
LLDB observed EXC_BAD_ACCESS in QtWidgets `mapToGlobalTransform` after native
window bootstrap. The driver did not record a successful action checkpoint,
so whether the Timeline action or Move to End was reached is **unknown**. This
is failed probe evidence, not proof of a particular product/input defect; no
inter-step focus repair or accepted native result is claimed. Native control
qualification remains a separate gate.

Actual-hit native menu/button input, physical OS keys, Windows/Linux runtime,
matched visual parity and broad-history/assembly behavior remain unqualified.
The private validation editor is closed after each API fixture.
