# Piston component hierarchy candidate

ASM-013 adds one retained `component.createFromBodies` transaction for selected
current native root solids. It creates one local definition and identity
occurrence per body and uses the existing `objectOwners.bodies` authority;
authored modeling features, body IDs, and world BReps remain unchanged.
`component.reparentOccurrence` moves existing identity occurrences beneath a
component definition or root. The Browser reads the same occurrence hierarchy
and offers conversion and component move actions. A parent occurrence export
includes its descendant bodies. Cycles and unavailable IDs fail atomically.

The independently authored fixture has four native solids: Lower arm, Upper
arm, Inner piston, and Outer piston. Both arms have the same Y-axis hinge hole
at X0/Z80 and mounting holes at X±90/Z0 or X±90/Z180. The piston end-eye
holes align on the +X mounts. The rod and sleeve overlap axially by 30 mm,
with 1 mm initial radial clearance and 0.8 mm after an editable rod-radius
change. Pairwise OCCT intersections have zero volume. This is a bounded
mechanical fixture; no pins, joints, motion, or exact source-asset claim.

`piston_components_test`, `component_reparent_test`, and
`local_components_test` pass. The real-app API workflow creates all four
native solids, converts and nests them, edits the rod's authored radius,
undoes/redoes, saves/reopens, and exports the two-body piston parent. The
independent `piston_fixture_geometry` check opens that saved document and
checks all four solids, circular Y-axis holes, coaxial piston walls, and six
zero-volume pairwise intersections. The Qt-native Browser test Shift-selects
four bodies, tests Escape Cancel, accepts conversion, and moves both piston
components through native context dialogs. The external evidence and exact
independent dimensions are at
`../Alloy-reference-records/deliverables/piston-components-2026-10-14/receipt.md`.

Component placement is identity-only. Existing source features retain root
modeling ownership, so new modeling operations on converted bodies remain
subject to the established cross-owner guard. A nested hierarchy refuses a
Slider joint because the current Slider evaluator does not move descendants.
Parent opacity is disabled/refused until it has defined descendant semantics.
No physical drag gesture is claimed; the native proof uses Qt-delivered
selection and context actions.
