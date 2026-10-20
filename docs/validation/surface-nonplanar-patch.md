# Nonplanar four-edge Patch — SRF-028 / #631

This accepted bounded core extends the retained Surface Patch feature with
four ordered ordinary sketch-Line references from earlier sketches. It makes
one nonplanar degree-(1,1) bilinear surface face, with one closed four-edge
boundary wire and no solid. It preserves the existing planar Patch file and
command shape. Connected (G0) means the output boundary attaches in position;
no tangent-continuity claim is made.

The fixture has XY lines A=(0,0,0)→B=(40,0,0)→C=(0,30,0) and YZ lines
C→D=(0,0,10)→A. The corner determinant is 12,000 mm³, proving that the
loop is not planar. Moving D to z=15 raises it to 18,000 mm³. The accepted
0.01 mm maximum boundary deviation is an **Alloy tolerance** for this
fixture, not a published reference-product tolerance.

## Evidence

- Implementation `8d5f199` built in Release at `-j4` with Qt 6.11.2 and
  OCCT 7.9.3. Focused planar and nonplanar Patch tests pass (2/2).
- Independent 64-response real-app fixture passes preview/Cancel, commit,
  source edit, feature edit preview, Undo/Redo, exact authored reopen and
  atomic refusal of wrong count, duplicate, disconnected, bad order and G1
  inputs. Body/face/edge and source identities remain stable on supported edit.
- Actual output has one valid open sheet, one face, one closed four-edge wire,
  zero solids and null volume. Bounds are [0,0,0]..[40,30,10] mm and become
  [0,0,0]..[40,30,15] mm after edit. OCCT areas are 667.266502 and
  727.170867 mm², versus independent bilinear-derivative quadrature of
  667.265055 and 727.169083 mm² (0.03 mm² acceptance tolerance).
- Whole-boundary deviation is bounded at 2.22×10⁻¹⁵ mm, far below 0.01 mm.
  The verifier checks actual output B-rep edge degree-one control poles
  against each independently authored finite source segment, requires full
  endpoint coverage, and checks that actual face pcurves cover all four
  natural isos of the degree-(1,1) support surface. Convexity of finite
  line segments bounds the entire curves; this is not a sample-only claim
  or merely a kernel-reported residual.
- Three actual Qt/OCCT framebuffer captures on Apple M4 hardware OpenGL 4.1
  at DPR 2 were inspected: the skew sheet is visibly shaded with readable
  four-edge/G0 controls. This is Alloy GPU evidence, not matched public
  styling or native pointer-input evidence.

The fixture is `tests/surface_nonplanar_patch_api.py`. The independent
report, authored model, transcript and captures remain outside git under
`../Alloy-reference-records/validation/surface-nonplanar-patch-2026-10-20/`.
Public source provenance and the original fixture proposal are outside git
at `knowledge/specs/srf028-nonplanar-four-edge-patch-2026-10-20.md`; the
accepted two-sketch A/B/C/D fixture above is an Alloy adaptation recorded in
the independent report. No proprietary runtime was operated.

## Remaining acceptance

The Mac console is locked. Actual-hit native canvas four-edge selection,
physical input, 1×/2× interactive controls and matched reference visual
comparison are **not accepted**. SRF-040 owns those first-v1 checks.
Arbitrary B-rep face edges, chaining, curved/larger networks, G1/G2 and
multi-face repair remain SRF-003; Windows/Linux packaged runtime remains a
release gate. This one bilinear sheet does not establish general Patch parity.
