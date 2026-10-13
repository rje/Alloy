# Straw support: video-variant workflow gate

Prepared 2026-10-12 for TUT-005 #171. This is a runner/contract, **not a passing
receipt**. `tests/support_tutorial_api.py` has only been syntax checked here;
root will run the integrated candidate after independent native review releases
the desktop. No application was launched for this preparation.

Public evidence comes from source `support-original-video-20261012`, creator
Diane Burton, University of Warwick WMG Outreach. Original PDF/video, retrieval
date, URLs, hashes and timestamped images remain in the external
`knowledge/tutorials/warwick/straw-support-ledger.json`. The root/reviewer resolved
the second fillet selection from nearby frames: **both vertical outer-prong
corners**, superseding the earlier top-rim inference. Two seed edges at radius
1 mm are the video variant. PDF step10 shows six selected edges at0.5 mm;
its complete selection map remains separate and unknown.

## Step ledger and proposed evidence

| Source steps | Inputs / authoritative API route | Evidence gate |
|---|---|---|
| 1–2 | XY sketch; rectangle32×10; driving32 horizontal and10 vertical dimensions | Authored input inspected; native command-first Rectangle and annotation fidelity separate |
| 3–4 | Normal7.5 guide from left midpoint; circle center(7.5,−5), diameter15; driving guide and diameter | Authored input inspected; coordinates are the declared Alloy fixture frame |
| 5 | Three adjacent3×10 strips at x23..26/26..29/29..32; observed inner/outer3 dimensions | Source has two visible3 labels; middle width follows layout, not an invented third label |
| Video170–192s correction | Trim interior top/bottom chords, both left halves, guide | Required video steps omitted in PDF; retain32, diameter15, two3 dimensions; guide7.5 and rectangle10 removed |
| 6 | Disk20 New Body | Actual full-circle region, positive closed BRep; no primitive substitution |
| 7 | Bridge and middle strip4 Join | Two selected region preview/cancel; Alloy connected-body counts1→2, reference Browser count unknown |
| 8–9 | Outer24 Join, inner22 Join | Source absolute heights; final one connected retained body; analytic volume and ownership inspected |
| 10 / video280–290s | Both outer x32 vertical corners at y−10/y0; radius1, tangent chain on | Provenance IDs, projected actual edge hit test, blue selected-edge preview/cancel; native/visual mapping reviewed independently |
| 11 / video~320s | Command-first Circle; original disk cap z20; center(7.5,−5), diameter5.2 Enter | Original producer/body/face tuple preserved through Join/Fillet; actual supported sketch and authored diameter |
| 12 / video380–395s | Cutter47 New Body | Video changes initial52 to final47 before commit; exact prism volume, bounds z20..67 |
| 13–16 / video419/434/449s | Move Body, Free Move; Y−10°, Z−31 mm, final X−2 mm; no copy | Incremental numeric observations; declared foot pivot/local frame below; native trajectory separate |
| 17–19 / video450–470s | Combine Cut: support target, moved cylinder tool, Keep Tools off | Preview/cancel, consumed tool, one valid closed retained solid, analytic removed volume, one Undo/Redo |
| 20 optional | Separate Alloy diameter edit5.2→6.2; restore by Undo | Supported edit evidence only; source outro points to a separate adjustment video |
| Printable artifact qualification | Native save/reopen and binary mm STL | Exact authored reopen; independent stdlib triangle readback verifies closed/oriented mesh, volume and bounds |

The optional text and hole chamfer shown after the core cut are explicitly
outside this gate, mapped to remaining emboss/text coverage #101 and solid Chamfer #50. The PDF
six-edge fillet variant remains under broader Fillet #48. Neither optional work
nor unknown reference internals is silently counted as implemented.

## Move convention and expected geometry

The public frame419 shows YAngle−10°. Frame434 shows ZDistance−31 mm after
rotation; frame449 shows final XDistance−2 mm. Other fields reset between these
incremental manipulator changes. The manipulator appears at the cylinder foot,
and its translation axis follows the tilted cylinder. Alloy declares the foot
pivot `(7.5,−5,20)` and ordered **local** steps `[{yDeg:−10},{zMm:−31},{xMm:−2}]`.
This composition is source-informed inference; the public video cannot expose
its internal pivot/frame definition. No exact transform-equivalence claim follows.

The pre-fillet volume is independently `π·7.5²·20 + bridgeArea·4 + 30·(4+24+22)`.
Each body remains one valid connected solid. The fillet must remove positive
volume and preserve the original disk cap; its curved-volume result is inspected
from actual OCCT geometry, not asserted equal to a speculative analytic formula.

Under the declared Move convention, the cutter axis is tilted10° in XZ and
crosses both disk planes fully while staying inside the disk perimeter. At
z20 its center is x`7.5−2/cos10°`; at z0 it is x`7.5−2/cos10°+20·tan10°`, with
y−5. The swept ellipse stays clear of the prongs and fillets. Removed volume is
therefore `π·2.6²·20/cos10°`; expected final volume is the independently inspected
filleted volume minus that analytic amount. The resulting outer bounds remain
32×15×24 mm. Export mesh volume must agree within0.5%, dimensions within0.05 mm,
and every mesh edge must have paired opposing orientation. No slicer or physical
print result is claimed.

## Run and acceptance boundaries

```sh
python3 tests/support_tutorial_api.py \
  --app build-support-integrated/alloy.app/Contents/MacOS/alloy \
  --output build-support-integrated/evidence/support-tutorial
```

The runner writes `support-tutorial-api.json` even on failure, full command
transcript, dimensioned sketch/selection/preview/final captures,
`straw-support-video.alloy` and `straw-support-video-mm.stl`. It checks one upstream
outer-height24→26 edit through Join, Fillet, supported sketch, Move and Cut,
restores it, and separately checks diameter resize. Syntax verification passes;
API, geometry, GPU captures, native interaction and matched visual review remain
pending. Current commands cover the intended route; no new implementation is
requested by this runner. Any real-app failure is a bounded blocker, not grounds
to replace source steps with equivalent primitives or pre-positioned cutters.
