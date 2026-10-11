---
name: Cumulative validation gate
about: Prove a useful Alloy workflow and retain earlier capabilities
title: 'Validation gate: '
labels: 'kind:validation'
---

## Workflow and prerequisites

Name the useful object/workflow to create, the earlier workflows to retain, and
the prerequisite issues. Use independently authored or licensed fixtures.

## Execution

- Launch the real Alloy app; discover commands and inspect authoritative state.
- Create/edit the object through Alloy's automation API. Use computer control
  only for Alloy behaviors that the API does not expose.
- Re-run earlier accepted workflow fixtures cumulatively, including relevant
  undo, save/reopen, parameter edits, and interchange checks.
- Capture actual Alloy results. Reference comparison uses public docs/screenshots
  only; never run or inspect reference CAD application.

## Acceptance and evidence

Specify expected geometry, units, tolerances, stable references, persistence,
export readback, interaction, and visual criteria as applicable. Report build,
API, geometry, physical UI, visual, and full-workflow evidence separately.
Record app/dependency revision, commands, fixture inputs, measured results,
capture paths, failures, and limitations. A build or mock does not close the gate.

## Outcome

Link evidence and regressions. Close only when prerequisites and stated criteria
are satisfied. Unsupported capabilities remain explicit backlog items.
