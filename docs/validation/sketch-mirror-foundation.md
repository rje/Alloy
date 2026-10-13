# SKT-053 sketch Mirror foundation candidate — 2026-10-13

Public source `wall-outlet-sketch-20261013` documents six selected upper-cutout curves and a straight construction Mirror Line. Separate public sketch documentation records that Mirror adds symmetry constraints; provenance and the bounded interpretation are stored outside this repository in `../Alloy-reference-records/knowledge/specs/sketch-mirror-public-association-2026-10-13.md`. The source does not specify bidirectional result editing or exact rough coordinates.

Alloy now retains one mirror group containing the straight axis UUID, ordered source/result line or arc UUID pairs, and deduplicated source/result point UUID pairs. PlaneGCS solves each point pair symmetric about the current axis. Reflected arc centers and endpoints determine radius; the signed angular sweep reverses. Copying source constraints or adding duplicate result arc rules would introduce redundant equations. The group adds no independent DOF. Source curves, the axis, and reflected points are editing handles. A result-point gesture maps through the current axis to a source-point solver seed; other free geometry may also move, so the requested coordinate is not an exact driving dimension. New independent driving relations on results refuse, while driven result measurements remain available. This is an explicit first-slice edit policy, not a claim that the public application uses the same policy.

Shared local commands are `sketch.mirrorPreview {sourceEntityIds,axisEntityId}`, `sketch.mirrorCancel`, and `sketch.mirrorAccept`. Preview retains no authored change; Accept records one sketch edit. The API returns the group mapping and solver diagnostics. Native source collection, Mirror Line picker, dialog, and keyboard control belong to SKT-054/#241.

The focused core fixture starts with a deliberately rough asymmetric four-line/two-arc cutout and an offset horizontal construction axis, rather than the lesson's final coordinates. It verifies six result curves, six shared-identity result points, construction flags, a retained source Horizontal constraint without copying, mirrored endpoint/center/radius/sweep, equal DOF, source endpoint/center/radius, result-point and axis edit propagation, missing-axis refusal, and exact undo/reopen. The focused core test and circle-offset/curve regressions passed 3/3. Run `cmake --build build-sketch-mirror --target sketch_mirror_test sketch_circle_offset_test sketch_curves_test` then `ctest --test-dir build-sketch-mirror -R '^(sketch_mirror|sketch_circle_offset|sketch_curves)$' --output-on-failure`.

The real-app API fixture `tests/sketch_mirror_api.py` passed: it authors the six rough curves with shared commands, previews and cancels, previews and accepts, captures the viewport, then finishes the sketch. Run `python3 tests/sketch_mirror_api.py --app build-sketch-mirror/alloy.app/Contents/MacOS/alloy --output build-sketch-mirror/evidence/sketch-mirror`. Inspect `build-sketch-mirror/evidence/sketch-mirror/mirror-preview.png`, `mirror-accepted.png`, `report.json`, and `app.log`. This establishes command/geometry/visual preview evidence, not native collection or the full wall-outlet lesson. The actual upper and lower rough profiles are visible in the 70% viewport capture; the palette reports 13 preview edges, 2 regions and 14 DOF while the authored draft stays unchanged.

A focused review correction cancels pending Mirror preview on either sketch-history Undo or Redo; a following Accept refuses rather than resurrecting the preview. The refreshed real-app API run passed 42 calls, including both stale-preview cases, with the authored draft unchanged. Finer sketch face triangulation and line presentation deflection remove the visibly coarse lower reflected arcs in both refreshed captures; the upper and lower arc outlines are comparably smooth at the same viewport scale. The private validation editor was closed after capture.

## Independent acceptance

Accepted `a5e9117` atop `49eda49a6dc4b65b301bb13cf7932c74a99c16df` on
2026-10-13 after independent implementation, persisted core log, API fixture/
report and refreshed preview/accepted capture review. No extra application run
was required. The three focused core tests passed; the 42-call real-entrypoint
report covers disposable preview, explicit Cancel, both history directions
cancelling pending input, subsequent Accept refusal, fresh preview/Accept and
Finish Sketch. History first cancels the pending preview without traversing
accepted draft history; a later history action can traverse it. This resolves
the stale-snapshot restoration defect identified in the first review.

Source/result points use real PlaneGCS point-pair symmetry about the current
retained axis, independently checked against reflection coordinates after solve.
Result arc radius and angles derive from the source and current axis; mirrored
endpoint/center equations preserve exact geometry without duplicate arc rules.
The focused source/axis/result-point edits, unchanged DOF, construction flags,
retained source relation and exact authored history/reopen support this bounded
association claim. Result-point moves map to a source solver seed and do not
promise exact target coordinates. Independent driving result relations refuse
by schema policy; driven measurements remain permitted. Earlier draft wording
that all result-point moves refuse does not describe this final implementation.

Both refreshed GPU captures were inspected at the same viewport scale. The
previously faceted lower arcs are now comparably smooth to the upper arcs, with
visible construction axis and clear preview/region/DOF labels. This establishes
this fixture's presentation, not universal zoom-level, display-scale or rendering
performance guarantees. The changes tighten sketch fill and wire deflection;
they do not change analytic geometry.

Native window collection, the source's unexplained 12-object count, Mirror Line
role selection, dialog and keyboard behavior remain #241. Six source curves and
six deduplicated point pairs are verified model counts, not proof of the public
selection-counter composition. Nested result-as-source/axis cases, source points
on the axis and independent result driving constraints are outside this slice.
The complete fully defined outlet sketch and physical OS input remain unclaimed.
