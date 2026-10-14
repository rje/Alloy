# Local STEP reference — ASM-010

Independently accepted at `a9d8914` and integrated on main, 2026-10-13.
The separately tracked ASM-006 parent remains Todo. The admitted input is the
independently qualified twelve-solid adapted board fixture. Its original STEP
file is left untouched; the real-app test copies it to a temporary local path.

The insertion creates one local component and one `stepAssemblyImport` feature
in a single undo transaction. The feature's embedded bytes and XDE graph are
the only geometry/hierarchy authority. A local file path is a locator for
explicit Update; a feature-owned outer rigid placement composes after XDE leaf
placements. The local component owns one presentation opacity value. Imported
internal definitions and occurrences remain read-only. Move previews a pending
position; Capture Position commits it, Revert/Cancel discards it. Modeling,
save and export refuse while a position is pending. A missing local source does
not remove embedded geometry. Byte-identical Update confirms current source;
changed-byte Update refuses without a verified face/edge correspondence. That
remaining changed-geometry contract belongs to ASM-011/#315.

Focused evidence:

- `local_reference` core: one qualified board, twelve leaves, stable IDs,
  transformed body/face/edge shapes, component opacity, undo and exact reopen.
- Real application `tests/local_reference_api.py`: passed. It verifies pending
  Move refusal and Revert, Capture, transformed viewport face and edge ray
  picks with inner and outer occurrence identity, a root-sibling Case sketch's
  retained board edge Project following a later placement, Case opacity 0.6,
  save/reopen, and atomic missing/changed-source Update refusal. The retained
  projected line ID stayed fixed while both endpoints moved +10 mm in local U.
  The pending-refusal authored-baseline assertion was tightened after that
  real-app run and passed syntax checking; it was not separately replayed.
- `local_reference_controls` native Qt test: passed after a typed X value in
  the actual Move/Copy dialog, disposable preview, Qt-delivered Escape, closed
  dialog, cleared pending position, and no authored change. The prior pass
  covered a Qt-delivered Cancel click. Dialog and hardware viewport captures
  were inspected.

External evidence: `../Alloy-reference-records/deliverables/local-reference-insert-2026-10-13/api-09/`
and `../Alloy-reference-records/deliverables/local-reference-insert-2026-10-13/native-06/`
and `../Alloy-reference-records/deliverables/local-reference-insert-2026-10-13/native-escape-07/`.
The first native attempts found a stale hidden dialog lookup in the test; the
accepted native passes cover preview, Cancel, and Escape. Insertion's file
picker and an OS-driven Browser context action have source wiring but no
separate OS-driven input claim. The outer rotation axis is an Alloy engineering
placement choice; the adapted fixture does not establish the tutorial's exact
axis or physical fit. Full source-order mounting/cutouts remain in the TUT-016 cumulative gate.

Main application build and local-reference/Split integration checks pass with the qualified external board fixture. Without `ALLOY_ASSEMBLY_FIXTURE`, CTest explicitly skips the fixture-dependent core check; it does not claim a pass.
