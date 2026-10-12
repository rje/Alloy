# Angular sketch workflow

The tutorial-first SKT-006 slice adds genuine two-line angles, independently of
arc sweep. Authored dimensions retain two line IDs, degree values and ray signs
for the chosen acute/obtuse sector. The D tool accepts successive line picks
without Shift, placement chooses the sector, and inline entry creates the driving
dimension. The renderer draws the arc, arrows, witness rays and degree label.
Changing length units does not scale angular values. Label movement preserves
geometry and sector intent.

## Local verification (2026-10-12)

`build-tutorial` is an isolated macOS candidate. The human `build-input` application
and document were preserved. No release or CI was triggered.

- Focused solver tests cover the 45-degree extension sector, all four signed-ray
  measurements, acute/obtuse editing, failures, retained IDs and reevaluation.
- `tests/angular_workflow_api.py` creates a fully constrained triangular sketch,
  extrudes it 5 mm, edits 45 to 60 degrees after changing display units to inches,
  checks the recomputed solid volume, undo/redo and native save/reopen. It also
  verifies actual pointer-sector containment in acute and obtuse wedges.
- Evidence is `build-tutorial/evidence/angular-core.txt`,
  `angular-workflow/angular-workflow-api.json`, `angular-workflow/angle-45.png`,
  and `angular-workflow/angular-plate-edited.png`. These represent a complete
  bounded API workflow, not the full illustrated source tutorial.

Reproduce:

```sh
python3 tests/angular_workflow_api.py \
  --app build-tutorial/alloy.app/Contents/MacOS/alloy \
  --output build-tutorial/evidence/angular-workflow
```

Astra's independent review found two issues corrected before acceptance:
nearest-bisector selection did not correctly classify unequal angular wedges;
second-line interception also needed to exclude curve-dimension placement.
Labels now follow the tangent at their actual anchor and remain upright.

Native testing exposed a crash in Qt macOS accessibilitySelectedChildren while
querying a selection list rebuilt on every pick. The list now preserves its items
on selection-only refreshes. This is a targeted mitigation, not evidence that
all accessibility lifetime behavior is qualified. Draft changes still rebuild.

## Limits

Driving collinear 0/180-degree dimensions are rejected. The implementation does
not yet complete origin/endpoint inference, the illustrated outline, construction,
fillets, face sketches or its multi-profile Press Pull. SKT-006 and TUT-003 remain
open for their broader acceptance criteria. Reference provenance is outside the
repository, including `knowledge/specs/sketch-curves.md` and the illustrated
bracket audit. A selected sector and one capture are not an exact visual parity
claim.

## Native and independent acceptance

Physical D, first line, second line without Shift, placement against the bottom
edge's extension and inline 45-degree acceptance passed after the selection-list
mitigation. Final candidate also passed native placement in the adjacent obtuse
135-degree sector and editing that input to 145 degrees. State and capture are
`native-angle-45-before-final.json`, `native-angle-145-state.json` and
`native-angle-145.png`. Initial lines were API seeded, not native inferred creation.
Astra reviewed final source, API report and captures and found no remaining
angular blocker. This accepts the bounded angular workflow only.

The existing synthetic circle-widget test passed before final UI corrections,
then failed once waiting for a diameter preview (`angular-widgets-final.txt`).
Its isolated rerun passed (`angular-widget-circle-recheck.txt`); no deterministic
circle regression was found. Track this unresolved input/test flake under FND-007.
The Combine widget check passed. Do not present this as unqualified general UI
stability or a release gate. No additional unchanged test sweeps were run.
