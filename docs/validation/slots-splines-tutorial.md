# Slots and splines tutorial gate — bounded acceptance

2026-10-13. Independent review for #257, source record
`sketch-slots-splines-20261013`. **Accepted after the focused #259 correction.**
The complete functional creation/history evidence below is supplemented by
corrected mixed-sketch color captures. The full creation movie predates that
visual fix and still shows the formerly blue defined ellipse.

## Provenance and evidence

The detailed 31-step source contract and public images remain external at
`../Alloy-reference-records/knowledge/tutorials/after-countersunk-plate/independent-contract.md`.
This is a complete sketch lesson ending in Finish/Save, with no source solid
or STL requirement. Runner checkpoint: `9455df2`,
`tests/slots_splines_tutorial_api.py`. Tested app SHA-256:
`8b5739ee2ab84a06f8470f435b9d858abbfe5e5e23e9b7839a82120109e81a3f`.

Successful evidence is external at
`../Alloy-reference-records/deliverables/slots-splines-2026-10-13/attempt-03/`:
`report.json` includes the complete transcript, source/edited/reopened authored
states and geometry oracle; `slots-splines-three-sketches.alloy` is the editable
source. Numbered PNG/JSON pairs retain the ten intermediate/final captures.
The complete creation recording is
`../Alloy-reference-records/demos/2026-10-13/slots-splines/attempt-03.mp4`.

The successful run reports 247 calls and all 31 stages, exit 0 with editor
closed. Media contains 645 frames at 10 fps, 64.5 seconds, zero capture failures,
224 delayed ticks and 420 repeated frames. It records actual Alloy chrome/GPU
output with explicit API captions. The generic recorder's inputScope field
does not turn these shared-command calls into native or physical mouse input.
Attempts 01/02 remain externally preserved as out-of-viewport fixture failures;
they are not counted as passing runs. No product change was needed for those
runner corrections.

## Independent functional findings

The reviewer inspected the runner and successful report, saved authored model,
actual stage images and representative movie frames 3, 210, 400 and 640. This
confirms blank start, intermediate slot/curve authoring and finished three-sketch
state; it is representative media inspection, not a claim of frame-by-frame
review. Accepted #251–256 ingredient receipts provide the separate focused
Qt input evidence. No additional editor or full workflow replay was launched.

The source chain is complete: rough centered straight slot, rough endpoint edit,
Horizontal/D150/R25; centered arc slot, R150/R15, horizontal endpoints, retained
construction radial line and D135; hide first sketch; three-point fit spline,
middle-point removal/reinsertion and tangent direction/weight edit; three-pole
control spline and cage edit, curvature comb; hide second sketch; conic picks,
actual shared Rho-handle command plus exact numeric correction and acceptance;
origin-centered ellipse, full-axis dimensions, Finish and Save. Final visibility
is `[false, false, true]`. No final curves were substituted by seeded geometry.

The saved file exactly equals both the report's `sourceAuthored` and
`reopenedAuthored`. First-sketch region areas are `7500 + 625π` and `2475π` mm²
(measured 9463.495408493622 and 7775.4418176347535), with zero DOF and signed
clockwise 90° arc path. Ellipse area is `6875π` mm²
(21598.449493429827), full axes 275 × 100 mm. Sketch3 has exactly seven DOF from
the separate free conic, whose retained Rho is 0.683. The fit spline remains
degree five/C2; three control poles produce degree two. Post-recording slot
length and fit-tangent edits are meaningful, preserve the other sketches, and
pass exact undo/redo. Reopen preserves authored data and visibility.

Actual slot dimensions, constraint markers, cage/comb controls, Rho controls
and final ellipse dimension witnesses are readable and geometry appears smooth
at the inspected capture scale. The final image
`10-dimensioned-ellipse-and-conic.png` proves the remaining color mismatch.

## Explicit choices and final visual acceptance

The origin-first slot interpretation follows the documented center-first tool
and source drawing, resolving imprecise prose. Fit/control/conic coordinates
are arbitrary declared fixtures because the lesson leaves them unspecified.
Rho's rational shoulder mapping, spline knot/interpolation policies and comb
gain/density/scale remain qualified Alloy choices. Source ellipse constraint
actions omitted by the prose are made explicit as origin Coincident, Horizontal
and full-axis dimensions. Shared API tool/selection/dimension calls are not
claims of a continuous native tutorial performance or exact public UI parity.

The sole visual blocker, #259 / SKT-064, is resolved by the independently
reviewed solver-freedom classification (`4f26782`, production unchanged from reviewed `f4cfdd7`).
Focused corrected evidence is external at
`../Alloy-reference-records/deliverables/mixed-sketch-colors-2026-10-13/attempt-03/`:
`report.json`, `defined-ellipse-free-conic.png`,
`underdefined-ellipse-free-conic.png`, `selected-defined-ellipse.png` and
`finished-mixed-sketch.png`. Tested corrected app SHA-256:
`8f2fa56b361d96b032aec2168d12b5411895a8549c82216d452f22dd6ff271a6`.
The 19-call focused API run reopened the actual three-sketch source model,
proved black fixed ellipse beside blue free conic, orange selection, blue
ellipse after actual dimension removal (seven to eight DOF), and exact authored
preservation on Cancel. All four actual captures were independently inspected;
geometry, dimensions and palette are readable at this framing. Core evidence
also preserves zero-DOF derived arc-slot coloring and explicitly free evaluator
parameters. See `entity-solved-colors.md` for the bounded implementation review.

This completes #257 using the existing full creation/model/history evidence
and the focused corrected visual evidence. The original 64.5-second recording
remains unedited and explicitly predates the color correction; it is not a
video of the corrected final presentation. No full workflow replay was needed.
Acceptance covers the complete researched sketch lesson with the documented
Alloy choices above and separate native ingredient evidence, not continuous
physical-input execution, universal solver rigidity or broad pixel parity.
