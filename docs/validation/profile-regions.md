# Nested profile regions and diagnostics — #165 candidate

The shared sketch evaluator exposes independently selectable bounded cells. A
40 × 25 mm rectangle surrounding radius5 and radius2 circles has three cells:
1000−25π,21π and4π mm². Selecting the outer annulus retains its inner void;
selecting the central island produces a separate body through ordinary Extrude.
Construction remains excluded; Centerline remains boundary capable. Public
contract IDs: CAD-PROFILES-01, CAD-PROFILE-LOOPS-01,
CAD-SKETCH-PROFILES-01, CAD-MULTI-CIRCLES-01 and OCCT-AREA-01. Detailed
provenance and unresolved source policies remain outside this checkout.

`command.preview.sketchGeometry.profileDiagnostics` and the active sketch session
report unused ordinary entities, unused Centerlines, ignored split portions,
coincident straight source sets, endpoint identities and measured gaps.
`sketch.profileStatus` presents region/void counts and useful warnings in the
existing palette. Discovery does not author corrections. The existing precise
single-cycle self-intersection diagnostic is preserved; valid arrangements,
zero-gap contacts and bounded cells from open chains remain supported.

An actual OCCT merge of distinct positively separated endpoints without authored
connectivity is refused as `undeclared_endpoint_merge`. Its payload records source
and point IDs, measured solved gap, kernel vertex tolerance and `closureApplied:
false`. Shared IDs, transitive Coincident and declared Tangent contacts remain
accepted. Roundoff discrimination is an internal Alloy policy, not a reference
closure tolerance. This is a bounded actual-merge check, not a universal nearby
endpoint classifier or repair interface.

## Reproduction and evidence

Candidate baseline: f39a2ba. Build directory `build-profile-regions` uses private
Qt6.11.2 and local OCCT on macOS arm64. Build app and tests normally, then:

```sh
ctest --test-dir build-profile-regions -R '^(sketch_nested_regions|sketch_regions|overlap_regions|sketch_centerline)$' --output-on-failure
python3 tests/nested_regions_api.py --app build-profile-regions/alloy.app/Contents/MacOS/alloy --output build-profile-regions/evidence/api
ALLOY_NESTED_REGIONS_EVIDENCE="$PWD/build-profile-regions/evidence/native" build-profile-regions/nested_regions_controls_test
```

Core4/4 passed. The new fixture checks exact nested areas, void classification,
source provenance, driving width/radius edits with stable profile IDs, exact
undo/redo and save/reopen, separate real extrusion volumes, open/duplicate status,
precise bow-tie refusal and the observed10nm accidental kernel closure. Controls
include transitive Coincident correction, zero-gap separate endpoints, and a
100 mm² valid open-chain cell with two excluded tails. Existing overlap/tangent
and Centerline gates remain passing.

The real-app API passed preview/Cancel, two accepted solids, upstream40→50 mm and
radius5→6 mm edits, stable IDs, units conversion, undo/storage and atomic invalid
Extrude refusal. `evidence/api/report.json`, transcript and app.log retain details;
PNG captures include the readable measured-gap warning.

Native QtTest passed3/3 in5.25s. API seeded the authored sketch; actual toolbar,
viewport annulus/island picks, distance arrows, Cancel/OK and Browser row/Space
visibility were exercised. It awaited evaluated visibility before beginning the
next command (Browser changes are deferred). Captures and state JSON under
`evidence/native` show preview geometry, retained sketch, separate solids and
readable open/duplicate warnings. These are injected Qt events, not physical OS
keyboard delivery or native sketch construction evidence. Accepted island blue
is ordinary selection styling; the surrounding body retains normal gray.

Visual inspection found readable controls/status and genuine GPU geometry. No
matched public screenshot exists for this bounded fixture; exact visual parity,
general touching/tangent policies and arbitrary malformed component repair remain
unknown. No automatic healing or relink UI is claimed. Independent candidate
review is pending; this receipt does not close the issue.
