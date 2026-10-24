# Stitch source-profile fill presentation

SRF-058/#718 is a bounded visual follow-up to the accepted
[two-sheet Stitch geometry/API ingredient](surface-stitch.md). SRF-057/#717
retains actual-hit native input, tolerance editing, effective DPR1/2 visual
qualification and cumulative workflow acceptance.

## Cause and fix

The original saved 0.005 mm fixture reproduces fill striping at 1280 × 900
logical pixels and Home/isometric. Hiding only the two source sketches removes
it; the same sewn sheet remains clean. The competing actors are the finished
sketches' transparent shaded profile fills and the independently tessellated
coplanar sewn sheet. The sketch outlines are separate actors. The original
exact-gap capture is clean; valid B-rep geometry alone did not establish correct
presentation. A one-unit polygon-depth-bias trial did not resolve this transparent
fill interaction and was discarded.

The renderer now resolves each Stitch source producer against authored planar
Patch profile IDs. Only the known source profile fills are suppressed while the
Stitch body or its replacement preview is visible. Sketch strokes, authored
sketch visibility, body color and geometry remain unchanged. Hiding the Stitch
restores profile fills; Cancel restores the source-body presentation. This
bounded mapping covers the accepted Patch-source fixture. Other source
producers and general sheet networks remain separate work.

The lookup is transient presentation state, rebuilt on document refresh and body
staging. No source/result geometry, topology, identities, tolerance, operation
output, authored visibility or persistence schema is changed.

## Evidence

External evidence is under
`../Alloy-reference-records/validation/surface-stitch-visual-2026-10-20/`.
The original before/after isolation is `isolate-before.png`,
`isolate-sketches-hidden.png`, and `isolate.json`. The failed transparent-fill depth-bias trial is
retained separately in `rejected-depth-bias/`. A single opaque Stitch-fill
depth-bias attempt did not improve seam visibility; it was removed and retained
in `rejected-seam-offset/`.

The final Release build uses `build-preview-release`, private Qt and OCCT 7.9.3,
with `-j4`; `build.log` and `final-build.log` record it. Focused `surface_stitch` CTest passes (1/1).
The unchanged SRF-056 real-entrypoint runner passes its 163-call geometry/API
fixture (`result.json`, `transcript.json`): exact/sub-tolerance success, excessive
gap atomic refusal, preview/Cancel/commit, tolerance edit, source edit, undo/redo,
save/reopen and stable body/face identities. It retains 400 mm², two faces, one
sewn/six free edges before the intentional source-height edit to 420 mm². The
0.005 mm gap and 0.005250000000000821 mm maximum output tolerance remain unchanged.

The companion `presentation_probe.py` opens the *original* saved fixtures,
compares authored/evaluated geometry across visual operations and restored visibility
(excluding the expected document-revision counter increase),
and saves `original-identity-preserved.alloy`. Its transcript and result are
`presentation-transcript.json` and `presentation-result.json`.
Matched original-fixture captures are `matched-original-preview.png` and
`matched-original-reopened.png`. The SRF-056 repeated-workflow captures are
`preview-gap-0.png`, `preview-gap-0.005.png`, and `reopened-gap-0.005.png`.
Visibility behavior is shown in `source-sketches-hidden.png`,
`result-hidden-profiles-restored.png` and `cancel-source-fills-restored.png`.
`resized-home.png`, `restored-size-home.png`, `preview-resized-home.png` and
`preview-restored-size-home.png` bound the resize/Home check.

Inspected preview and reopened native-framebuffer captures show no fill
striping. The preview seam is darker and readable, with intermittent segments after
resize; with visible source sketches, the reopened seam remains
light/intermittent. With both source sketches hidden,
the fill remains clean but the actual sewn seam is not visibly established.
That seam visibility limit remains explicitly **unaccepted under SRF-057**;
this receipt records only the bounded fill correction. Restored profile fills
and the resize/Home captures were inspected separately.

The actual viewport reports Apple vendor, Apple M4 renderer, OpenGL 4.1 Metal -
90.5 and effective DPR2. Whole-app captures are 2560 × 1800 pixels at the matched
1280 × 900 logical size; the original fixture and camera/viewport dimensions
are reused, rather than approximated with a different model.

No proprietary application was operated. This is an Alloy before/after visual
correction, not matched public-reference parity. No physical/native actual-hit,
effective DPR1, Windows/Linux GPU runtime or broad Stitch replacement claim is
made. Private validation instances exited cleanly; user-owned PID72618 was preserved.
Independent Sol read-only review accepted commit `48cc314` for the bounded
fill-striping correction. It checked renderer-only scope, original-fixture
before/after captures, fill restoration/Cancel/resize evidence, unchanged
authored/evaluated geometry except the expected revision counter, the focused
CTest and 163-call real-app fixture, and all 32 external manifest hashes.
The reviewer explicitly did not accept seam legibility; [SRF-059/#719](../backlog/github.md)
preserves that visual remainder before SRF-057 native acceptance.

The 2026-10-24 follow-up did not resolve that remainder: positive fill bias,
depth-inheriting shared-edge wire and a fresh presentation-only curve each
failed the sub-tolerance hidden-sketch criterion. All trials were reverted.
Actor display and correct final curve coordinates were measured; the precise
GPU failure remains unresolved. The compact external receipt is
`../Alloy-reference-records/validation/surface-stitch-seam-2026-10-24/attempts.md`.
SRF-059 remains Todo, and RHI-002 retains these fixtures as depth-aware CAD-edge
acceptance cases. No fix or native Stitch acceptance follows from this work.
