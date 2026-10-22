# Implementation backlog

Prepared 2026-10-11 from public references and the accepted Alloy requirements.
[issues.json](issues.json) is the machine-readable source for GitHub publication;
[sources.json](sources.json) records the 162 dated reference entries cited by its
issues. Stable keys survive GitHub issue numbering. Published numbers and links
are in [github.md](github.md) and [github.json](github.json); do not replace
dependency keys with issue numbers.

The current local backlog contains **723 issues**: **571 implementation, 25 research,
6 research/design, 22 epics, and 99 validation gates**. Counts describe decomposition, not coverage.
Use the live issue state for completion and readiness.
[Connected-Line Surface Revolve](../validation/surface-revolve-chain.md) accepts
the joined-Line first-v1 minimum under SRF-037/#660. Canvas selection and angle
drag remain SRF-036/#659; later Line/Arc chains are SRF-055/#715.
[Two-sheet Surface Stitch](../validation/surface-stitch.md) accepts the bounded
open-shell core/API under SRF-056/#716; native actual-hit and sub-tolerance
preview appearance remain SRF-057/#717. [The fill-striping fix](../validation/surface-stitch-visual.md)
is accepted under SRF-058/#718; seam legibility remains SRF-059/#719.
SKT-097/#502 accepts a bounded retained rectangular sketch-circle pattern and
shared ownership foundation; SKT-177/#700 accepts circular Full/Angle patterns.
Familiar rectangular canvas picks/manipulators are accepted under SKT-178/#701;
circular picks/manipulators are accepted under SKT-182/#705. Nonorthogonal
rectangular axes are accepted under SKT-183/#706 after public tutorial evidence.
Both pattern modes are required for proposed-v1 sketch patterns, with the public
contract and engineering note outside the repository. SKT-098/#503 owns atomic
authored Move and SKT-184/#707 owns retained editable Move. [Copy/Paste](../validation/sketch-copy.md)
accepts SKT-179/#702. The [general Move/Copy Create Copy route](../validation/sketch-create-copy.md)
accepts SKT-185/#708; its sketch-specific retained-history research SKT-186/#709
closed with an evidence-bounded unknown. [Retained planar Rotate](../validation/sketch-rotate.md)
accepts SKT-180/#703; [bounded active-sketch Scale](../validation/sketch-scale.md)
accepts SKT-181/#704.
[Normal 3D construction Line](../validation/sketch-normal-axis.md) accepts
SKT-187/#710 as the Rotate prerequisite; [source-size composition](../validation/sketch-rotate-source-size.md)
accepts SKT-188/#711.
[Copy/Create Copy beside retained Rotate](../validation/sketch-copy-rotate-coexistence.md)
accepts SKT-189/#712 after the source-size validation revealed the isolated
candidate refusal. SKT-186/#709's external follow-up record gives the evidence
trigger for reopening the unknown sketch-specific Copy history question.
Independent cumulative review accepts the bounded connected four-Line XY
`sketch.transform` minimum. [SKT-190/#713](../validation/sketch-dense-dimensions.md)
accepts directly editable, separated labels in the dense multi-profile fixture;
[SKT-191/#714](../validation/sketch-dimension-overflow.md) accepts a bounded
two-margin overflow layout at the supported minimum window size.
The [atomic Move receipt](../validation/sketch-move.md) accepts SKT-098/#503,
and [Retained Edit Move](../validation/sketch-retained-move.md) accepts SKT-184/#707.
The public transform contract is outside the repository.
SKT-172/#695 accepts the [bounded illustrated-bracket visual checkpoint](../validation/bracket-visual.md). SKT-173/#696 accepts the [active-sketch canvas menu component](../validation/canvas-marking-menu.md); SKT-175/#698 accepts original icons and keyboard access, and SKT-176/#699 accepts the centered content-sized list. Native input remains SKT-171/#694, broader contexts SKT-174/#697. TUT-003/#155 still depends on its native gate.
REL-052/#587 accepts the [native Save As Mesh sheet keyboard and return slice](../validation/mesh-export-keyboard.md);
REL-053/#607 retains actual OS chooser Cancel/Save and independent STL readback,
and blocks the cumulative REL-012 gate. A [bounded native chooser diagnosis](../validation/native-mesh-chooser-investigation.md) reached a real NSSavePanel but not usable key-window return after Cancel; both draft fixes were reverted and the issue is Todo.
FND-028/#589 accepts the [unlocked macOS native first-input prerequisite](../validation/ordinary-bracket-modal-readiness.md):
AppKit key-window and Qt focus agree, an actual-hit click followed by C works,
and a native window capture is inspected. REL-052/#587 accepts the export-sheet
keyboard/return repair; REL-053/#607 retains the OS chooser workflow.
NAV-005/#411 accepts the [cumulative visible-navigation workflow](../validation/navigation-cumulative.md):
the shared controls, placed-assembly-face Look At, camera invariants, save/reopen
and GPU captures pass in a real-app API run. Physical mouse/trackpad and native
input stay in NAV-008/#414 and NAV-009/#428; packaged platform runtime stays in
GATE-005/#134.
TUT-016/#299 accepts the [cumulative adapted enclosure tutorial](../validation/enclosure-cumulative-gate.md):
fresh blank-document creation, exact saved-model rim continuation, two checked
printable halves, nominal connector clearance and independent Astra review.
Exact creator-board fit, physical snaps and native gesture parity remain open
product quality concerns rather than evidence claimed by this gate.
SKT-164/#610 accepts [retained finite line-intersection inference](../validation/sketch-line-intersection.md) through a sketch-to-Extrude workflow, source edits, native Qt cue/badge input and independent Astra review. SKT-021/#162 retains other inference and numeric behavior.
SKT-165/#611 has a committed direct Toggle Driven/Driving candidate with independent core/API/sketch-to-Extrude/GPU review, but remains Todo until its native actual-hit sequence can run on an unlocked Mac; see [the candidate receipt](../validation/sketch-dimension-driving.md). SKT-166/#612 retains a separate recoverable relation-add conflict that converts an existing dimension; this is distinct from the accepted SKT-089 new-dimension offer.
SKT-167/#613 has an independently reviewed point-edge-specific redundant Driven offer through core/API/GPU, but remains Todo for the strict native actual-hit/focus gate while the Mac console is locked; see [its candidate receipt](../validation/point-edge-driven-offer.md). SKT-034/#205 already accepts the core two-offset point-to-edge workflow; SKT-016/#144 retains the wider dimension family. The dated public audit is outside the repository.
SKT-100/#519 has a direct same-line H/V conflict candidate with solver-certified accepted relation, atomic API rejection, repair and independently reviewed 63-call real-app/GPU evidence; native input remains pending. SKT-168/#614 separately retains rejected driving-dimension-edit diagnosis and recovery.
GATE-040/#85, XCH-008/#75 and PRT-002/#76 now require only first-v1 supported import formats plus Core 3MF export; later 3MF import remains XCH-005/#73 without transitively blocking the initial package gates. REL-054/#615 is the stable v1 exit after REL-049's post-packaging intrinsic-cost audit, separate from earlier preview releases. No CI trigger follows from this planning change.
SOL-078/#608 accepts [retained Simple Hole](../validation/simple-hole.md) on a supported sketch Point with explicit target and Distance/All, including native controls, upstream edit, downstream Chamfer and independent STL. SOL-079/#609 accepts [retained Counterbore](../validation/counterbore-hole.md) with editable recess diameter/depth, native controls, source following, downstream Chamfer and independent STL; wider placements and thread subtypes stay SOL-007/#52.
SOL-062/#507 accepts [Cone and Torus New Body primitives](../validation/cone-torus.md)
through real-app API/GPU, retained editing and independent STL checks.
SOL-077/#606 accepts native Qt actual-hit Create/Edit controls and readable
1×/2× panels after the FND-028 window prerequisite; physical/platform input
remains separate.
DOC-034/#496 accepts the [active design-unit workflow](../validation/display-units.md)
through sketch, Extrude and independently read mesh bounds. DOC-038/#597 accepts
[selected-solid Properties](../validation/body-properties.md) with active-unit
dimensions, area, volume and exact copied-instance identity.
SOL-067/#512 accepts [retained planar Offset Face](../validation/face-offset.md)
with exact moved support, a downstream Hole, native signed preview/edit and
independent STL bounds. SOL-072/#598 accepts in-canvas signed normal dragging;
SOL-073/#599 accepts qualified straight-edge and planar-boundary continuation
through downstream Fillet and linked Project. SOL-074/#600 accepts a full
circular cap rim through linked Project and Fillet; SOL-076/#603 qualifies its
native actual-hit selection and R1 preview. Other changed curves remain
SOL-075/#602. GATE-120/#601 accepts a valid-solid
deleted-edge refusal; split and merge remain source-reviewed without a runtime fixture.
GEO-006/#309 accepts the circular-seam provenance repair with both retained brackets reopening. REL-051/#586 now accepts the native sketch→Extrude→Undo→Browser popup prefix plus separate API STL/readback; see [its receipt](../validation/ordinary-bracket-input-prefix.md). REL-012/#415 remains Todo/blocked on REL-053/#607 for the actual Save As Mesh chooser and same-fixture mesh readback. The earlier locked-console AppKit activation prerequisite is accepted under FND-028/#589; the export-sheet keyboard and native return are accepted under REL-052/#587. External `../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/README.md` retains both failure and accepted evidence.
SKT-115–121 and SKT-123–124/#540–546, #548–549 are accepted analytic constraint pairs and endpoint contacts; SKT-129/#554 accepts distinct-ID arc/line endpoint Tangent and SKT-022/#163 is closed for its declared v1 analytic scope. SKT-122/#547 and SKT-130–132/#555–557 retain later multi-pair Symmetry, spline constraints and branch-control research.
SKT-125/#550 accepts per-entity sketch status and Find Under-Constrained; SKT-126/#551 accepts independent sketch visibility and hidden-target picking; SKT-127/#552 accepts contextual point relations and Delete Coincident. SKT-128/#553 accepts a bounded glyph/edit inventory, including a readable Vertical mark; SKT-133/#558 accepts point/line Fix/UnFix; SKT-134/#559 accepts Center Circle Fix/UnFix; SKT-135/#560 accepts ordinary finite Arc Fix/UnFix; SKT-139/#564 accepts Two-Point Circle Fix; SKT-140/#565 qualifies the other v1 authored circle modes. SKT-023/#164 is accepted for its declared v1 analytic scope; later geometry/ownership work remains SKT-136–138/#561–563.
SKT-114/#538 retains crossing-line Chamfer kept-side selection after the
accepted connected-corner modes. REL-050/#539 retains physical/native failure
message and recovery checks after the [packaged REL-015 slice](../validation/actionable-failures-packaged.md).

The 2026-10-17 v1 inventory reconciliation added 20 bounded implementation
children, [#496–#515](github.md), for proposed tools previously owned only by
broad capability families or already closed partial slices. All are Todo on the
project board. Their public workflow contracts and real-object fixtures remain
to be researched and implemented; REL-030/#469 still owns final v1 scope review.
The subsequent ownership audit added six narrow Todo implementation issues
[#518–#523](github.md) for sketch deletion/diagnosis/intersection, interference
inspection, OBJ output and active-preview visibility responsiveness. XCH-010/#522
now accepts the bounded OBJ command and independent Blender readback; XCH-012/#604
retains native chooser qualification and XCH-013/#605 retains broader options. Other
broader parent issues must be split when pulled; see
[the ownership audit](../v1/ownership-audit.md).
The [partial-row disposition](../v1/partial-row-disposition.md) names the
working minimum v1 variants; SOL-071/#566 now accepts ordinary Combine naming
and invalid-result recovery with native/API evidence.
The user's first-v1 scope decision requires 3MF export and modeled printable
threads. XCH-011/#524 has a bounded [Core 3MF export receipt](../validation/3mf-export.md)
with actual app/native/independent consumer evidence; XCH-005 retains import
and extensions. SOL-059/#504 has a bounded
[modeled external Thread receipt](../validation/modeled-external-thread.md).
SKT-102/#525 accepts authored spline, ellipse and conic deletion after the
[ordinary line/circle/arc/point SKT-099 slice](../validation/sketch-geometry-delete.md).
SKT-141/#567 accepts whole-owner Slot, Arc Slot, Polygon and Chamfer deletion
after core, API, native Qt and independent review. SKT-142/#568 accepts
line/arc Mirror and open-chain Offset owner deletion after the same bounded
checks. Linked/projected line/circle/point Delete is accepted under SKT-143/#569
with native/API, exact history/reopen and GPU evidence; its independent Astra
review found one palette overlap, corrected in final native captures. Broad
source repair remains SKT-026/#160. Overlapping projected/authored selection
is accepted for straight and circular pairs under
[SKT-155](../validation/sketch-overlap-selection.md). Hover preview is accepted under [SKT-157](../validation/sketch-select-other-hover.md); real Depth/Parents hierarchy is blocked Todo SKT-159/#585 on DOC-006/#18; multi-boundary face-projection selected-edge Delete is accepted under
[SKT-156](../validation/sketch-face-projection-delete.md). SKT-144–147/#570–573
now have independently accepted bounded Offset owner deletion, including the
single-owner support-face loop under SKT-145/#571. Its planar-inset actual-app variant is now accepted under SKT-152/#578
with a retained 18-edge enclosure fixture, native/API input, exact history and GPU captures. Two classic annulus owners in one face sketch are accepted under [SKT-153](../validation/sketch-multiple-loop-offset-delete.md), with both schemas passing core. Multiple-owner planar-inset real-app/native qualification is accepted under [SKT-158](../validation/sketch-multiple-planar-inset-delete.md). [HIS-009](../validation/failed-edit-repair.md) accepts transient missing-support inspection, and [HIS-010](../validation/failed-boundary-inspection.md) accepts its observed changed-boundary variant. [SKT-154](../validation/sketch-loop-offset-recovery.md) gives bounded recovery guidance for both Delete refusals. [SKT-160](../validation/sketch-plane-redefinition.md) accepts ordinary explicit parallel plane reassignment; [SKT-161](../validation/generated-loop-repair-policy.md) defines generated-boundary identity. [SKT-162](../validation/generated-loop-replacement.md) accepts distinct-face generated owner recreation; [SKT-163](../validation/generated-loop-same-face.md) accepts the same-face changed-valid route. Classic/multiple owners, projected support, arbitrary frames and old-output consumer repair remain under broad SKT-025/#159.
SKT-148/#574 now accepts
fit-spline Mirror Delete through core, real-app API, native Qt and independent
review. SKT-149/#575 accepts explicit fit-spline Offset Break Link, retaining both curves and converting the result to independently editable geometry; see [its receipt](../validation/sketch-fit-spline-offset-association.md).
DOC-016/#167 has a bounded [native exchange-sheet receipt](../validation/native-exchange-sheet.md);
DOC-035/#516 has a bounded [File-menu export receipt](../validation/native-exchange-menu.md);
DOC-036/#517 has a bounded [Browser body mesh receipt](../validation/native-browser-mesh.md).

DOC-009/#21 now has a bounded, independently reviewed two-body Browser
identity, visibility, selection and reopen receipt in
[browser-document-workflow](../validation/browser-document-workflow.md).
Native body selection matches Browser rows and GPU highlighting; wider assembly
Browser and physical-interaction parity retain their own owners.

CI-010/#494 and CI-011/#495 retain the bounded Mac and Windows signing work
identified in [the build/distribution plan](../build-distribution.md). Both
are Todo; GitHub Actions already supplies tag/manual build runners, while
trusted signatures, clean-host qualification and release promotion remain open.
DOC-013/#25 has a [focused local file-workflow receipt](../validation/document-local-files.md)
covering native dialogs and the shared API, including identified pending
Save/Discard/Cancel decisions.

REL-033/#476 independently fixes the observed 119.4-second Surface Extrude
edit-preview stall for command sessions; see the bounded real-app
[receipt](../validation/surface-extrude-responsive-preview.md). REL-016 owns
the cumulative responsiveness gate; the corrected iron's functional recording
is separate from a human-ready interaction claim.
REL-034/#477 independently fixes the 94.46-second final Boundary Fill commit
stall for the saved four-tool fixture; see its [receipt](../validation/boundary-fill-responsive-commit.md).
Initial cell enumeration and large-model reopen remain synchronous.
REL-035/#478 independently fixes the cell-enumeration UI stall on the saved
iron fixture; see its [receipt](../validation/boundary-fill-responsive-cells.md).
REL-036/#479 independently fixes the large-model interactive Open stall;
see its [receipt](../validation/responsive-native-open.md). A 1.279 s final
presentation interval was a measured follow-up under REL-037/#480. The latter
now independently accepts a bounded reduction in final Open presentation:
current-build terminal polling measured 520–578 ms before and about 470 ms
after reusing evaluated body bounds. A roughly 0.42 s final UI pause remains;
see [the receipt](../validation/open-final-presentation.md). The corrected iron full recording is now independently accepted;
REL-038/039/#481–482 fix the measured Trim and Inner Loft stalls;
REL-040/041 fix the bounded sketch edit paths.
REL-040/#483 now accepts the bounded historical Sketch Edit worker path and
focused Finish command foundation. The saved iron Edit remained responsive
through 53.331 s background evaluation, with exact draft publication; see
[the receipt](../validation/responsive-historical-sketch.md). The initial
saved-fixture Finish attempts stopped at native activation; REL-042 later
passed that exact runtime/geometry/history/reopen gate using a LaunchServices
bundle launch. See [its receipt](../validation/responsive-historical-sketch-finish.md).
REL-041/#484 now accepts the direct saved-iron source edit: 29.598 s worker
evaluation with responsive status/pan/GUI heartbeat, exact changed solid,
stable face IDs, one history operation and reopen. See
[the receipt](../validation/responsive-sketch-source-edit.md). This does not
close cumulative REL-016.
REL-043/#489 independently fixes the pre-existing Surface Trim preview/commit
geometry mismatch caused by viewport-dependent bounds. Live meshed and private
fresh previews now match the accepted committed projection, regions and selected
geometry; see [the receipt](../validation/mesh-independent-surface-trim.md).
REL-045/#491 now independently accepts background partitioning for the
separately measured interactive click while preserving its original region
through navigation. See [the receipt](../validation/responsive-interactive-trim.md).
REL-046/#492 is the cumulative from-blank corrected-iron recording gate for
the integrated responsive paths. Its first 44-stage replay stopped at a measured
2.088 s actual-frame gap over the frozen 2 s limit: synchronous STL export took
2.035 s on the UI thread. The final solid and STL matched, but STEP and the
postmovie source edit/reopen were not reached. REL-047/#493 independently
accepts the bounded single-solid millimeter binary-STL worker path on a saved
iron fixture; see [the receipt](../validation/responsive-stl-export.md). The
candidate15 failure evidence is retained externally. REL-046 subsequently
accepted the full 44-stage responsive candidate16 replay; see
[the cumulative receipt](../validation/iron-responsive-cumulative.md).
REL-038/#481 now independently accepts the panel and connected-rear
partition/commit worker paths, exact committed geometry/ancestry and
history/reopen; see [the receipt](../validation/responsive-surface-trim.md).
REL-043 fixes preview/commit consistency, and REL-045 fixes the bounded
interactive target-click partition stall.
REL-039/#482 is narrowed to the two measured corrected-iron no-guide New Body
Loft waits. REL-044/#490 retains guided-rail, new-component and ordinary Loft
responsiveness so the original broader acceptance is not silently reduced.
REL-039 now independently accepts the 19.149 s off-thread third-boundary
preview and 338.034 ms cached OK on the saved Inner handle fixture, with exact
committed geometry, history and reopen; see
[the receipt](../validation/responsive-inner-loft.md).

## Active delivery order

User-prioritized 2026-10-15: finish the complete shoehorn TUT-017, then pursue
TUT-018 surface iron. TUT-017's complete functional/API chain is independently
accepted, including exact corner/perimeter rounds, retained hole, organization,
movie and checked STL. ASM-012 retains separate physical Browser gesture
qualification. SRF-017 exterior and bilateral Trim are independently accepted. SRF-015 retained
handle bridge and REL-026 warm Trim responsiveness are accepted. SRF-020 separate profile extrusion bodies are accepted; SRF-016 curved
sheet trials are independently accepted. TUT-032 closing-sheet qualification is
accepted; SRF-011 Boundary Fill is independently accepted. REL-029 independently fixes oversized
canvas-backed preview inspection; TUT-033 independently corrects the rear handle web with genus1 mesh evidence.
TUT-018 complete-03 had functional and export evidence, but its tutorial acceptance was withdrawn on 2026-10-17 after the user rejected the final shape. TUT-034/#470 corrected the iron; candidate12 passed independent pointed-nose, solid, source-edit, export and history review. Candidate13's from-blank geometry matched exactly but the recorder refused after UI-thread stalls. Candidate14 now passes the full 44-stage movie, exact native solid, checked STL/STEP and history/reopen; see [the cumulative receipt](../validation/iron-corrected-cumulative.md). Its synchronous Trim/Loft/sketch gaps remain separate REL-016 work. SRF-021 owner-local sketch Trim and
SRF-022 coincident endpoint chains are independently accepted through the
127-call from-blank surface/trial workflow. Full product closure stays open.
SRF-023 solid/mixed tools and SRF-024 multiple selected cells preserve broader
Boundary Fill scope as Todo children required by GATE-050.
The broader 2026-10-14 priority follows: [initial-build readiness](../initial-build-readiness.md)
now leads: navigation, everyday input, presentation, recovery, responsiveness and
relocatable packages, interleaved with enclosure/shoehorn/surface-iron completion.
Twenty-two new bounded children #407–428 reuse existing broader owners and gates.
NAV-001/#407 is independently accepted at2d1a699. GEO-017/#347 is independently accepted at6d9b8dc for exact inset geometry
and derived source lineage, unblocking SKT-076 authoring. CI-008/#426 is independently accepted at `098e485` for the macOS runtime recipe;
CI-009 retains Windows/Linux alignment and CI-004 stays Todo. CI-012/#588 accepts the bounded package-to-runtime inventory join on the unchanged local macOS arm64 archive; other platform and current-build provenance gates remain open. SKT-076/#346 is
accepted at `c625649` for retained inset and exact Join2/selected R0.5 downstream
lineage. NAV-002/#408 is accepted for source/API/GPU camera behavior; NAV-009/#428 retains
the blocked native-input route. TUT-023/#324 final-stage rim/Cut/export gate is accepted. NAV-018/#439
application navigation presets and NAV-019/#443 orientation conventions are
accepted. SRF-018/#445 planar crossing-sheet Trim is accepted at `1017d0a`;
REL-014/#417 committed-work recovery is accepted; REL-011/#374 native assembly
drag profiling is complete with a failed 100 ms budget; REL-020/#446 remains
Todo after bounded diagnosis and a reverted experiment. REL-013/#416 visual
audit split observed minimum-size ribbon/navigation clipping into accepted
REL-021/#447 (`c562c85`); remaining visual-quality scope stays open. GEO-019/#352 is accepted for the exact corner construction;
TUT-030/#442 now closes the research decision: public dimensions remain
insufficient for exact connector parity, and the user-approved nominal-fixture
adaptation passed under TUT-035/#472. [The final receipt](../validation/enclosure-connector-clearance.md)
measures 0.5 mm from each half; physical fit and cumulative TUT-016 remain open. The current
sequence JSON takes precedence over historical delivery notes below.
SRF-019/#451 three-section no-rail Loft and NAV-021/#452 cube accessory disclosure
are independently accepted. The full surface-iron chain and REL-013 wider
visual audit retain their remaining scope.


Current checkpoint (2026-10-14): the short expression exercise and Base Sketch
construction, dimensions and first solid are independently accepted. TUT-028/#395
is the current cumulative gate: native face sketch, three retained center references
and Cut All with flipped direction. SKT-088/#397 is independently accepted for the
coincident origin/source-center reference conflict. SOL-056/#398, DOC-033/#399 and SOL-057/#400 are accepted for Auto targeting, idle
selection and retained edit sources. TUT-028 is accepted at f9620ab with final21s continuation and a124s joined creation
film. TUT-019/#305 cumulative native piston walkthrough is accepted at acc8616; next
FND-015/#158 remains Todo: constraint activation passed, but OS AX history
observation lost window binding. SOL-016/#154 remains Todo after an unaccepted popup-to-OK route; its geometry
and modifier evidence remain partial. SOL-058/#403 preserves physical held-modifier
checks. SKT-089/#404 is independently accepted for the numeric redundant-dimension
driven offer, with refreshed production-theme native04 captures. SOL-016 remains
Todo after its production-theme recheck still failed popup dismissal.
SKT-090/#406 is independently accepted at81a363e for straight-line Extend. The source-grounded
SOL-016 popup harness correction stopped at initial activation in native05,
before exercising the workflow; that gate remains Todo.
PAR-008/#405 is independently accepted at137bd16 for retained arithmetic in
One Side/Distance Extrude, with parameter updates, units, history and reopen.
ASM-012/#343 is Todo after an inconclusive physical Browser drag recheck; its
separate body and Sketches-folder drag gate remains unaccepted. See [the current sequence](tutorial-sequence.json)
and [continuation context](../context.md) for accepted evidence and remaining work.

Current authorized queue (2026-10-13): **sliding-lid box → snap-fit electronics
enclosure → surface shoehorn → surface iron → piston subassemblies**. TUT-015–019
are cumulative gates with explicit feature dependencies. Thirty-eight new tasks
include these five gates, three source qualification tasks and30 bounded feature
children. Existing broad requirements are reused, not duplicated; split them
before pulling an implementation too large for one increment. PAR-004 and DOC-022 are independently accepted and closed.
The sliding-lid workflow and enclosure mounting prefix are accepted. The port/card and split/snap checkpoints are also accepted. Calibrated reference canvases, retained surface removal and Surface workspace
navigation and bounded exterior guide-rail Loft are accepted. Paired-sketch Surface Trim is accepted. Exterior quality blocks the bilateral
handle bridge, with explicit geometric evidence in SRF-017. Piston research is
accepted; four bounded assembly children now preserve conversion/hierarchy,
Revolute/Pin, coupled motion and linked parent copy. The native component fixture/hierarchy is independently accepted. Retained
Revolute/Pin, retained Cylindrical and coupled motion are independently accepted.
Linked local occurrence subtrees, internal joint remapping, the second piston
loop and occurrence-aware export are independently accepted. The cumulative
creation/motion/output API recording is independently accepted. Independent source-flow audit added
ASM-019 native Copy/Paste shortcuts, ASM-020 cylindrical-face origin locking and
Show/Hide, ASM-021 paste drag/contextual Revert, ASM-022 Browser gestures and
ASM-023 shared As-Built Type/occurrence selection. The direct copy/edge-selection
routes do not close these exact native steps. ASM-019 through ASM-023 are accepted; TUT-019 retains its cumulative native gate. The full semantic API creation/motion/export movie is recorded.
The next short sketch-expression exercise is audited under RES-021, with
SKT-083 for the exact Center to Center Slot input, PAR-007 for same-sketch
driving-length formulas and TUT-024 for the source-step gate. The public images
and full source ledger remain outside the repository; the compact contract is
in [the exercise preflight](../research/sketch-expression-exercise-contract.md).
Exact shoehorn corner rounding and the enclosure
alignment rim remain blocked geometry follow-ups. Complete source operations, readable constraint and
selection states, edit/history/reopen, full creation recordings and independently
checked outputs remain acceptance requirements. Detailed source URLs and
research remain external under `knowledge/tutorials/advanced-audit-2026-10-13/`.

The older overnight sequence below is completed history. Its accepted receipts
remain valid and do not supersede the current sequence JSON.


The [tutorial sequence](tutorial-sequence.json) takes precedence over broad roadmap
phase order. `tools/backlog.py ready` ranks available tutorial owners first while
still checking live dependencies. The project board exposes the same **Delivery
order** number. A blocked broad issue may receive a bounded implementation slice;
that does not close the issue or waive its remaining acceptance criteria.

1. **Nested cubes #172**: inward Shell #185, offset planes #186, body Mirror,
   multi-face sketches and center rectangle. Sol implements bounded children;
   Astra researches and independently verifies the integrated workflow.
2. **Egg cup #173**: reuse Shell/planes; add ordered circular-section Loft.
3. **Toy block #146**: full lesson, including rectangular feature patterns,
   underside face references, offset annuli/tubes and the final fillet selections.
4. **Overflow: die #147**, then coherent ready backlog work if the first three
   pass before the user returns. Do not substitute cylindrical pips or omit the
   source component workflow.

This overnight order was authorized 2026-10-12. Each completed tutorial delivers
source-step coverage, a full creation movie, editable model and checked STL.
Keep physical input, API, geometry and visual evidence distinct. Split broad
prerequisites into finishable children before pulling them; retain wider scope
as Todo. No full tutorial is complete merely because its final shape looks right.
The accepted bung/support gates and earlier bracket receipts remain valid;
outstanding exact-native bracket follow-ups remain Todo after this sequence.
Source PDFs, URLs, visual references and detailed ledgers stay in external
`knowledge/tutorials`; nested cubes, egg cup and the adapted toy block are accepted with documented API/input limits. The adapted die gate is also accepted. Next: direct center-to-edge dimension
input #205 and curved ASCII export fix #206 are accepted. Timeline readability
#207 is also accepted; the updated full toy recording uses actual edge dimensions.
The subsequent Centerline/Revolve lesson #217 and native prerequisites214–216
are accepted, as is compact original ribbon artwork #218. Nested-profile/island
handling and diagnostics #165 are independently accepted.
Bounded retained-target Extrude Intersect #219 is accepted; its broader
operation-edit parent #161 remains Todo. Native compile matrix #132 is accepted
after all four targets passed at source d072534 and independent artifact review. The bounded Windows
transport child #220 is accepted with actual native pipe/ACL evidence; #131
retains actual Windows desktop qualification.
The independently transcribed paper-clip lesson is queued as #221–225: shared-line
fillets, typed line locks, retained Sweep, native controls and full workflow gate.
Sweep foundation #223, shared-line fillets #221 and typed line locks #222 are
accepted, as are native Sweep #224 and corrected Circle Tab-to-Enter #226.
Full paper-clip gate #225 and tangent-symbol correction #234 are accepted and
closed. The full movie predates that visual fix; the receipt and external gallery
include the corrected sketch capture.
The independently reviewed initials-keyring lesson follows as #227–233: typed
two-point Rectangle, retained rounded dimensions, mixed-loop Offset, two-point
Circle, text geometry, text placement and the full tutorial gate. Rectangle #227,
text geometry #231, rounded fillets #228 and native text placement #232 are
accepted and closed, as are rounded-loop Offset #229 and two-point Circle #230;
Cut rim provenance #236 is also accepted. Full gate #233 is independently accepted and closed, with a full creation
movie, editable file and checked STL in the seven-card review gallery. Follow-up #235 is independently accepted and retains driving rectangle size and
origin intent through fillets. The older tutorial movie and saved model preserve
their disclosed measured-span policy. Rough pointer rectangle #237 is accepted;
Mirror foundation #240 is also accepted. All five wall-outlet prerequisites #237–241 are independently accepted. Full sketch-only gate #242 and snap fix #243 are independently accepted and closed.
The eight-card gallery includes its full creation movie and editable sketch;
exact history/reopen passed separately. See validation/wall-outlet-tutorial.md.

The next independently audited four-page countersunk-plate practice is #244–249:
named length bindings, post-fillet rim lineage, standalone sketch Point, retained
All Countersink Hole, Spacing/Adjust Hole pattern and cumulative movie/model/STL
gate. #244–248 and parameter-dialog capture #250 are independently accepted.
The full creation/movie/model/STL gate #249 is independently accepted and closed.
See [the accepted receipt](../validation/countersunk-plate-tutorial.md). The cumulative [gate preparation](../validation/countersunk-plate-prep.md)
records the source sequence, output checks and visual acceptance.
Full arithmetic parameters, other Hole types and general pattern families remain
in their broad parent issues.

The next full thirteen-page sketch lesson is #251–257: center-point slots,
arc slots, fit/control splines and their editing/curvature combs, conic Rho,
center-point ellipse, and a cumulative all31-step gate. Straight slot #251 is
active alongside fit spline #253 after independent finite-geometry design review.
Remaining implementations and full gate stay Todo. Source is sketch-only, so
no invented extrusion or STL replaces the documented endpoint. Detailed public
references and independent corrections stay outside Git under
`knowledge/tutorials/after-countersunk-plate/`.

After every slice, run the smallest complete author/edit/finish/undo/reopen
workflow it enables in a private real application instance. Record source steps
as passed, mismatch, blocked or unknown. For documented input gestures, use native
input and inspect shared API state; direct coordinate/API construction alone does
not pass snapping or command interaction. Capture constraint-stage and endpoint
views; check badge identity, actual target, visibility, contrast, placement and
state colors. Full exact acceptance is [TUT-003](../research/illustrated-bracket-gate.md).

The external `knowledge/tutorials/short-candidates.json` records a supplementary
illustrated wall-outlet sketch lesson and two prerequisite-bound bracket activity
checkpoints. The wall-outlet exercise adds mirror/concentric requirements, so it
is not a shortcut around the bracket prerequisites. Activity excerpts are not
standalone tutorial passes. Packaging, RHI, import and broader features remain in
the backlog; urgent regressions interrupt this sequence. Milestone releases still
require macOS/Windows/Linux qualification; CI remains tag/manual only.

| Phase | Count | Result |
| --- | ---: | --- |
| P0 Foundation | 35 | Public baseline, MIT dependency audit, document/naming/solver decisions, portable controls, native CI/packages/releases and hardware evidence |
| P1 Geometry/document | 20 | Real hardware-backed OCCT viewport, authored primitive document, selection, undo/save and STEP |
| P2 Parametric modeling | 219 | Constrained sketches, parameters, editable extrusion/fillet chain and broader basic solid tools |
| P3 Shape/import/print breadth | 45 | Solids/surfaces, common interchange and qualified printing export |
| P4 Initial release | 34 | Robustness, recovery, packaging, performance, physical/visual workflow evidence |
| P5 Assemblies/drawings | 41 | Components/joints, configurations and associative drawings |
| P6 Advanced design | 11 | Sheet metal, direct/mesh/Form, advanced modeling, plastic rules and local presentation |
| P7 Manufacturing | 9 | Setups, toolpaths, verification/posts, additive, nesting and metrology |
| P8 Analysis/electronics | 10 | Traceable study/result data, local ECAD/MCAD and fabrication |
| P9 Full replacement | 4 | Remaining interchange, customization, generative lifecycle and full coverage reconciliation |

## Pullable work and current evidence

The shell gate and runner, authored-history decision, dependency admission,
geometric-reference prototype, command sessions, native-toolchain audit and
thin modeling kernel are complete with independently reviewed evidence. The
first real primitive workflow now verifies rendering/picking, editable box and
cylinder intent, one undo authority, trustworthy measurements and native
save/reopen. See [development](../development.md) and the
[primitive receipt](../validation/primitives.md).

Run `python3 tools/backlog.py ready` before pulling work; live issue states and
dependencies determine readiness. The active modeling order is the tutorial sequence above. Its dependencies no longer wait for the broad GATE-010/platform gates. Remaining geometry/document work includes
face/edge selection, Browser breadth, STEP interchange, persistent-reference
integration and observable document decisions. Foundation work still includes
a dated compatibility baseline, portable Windows controls and
qualified runtime packages. The issue count measures the starting backlog,
not product coverage or a replacement claim.

HIS-001/SOL-001 and the local SKT-015 checkpoint are now closed with independently
reviewed [sketch/extrusion evidence](../validation/sketch-extrusion.md). The first
sketch-led part supports upstream edits, history and storage through the actual
UI/API. Current board items are Done; the 2026-10-12 snapshot has 28 closed and 130 open. Live readiness is obtained with the command above. Full tutorial and cumulative product gates remain open.

MIT is accepted for Alloy's original source and documentation. Each dependency
keeps its own terms. A GPL-only solver or format library must not slip into the
distributed application through this research; require an explicit deliberate
later licensing decision or an audited compatible alternative. C++20 is selected;
Rust is outside this plan.

## Cumulative product gates

| Gate | Useful result | Additional evidence |
| --- | --- | --- |
| `GATE-000` | Existing native shell and shared commands | Physical S/search/Home/browser path, API inspection, actual window capture; zero CAD claims |
| `GATE-005` | Installed packages on macOS, Windows and Linux | Native runtime relocation, portable control API, actual hardware renderer, pre-capture drawable/scale/resize and physical inputs |
| `GATE-010` | Gauge block and cylinder | Actual hardware GL diagnostics, rendered/picked BRep, analytic measurements, undo/save and independent STEP readback |
| `GATE-020` | Editable mounting bracket | Constrained sketch → extrusion/join/cut → fillet → upstream edit → reopen/export |
| `GATE-030` | Surface-authored duct plus enclosure/knob/slot fixtures | Surface continuity, gaps/stitch, thickening, new solid/sketch operations and editable history |
| `GATE-040` | Imported adapter and printable parts | Independent source files/readback across declared formats, units, losses, mesh validity |
| `GATE-045` | Native Metal/Vulkan CAD viewport | Packaged hardware and cumulative workflow evidence on every declared target |
| `GATE-050` | Dependable initial shape release | Packaged app, recovery/negative corpus, performance, physical workflow and qualified visuals |
| `GATE-060` | Adjustable clamp assembly and drawings | Occurrences, DOF/limits, variants, drawing association/BOM and exports |
| `GATE-070` | Configured instrument enclosure | Folded/flat metal, Form/direct/mesh edits, advanced modeling, local rendering/animation |
| `GATE-080` | Manufacturing package | Toolpath/stock/collision/post, layers, nesting, process-specific outputs and units |
| `GATE-090` | Traceable analysis study/results | Real study/mesh mapping, numerical probe checks and invalidation; imported results are not a solve |
| `GATE-100` | Electronics-to-enclosure product | ERC/DRC, connectivity, board/package alignment and independent fabrication-file checks |
| `GATE-110` | Full noncloud scope reconciliation | Every accepted command/option/family traced to implemented children and cumulative workflows; only evidenced cloud suboperations may defer |

Each gate depends on actual feature issues plus earlier gates. Every newly
implemented capability must map into the growing fixture ledger; the main
showcase object need not exercise every option, so retain small focused fixtures
alongside it. Run **all previously accepted representative workflows** on the
same candidate build. A previous green issue is not a fresh regression result.

The runner is implemented by `FND-010`; the [ledger](../../tests/workflows/ledger.json)
lists current shell, authored, session, kernel, native-audit and primitive cases.
`python3 tests/workflows/run.py --gate GATE-020 --cumulative
--evidence build/evidence/GATE-020` refuses acceptance until that gate's evidence
specification, actual feature cases and live prerequisites are complete. Read
[workflow validation](../workflow-validation.md). Existing `ctest` remains valid.
Use Alloy automation for modeling/inspection/capture; direct computer input is
reserved for uncovered physical interaction. Gate reports separate build, API,
geometry, save/reopen, export, physical input and visual results. Unit tests,
mocked state, static drawings, issue counts or kernel-only demos cannot close
product gates. No reference CAD application runtime comparison is permitted.

If later research finds a missing noncloud family or option, add a real task and
update the appropriate gate prerequisites. Do not use the cloud exception to
discard an entire family: `FULL-001` owns dated operation-specific deferrals and
unknowns. For analysis, current reference solves are cloud-dependent; local
study/result capability is planned separately. A future local solver must add
blocking numerical/convergence benchmark tasks; result import cannot count as
solver validation.

## Validation and publication

```sh
python3 tools/backlog.py validate
python3 tools/backlog.py ready
```

The current publisher validator checks root/issue schema, unique keys, dependency existence,
acyclicity, dependency-first publication order, required issue-body sections,
dated source links, epic decomposition requirements, the cumulative gate chain,
and reachability of all tasks from the final gate. Initial tasks reach the
initial-release gate, apart from the explicitly separate cloud inventory which
reaches the full gate. The recorded output is
[validation-summary.json](validation-summary.json).

Each issue includes problem/result, bounded scope, acceptance, exact proposed
verification, evidence dimensions, public source/date and limitations. Most
source pages are living documentation, not release-pinned specifications; see
[the deep dive](../research/cad-workflows.md) for contradictions and missing
defaults. All 135 issues were published and checked through `gh`/GitHub APIs:
unique keys, complete bodies, linked dependencies, kind/phase/priority labels,
ten milestones, and readiness labels. See [publication.json](publication.json)
for the dated receipt. The repository was returned to public visibility on
2026-10-21; Alloy original source remains MIT-licensed.

The public [project board](https://github.com/users/rje/projects/1) has a
repository auto-add workflow. Projects access is now available through `gh`;
all 135 board items were verified by API. Keep Status synchronized as tasks start
and finish. Issue publication and the pull queue are verified independently.

To synchronize dependency labels after closing work:

```sh
python3 tools/backlog.py ready --apply-labels
```

`python3 tools/backlog.py publish` explicitly creates or reconciles the authored
issues, labels, and milestones. It is resumable using stable issue-body markers.
Review changes to `issues.json` before republishing; remote states are preserved.
All ongoing GitHub operations use `gh` and documented APIs.

The cross-platform requirement added five bounded tasks to the original 130:
`FND-012` Windows local control, `CI-001` native build matrix, `CI-002` runtime
bundles, `GATE-005` hardware package qualification and `CI-003` milestone release
promotion. Existing dependency/packaging/product gates now require all three
OSes. The [release plan](../releases.md) distinguishes the current compile-only
CI seed from the future qualified runtime packages.

The later input/editor and renderer decisions added UX-001, RHI-001/RHI-002 and
GATE-045. The sketch reprioritization split SKT-013 face attachment, SKT-014 simple
profiles and HIS-004 broader recompute from the first origin-plane path, and added
SKT-015 as its focused human-test checkpoint. Original publication.json is the
historical 135-issue receipt; github.json and validation-summary.json reflect the
current 150-task backlog. SKT-015 is a bounded local workflow check, distinct from
the cumulative all-platform product gate chain.

The public beginner-project [tutorial recipes](../research/tutorial-gates.md) add
TUT-001 #146 (complete toy block) and TUT-002 #147 (die/component/STL). They are
blocked future workflow gates and have not been executed. SKT-016 #144 supplies
construction/midpoint/point-to-edge coverage; SOL-014 #145 supplies genuine
face-positioned Sphere/Cut/Repeat. TUT-001 feeds GATE-030; the full component die
feeds GATE-060. SKT-015 remains the earlier rectangle/extrusion human checkpoint.

The source registry now also includes the annotation workflow and tutorial
evidence. One supplied crop has unknown public provenance, version and scale;
its observation is qualified separately from retrieved public documentation.

The exact illustrated bracket audit adds five tasks (155 total): DOC-015,
SKT-018, SKT-019, SOL-016 and TUT-003. See
[the source-step and visual acceptance contract](../research/illustrated-bracket-gate.md).
TUT-003 feeds GATE-020 and remains blocked; no skipped or substituted operation
counts as completion. Constraint icons, visibility, construction styling,
annotations and preview presentation are required alongside real geometry.

The older `docs/backlog/validate.py` assumes the original seed prose headings and
publication order. It currently fails on preexisting SKT-005 heading changes; it
is not a passing validation of the evolved backlog. The publisher validator above
checks current schema and acyclic dependencies; it does not establish reference
coverage or cumulative product acceptance.

Slot drag presentation follow-up #258 tracks solved geometry during pointer motion;
the initial slot candidate evaluates the final shape on release. It remains Todo.

[XCH-014/#720](../decisions/004-mesh-import-foundation.md) admits the pinned STL/OBJ decoder with strict validation required. XCH-015/#722 owns retained STL and measured limits; XCH-016/#723 owns native/GPU presentation; XCH-017/#724 owns strict OBJ. XCH-004/#72 remains the cumulative import gate.

DOC-043/#721 accepts a bounded Body Priority/Browser/API multi-body set after independent review. DOC-045/#726 retains the ordinary Auto canvas body route; DOC-044/#725 repairs a separately diagnosed preexisting edge-refresh mismatch.
