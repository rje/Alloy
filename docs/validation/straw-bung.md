# Straw bung: integrated modeling and export checkpoint

2026-10-12. The combined macOS candidate at code commit `11b5c61` passed
`tests/straw_bung_api.py` through the real GPU-backed application. Later
documentation commits do not change that executable. The complete exact-native
tutorial gate #170 remains separate from this API checkpoint.

## Source route and result

The driver follows numeric values observed in the original public video,
source `bung-original-video-20261012`: diameter26/depth3 flange; diameter19/depth8
Join; diameter6.2/depth40 New Body cutter; Y rotation10 degrees followed by local
Z translation-20 mm. The inferred foot pivot is explicitly (0,0,11). The PDF
uses a different cutter length and rotation axis; see the
[source contract](../research/straw-bung-gate.md). These variants are not merged
into an exact PDF claim.

Command-first Circle, origin/support choice and typed-diameter commands create
all three sketches. Join retains the flange identity; the cutter sketch attaches
to the Join-created cap. An authored Move transforms the cutter and Combine Cut
consumes it. No primitive or pre-positioned cutter substitutes for these steps.

The final body is one valid closed solid, 26 x 26 x 11 mm, volume
3523.7964686 mm3. The analytic angled-bore volume agrees with the BRep result.
Undo/redo, one upstream Join-depth edit and exact authored save/reopen pass.
Binary mm STL readback verifies closed oriented edges and consistent facet
normals: 1788 triangles, mesh volume3522.6592657 mm3 (about0.0323% below BRep).
No slicer or physical-print result is claimed for this object.

## Reproduce and inspect

```sh
python3 tests/straw_bung_api.py \
  --app build-tutorial-polish/alloy.app/Contents/MacOS/alloy \
  --output build-tutorial-polish/evidence/straw-bung
```

That directory contains `straw-bung-api.json`, five stage captures,
`straw-bung.alloy` and `straw-bung-mm.stl`. The receipt retains the full command
transcript and GPU inspection. Root inspected the final actual-app capture:
readable shell controls, stepped solid and visible bore. Independent review is
recorded separately; screenshots do not establish all native gestures.

Remaining gate evidence includes the uninterrupted native sequence and its
input-overlay movie, exact source-step/selection ledger and bounded visual
comparison. Separate feature-level native checks cannot be relabeled as that
complete walkthrough. Other-platform runtime qualification remains outstanding.
