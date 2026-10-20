# Selected-face Zebra Analysis — SRF-034 / #637

Alloy now provides a transient **Zebra Analysis** view for one current planar
or cylindrical face. A cached GPU fragment shader draws view-fixed reflected
dark/cream bands from interpolated tessellation normals on that face. The
selected face retains a blue outline, and turning the view off restores its
ordinary material. This is an Alloy first slice: public instructions describe
a body-selected, saved Analysis result with controls, tracked separately as
SRF-051. Stripe appearance is qualitative and provides no numeric continuity
measurement.

## Evidence

- Implementation `8c78eef` and source-edit identity correction `59a132b`
  built the Release app with `-j4`. Focused per-face OCCT aspect and existing
  face-normal tests pass. The shader compiled on the Apple M4 GPU; an initial
  missing GLSL version was corrected and its failure log retained externally.
- Independent `tests/zebra_api.py` passed 114 real-app responses on the final
  build. It covered planar/cylindrical selection, on/off, face switching,
  orbit/resize, hidden body, source edit, Undo/Redo, New/Open reset and exact
  off-before/off-after framebuffer hashes. The first candidate lost the view
  after a source edit because a null optional occurrence ID differed from an
  empty root ID; the final fixture passed without reselection or re-enabling.
- Transient operations leave authored JSON, revision, dirty state, history,
  Undo and evaluation unchanged. A separate probe reopened saved models and
  confirmed exact before/after B-rep hashes, unchanged planar geometry and
  only the expected cylinder B-rep change after its edit. The restored saved
  authored file is byte-identical to baseline.
- Inspected actual Apple M4 OpenGL 4.1 / DPR2 captures show distinct smooth
  curved bands on both sides of an open sheet, selected boundary and ordinary
  neighboring face. Off restores the exact prior framebuffer. At the tested
  orthographic view, the plane displays one uniform gray reflection band;
  this is a declared property of the method, not a multi-stripe result.
  Resized viewport 848×469 logical rendered a 1696×938 framebuffer. The
  private validation editor was closed.

The public-source contract and its unknowns live externally at
`../Alloy-reference-records/knowledge/specs/srf034-zebra-2026-10-20.md`.
Implementation probe, preserved failed source-edit run, independent transcript,
saved models, B-rep check and captures live under
`../Alloy-reference-records/validation/zebra-2026-10-20/`.
No proprietary runtime or CI was used.

## Remaining acceptance

Physical actual-hit Inspect activation, face switching and 1×/2× native
qualification remain first-v1 Todo SRF-050/#673 while the Mac console is
locked. The documented body-level saved Analysis, Browser visibility/Edit and
Direction/Repeats/Opacity/Lock Stripes/High Quality controls remain SRF-051/#674.
Exact reference styling/defaults, joined-seam stripe flow, transparent and
clipped surfaces, broad surface families and Windows/Linux packaged GPU runtime
are unqualified. Numeric positional/tangent continuity is SRF-035.
