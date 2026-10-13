# Circle-prism body selection candidate

DOC-017 / #183 follows a concrete native support-workflow failure: right-click
inside the tall cutter reached the viewport but showed no body menu, and a
Combine tool click failed. Stock picks worked. The exact pre-Move authored
fixture reopened in one private editor with matching 1136×790 window, 884×496
viewport and recorded camera. Recorded cutter-side and projected-center picks
failed; identical primitive-cylinder comparison picked normally. No authored
model change is required.

A pinned OCCT 7.9.3 diagnostic confirms the cause. The actual prism spans
z=20..67 mm, but whole-shape selection's optimized SensitiveCylinder spans
z=-27..20. Face selection creates the correct z=20..67 cylinder and caps.
The [public OCCT source](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/StdSelect/StdSelect_BRepSelectionTool.cxx)
contains that whole-cylinder special case. The external evidence lives at
`demos/2026-10-12/body-picking-diagnostic/` in the reference-records sibling,
including `occt-sensitive-bounds.txt`, reloaded fixture/state/capture and
primitive comparison transcript. This is an observed library selection result,
not a geometry or native-event failure.

Alloy now activates the supported face selection mode for model hit testing;
`pickBody` still maps DetectedInteractive to the same stable body ID. The
inactive whole-shape owner remains available for the existing whole-body blue
selected cue. Explicit face-mode redisplay prevents a hidden/re-shown model
from reactivating the faulty whole-cylinder selector. No kernel mutation,
reference rebinding, proprietary application inspection or ray fallback is used.

## Verification

Application builds; the focused widget check passes (three QtTest results,
zero failures). `circlePrismBodyPicking` checks
an actual generated circle prism's center pick, stable body selection and
Combine tool picking. Independent native popup,
Move activation, whole-body cue and Combine tool selection remain pending.
