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


## HIS-013 native-control candidate — 2026-10-19

The preserved driver crash is now diagnosed as a test pointer lifetime error.
The reproduced full LLDB backtrace enters `QTest::mouseEvent` and then
`QWidget::mapToGlobal`: the Timeline arrow press enters a nested menu loop;
the selected action rebuilds the Timeline and deletes the original tile;
the outer QWidget click then attempts its release through the deleted tile.
A guarded tile pointer in the corrected driver becomes null before that outer
click returns. This evidence does not require a product change.

`tests/history_rollback_controls.cpp` delivers that outer click through the
surviving top-level `QWindow`, allowing Qt to route input to the current widget.
Every target is checked with `QApplication::widgetAt`; the menu item and
Move to End receive actual-hit widget input. Initial native window activation
is observed separately; no inter-step focus or selection repair occurs. The
menu must become the active popup and close before the next user step.

The bounded native run opens the Offset Face tile's arrow on Box 20 × 15 × 10
plus top Offset Face +5, clicks **Roll History Marker Here**, then **Move to End**.
Shared inspection checks one body ID, 3000 mm³ prefix and 4500 mm³ latest,
unchanged authored/revision/dirty/Undo state, inactive future tile, exact full
evaluation restoration, read-only feature-edit refusal, camera navigation and
empty selected-face overlay. The inspected prefix/restored GPU captures show
the height change, gray future tile/blue boundary, readable controls and no
stale future-face highlight. Public source IDs and visual unknowns above still
apply: this is bounded native input qualification, not matched pixel parity.

```sh
cmake --build build-preview-release --target history_rollback_controls_test -j4
ALLOY_ROLLBACK_EVIDENCE="$PWD/../Alloy-reference-records/validation/history-rollback-2026-10-19/native" \
  build-preview-release/history_rollback_controls_test
```

External evidence is in the existing dated output directory: full diagnosis in
`native-controls-diagnosis-lldb.log`, final build/run logs, and `native/` JSON
states, popup capture and native framebuffer captures. `native/manifest.json`
records exact executable, source and evidence hashes. Runtime identifies Apple
M4, Apple OpenGL 4.1 Metal, 4× MSAA and 2× display scale; the GPU viewport is
1148 × 608 logical / 2296 × 1216 framebuffer pixels. The private test editor
exits at completion. Independent bounded review remains with the integrator.
Physical device input, Windows/Linux runtime, assembly rollback and exact
public-reference styling remain unqualified under their existing owners.
