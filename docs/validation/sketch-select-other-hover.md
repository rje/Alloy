# SKT-157: Select Other sketch hover preview

Bounded candidate, 2026-10-18; independent review pending. Holding overlapping
visible authored/projected sketch curves still opens the SKT-155 chooser.
Pointing at a row now cross-highlights its current curve in cyan through a
separate GPU presentation. Leaving the popup, hovering its nongeometry header,
choosing a row or Escape clears the preview. Returning to the same row restores
it. Hover never invokes `sketch.selectionSet` and authors no selection, draft or
history changes. Choice still uses the existing stable-ID selection command;
Delete remains the shared SKT-143 authority.

Presentation inspection is available in
`app.state.viewport.gpu.selectOtherPreview` and `selectOtherPreviewVisible`.
Same-state viewport synchronization preserves the preview for inspection and
capture. Changed sketch/selection, unavailable geometry and sketch teardown
clear it. No document schema or modeling options changed.

The compact public-source contract is external:
`../Alloy-reference-records/knowledge/specs/skt157-select-other-hover-2026-10-18.md`
(S1–S3, retrieved 2026-10-18). S2 documents cross-highlighting for overlapping
sketch/projected curves. Cyan and explicit reset semantics are Alloy policy;
exact reference hover color, leave/Escape focus and popup metrics remain unknown.

## Verification

The accepted Release cache points to this checkout, private Qt 6.11.2 runtime
and Homebrew OCCT. Built `alloy` and `sketch_overlap_selection_controls_test`.
The extended retained native test passes 3/3 entries in 18.8 seconds, covering
all four authored/projected line/circle rows, exact preview ID, unchanged draft,
selection/history, leave/reentry/Escape clearing and natural viewport focus.
After choosing each row, the existing native context/key Delete checks retain
exact counterpart/source survivors, one-step Undo/Redo, Finish and save/reopen.

Hover uses Qt mouse-move events delivered to the native QMenu at its verified
row hit rectangle; leave uses the corresponding Qt Leave event. Hold, choose,
Escape and Delete use the existing Qt widget input route. This is Qt-delivered
input; physical OS pointer delivery remains unqualified. The earlier failed
attempts exposed cursor-delivery/inspection-path errors and a same-state sync
reset, corrected before this passing run. The known nonfatal Qt popup warning
remains.

Actual framebuffer and full-app captures were inspected for the line and circle
previews and readable chooser identity. GPU reports Apple / Apple M4, hardware
OCCT OpenGL, `4.1 Metal - 90.5`. Native evidence is external under
`../Alloy-reference-records/validation/select-other-hover-2026-10-18/native-final/`.
The final rebuild only replaces an equivalent OCCT display-priority integer with
its named enum. Actual-entrypoint API evidence for that binary is in `api-final/`:
empty idle preview inspection, both candidate pairs, stable shared selection and
Delete, exact history/projection context and unchanged committed document pass.
The API receipt retains the final app SHA-256. The user-owned editor PID 33805
was preserved; private validation editors were closed.

Reproduce from repository root:

```sh
ALLOY_OVERLAP_DESIGN="$PWD/../Alloy-reference-records/validation/sketch-linked-projection-delete-2026-10-18/api/native-before.alloy" ALLOY_OVERLAP_EVIDENCE="$PWD/../Alloy-reference-records/validation/select-other-hover-2026-10-18/native-review" ./build-preview-release/sketch_overlap_selection_controls_test
python3 tests/sketch_overlap_selection_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --fixture ../Alloy-reference-records/validation/select-other-hover-2026-10-18/native-final/coincident-before.alloy --output ../Alloy-reference-records/validation/select-other-hover-2026-10-18/api-review
```

## Compatibility and retained remainder

Matched: documented sketch row hover cross-highlighting. Alloy policy: cyan,
stable role/ID cues and nonmutating reset on leave/Escape. Unknown: exact reference
styling, color, scale and focus/reset behavior; screenshots are unmatched.
Mismatch retained separately: Depth/Parents and mixed body/occurrence candidates.
Public hierarchy means selectable Face → Body → Component → Parent Component,
not projection provenance buckets. The root-approved split contract is external
`knowledge/specs/select-other-depth-parents-child-2026-10-18.md`; it retains actual
ray ordering, selectable ancestry and one nested occurrence fixture. No general
depth/hierarchy, broader target-family or other-platform runtime parity is claimed.
