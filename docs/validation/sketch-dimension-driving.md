# Existing sketch dimension driving state — SKT-165 / #611

Candidate, 2026-10-19. Direct Toggle Driven / Toggle Driving is implemented;
independent review and native input acceptance remain pending.

## Command and workflow contract

In an active sketch's Select tool, right-click a supported existing dimension
annotation and choose **Toggle Driven** or **Toggle Driving**. The menu invokes
`sketch.dimensionDriving {dimensionId, driving}`; optional `session` rejects a
stale session. No new dimension or replacement geometry is authored. The stable
UUID, geometry references, annotation and local name remain retained. Conversion
back uses the current measured value and refuses atomically if another retained
driver would make it redundant. Missing dimensions, stale documents and pending
dimension input refuse. There is no conversion preview or confirmation dialog;
dismissing the context menu authors nothing.

Driven dimensions use the existing parenthesized canvas label and read-only inline
measurement editor. They update from solved geometry. Toggle Driven removes the
dimension's own expression or direct user-parameter binding; toggling back does
not restore either. Another local expression referencing the selected driver
causes a refusal explaining that the dependent expression must first be replaced.
Retained constructor invariants can also refuse conversion; this increment does
not redefine constructor ownership. Existing driving-only Model Parameters rows
omit the converted measurement; a grey read-only Parameters row is not claimed.

The clean-room implementation evidence and documented/unknown distinctions are
external in `../Alloy-reference-records/knowledge/specs/skt165-driven-conversion-2026-10-19.md`.
The separate workflow that adds a relation and offers conversion of an existing
redundant dimension remains #612. The earlier new-dimension offer is unchanged.

## Verification

Accepted build directory `build-preview-release` was inspected: Release, source
root `/Users/rje/src/cpp/Alloy`, private Qt 6.11.2 and macOS SDK. Built application,
core and controls targets locally with `-j4`; no CI dispatched.

- `sketch_driven_offer_test` passes existing offer/refusal checks plus direct
  rectangle conversion, exact identity/name/reference/point preservation,
  reciprocal restoration, redundant reciprocal atomic refusal, circle conversion,
  own-expression removal and dependent-expression refusal.
- `tests/sketch_dimension_driving_api.py` passes 57 calls through the real app
  entrypoint. It checks exact conversion intent, draft Undo/Redo, reciprocal
  restoration, a new width driver changing the retained read-only measurement
  from 40 to 50, direct-value refusal, overconstraint refusal with unchanged
  draft/history, Finish and actual framebuffer capture. The retained profile then
  Extrudes 5 mm to one valid closed 6250 mm³ solid with 50 × 25 × 5 mm
  bounds. Editing the source width driver to 60
  updates the same driven measurement to 60 and the same profile/Extrude to
  7500 mm³ with 60 × 25 × 5 mm bounds. Exact authored Undo/Redo checks cover Extrude and the upstream
  edit; save/reopen preserves the edited authored state and 7500 mm³ volume.
- Inspected `api/measured-50.png`: top-view rectangle, readable retained blue
  `(50)` annotation and separate driving `50` annotation. GPU state reports Apple
  M4, Apple vendor, OpenGL 4.1 Metal - 90.5, framebuffer capture at 2× display
  scale. This is actual application rendering; no matched public screenshot
  comparison or cross-platform GPU qualification is claimed.
- The compiled Qt `directToggle` test exercises the actual hit annotation menu,
  actual menu action clicks, reciprocal toggle without inter-step focus/selection
  repairs, parenthesized label, read-only editor, history and saved model.
  Two attempts stopped at the initial `window.isActiveWindow()` prerequisite,
  before demonstrated input. Root's read-only native preflight established
  `consoleLocked=true`; native input remains **unverified**, blocked by the
  console-session prerequisite. No focus bookkeeping workaround was used.

Evidence: `../Alloy-reference-records/validation/skt165-2026-10-19/api/`
contains the app log, transcript, report, GPU state, geometry snapshots, capture and
`converted.alloy`. All private validation processes were closed. Resume native
verification after console unlock with:

```sh
ALLOY_DRIVEN_OFFER_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/skt165-2026-10-19/native build-preview-release/sketch_driven_offer_controls_test directToggle
```

Physical OS input, independent compatibility acceptance and Windows/Linux runtime
remain separate. Do not close #611 on the API/core evidence alone.
