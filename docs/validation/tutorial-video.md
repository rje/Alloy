# Tutorial video evidence

The illustrated bracket API case has an optional video mode. It starts from a
blank real Alloy document, performs the same asserted construction as
`tests/tutorial_bracket_api.py`, and records through the application's local
recording commands. Stage captions precede the actions they describe. Visible
modeling changes are paced; geometry/state inspection calls run at normal speed.
The on-screen source label identifies this as local API control. It is not a
physical keyboard or mouse walkthrough.

Run the short sketch-to-solid qualification before the full case:

```sh
python3 tests/recording_smoke_api.py --app /absolute/path/to/alloy \
  --output ../Alloy-reference-records/demos/recording-smoke-evidence \
  --video /absolute/path/to/Alloy-reference-records/demos/recording-smoke.mp4
```

Then record the bracket construction:

```sh
python3 tests/tutorial_bracket_api.py --app /absolute/path/to/alloy \
  --output ../Alloy-reference-records/demos/bracket-evidence \
  --record-video /absolute/path/to/Alloy-reference-records/demos/illustrated-bracket.mp4 \
  --pace 0.55
```

The MP4, recorder log/metadata, authored `.alloy` model and `.events.json` stage
ledger belong outside Git in `../Alloy-reference-records/demos/`. The full case
still writes its normal receipt and source-stage PNGs to `--output`. Recording
ends after the finished five-hole body and saved authored design; the existing
historical edit, undo/redo and save/reopen assertions continue after the video.
Check the recorder's frame/drop/delay totals, video duration and readable
captions, plus the actual five-hole geometry, before citing a finished artifact.

This video can show the API-authored sequence, current application presentation
and final geometry. It cannot establish held native modifier gestures, platform
profile toggles, or exact physical tutorial parity. Those remain separate gates
in [the bracket receipt](illustrated-bracket.md).

## Recorded result, 2026-10-12

The short real-app smoke passed at
`../Alloy-reference-records/demos/2026-10-12/sketch-to-solid-api-smoke-v2.mp4`.
The 40 × 25 × 5 mm rectangle extrusion produced a valid 5,000 mm³ body; recorder
start/stop did not change authored state. `ffprobe` reports H.264, 1280 × 902,
190 frames at 15 fps and 12.667 seconds. The recorder measured 12.619 seconds,
with no capture failures. The [smoke script](../../tests/recording_smoke_api.py)
and external `smoke-evidence-v2/recording-smoke.json` retain command and authoring
results. An earlier test clip failed to drain its encoder queue; it is excluded
from acceptance and retained externally for debugging.

The full bracket run passed at
`../Alloy-reference-records/demos/2026-10-12/illustrated-bracket-api.mp4`.
`ffprobe` reports H.264, 1280 × 822, 5,510 frames at 15 fps and 367.333 seconds.
The recorder measured 367.328 seconds, with 1,802 captured frames and 3,708
explicit repeats preserving wall-clock time, no capture failures, and no encoder
error. Effective unique capture rate is about 4.9 frames per second; repeated
frames preserve duration but motion is less smooth than native 15 fps capture.
Its `.mp4.jsonl` log contains 367 app command events and 22 step captions;
the `.events.json` file is a curated timestamped stage/action index. The saved
`.alloy` file is the finished authored design. All are beside the MP4 outside Git.

The external `bracket-api-evidence/tutorial-bracket-api.json` records the actual
application binary SHA-256, script SHA-256, Git base commit and dirty source paths,
GPU state, full command transcript, 14 workflow stages and 15 captures. The same
run passed independent area/volume checks, one closed five-hole body, exact
authored undo/redo, and save/reopen. Hardware diagnostics report Apple M4,
OpenGL 4.1 Metal. Representative frames at 50, 152, 335 and 365 seconds were
inspected: dimensions, rounded profile, five-region selection in progress and
the final isometric five-hole solid are visible with readable API/caption labels.
The final body uses Home and 2.3× zoom. Astra independently inspected blank,
dimensioned, guide and final frames and accepted this bounded recording result.
These captures establish current Alloy presentation, not matched reference
appearance.

The recording is API-authored construction. Native held-modifier selection and
physical platform profile toggling remain unverified, as documented in
[the bracket receipt](illustrated-bracket.md). The validation edit/undo/reopen
tail ran after recording stopped; it is covered by the receipt, not the MP4.

## Native input evidence

`sketch-to-solid-native.mp4` preserves the complete 750.777-second native run:
11,262 frames / 750.800 seconds, 2,756 captures plus 8,506 explicit repeats,
zero capture failures. Three short encoder delays remain in the log. Desktop
control waits dominate this raw evidence; it is not a polished short demo.
Astra inspected C, the origin click halo, Return with diameter 20, E opening
Extrude, Tab with distance 10 and the final isometric cylinder. The final state
reports a valid closed body, bounds 20 × 20 × 10 mm, volume 3141.592653589793 mm³,
and Apple M4 hardware rendering. Geometry actions originate in the UI; API calls
only captioned, inspected/projected coordinates and saved the result. AX button
activation is recorded as a UI command, without inventing a mouse event. The
saved `.alloy` and `native-final-state.json` retain the result externally.

Held-modifier clicks, smooth capture and exact native tutorial parity remain
separate qualification work. No operating-system release qualification is implied.

The native edited highlight `sketch-to-solid-native-edited.mp4` is 89.8 seconds,
with a persistent "EDITED: idle waits removed" banner. Nine normal-speed windows
retain observed UI commands and native key/mouse presses; the external
`.edit-map.json` maps each segment back to raw time. 661 seconds of idle waits
are removed. The raw recording remains unchanged. This is an explicitly edited
demonstration, not a continuous timing receipt.

Post-capture source fixes f9a2d8b/3376ab5 apply focused-editor redaction to Qt
parent propagation and accurately label allowed numeric input. The incremental
build passes; this does not alter model behavior or the recorded geometry.

Astra accepted the edited highlight: all 37 relevant observed actions remain in
the nine segments, hashes/timing reconcile, and sampled overlays are readable.
Final redaction source fixes were accepted with build evidence; no unchanged
geometry rerun was required. Issue #157 is closed; #154/#155 remain open.
