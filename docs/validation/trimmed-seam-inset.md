# Retained trimmed-seam inset

SKT-076 / #346, 2026-10-14. The public workflow contract is indexed by
`knowledge/specs/trimmed-seam-offset-integration-2026-10-14.md` under
`../Alloy-reference-records/`; the detailed source record and image provenance
stay outside this repository. This is a bounded Alloy inward inset for the
saved, split enclosure cap, not a general Offset replacement.

An explicit `kind: "planarInsetV1"` loop offset retains its own UUID, exact
support producer/body/face, a source-loop identity from the sorted stable
boundary-edge IDs, and a negative signed distance. Its evaluation uses the
accepted analytic planar boundary-neighborhood kernel and yields one **interior**
profile, rather than a band between two wires. The profile UUID derives from
the offset UUID. Each generated boundary edge has a stable source-ancestry ID;
adjacent generated-edge pairs give stable endpoint contacts. These identities
reach the existing Extrude Join and selected end-rim Fillet lineage. No BRep or
display tessellation is authored. An absent `kind` retains the earlier
eight-edge rounded annulus evaluator and saved-document meaning.

The shared Offset command recognizes a click on the exact face-support edge
when a fresh face sketch still has its auto-seeded projections. Chain Selection
previews the complete boundary; the new variant defaults to −1.25 mm for this
cap. The palette identifies the interior inset and the viewport highlights its
U boundary and interior. An outward/Flip request refuses and invalidates the
previous valid pending preview, so OK cannot commit stale geometry. Escape
clears preview; Return accepts the valid signed distance. Existing explicit
empty-projection face sketches retain the same boundary admission. Other
projected chains continue through their prior Offset path.

The focused core fixture uses the saved actual 18-edge split cap. The −1.25 mm
inset forms one 17-edge valid interior profile of 233.8576963 mm² with 17
unique edge IDs and 17 contacts. Join 2 mm and a selected straight end-rim
Fillet R0.5 mm succeed. Source stock width 64→65 mm changes the inset area to
236.289 mm² while preserving profile/edge IDs and downstream references;
undo/redo and save/reopen preserve the authored graph. A collapsed −100 mm
inset and missing support refuse atomically. The collapse Boolean took roughly
30 seconds on this fixture; REL-016 tracks responsiveness separately.

The focused native Qt fixture performs an actual hit-owned face-edge click,
clicks and types in the actual distance field, Escape-cancels, reselects and
accepts with Return under natural focus. Initial window activation is harness
setup. Document/support setup, the invalid Flip check, sketch Finish/save, and
the rear-isometric camera capture use the shared API and are not OS input claims. The top-view preview
visibly distinguishes the blue U inset against the gray split cap; the
camera-only rear-isometric view places that contour against the enclosure ports.
The other split half remains visible in the top view, so the gray center is not
an empty-cavity depiction. Styling is Alloy policy without an exact matched
reference-color claim.

Evidence: the manual focused core log at
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/retained-inset-core-exact-values.log`
with its source/binary/fixture hashes and exact operation values in the adjacent
`retained-inset-core-exact-values.metadata.json`,
and native log at
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/retained-inset-native-09.log`.
The saved model and captures are at
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/retained-inset-native-09/`.
This verifies this supported cap and its downstream retained lineage; arbitrary
multiwire faces, outward side and a complete enclosure tutorial are outside
the accepted slice.
