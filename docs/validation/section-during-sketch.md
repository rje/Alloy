# Retained section edit during sketch — candidate (#333)

Status: bounded candidate, 2026-10-13; independent review pending. The
enclosure port/card source stages its zero-offset plane and sketch before
adjusting the retained Width section. Alloy's active-sketch command guard
previously refused that analysis-only edit with `session_busy`.

Existing retained analyses can now change visibility or direct numeric fields
while a sketch is active. Both routes use the existing validated
`sectionCandidate`/`commitAnalyses` authority, reject a stale sketch revision,
and advance that revision only after a successful analysis-only commit. The
draft, selection and pending Project/Offset state remain intact. Canceling the
sketch leaves the separately accepted section edit; finishing the sketch keeps
it. New analyses and the nested Section Analysis dialog still require leaving
the sketch. The preexisting dialog's own commit path remains available outside
sketch edit. Native nested-dialog access is tracked separately by #334.

`tests/section_during_sketch_api.py` passed 33 real-app calls: retained plane
sketch, existing Width edit/visibility, pending Project preservation, four
linked projected edges, finish, invalid edit refusal, and separate sketch
Cancel with the accepted Width edit intact. `section_analysis_controls_test
nativeLifecycle` passed 3/3, including its existing native dialog flow and a
real Browser checkbox click while a sketch is active. The 327 KB actual-app
capture `doc028-native/native-hidden-during-sketch.png`, API ledger and receipt
are under the external enclosure port/card deliverable directory.

This verifies shared API and Browser visibility interaction; it does not claim
that the numeric Width field is natively editable in a nested dialog while a
sketch remains active. The complete enclosure port/card gate is #322.
