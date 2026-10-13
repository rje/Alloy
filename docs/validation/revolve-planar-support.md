# Full-Revolve planar support candidate

2026-10-13, #260, isolated `feat/revolve-planar-support` from e805afb. Independent
acceptance is recorded below; scoped native input and GPU inspection pass. Source/implementation
contract: [revolve-planar-support.md](../research/revolve-planar-support.md).

Before changing UI, the isolated OCCT proof used the actual vertical 80,
horizontal bottom 5 and right-bulging R50 source profile. Full 360 Revolve generated
one planar disk through actual retained radial-edge kernel history. Removing
that exact disk with Inside Shell 5 produced one valid positive-volume hollow
solid, volume approximately 37826.7 mm³. Three regular off-seam/apex normal intersections measure
5 mm within 1e-5 mm; wall/cavity/exterior classifications and an actual two-wire
bottom rim demonstrate material and opening. Initial seam nearest-distance
queries were inconsistent; final acceptance uses shape intersections, with no
relaxation of wall tolerance or production Shell parameters.

The authoritative CadDocument fixture separately passes the source profile,
Full 360, generated disk support, actual SolidShell 5, annular rim/opening and
regular wall measurements. Retained profile dimensions are edited to height 85
and R55; Shell rebuilds with the same producer/body/face reference and changed
positive-volume geometry. Exact authored undo/redo/save/reopen pass. Duplicate
current support identity refuses explicitly; changing the upstream Revolve to
partial makes the full-disk descendant unavailable and refuses dependent Shell
atomically. Missing face UUID also refuses without changing the accepted state.
Existing Full/partial Revolve and sharp Shell core fixtures pass separately.

A real generated indirect plane placement exposed an orientation assumption:
its placement Z direction is opposite the parametric X×Y normal. The candidate
uses the actual parametric normal plus solid-held face orientation, yielding the
correct bottom outward frame. It does not name by height or flip the solid to
satisfy a fixture. This finding is preserved in core-attempt-01/02 logs.

External kernel proof/evidence lives under
`validation/revolve-planar-support/proof` and `core-final.log` in the reference
records. Source file `source-full-revolve.alloy` is emitted by the same passing
core authority for the bounded native ingredient, rather than a separate mock
or imported reference mesh. The fixture proves only three regular wall samples;
no uniform singular-tip thickness or complete ornament tutorial is claimed.
Native Windows/Linux remains unverified; GPU selection and actual app captures
are verified separately below.


The focused native Qt fixture passed 3/3 in 10.566 s on its first run. Setup
opens the exact core-produced full source. After one initial harness activation,
actual Qt mouse input opens the existing Shell panel and picks the real bottom
disk; inspection compares the selected producer/body/face triple to its generated
lineage. Actual spin-box typing enters Inside Thickness 5; evaluated preview
leaves authored source unchanged. Actual Cancel, fresh face pick, new 5 preview
and OK produce the retained hollow body. Shared history Undo/Redo and exact
save/reopen pass. No focus or selection repair is inserted between delivered
user steps. This is Qt input, not physical OS input or native source-profile
creation; unchanged Revolve/source creation evidence is reused.

Both actual native captures were inspected: selected bottom opening preview,
readable one-face/Inside/5 mm controls, smooth full-revolved shell and usable
viewport framing. Framebuffer state reports Apple M4, OpenGL 4.1 Metal and
resolution ratio 2. The final private editor is closed. Evidence:
`native-attempt-01.log`, `native-bottom-shell-preview.png`,
`native-revolved-shell.png`, `native-preview-state.json`, `native-final-state.json`
and `native-source-shell.alloy` under the same external validation directory.

Candidate app SHA256:
`7a197244352a9214d2f8158861ac1229c399fa35c16de5fca60c345141e2be07`.
Native fixture SHA256:
`7f0db291864a3271f08657ecaaa8c8c48024a937c6fbaf26c4cd983216ffdd63`.
This ingredient does not accept the complete ornament-pattern workflow or broad
generated-face naming beyond the documented full radial planar scope.

## Independent bounded review

2026-10-13: candidate `254ba9b` independently reviewed without implementing its
production changes or launching another editor. The review inspected the external
`knowledge/tutorials/after-slots-splines/independent-contract.md`, source changes,
core and native fixtures, passing logs, authored fixture and both actual captures.
Existing focused evidence was sufficient; no broad replay was performed.

| Contract | Result | Evidence and limit |
| --- | --- | --- |
| Full 80/5/R50 profile and real bottom disk to Inside Shell 5 | Matched | Actual kernel Generated ancestry; one valid hollow body, opening classification and three regular normal-wall samples. |
| Retained support and editable history | Matched | Generator-based UUID, actual outward frame, height85/R55 rebuild, missing/ambiguous refusal and exact undo/reopen. |
| Face selection, preview, Cancel and OK | Matched | Native Qt 3/3 log and fixture use actual picking and typing with initial-only activation; selected lineage is asserted. |
| Readable GPU presentation | Matched within scope | Bottom preview and final shell captures show legible controls and smooth geometry on Apple M4. |
| Exact reference pixels, physical OS input and other platforms | Unknown | Reference viewport/scale is unmatched; native evidence is injected Qt on macOS. |

Accept #260 as the scoped generated planar support/Shell ingredient. This does
not establish singular-tip thickness, arbitrary generated-face coverage or the
complete ornament lesson; its cumulative geometry and Join remain separate gates.
