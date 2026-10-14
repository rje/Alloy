# Piston component hierarchy

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

Independent Astra review accepts candidate `3c0e208`; main integration is
`7bbf456`. The main application build and focused `piston_components` check
pass (0.56 s). Acceptance is bounded to the contract and exclusions above.

## Full creation checkpoint recording

The external `creation-recording-01/piston-components-from-blank.mp4` records
69.8 seconds from an empty document through all four native parts, conversion,
nesting, radius edit, history, reopen and subtree export. The wrapper adds
API command captions and dwell time to the accepted workflow. Four sampled
frames were inspected and no capture failures were reported. Small sketch
details and the nested Browser subtree are not close-ups in the film; the
separate native hierarchy image supplies the latter evidence. Independent
STL readback passes 2,042 oriented closed triangles in two positive-volume
components. This is a component checkpoint, not a joint/motion tutorial gate.
The local gallery links it under `#piston-components`; video, model, mesh and
receipt remain outside Git.

Independent Astra review also accepted the bounded recording claims and asset
links; no new physical-input or completed-motion claim is made.
