# Driving dimension to sketch origin

SKT-091/#455, 2026-10-15. Independently accepted by Astra; root reviewed the
final actual-hit guards and manual-only native target configuration.

While Dimension is active, clicking an otherwise empty visible sketch origin
materializes one fixed solver point with `originPointId`. Existing authored
points take precedence. No Point entity or new profile geometry is added.
The ordinary two-point dimension then uses the origin and circle center.
Untouched sketches are unchanged; this shared route applies beyond tangent
planes and adds no tutorial-specific branch or curved-edge projection system.

The existing tangent-plane core check passes in 0.50 s with a 5 mm circle,
vertical axis relation and driving vertical 10 mm dimension, upstream spline
support edit, undo/redo and reopening. The manual native check passes 3/3 in
4.42 s: real C/typed5/Tab/Return creation followed by D and actual circle-center /
origin clicks, dimension placement and typed10. Actual hit-recipient guards
precede all viewport clicks. One circle entity and origin witness survive save
and reopen. API setup creates the tangent plane and sketch, controls camera,
applies the existing axis relation, and finishes/saves/reopens; those steps are
not native-input claims. Capture shows readable diameter and locating dimension.
The independently chosen existing sheet endpoint is a fixture, not the final
shoehorn corner or an accepted hole cut.

Evidence is external in `deliverables/tangent-origin-dimension-2026-10-15/`:
core log and native-03 test log, preview/committed captures and editable model.
Native-01 failed harness hover delivery; native-02 corrected hover and passed;
native-03 adds actual-hit guards and uses the manual-only artifact target.
No broad suite or tutorial replay was required.

The tangent plane already carries its selected retained endpoint as its origin.
Thus the circle location follows the same support transitively. GEO-019 and
GEO-018 still own exact rounded geometry and generated endpoint identities;
GEO-004/TUT-017 retain the actual finished-support plane, through-hole and
cumulative tutorial acceptance. Public source ID `surface-shoehorn` and source
URLs/images remain external in advanced-audit-2026-10-14/shoehorn-finishing-contract.md.
