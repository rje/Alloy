# Independent successive Join and solid-edge Fillet review

2026-10-12. Bounded review of #176/#182 on combined candidate `aa4e81f`; full support tutorial #171 remains separate. Reviewed the public-source contracts, retained Join result ownership, extrusion edge provenance/history transport, constant-radius OCCT evaluation, exact surviving cap support, and preview/commit control paths. Existing focused core and real-app API receipts were reused; no broad geometry suite repeated.

The source scope is the original-video variant: source-order heights 20/4/24/22 mm, then two localized outer-prong edge seeds and a 1 mm fillet. Seed coordinates are geometric inference from public imagery, not reference topology identifiers. The PDF six-edge/0.5 mm variant, radius dragging, and Tangent Chain Off remain unclaimed/deferred.

## Native evidence

Opened the saved API-authored `successive-join/support-successive.alloy` fixture through the shared API. Native F opened Fillet with radius zero and disabled OK. Two physical viewport edge clicks selected the intended side corner and top rim. Typed 1 mm plus Tab produced a valid preview with readable blue seed cues, neutral solid and legible controls. Native Cancel restored exact authored data. Reopening via toolbar, repeating the physical picks/radius and pressing native OK committed a valid single body. Inspection independently confirmed the original disk cap at z=20 retained its exact face ID. Native timeline double-click reopened the feature with the two seeds and 1 mm restored. Native Undo restored the exact source authored data.

Native history edit of the earlier two-profile, 4 mm Join exposed a defect: ownership listed two results, but the preview showed downstream final prongs and only one evaluated result. The preview used full-history evaluation instead of the edited feature prefix. Labels and target control were readable. Cancel preserved the exact source.

A second defect was reproducible with accessible and physical Fillet Cancel: subsequent F did nothing; app.state focus remained a QPushButton, with viewport false. The initial F worked and toolbar reopen worked. These were reported for bounded correction rather than accepted as generic automation limitations.

Evidence is under `build-tutorial-polish/evidence/independent-support-solid/`: native-preview, native-cancel, native-commit, native-undo, native-join-preview, native-join-cancel and post-physical-cancel JSON/PNG files. API requests in this supplement only loaded the fixture, inspected state/projected edges, captured images, and closed the saved editor; modeling and history actions above were native. The editor was closed after restoring the unchanged saved source, before rebuilding either correction.

## Corrections and final verdict

Both native defects were corrected in the integrated follow-up. The Join preview validates the complete candidate, then evaluates the display prefix through the edited feature and stages the preceding input bodies. Native timeline editing now visibly shows the disk with its 4 mm bridge plus the detached 4 mm middle strip, with two actual result bodies and count two; downstream prongs are absent. Fillet finish restores viewport focus immediately and after dock layout under a same-tab/no-new-session guard. Separate native physical Cancel then F, and native Commit then F, now reopen correctly. Evidence: `fixed-native-join-preview`, `fixed-cancel-reopen`, `fixed-commit-reopen`.

Independent public-frame comparison also corrected the prior inferred second seed: frames 07/10 show the second pointer on the rear vertical silhouette, distinct from the diagonal top rim; frame 14 shows a planar cap with both outside corners rounded. Both seeds are vertical side corners at x=32, y=-10 and y=0, spanning z=0..24 in the fixture. Coordinates remain geometric inference; no reference topology IDs are claimed. The earlier end-rim interpretation and initial native captures are retained as superseded evidence, not a source match.

On the unchanged geometry implementation, physically selected both corrected vertical edges, typed 1 mm, inspected the preview and committed through native OK. `corrected-seeds-preview.json` records the two stable side-corner IDs; the corresponding PNG and `corrected-seeds-commit.png` show the planar outer-prong cap with its two rounded outer corners. This agrees with the bounded public silhouette; camera, scale, platform and theme are unmatched, so pixel-level equivalence is unknown. The committed solid is valid, and its original z=20 disk cap retains the exact face ID. Saved through the shared API as `native-support-filleted.alloy`, then closed the only editor.

Independent acceptance: #176 and #182 pass their bounded corrected contracts, combining reviewed source/core/API evidence with the native supplement above. Corrected-seed core coverage checks upstream height 26, radius 0.5, original disk-cap attachment, atomic invalid-radius refusal, history and reopen; those geometry cases were not redundantly rerun here. The full source support workflow remains #171. No six-edge PDF, radius-drag or general topology-naming acceptance is implied.

