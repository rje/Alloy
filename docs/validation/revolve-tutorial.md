# Illustrated Revolve workflow gate

TUT-008 / #217 candidate gate, 2026-10-13. Runner:
`tests/revolve_tutorial_api.py`. Accepted controls checkpoint `8529a0a` was merged
before execution. The integrated `build-tutorial-polish` application supplied the
real entrypoint; no product implementation changed for this gate.

## Source-step contract

Source ID `tutorial.next-revolve-feature-checkpoint`, public tutorial steps 39–50,
retrieved 2026-10-13. Source ledger, images and URL provenance remain in external
reference records. The lesson explicitly asks for a rough one-sided outline and
leaves dimensions and constraints to the learner. Ø100/Ø165 are observed labels;
70° is an observed manipulator result. All exact coordinates below are Alloy choices.

| Source step | Runner action | Evidence boundary |
| --- | --- | --- |
| 39 | Blank design, Front/XZ sketch | Shared authoritative API; no physical OS claim |
| 40 | Line tool, origin-up axis, length 55 | Length and explicit origin/vertical constraints are Alloy choices |
| 41–42 | Select axis, real Centerline conversion | Distinct boundary-capable intent; #215 native conversion/D child evidence reused |
| 43 | All eight one-sided closed outline edges | Rough source-inspired silhouette; no exact reference geometry claim |
| 44–45 | D command route, axis/vertical-edge picks, placement and Ø100/Ø165 | Real diameter, mirrored full span, explicit orientation relations and truthful remaining DOF |
| 46–47 | Real Revolve dialog; inspect automatic sole profile and Centerline axis | Must use accepted #216 controls; manual selectors would be an adaptation |
| 48 | Partial and coordinate-derived angular drag command | Same command as native pointer; 70° is an Alloy test choice, not numeric entry substituted for drag |
| 49 | Full/360°, preview then OK/shared commit, New Body | Actual body/history, not a rendered shell |
| 50 | Change top/right/isometric named views | Existing view API rotates orientation; no orbit gesture/camera parity claim |
| Alloy extension | Save editable design and strictly inspect millimeter binary STL | Additional interchange qualification, not a source instruction |
| Alloy extension | Upstream diameter edit, analytic solved-polygon volume, undo/redo and exact reopen | Additional retained parametric/history/storage checks |

## Independently authored fixture and oracle

Axis `(0,0)`→`(0,55)`, millimeters. Closed polygon:
`(50,0),(67.5,0),(72.5,7),(72.5,22),(82.5,22),(82.5,32),(69,50),(50,50)`.
Only applicable Horizontal/Vertical relations, fixed axis origin, axis length 55,
and full diameter 100/165 are retained. Profile points are not all frozen; remaining
DOF must be reported honestly. The planar area is 1266 mm²; independent polygon
first moment gives Full volume `(479338/3)π = 501961.5797954747 mm³` and Partial70
is 7/36 of that. The expected world outer diameter is 165 mm and height 50 mm.
These are Alloy fixture expectations, not reference product dimensions.

The shared-API movie must begin blank and include all construction stages, captions
and actual Partial drag. It remains external, with event/timing sidecars, editable
`.alloy`, strict serialized-STL readback and captures of profile, dimensions,
automatic choices, Partial, Full preview and final body. No EDITED banner or
unrecorded replacement model is used. Source and Alloy images are unmatched;
workflow/geometry/history and visual readability are assessed separately.

## Prepared invocation

```sh
python3 tests/revolve_tutorial_api.py --app /absolute/path/to/alloy \
  --output /absolute/path/to/evidence \
  --record-video /external/path/to/revolve-tutorial.mp4
```

## Actual results and evidence

The complete shared-API movie starts blank, records all axis/profile creation,
real Centerline and D pair/place diameter operations, automatic selections,
coordinate-derived angular drag and Full commit, named-view exploration, then
explicitly labeled save/STL extension. It contains 865 frames at 10 fps,
1120×902 pixels, 86.5 seconds. Recorder reports zero capture failures; 665 repeated
frames preserve wall-clock timing through slower GPU readbacks. API command and
step captions are visible, with no EDITED banner. Encoded profile-creation and
Partial-drag frames were decoded and inspected separately from PNG captures.

- Full CAD volume: **501961.5797954747 mm³**, matching the independent oracle.
- Coordinate-derived Partial angle: **70°**; volume **97603.64051578674 mm³**.
- Sketch: **8 remaining DOF**, zero residual, no redundant/conflicting relations.
  Applicable profile Horizontal/Vertical relations and axis origin/vertical/55
  intent are retained. Profile points are not frozen.
- Strict binary STL: **2392 facets**, **119684 bytes**, one closed consistently
  oriented component, finite/unit/winding-aligned normals, exact bounds
  `[-82.5,-82.5,0]`→`[82.5,82.5,50]` mm. Signed volume **501761.5971722183 mm³**,
  about **0.040%** below CAD volume at 0.05 mm / 8° meshing. Counts/bytes agree
  with export metadata and authored data is unchanged. Reader was not weakened.
- Upstream Ø100→Ø110 edit yields **419494.77263874287 mm³**, matching independent
  Pappus volume from the solved polygon. Sketch points/lines, profile, Revolve
  feature and body IDs survive. Document undo/redo returns exact authored states.

Evidence directory: `build-tutorial-polish/evidence/revolve-final` in main. It
contains `revolve-tutorial-api.json`, `app.log`, `runner.log`, eight named PNGs,
`stepped-revolve.alloy`, `stepped-revolve-edited.alloy`, `stepped-revolve-mm.stl`,
and `reopen-recovery/app.log`. The external movie and event/metadata sidecars are
`knowledge/tutorials/revolve-next-slice/revolve-tutorial-api.mp4` plus `.jsonl`
and `.json`, under the external reference records root.

Canonical external copies are registered in
`demos/2026-10-13/revolve/revolve-complete.mp4` with sidecars and
`deliverables/revolve-2026-10-13` with original/edited models, STL, full report and
`summary.json`. Copies were compared by SHA-256; original evidence paths remain.
The movie summary links original recording provenance and deliverables.

The first attempt stopped at the export extension because the runner omitted a
required body selector; that incomplete attempt is preserved separately. The
corrected complete source/movie/export/history run then reached final reopen,
where omitting `discard:true` after saving the edited variant and undoing triggered
the expected unsaved-changes refusal. Both runner arguments are corrected. Parent
approved a bounded reopen recovery instead of a third full replay: a separate real
app opened original, edited and original files, compared each exact authored
object to its saved JSON, and checked retained body IDs and respective analytic
volumes. It exited 0. The report preserves original transcript and separate recovery
transcript as `api-pass-with-reopen-recovery`; it is not an uninterrupted success
claim. No private editor remains open.

## Visual and compatibility assessment

| Evidence | Assessment within this slice |
| --- | --- |
| Profile/diameters | Complete one-sided stepped boundary, distinct Centerline and mirrored full Ø100/Ø165 spans are visible; all creation vertices and witness/label extents remain onscreen |
| Auto choices / Partial | Real dock shows one Profile and Axis, Partial/70°, New Body; complete angular ring and partial body remain in frame |
| Full / final | Full preview and hollow stepped body are fully framed and readable; source sketch hides on commit |
| Source mapping | Public actions 39–49 are matched by authoritative command paths; source exact geometry is unspecified; step50 uses explicitly qualified named-view exploration |
| Visual parity | Unknown for exact camera, spacing, colors and pixel parity: source and Alloy viewports/selection states are unmatched; palette fill, ring styling and label precision are Alloy choices |
| Input evidence | This movie uses shared API command paths; accepted #215 and #216 native Qt child proofs are reused for conversion, D/typed editor and actual angular mouse input; no physical OS input claim |

Astra independently reviewed source mapping, ten representative movie frames
including blank/creation/dimensions/controls/final, saved original/edited retained
IDs and all three exact reopen responses. The unchanged strict STL reader was
independently rerun and passed. Review reported no bounded blocker. Final issue
acceptance/integration remain parent-owned; broader Revolve modes remain #49.
