# Remaining authored Circle Fix/UnFix qualification

SKT-140 / #565 accepted, 2026-10-18. An independent Sol reviewer checked
the five authored-source combinations, native input without inter-step repair,
actual-app API geometry, exact history/reopen and GPU captures; no blockers
were found.
This is a bounded qualification of the existing `fixedCircle` authority; no
application or solver change was required.

The permitted public contract is external
`knowledge/specs/skt-128-glyph-inventory-2026-10-18.md`, P2/P3, retrieved
2026-10-18: Fix locks geometry size/location and shows green; repeat tool/click
unfixes it. Exact public pixels remain unknown. No proprietary software was
operated or inspected.

## Workflow and ownership

Select **Fix/UnFix** in the Sketch Palette and click a circle circumference.
The command commits one relation, clears selection and stays active. A repeat
circumference click removes that explicit ownership. Escape returns to Select.
There is no numeric input, disposable preview or permanent Fix badge.
`sketch.fixToggle` and `sketch.constraint` with `type: fixed` use the same
application command as native input. Existing circle schema locks center and
radius under one stable relation ID; unrelated retained source constraints and
dimensions remain in the authored sketch.

Five separated fixtures use a fixed-origin 30 x 30 construction rectangle with
retained width/height dimensions. They qualify:

| Circle | Retained source | Edit refused while fixed; succeeds after UnFix | Independently checked edited center/radius, mm |
|---|---|---|---|
| Three-Point | Three snapped source corners / point-on-curve relations | Height 30 to 40 | (15,20), R25 |
| 2-Tangent | Bottom/right source lines, retained radius dimension | Width 30 to 40 | (30,10), R10 |
| 3-Tangent | Bottom/right/top lines including a parallel pair | Height 30 to 40 | (10,20), R20 |
| Center Circle | Distinct center Coincident with top-right source, radius dimension | Height 30 to 40 | (30,40), R10 |
| Center Circle | Center shares retained top-right source point ID, radius dimension | Height 30 to 40 | (30,40), R10 |

For every fixture, conflicting source edits refuse atomically with exact draft
and draft-history preservation. UnFix restores the exact prior draft, leaves
source relations/dimensions intact and permits the same source edit. Reapplying
Fix retains the edited result. Exact authored save/reopen preserves IDs and
solver ownership. Existing independent Center Circle and Two-Point Circle
receipts cover other radius/diameter conflicts; this gate focuses on source
combinations rather than repeating those tests.

## Evidence

Build: inspected `build-preview-release/CMakeCache.txt`, Release, AppleClang 17,
Qt 6.11.2 private runtime and OCCT 7.9.3. Candidate evidence:
`../Alloy-reference-records/validation/sketch-fix-circle-modes-2026-10-18/`.

`tests/sketch_fix_circle_modes_controls.cpp` runs five data rows through the
actual application window and Palette/circumference input. One initial harness
activation is separate; later user steps do not repair focus or selection.
Each click targets the actual native viewport hit, verifies native focus,
cleared selection and active tool, then checks native repeat-click removal,
Escape, exact one-step Undo/Redo, source conflict/recovery, fixed role, absent
permanent Fix badge and exact save/reopen. Result: 7 Qt passes (five fixture
rows plus init/cleanup), zero failures, 18.160 seconds. Native receipts and
captures are in `native/<mode>/`.

`tests/sketch_fix_circle_modes_api.py` launches the real `alloy` entrypoint.
Its 250-call receipt passes the same five source combinations, shared
constraint/toggle paths, independently calculated center/radius results,
atomic source refusal, exact history, retained relations and exact authored
save/reopen. API captures/receipts are in `api/`.

The inspected native fixed Three-Point view and reopened views of the other
four modes show green circles/centers, readable dimensions and normal profile
fill with the retained construction fixture. Captures combine QWidget chrome
with the actual OCCT framebuffer. GPU reports Apple M4, vendor Apple, OpenGL
`4.1 Metal - 90.5`, 4x MSAA, 932 x 666 logical / 1864 x 1332 framebuffer and
2x scale. These are unmatched Alloy views, not measured public pixel parity.

The first native Center fixture attempted Coincident between two IDs already
shared by exact coordinate reuse and was correctly rejected. The corrected
fixture creates a distinct center before authoring Coincident, and an
additional fixture separately verifies shared-ID snap ownership. No product
change was needed. Earlier native logs remain under `failed-attempt/`.

## Limits and reproduction

This bounded gate covers these authored source combinations. Circle Offset,
group/derived ownership and projected geometry remain SKT-138/#563. Physical
hand input, other-platform runtime and exact reference styling remain unknown.
The private native/API editors exit sequentially; user-owned PID 33805 was
preserved.

```
cmake --build build-preview-release --target sketch_fix_circle_modes_controls_test -j 6
ALLOY_FIX_CIRCLE_MODES_EVIDENCE=/absolute/evidence/native build-preview-release/sketch_fix_circle_modes_controls_test
python3 tests/sketch_fix_circle_modes_api.py --app build-preview-release/Alloy.app/Contents/MacOS/alloy --output /absolute/evidence/api
```
