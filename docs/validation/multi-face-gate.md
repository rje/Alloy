# Multi-face cumulative candidate — SKT-017

2026-10-13. This is a partial validation candidate, not acceptance of the full
#150 gate. SKT-035 diagnostic enrichment is separately committed as 69c98a3.

## Verified evidence

The focused core test passes (1/1). The real application API runner passes
geometry, authored references, preview/Cancel, undo/redo and save/reopen:

- A 40×25×5 plate retains a radius-3 through Cut, 5 mm from the right edge.
  Width 40→50 moves its center 35→45. An independently dimensioned 8×3 side
  sketch, attached to an actual named planar face, joins 4 mm (+96 mm³).
  Editing the earlier width after adding the boss preserves both dependencies.
- An XZ trapezoid extruded 5 mm supplies a genuinely inclined named face.
  A separately dimensioned 6×3 sketch joins 2 mm along its normal (+36 mm³).
  Changing the slope vertex from (40,20) to (40,24) changes the attached frame
  and rebuilds the boss while retaining support identity.
- Split and deleted support edits fail atomically with `support_unavailable`,
  affected sketch ID/type and the original body/producer/face triple. Revision,
  history and authored state remain unchanged. Cancel or correcting the original
  tool restores the accepted workflow. This is not relink or Redefine support.

Native Qt side and inclined workflows separately pass actual face picking,
Create Sketch, projected pointer/frame agreement, rectangle placement, D and
line selection, separate label placement, typed driving dimensions and Enter,
Finish, profile selection, Join/target controls, distance spin arrows and OK.
Saved volumes are 4954.62833058846 mm³ (plate/Cut plus96) and3036 mm³
(inclined plus36). Independent review confirms saved geometry and dimensions.
The Join distance text/Return route did not commit in the inactive test window;
ordinary spin arrows and OK were used. No exact keystroke parity is claimed.

## Remaining native recovery evidence

The single final recovery-only attempt failed before changing the tool set.
Styled item-view indicator clicks highlighted Split but left Notch checked and
Split unchecked. The session therefore retained its original valid preview.
This does not establish a modeling failure, but actual native visible refusal,
disabled OK, Cancel and correction are still unverified. The opt-in `ALLOY_MULTI_FACE_CASE=recovery` probe remains failing at that
assertion; the registered widget gate covers side/inclined authoring only.
No further focus/target experiments were performed. The editor exited and the
private GUI slot was released. #150 must retain this exact remaining gate.

The joined native captures show an empty Timeline immediately after refresh;
API captures show populated Timeline. Deferred show/layout timing is suspected
and has not been established by a settled saved-fixture capture. Sketch summary
clipping/overlap was observed and tracked separately in FND-022/#211; the accepted
fix is outside this candidate. Controls and authored dimensions remain readable.

## Reproduction and artifacts

Build targets: `alloy`, `multi_face_gate_test`, `multi_face_controls_test`.
Run core with `ctest --test-dir build-multi-face -R '^multi_face_gate$'`.
Generate fixtures with `build-multi-face/multi_face_gate_test
build-multi-face/evidence/fixtures`, then run `tests/multi_face_gate_api.py --app <binary> --fixtures
<fixture-dir> --output <evidence-dir>`. Native uses
`ALLOY_MULTI_FACE_FIXTURES`, `ALLOY_MULTI_FACE_EVIDENCE` and optional
`ALLOY_MULTI_FACE_CASE=plate40|inclined|recovery`; serialize private GUI runs.

Local artifacts are under `build-multi-face/evidence/{fixtures,api,widget}`:
API report/transcript and saved models; native rectangle/preview/joined captures
and saved models; `splitBodyId-selection.json/png` records the unresolved native
checkbox transition. These generated files are not committed. Public workflow
contract is indexed by REF-004; reference evidence remains outside the repository.
Broad #159/#160/#161 repair, projection policies and Intersect remain Todo.
