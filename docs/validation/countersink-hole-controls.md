# Native Countersink Hole controls — candidate (#247)

Status: bounded #247 independently accepted, 2026-10-13; integration is separate.
Public basis: `plate-countersink-pattern-20261013`, page 4, retrieved 2026-10-13.
Its pictured Hole uses one sketch point, Countersink, Simple tap, All extent,
bore Ø4 mm, countersink Ø10 mm and 90° included angle. The drawing's sink depth
3 mm is derived from those inputs. Public provenance/images/unknown gestures
remain outside the repository in
`knowledge/tutorials/next-after-outlet/independent-countersink-contract.md` and
`independent-visual-acceptance.md`. Defaults before source editing are unknown;
4/10/90 are explicit Alloy choices matching the finite fixture.

The native controls extend the [authoritative backend](countersink-hole-backend.md).
Create → Hole launches From Sketch placement with one explicit standalone Point,
one Objects to Cut target, Countersink / Simple / All, separately labeled Bore
Diameter, Countersink Diameter and Included Angle. Fixed mode rows are disabled;
unsupported modes are not presented as implemented. Derived Sink Depth is a
read-only measurement, never a contradictory independent input. A preselected
finished Point retains its exact sketch/entity/point identity; command-first
creation can select the actual marker. Point selection infers its support body
only when no target was selected. Later target/point picks do not silently
replace the other reference.

`solid.hole`, `solid.holeDialog`, shared session Update/Preview/Cancel/OK and
Timeline edit invoke `CadDocument::holeCandidate` and the same OCCT evaluation.
Direct edits preserve omitted existing fields. Full candidate validation supplies
attributed errors before display extraction; failed preview disables OK. Session
callbacks use owned IDs and read-only identity guards so late signals do not
insert a phantom canceled session.

The preview shows the actual result plus red removed material. Display clipping
uses stock ∩ the backend's actual through cutter; it does not alter authored
geometry or termination. Select Target clears the disposable preview first,
restoring the selectable stock AIS model, then an actual body pick rebuilds the
preview. Cancel clears the cutter/prefix stage and preserves authored state.
A small OCCT polygon offset affects only the disposable red presentation to
avoid coincident cavity/removed-material depth fighting. The result and cutter
geometry remain exact and unchanged.

## Bounded evidence

Shared isolated worktree: `Alloy-worktrees/countersink-hole`.
Backend commits: `caca678`, `7288408`; CMake controls target: `6ff47ef`.
Native evidence: `build-hole/evidence/hole-controls/`.

- Combined Debug application and `countersink_hole_controls_test` compile.
  The existing Sketch Text Qt deprecation warning remains; no new build failure.
- Final native Qt test passes 1/1 in 8.03 s. Shared API setup authors a centered
  75 × 50 × 10/R5 plate and actual supported Point with two retained 10 mm edge
  dimensions. Native steps then select that finished Point, click Create → Hole,
  explicitly select the target via a real viewport click, edit each numeric
  field (5→4, 12→10, 100→90), inspect real preview, Cancel exactly, launch a fresh
  command, explicitly pick Point, OK, click the actual Timeline feature and edit
  bore Ø4→5. The same Hole UUID persists and the fixture's accepted source result
  has analytic volume `36500 + 183π` mm³.
- Initial harness activation occurs once **after** API fixture setup and before
  the first native source action. It establishes Qt's active window/native
  viewport and asserts focus. No focus or selection repair occurs in the native
  sequence. Numeric input asserts the actual receiving QLineEdit has focus;
  `native-detail.log` and `native/native-focus.json` record observed active-window
  and focus state. Target/Point picks assert role transitions to `ready`, so an
  ignored click cannot pass by retaining a preselected ID. These are injected Qt
  events, not physical OS input.
- Inspected `native/native-hole-preview.png`: readable From Sketch, one Point,
  target, Countersink/Simple/All, both diameter labels, 90°, derived 3 mm and
  Cancel/OK. `native-edited-hole.png` shows the actual smooth conical entry and
  bore after a native edit. Actual GPU reports Apple / Apple M4 / OpenGL 4.1
  Metal, with hardware acceleration and native framebuffer capture.
- The 8.03 s behavior run precedes only the final preview polygon-offset change.
  A focused real-app capture from its recorded authored stock verifies the final
  production rendering and exact Cancel. Final accepted visual artifact:
  `final-preview/hole-preview-depth-bias.png`, with `state.json` and
  `transcript.json`. Its red cone is smooth; earlier radial striping is resolved.
  The unchanged native workflow was not replayed solely for a presentation fix.
- Reuse the backend's real executable API/history/reopen evidence. That passing
  API binary precedes the Select Target presentation reset, read-only session
  guards and polygon offset; those UI changes are covered by final native or
  focused capture evidence. Backend geometry and direct valid API behavior are
  unchanged. Exact production fingerprints are retained with that API receipt.

Earlier attempts are retained, not claimed as passing: immediate/settled focus
assertions preceded the mouse pick, then a role assertion exposed the preview's
unselectable target. A numeric focus diagnostic established the harness was
inactive after API setup, so the sole initial activation moved to the actual
native boundary. These observations are in `native-immediate-focus.log`,
`native-settled-focus.log`, `native-spin-parent.log`,
`native-target-preview-blocked.log` and `native-inactive-before-boundary-fix.log`.
The target-pick defect was fixed before the passing run; no mid-sequence focus
repair or manufactured geometry was used. Private editors exited.

Matched within the finite source ingredient: one retained standalone Point,
Countersink/All, source numeric values, actual cut preview, Cancel/OK and editable
Hole history. Exact source pointer timing, keyboard launch defaults, pixel/style
comparison, broad Hole families, physical OS input, other-platform runtime,
nine-hole Pattern, the complete named-parameter tutorial and release remain
unqualified. Viewport size/scale are unmatched to the public page.

## Independent acceptance

Accepted candidate `a2debb81bc01ae0860d8965240578b87af2d9832` for the bounded
#247 Hole prerequisite after independent backend/control source, core/native
logs, actual GPU images, API transcript and persisted authored-model review.
No verifier GUI replay was needed.

The kernel resolves the retained standalone entity/point against its earlier
supported sketch, checks the surviving trimmed face and current material entry,
and drills opposite the outward support normal. All reach is recomputed from
current target bounds; the reusable occurrence tool does not retain a finite
seed depth. The focused XZ fixture verifies a non-world-Z direction. The
countersink formula yields 3 mm depth and 67π mm³ removal for the source seed.
The requested diagnostic correction preserves the underlying error code while
attributing Hole feature, source sketch/entity/point and target; the existing
atomic missing-point check now verifies those fields.

Independent artifact readback found 43 API calls and exact equality between the
saved model and reported authored state. The width-85 result volume differs from
`41500 + 183π` by about 3.7e−9 mm³, retaining the point at x−32.5. Core and API
evidence support the stated edits, refusals, exact history and reopen limits.
The corrected native test has initial-only activation, actual menu/point/target
picks, focused numeric entry, exact Cancel, OK and Timeline edit; its passed
8.03 s run is distinct from the API fixture and from physical OS input.

The initial red cone had visible depth-fighting stripes. Independent inspection
of `final-preview/hole-preview-depth-bias.png` confirms their removal, with a
smooth red entry, gray stock and readable complete controls. The source change
biases only presentation, and independent transcript comparison confirms exact
authored preservation after the focused Cancel. The earlier native behavior run
remains applicable; no unrecorded full replay is implied. Broader Hole modes,
occurrence-specific Adjust #248 and cumulative #249 remain separate gates.
