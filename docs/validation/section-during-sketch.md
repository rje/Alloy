# Retained section edit during sketch — accepted (#333)

Status: independently accepted and integrated, 2026-10-13. The
enclosure port/card source stages its zero-offset plane and sketch before
adjusting the retained Width section. Alloy's active-sketch command guard
previously refused that analysis-only edit with `session_busy`.

Existing retained analyses can now change visibility or direct numeric fields
while a sketch is active. Both routes use the existing validated
`sectionCandidate`/`commitAnalyses` authority, reject a stale sketch revision,
and advance that revision only after a successful analysis-only commit. The
draft, selection and pending Project/Offset state remain intact. Canceling the
sketch leaves the separately accepted section edit; finishing the sketch keeps
it. At this #333 checkpoint, new analyses and the Section Analysis dialog
required leaving the sketch. The later accepted DOC-029 change adds the bounded
existing-analysis dock path described below; new analysis creation still
requires leaving the sketch.

`tests/section_during_sketch_api.py` passed 33 real-app calls: retained plane
sketch, existing Width edit/visibility, pending Project preservation, four
linked projected edges, finish, invalid edit refusal, and separate sketch
Cancel with the accepted Width edit intact. `section_analysis_controls_test
nativeLifecycle` passed 3/3, including its existing native dialog flow and a
real Browser checkbox click while a sketch is active. The 327 KB actual-app
capture `doc028-native/native-hidden-during-sketch.png`, API ledger and receipt
are under the external enclosure port/card deliverable directory.

The original #333 evidence verifies shared API and Browser visibility only.
The independently accepted DOC-029 change (`eddc86e`) keeps `sketch.edit` active while Browser Edit opens a
separate section sub-edit. Its focused 59-call actual-app continuation verifies
preview/Cancel, one analysis history action on OK, pending Project preservation,
four linked projected edges followed by retained Offset, Finish and later sketch
Cancel. A Qt-delivered native Browser edit, typed Distance, Escape and OK passes
3/3 with readable section capture. Its Project selection begins empty, so this
does not prove preservation of a nonempty selected source set. The complete
enclosure port/card gate is #322.

Astra independently reviewed the final guards, API ledger and native capture.
