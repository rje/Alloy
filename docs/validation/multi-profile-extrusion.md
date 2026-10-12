# Connected multi-profile extrusion checkpoint

2026-10-12: the actual three-region rounded bracket creates one 50-mm solid via
Press Pull. This is the typed-depth A2 route, not full SOL-016 or tutorial closure.

The feature retains distinct profile IDs from one earlier sketch, preserving the
legacy singular profile format. OCCT unites selected coplanar regions before one
prism. The candidate must contain one valid connected face, additive source area,
and one valid closed solid with area×distance volume. Invalid, missing, duplicate
or disconnected input rejects transactionally.

## Evidence

- `build-tutorial/evidence/multiprofile-core-build.txt` and
  `multiprofile-core.txt`: focused core case passed connected three-region union,
  legacy behavior, invalid-selection preservation, history and storage.
- `multiprofile-ui-build.txt` and `presspull-collection-build.txt`: app builds pass.
- `sketch-multiprofile-ledger/report.json`: 123-command actual rounded-bracket
  author/edit/pick/reopen→3-region50-mm extrusion→upstream cap120→140→history and
  exact save/reopen passes. Body/feature/source-profile identities survive edits.
- Native `presspull-native-{preview,committed}.json/.png`: API-prepared retained
  sketch, native Q before selection, three ordinary clicks, type 50 in Distance, Tab,
  OK. Navigation/projection/inspection were API. One valid closed solid, depth 50,
  source area 103711.84601016436 mm², volume 5185592.30050822 mm³; one Extrude feature.
  Saved result: `build-tutorial/evidence/rounded-bracket-50.alloy`.
- Astra independently accepted bounded core, API and native typed-depth behavior.

Public staff guidance supporting pre-distance click collection is external in
`knowledge/specs/press-pull-profile-selection.md`, retrieved 2026-10-12. Shift adds
only; platform toggle is implemented but physical-modifier input remains untested.
The repeated ordinary click is idempotent; exact source behavior is uncertain.

Final preview presentation is selected blue with a screen-scaled arrow and
readable, white-backed `50.00 mm` canvas annotation. The 100-mm projection baseline
avoids low-zoom quantization. `presspull-preview-final.json/.png` records the
focused rebuilt candidate; Cancel preserved the exact saved design. Astra accepted
this readable improvement. Camera/scale and arrow styling differ from the public
reference crop; pixel parity is not claimed. Arrow dragging, operation selection/Cut, five circles,
face-supported sketching and full tutorial-source visual parity remain ahead.
Do not close SOL-016 from this subset. Native tests used the isolated patched Qt
runtime; default deployment qualification remains under FND-015.
