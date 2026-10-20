# Sampled shared-edge Surface Continuity — SRF-035 / #638

Alloy now reports sampled positional (G0) and tangent-plane (G1) deviation
for two exact retained planar faces on one exact shared straight edge. Nine
equally spaced edge-parameter samples, including endpoints, independently
evaluate each face's B-rep pcurve, world point and normal. The report includes
each sample, maxima, mm/degree values, active-unit display, separate normal
orientation, and explicit provisional Alloy thresholds of 0.001 mm and 0.1°.
It is a finite-sample diagnostic, not a whole-boundary certificate or a Zebra
appearance judgment. The transient panel and shared API leave modeling history
unchanged.

## Evidence

- Core `27bf194` built in Release with `-j4`; focused C++ checks pass exact
  shared-edge topology, coplanar and 45° cases, world placement, orientation,
  units, refusals, source edits, Undo/Redo and reopen. Panel capture/identity
  fixes are `ea82da0`; placement beside the Browser is `e7d9b15`.
- Independent `tests/surface_continuity_api.py` passed 75 real-app responses.
  The authored connected two-line Surface Extrude retains two named faces and
  one common edge. Analytic plane equations and the shared Z-axis segment
  independently confirm every reported sample: 0 mm/0° when coplanar,
  0 mm/45° after a source endpoint edit. G0 passes both; G1 fails the kink.
- Explicit and retained panel references follow source edit, Undo/Redo and
  save/reopen. Inspection/capture leaves authored JSON, revision, dirty flag,
  Undo state and evaluation summaries unchanged. Inch display changes only
  length presentation. The post-inspection saved model is byte-identical to
  baseline, and reopening the edited save reproduces the exact numeric result.
  Incomplete, duplicate, nonshared and stale inputs return structured errors.
- Apple M4 hardware OpenGL 4.1 / DPR2 captures show a readable panel with
  distinct face IDs, edge ID, maxima, sample limits and thresholds. An initial
  centered panel hid the small selected seam; targeted independent captures
  after `e7d9b15` confirm both faces and seam visible at 1280×900, and the seam
  plus right face visible at 1100×760. The movable panel partly covers the left
  face at the smaller size. Resized native viewport 848×469 logical maps to a
  1696×938 framebuffer. The private validation editor was closed.

Public-source evidence, Alloy method choices and unknowns live externally at
`../Alloy-reference-records/knowledge/specs/srf035-sampled-continuity-2026-10-20.md`.
The independent review, analytic fixture, transcripts, saved models, failed
initial capture, final GPU images and targeted placement evidence live under
`../Alloy-reference-records/validation/surface-continuity-2026-10-20/`.
No proprietary runtime or CI was used.

## Remaining acceptance

Physical actual-hit face/edge selection, panel lifecycle and 1×/2× native
qualification remain first-v1 Todo SRF-054/#675 while the Mac console is
locked. Distinct-edge positive-gap comparison is SRF-052/#676; curved and
nonuniform boundaries are SRF-053/#677. G2, combs and broad settings remain
SRF-008. Public sources do not establish exact equivalent numeric command
appearance/defaults; Windows/Linux packaged runtime remains a release gate.
