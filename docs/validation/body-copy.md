# Independent retained Body Copy

SOL-083/#621 accepts a bounded root-body snapshot core. Public behavior
is indexed by the external dated workflow contract
[`sol083-body-copy-2026-10-19.md`](../../../Alloy-reference-records/knowledge/specs/sol083-body-copy-2026-10-19.md).
That contract records source attribution, retrieval date, documented Copy → Paste
and independent edit behavior, inferred transaction behavior, and visual unknowns.
No reference application was operated.

## Command and retained contract

- Browser body **Copy** freezes one committed root solid immediately. Later source
  edits, including edits before Paste begins, do not update this clipboard snapshot.
- Browser root **Bodies → Paste** opens **Paste Body**, with independent Name and
  explicit X/Y/Z Distance in millimeters. All three initial distances are zero
  (an Alloy policy, not a documented universal reference default). Preview does
  not change authored state. **OK** commits one transaction; **Cancel** discards
  it. Fit/Home includes the valid staged copy as well as the source.
- Shared local commands are `body.copySelection {bodyId}`, `body.pasteDialog
  {name?,xMm?,yMm?,zMm?}` and direct `body.paste` with the same placement inputs.
  The dialog uses ordinary `command.update`, `command.preview`, `command.commit`
  and `command.cancel`. Discovery, session/geometry inspection and screenshot
  capture use the existing authoritative local API.
- The retained `bodyCopy` feature owns a new feature UUID and body UUID, a frozen
  OCCT BRep v3 payload, canonical Base64, byte count and SHA-256, plus explicit
  XYZ translation. The payload is capped at 32 MiB and must evaluate to one valid
  closed positive-volume solid. No source-body, source-feature or source-topology
  dependency is retained. Each evaluation reads independent kernel geometry.
- Undo/Redo preserves the copied identity, snapshot and placement. Save/reopen
  retains authored geometry separately from evaluated/display data. Export uses
  the same current solids and placements as modeling.
- The copied solid accepts later body-targeted Cut and Move. Its initial
  snapshot feature selects the body from the Timeline; it does not offer initial
  paste-placement editing. Add/edit a later Move to adjust it.

## Qualification and limits

Focused core and real-entrypoint/API evidence exercise a 20 × 20 × 10 source,
+30 mm X snapshot, source Extrude edit to 14 mm, independent Ø4 through Cut of
only the snapshot, independent Move and Undo, history, save/reopen, selected-body
and all-visible STEP/STL readback. The exact final volumes are 5600 mm³ and
`4000 − 40π` mm³. The STEP reader is a separate OCCT executable that does not
link Alloy's interchange code; STL is read independently with strict directed-edge,
normal, winding and connected-component checks. STEP bounds/volume tolerance is
0.00001 mm/mm³; STL bounds tolerance is 0.00001 mm and volume tolerance 0.1%.
Snapshot hash/count/Base64/size corruption, hash-correct malformed BRep,
null/multiple-solid inputs and an attempted live source dependency are refused.

External evidence directory:
[`validation/body-copy-2026-10-19`](../../../Alloy-reference-records/validation/body-copy-2026-10-19/).
Build: macOS arm64 Release, Qt 6.11.2, OCCT 7.9.3 in
`build-preview-release`; use at most `-j4`.

Native actual-hit Browser Copy/Paste, numeric entry, Cancel/OK test is implemented
in `tests/body_copy_controls.cpp` and builds. It is **unverified**: strict initial
window activation failed before the first input; saved session preflight confirms
`consoleLocked=true`. No inter-step focus repair or unlock was attempted. The
session owner must unlock the console before rerunning that unchanged native
gate under SOL-090/#652. API-driven QWidget/Apple M4 framebuffer captures are separate evidence.
Public references do not establish matched-size visual styling or key/focus parity.

This slice accepts root local solids only and XYZ translation only. Copied face
and edge identities are intentionally unavailable; the fixture uses an origin-plane
circle and an explicit target body instead of claiming a copied-face sketch.
Rotation/manipulators, editing initial placement, component ownership/instances,
linked copies, multi-body and cross-document transfer remain under SOL-008 or
later bounded owners. Other-platform native/runtime evidence is unqualified.

Final local run: **80 successful/expected-refusal API responses**, with Apple M4,
Apple vendor, OpenGL `4.1 Metal - 90.5`, 4× MSAA and native 2× framebuffer capture.
Selected STEP readbacks measure 5600 and 3874.3362938564023 mm³; all-visible STEP
contains both solids and STL contains two separate closed mesh components.
The final preview and reopened holed-pair captures were visually inspected: both
staged solids are fully framed and all placement fields/buttons are readable.

| Evidence | Result |
| --- | --- |
| Documented Browser Copy → Paste and independent edit outcome | Matched for the bounded root-solid/API path; native input pending |
| Frozen snapshot, explicit XYZ, Undo/Redo, reopen and exports | Matched to the Alloy issue contract |
| Native actual-hit selection/menu/typing/Cancel/OK | Unknown; session prerequisite blocked before first input |
| Matched public layout, typography, styling and physical keyboard behavior | Unknown; no matched public capture or physical-input qualification |

Independent Sol review accepted the frozen snapshot/API/export core and inspected
the fully framed preview and independent solids. SOL-090 retains the strict
native interaction requirement; the locked session cannot satisfy it.

Reproduce the qualified checks locally:

```sh
cmake --build build-preview-release --target alloy body_copy_test body_copy_controls_test step_fixture -j4
build-preview-release/body_copy_test
python3 tests/body_copy_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --step-tool build-preview-release/step_fixture --output ../Alloy-reference-records/validation/body-copy-2026-10-19/api
```

After an unlocked session passes `tools/native_input_preflight.py`, run the native
test with `ALLOY_BODY_COPY_EVIDENCE` set to the external native evidence directory.
Do not weaken initial activation or repair focus between demonstrated steps.
