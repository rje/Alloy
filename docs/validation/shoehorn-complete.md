# Complete surface shoehorn

Independently accepted 2026-10-15: TUT-017, GEO-018 and GEO-004, using product
commit `498b165` and the cumulative `tests/shoehorn_workflow_api.py --complete`
runner. The complete functional workflow creates the dimensioned spine,
endpoint planes and sections, centerline surface Loft/New Component, +3 mm
Thicken, grouped R12/R16 corners, separate R1.5 perimeter, retained rear-endpoint
tangent plane, fully constrained diameter5/location10 circle, Cut All, component
organization and millimeter STL.

The 177-second creation movie starts from a blank document and includes the main
shape and all finishing operations. It uses the shared local application API
with command overlays. Body reparenting and subsequent four-sketch organization
are separate operations. The source sheet stays hidden in the same component;
only the visible finished solid is printed. Native creation through Thicken has
its separate accepted TUT-031 movie. Physical Browser drag remains ASM-012;
closing this functional tutorial does not close that native-gesture requirement.

The baseline is one valid closed 11-face solid. Independent strict STL readback
finds one closed component, 36,770 triangles, positive volume16650.633 mm³ versus
CAD16657.885 mm³ (0.044% difference). The source-length130→140 edit moves the
retained tangent support approximately9.94 mm and rebuilds a valid closed solid
without changing the authored hole. Exact authored undo/redo and saved variant
reopening pass. Earlier focused core gates cover semantic endpoint identity and
atomic refusal of unavailable references; this cumulative replay covers their
actual use after both fillets and through the final hole.

Readable hole dimensions/constraints and smooth finished/reopened geometry were
inspected in actual Apple M4 GPU framebuffer captures. Source operations and
explicit dimensions match the public workflow contract. Exact spline
interpolation, screenshot camera matching and complete native-input parity
remain unknown or qualified; no proprietary application was operated.

Evidence is external at
`deliverables/shoehorn-workflow-2026-10-15/complete-01/`: creation movie/recorder
ledger, seventeen stage captures, `shoehorn-finished.alloy`, `shoehorn-mm.stl`,
`shoehorn-length-140.alloy`, and `edit-validation/report.json`/`transcript.json`.
The original creation run stopped after recording/export because a post-recording
assertion assumed nonempty final Cut support-face metadata. This was a harness
assumption, corrected without product changes. The separate17-call existing-model
validation passed; it does not purport to be the original run's missing report.
Its script is preserved beside the evidence. Astra independently checked movie
metadata, captures, model/history evidence and mesh readback. Gallery links were
verified after adding the full movie, models and export.

No new package, cross-platform runtime qualification or physical print is claimed.
