# SKT-110: Two Distance sketch Chamfer

Accepted 2026-10-18 for a connected corner of authored straight lines. The
dated public workflow contract and source URLs remain outside the repository
at `../Alloy-reference-records/knowledge/specs/skt-096-sketch-chamfer-2026-10-18.md`.
Public help documents two independent distances and says moving the first
handle initially adjusts both. It does not fix native edge order or numeric
defaults. Alloy assigns Distance 1 to `sourceEntityIds[0]` and Distance 2 to
`sourceEntityIds[1]`; two-line picks keep pick order, while vertex picks use
authored entity order. That is a stable Alloy policy, not a reference parity
claim.

Sketch > Modify > Chamfer > Two Distance and Sketch Palette Feature Options
switching between Equal Distance and Two Distance use the shared retained
command. Both distances have palette fields, driving canvas annotations and
side-specific drag handles. The first handle initially couples the values;
once the second side has an independent value, each handle edits its own side.
The original lines/dimensions remain authored, and the evaluated profile
derives both endpoints and its diagonal from stable source IDs.

Evidence from Release `build-preview-release`, binary SHA-256
`0a102480b074d7b6c1380e95ce9a755820079731911cac7d28b49b6bd9d0d61b`:

- Focused core/native Qt tests passed 2/2 in 19.71 s. Native input exercised
  menu selection, Feature Options switching, typed 10/20 mm preview, both
  driving labels, first and second handle paths, independent annotation edits,
  transactional invalid distance, source edit, extrusion, Undo/Redo and native
  reopen. Actual captures, framebuffer, model and state are at
  `../Alloy-reference-records/validation/2026-10-18-sketch-two-distance-chamfer/native/`.
  The later native pointer drag yielded a measured 13.8177998472 mm first
  setback, which its area/volume assertions use.
- Independent [real-app runner](../../tests/sketch_two_distance_chamfer_api.py)
  passed 33 commands. The initial 150×75 mm rectangle with 10/20 mm setbacks
  made an 11,150 mm² closed region. Editing Distance 2 to 18 mm left Distance 1
  at 10 mm; editing Distance 1 to 12 mm and source dimensions to 160×80 mm
  retained Chamfer/contact/diagonal and body IDs. The final area was
  12,692 mm² and the 5 mm solid volume 63,460 mm³. An out-of-span 160 mm
  second setback refused without changing the draft; Undo/Redo and native
  save/reopen reproduced the authored document and solid. Model, captures and
  transcript are at
  `../Alloy-reference-records/validation/skt110-two-distance-candidate1/`.
- Inspected GPU captures show the shortened profile, two distinct distances,
  readable controls and retained 160×80 mm source dimensions. The app reported
  Apple M4 hardware acceleration through OCCT OpenGL 4.1 Metal with a 2×
  framebuffer.

The accepted route reuses the previously checked extrusion/STL pipeline; no
second STL is claimed here. Distance And Angle remains SKT-111/#535. Physical
hand input, Windows/Linux runtime and exact native reference styling remain
unqualified.
