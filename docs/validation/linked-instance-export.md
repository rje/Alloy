# Placed local instance export checkpoint

ASM-018/#368 is independently accepted through `20aa7a4`, with the subsequent
one-line STEP dialog capture addition. Integrated actual-app and readback checks pass.

`document.exportStep` accepts `path`, `units` and a body, occurrence, or
`allVisible: true` scope. A shared `bodyId` must be qualified with `occurrenceId`.
STL retains its existing mesh controls and the same scopes. Results include
`instances` with occurrence/definition/body IDs and placed bounds, plus
`bodyCount`, `scopeLabel` and `authoredChanged: false`.

Selection follows occurrence ancestry. Already placed evaluated geometry is
exported once per intended instance, including repeated source body IDs. Hidden
source sheets remain excluded from visible component/design scopes; explicit
body behavior is preserved. Transient joint/reference poses refuse with
`staged_pose`; committed solved poses export as displayed.

## Evidence

- Focused core checks six-leaf STEP and two-leaf selected subtree output,
  visibility, ambiguous source-body refusal and legacy geometry behavior.
- `tests/linked_instance_export_api.py` passes 36 actual-app calls. Independent
  STL triangle readback checks six and two separate closed, consistently oriented,
  positive-volume components and placement bounds. STEP reimport checks the same
  six/two placed solids. A committed seven-joint pose exports and reimports six
  solids; a staged preview refuses without creating a file.
- Selecting the copied Inner body and opening Export STEP retains that exact
  occurrence. The real dialog capture shows its full readable path and millimeter
  units. Save-file interaction itself is not tested through physical input.
- Main app builds; linked/export and copied-assembly core checks pass 2/2.
- The first API attempt passed geometry checks but found STEP missing from the
  dialog-capture whitelist. Adding its object name beside the existing mesh
  dialog enabled the final capture; API-02 passes completely.

Evidence: `deliverables/linked-instance-export-2026-10-14/api-02/` outside Git.
These files are geometry/export evidence, not a physical-print qualification.
