# Bracket construction and midpoint end circles

SKT-170/#647 bounded Qt/API and diagnostic increment accepted 2026-10-20.
Independent Sol source/evidence review found no product blocker in that bounded
increment. The original uninterrupted native and visible-checkpoint requirements
were explicitly transferred to SKT-171/#694 and SKT-172/#695 before closure;
they remain Todo and block TUT-003.
The only product correction exposes the already implemented **Normal/Construction**
context action in viewport diagnostics; geometry and command behavior are reused.

The public contract is external
`knowledge/specs/skt170-bracket-midpoint-2026-10-20.md`, with the compact workflow
and comparison record in `knowledge/specs/skt170-bracket-workflow-record-2026-10-20.json`.
Source IDs remain `tutorial-illustrated-bracket-a1` and
`tutorial-illustrated-bracket-a2`. Public images and provenance remain external.

## Verified candidate

`tests/workflows/fixtures/bracket-activity1.alloy` is an independently authored
six-edge A1 checkpoint (500 mm, 45 degrees, 100 mm); it is test data, not an
application asset. Setup opens the checkpoint, enters its sketch and frames the
camera through the shared API. The demonstrated A2 steps then use actual widget
hits, without modeling substitutions or interstep focus/selection repair:

1. Upper end click, Shift plus lower end click, right-click, actual
   **Normal/Construction** action. Both authored IDs survive; both ends become
   construction and the previously closed outline has no closed profile.
2. C activates Center Diameter Circle. Lower then upper end picks each show a
   midpoint triangle and adjacent-long-edge tangent square before their clicks.
   Esc returns to Select. No numeric circle size is entered.
3. Shared inspection proves each circle center has Midpoint to its actual cap
   and each circle has Tangent to the actually picked long edge. Both radii are
   50 mm and independent point-to-line distances agree.
4. Undo/Redo restores each circle and conversion exactly. An additional shared
   API edit changes the driving cap from 100 to 120: linked centers follow the
   cap midpoint and radii become 60. Undo/Redo and authored save/reopen are exact.

Build: `build-preview-release`, Release, private Qt 6.11.2, AppleClang 17,
OCCT 7.9.3, `cmake --build build-preview-release --target alloy bracket_midpoint_controls_test -j4`.

Replay compiled Qt actual-hit gate:

```sh
ALLOY_BRACKET_MIDPOINT_EVIDENCE=/tmp/alloy-bracket-midpoint build-preview-release/bracket_midpoint_controls_test
```

Replay the separate real-executable lifecycle:

```sh
python3 tests/bracket_midpoint_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output /tmp/alloy-bracket-midpoint-api
```

External evidence root: `../Alloy-reference-records/validation/skt170-2026-10-20/`.
`qt-final/receipt.json` records the compiled Qt route and eight captures;
`api-final/receipt.json` records the successful actual-executable lifecycle and
source edit/history/reopen/resize captures. Earlier successful Qt candidate
receipts are `final/` and `final2/`; failed probes remain separately labeled.
The native framebuffer reports Apple M4, Apple, OpenGL 4.1 Metal - 90.5,
GLSL 4.10, 2x DPR; the Qt fixture viewport is 932 x 616 logical / 1864 x 1232
native pixels. Real-executable resize verifies framebuffer size against DPR.

## Native readiness limit

The compiled Qt route permits initial native activation and one initial Qt
activation before setup. AppKit active/key-editor state and natural viewport
focus are required before each A2 input and recorded at captures. This is Qt
event delivery, not a physical OS mouse/key stream. Popup closure is observed,
then allowed 500 ms to settle before the next input; no focus repair follows.

A stronger opt-in mode omits the initial Qt bookkeeping activation:
`ALLOY_BRACKET_MIDPOINT_STRICT_NATIVE=1`. Its saved `native-final/failure.json`
shows active/key-editor true through the upper midpoint capture, then native
application inactive, editor no longer key and Qt focus null before the next
input. It stops without repairing focus or delivering the next tangent pick.
The console preflight was unlocked. Earlier native probes also lost activation
at a different circle step. Saved checkpoints isolate a native deactivation,
but do not establish its cause or a product defect. A pre-existing child-window
activation warning occurs during sketch setup; the evidence does not establish
that warning as the cause. No speculative activation workaround is added.
Strict native/physical acceptance remains unqualified under SKT-171/#694.

## Visual comparison and scope

The inspected archived A2 construction-menu and two-circle rasters support a
matched bounded action sequence, midpoint/tangent cue categories and cap-circle
outcome. Construction display is dashed; normal curves are solid. The palette
uses the documented Center Diameter Circle name and the resulting three
regions use the pale warm profile fill.

Menu contents/layout and selection colors differ: Alloy's bounded popup has
Delete and Normal/Construction with orange selected caps, while the public
image has a larger menu/marking layout and blue selection. Retained badge
placement also differs, with some Alloy badges stacked farther from the caps.
Exact pixel parity is unknown because the public image's version, platform,
DPR and viewport are unspecified. The Alloy window is 1500 x 950 logical,
light theme and top XY orientation. These differences and the native gate need
bounded follow-up; they are not concealed by the geometry checks.

This does not qualify uninterrupted A1 creation, later fillets/extrusion,
second-sketch guides, broader midpoint variants, packages or other-platform
runtime. The original native right-click dispatch was not established: the Qt
fixture sends a separate `QContextMenuEvent` after the actual-hit right click.
The public menu's broader layout, blue selection and closer badge placement
remain SKT-172/#695. No release, CI dispatch or tag was performed.
