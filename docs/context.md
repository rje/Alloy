## Current execution order

2026-10-24 execution resumed: Search FND-029 is closed/Done after final review.
SRF-059's bounded renderer investigation failed the 0.005 mm seam visibility
criterion; all three trials were reverted and the issue is back in Todo, not
accepted. Evidence is external at
`../Alloy-reference-records/validation/surface-stitch-seam-2026-10-24/attempts.md`.
The fixture is retained in RHI-002 acceptance; migration does not imply a fix.
SRF-057 still owns the complete native Stitch minimum and remains blocked.
The on-disk build executable contains the last rejected seam candidate and must
be rebuilt from intended source before validation or showing it to the user.
SOL-060/#505 is now active for the complete bounded circular-section Coil
workflow. Its external public contract is `knowledge/specs/sol060-coil-2026-10-24.md`
under the reference records: numeric XY center, centerline Diameter, Revolutions
And Pitch, circular On Center section, +Z clockwise/New Body and named pitch
binding. Numerical defaults and explicit viewpoint are Alloy choices; canvas
placement and wider variants remain ADV-005 Todo. Maintain one active issue, obtain
proportionate independent Sol review, push the accepted result and mark its
issue/board Done before moving on.

[FND-029/#734](validation/command-search-focus.md) accepts coherent Command
Search query/focus/reentry, Escape and single result activation at `c4b4322`
after final independent Sol review on 2026-10-24. The existing DPR2 native
fixture passes 3/3; evidence is archived externally. FND-007 keeps the remaining
shortcuts and cumulative workflow. No paid CI was run.
[NAV-022](validation/native-orbit-pivot.md) accepts the hardened native
orbit/pivot validation harness and its first safe-stop receipt. NAV-009 remains
Todo because PID-directed OS right-click did not reach the private editor;
Set/Pick/Reset and subsequent checks were not run.

The [2026-10-21 first-v1 burndown](v1/priority-burndown-2026-10-21.md)
prioritizes Mac input/selection, retained mesh import, native Metal/Vulkan
rendering and cumulative three-platform release qualification. The scope freeze
and live GitHub issue states remain authoritative for readiness.
[DOC-043](validation/body-selection-set.md) accepts shared Body Priority/Browser/API
multi-body selection; [DOC-045/#726](validation/auto-body-route.md) accepts the
documented Body Priority/Browser whole-body route while keeping ordinary Auto
face-first. [DOC-044/#725](validation/root-edge-refresh.md) accepts the separate
root-edge refresh repair.
[XCH-015/#722](validation/retained-stl-import.md) accepts strict retained STL
core/API import with original geometry, diagnostics, source-independent reopen
and measured capacity. [XCH-018/#727](validation/retained-mesh-presentation.md)
accepts distinct Browser/GPU mesh presentation, whole-mesh selection/visibility,
source-independent reopen and effective DPR1/2 Mac native captures after
independent reviews. [XCH-019/#728](validation/native-stl-import.md) accepts the
native chooser product entry and bounded ready-preview/API/Qt panel workflow;
strict OS chooser routing is explicitly unqualified.
[XCH-021/#730](validation/responsive-stl-import.md) accepts bounded responsive
in-flight loading/Cancel and exact full-cap prepared commit after independent
review. XCH-022 retains PID-safe
chooser/popup and DPR1/2 focus/input qualification after the earlier delay-based
test proved insufficient; it is also required for XCH-016.
XCH-020 records the measured subsequent-Box cost.
[Strict OBJ core/API](validation/strict-obj-core.md) accepts XCH-023/#732:
direct deterministic concave triangulation, no sidecar access, retained
labels/diagnostics, source-independent history/reopen, 2/2 focused CTests and
114 real-app API responses. Independent review passed after correcting
duplicate-position connectivity quality and late-error line diagnostics.
[XCH-024/#733](validation/native-obj-import.md) now exposes OBJ through the native
filter and shared cancellable ready preview. Owned source bytes preserve the same
canonical-mm polygon policy for direct import and repeated mm/in preview, including
retry after failed default mm. Final build, 2/2 CTests, 248 real-app responses and
isolated actual-hit Cancel/OK at effective DPR1/2 pass; Apple M4 captures and API
pick/highlight/resize were inspected. The first after-Cancel native key-window loss
is retained under XCH-022/#731; no uninterrupted OS chooser or focus acceptance
is claimed. Independent source and final evidence reviews accept this bounded
child. Only user PID72618 remains.
Cumulative bounded OBJ XCH-017/#724 accepts the combined child evidence after
independent review; strict chooser/focus and combined STL/OBJ parity remain
XCH-022 and XCH-004. The dated public format contract
is external at `../Alloy-reference-records/knowledge/specs/xch017-obj-import-2026-10-21.md`.
The [printable bench-vise capstone](v1/capstone-candidate.md) is a researched
proposal, not a selected release gate; detailed public references remain outside
the repository.

## 2026-10-20 two-sheet Stitch work started

The first-v1 [Surface Stitch minimum](v1/open-row-disposition.md) has a dated
public/OCCT contract outside git under
`../Alloy-reference-records/knowledge/specs/srf006-stitch-2026-10-20.md`.
[SRF-056/#716](validation/surface-stitch.md) accepts one retained two-sheet
matching-edge open-shell kernel/API slice after independent Astra review of
`c6cc33e`, a focused CTest, a 163-call real-app fixture and inspected Apple M4
GPU captures. SRF-057/#717 still owns native actual-hit selection, preview,
gap/tolerance display and physical/GPU qualification; the 0.005 mm result has
[fill striping](validation/surface-stitch-visual.md) now corrected and independently
accepted under SRF-058/#718. The sewn seam remains light/intermittent in some
views; SRF-059/#719 owns that preserved visual remainder before SRF-057 native
acceptance. SRF-006/#65 retains general networks, healing choices and closed-solid
promotion. The canonical backlog has 716 issues; no paid CI or release was run.
All private test editors exited; user PID72618 was preserved.

## 2026-10-20 connected-Line Surface Revolve accepted

[SRF-037/#660](validation/surface-revolve-chain.md) accepts one retained connected
2–32 Line open section as an editable non-solid sheet. Independent Astra review
accepted commit `0d53862`: 3 focused CTests, a 77-response real-app analytic
fixture, unchanged one-Line regression, persistence/Undo/Redo and inspected
Apple M4 GPU captures. SRF-036 still owns canvas selection, angular drag and
native 1×/2× input/visual acceptance; later SRF-055/#715 owns Line/Arc sections.
The canonical backlog now has 712 issues. No CI/release was run and the private
editor was closed; user-owned PID72618 was preserved.

## 2026-10-20 sketch dimension overflow accepted

SKT-191/#714 adds a bounded two-margin fallback after the accepted local lanes
exhaust. The supported1040×640 fixture retains seven full direct-value targets,
source IDs/values, authored save/reopen and exact evaluated bodies; one new STL
is byte-identical to the accepted SKT-190 readback. Effective DPR1/2 Qt actual-hit
and GPU evidence is recorded in [the accepted receipt](validation/sketch-dimension-overflow.md).
Public automatic-layout parity, physical input and arbitrary finite-canvas density
remain unknown or limited; independent Sol review accepted `2c21d9f`. Native DPR1 backing
height can differ by one pixel before resize; exact same-state stability and
bounded resize-return centers are checked separately. SKT-191/#714 is closed/Done;
the canonical backlog remains 711 issues. No CI/release was run; private
fixtures exited and user PID72618 was preserved.

## 2026-10-20 SKT-165 native retry remains blocked

The existing-dimension Toggle Driven/Driving candidate retains independent Sol
source/evidence and prior core/API/GPU coverage. A bounded unchanged
`directToggle` retry stopped at initial native activation before demonstrated
input; read-only preflight again observed `consoleLocked=true`. No product
change, rebuild or CI was needed. See [the retry receipt](validation/sketch-dimension-driving.md).
Keep #611 open until an unlocked-session strict native replay passes. Private
fixture exited; user-owned PID72618 was preserved.

## 2026-10-20 dense sketch dimension readability accepted

[SKT-190/#713](validation/sketch-dense-dimensions.md) is independently accepted
for the five-driver retained Rotate fixture at zoom 0.55. Five full values are
separately visible and directly editable through actual native hits; pan/resize,
normal authored placement, supported drag Undo/Redo, exact authored/body
save/reopen and byte-identical three-component STL pass. The view-only layout
does not change dimension or solver authority. Four local lanes can exhaust in
smaller viewports, so SKT-191/#714 retains the compact overflow stack as Todo.
The earlier visual baseline had a different camera scale, and exact reference
automatic-layout behavior is unknown. The canonical backlog has 711 issues.
No CI/release was run; private editor closed and user PID72618 preserved.

## 2026-10-20 bounded sketch-transform family accepted

Independent Astra read-only review accepted the proposed-v1 connected four-Line
XY Move, Copy, Rotate and Scale minimum through `753f2c8`, including source/edit
authority, preview/cancel, history/reopen and independently read printable
geometry. `sketch.transform` row098 moves from S to E for this bounded minimum;
arbitrary stacked transforms, other curves/planes and exact reference behavior
remain broader or unknown, and physical/package release gates remain open.
The multi-profile SKT-189 capture has crowded Rotate dimension labels; visual
follow-up SKT-190/#713 is accepted above. The prior 710-issue snapshot and
unmatched visual baseline are retained as historical context. No CI/release was run.

## 2026-10-20 SKT-186 Create Copy history research disposition

Additional permitted public help, support, product-team and tutorial evidence
confirms active-sketch Create Copy/Translate, but does not demonstrate a
sketch-specific Timeline/Edit Feature result followed by later source edit.
The comparison and evidence trigger are external in
`../Alloy-reference-records/knowledge/specs/skt186-create-copy-history-followup-2026-10-20.md`.
SKT-186/#709 is closed/Done as an evidence-bounded unknown; the accepted
independent-copy policy remains and exact history parity is not claimed. The
cumulative sketch-transform minimum is accepted above.

## 2026-10-20 SKT-189 Copy/Rotate coexistence acceptance

[Bounded receipt](validation/sketch-copy-rotate-coexistence.md) accepts ordinary
Copy/Paste and Move/Copy Create Copy alongside an unrelated retained Rotate.
Only the disposable copy-profile check now uses the qualified clipboard model;
the complete sketch still passes strict Rotate validation. Native actual-hit
input passes 3/3, real-app API passes 70 calls, identities/history/reopen stay
stable and independent STL readback finds four watertight components totaling
7,712 mm³. Independent Sol read-only review accepted `753f2c8`.
SKT-189/#712 is closed/Done. Sketch-specific Create Copy history is disposed
above as an evidence-bounded unknown. The canonical backlog later reached 710
issues. No CI/release; private editors closed and user PID72618 preserved.

## 2026-10-20 SKT-181 active-sketch Scale acceptance

[Bounded receipt](validation/sketch-scale.md) accepts uniform factor Scale for
one constrained four-Line XY profile about a separate authored Point. Native
actual-hit controls pass 3/3, real-app API passes 188 calls, source/cache and
same-owner edits keep identities/history stable, and independently read STLs
are watertight at 3,762 and 3,510 mm³. Independent Sol read-only review found
and verified the Pattern-input/Scale-accept lifecycle correction in `0c7a457`
after initial candidate `3569a46`. SKT-181/#704 is closed/Done. Copy/Rotate
coexistence and Copy history research are disposed above.
No CI/release; private editors closed, user PID72618 preserved.

## 2026-10-20 SKT-188 retained Rotate source-size acceptance

[Bounded receipt](validation/sketch-rotate-source-size.md) accepts numeric edits
to existing internal driving Linear dimensions on one retained four-Line XY
Rotate. A fixed source anchor/directed heading lets the source solve resize
without double-applying the +30° transform; source and dimension snapshots
change in one transaction. Native actual-hit dimension input passes 3/3,
98-call real-app API and exact history/reopen pass, and independently read
watertight STLs agree with 2,003.0545 and 2,026.4775 mm³ analytic volumes.
Independent Astra read-only review accepted `294ec4f`. SKT-188/#711 is
closed/Done; Scale is accepted above.
Unrelated Copy/Create Copy alongside an existing Rotate owner refuses and is
preserved in Todo SKT-189/#712. The canonical backlog has 709 issues. No CI or
release; private editors closed and user PID72618 preserved.

## 2026-10-20 SKT-180 retained Rotate acceptance

[Bounded receipt](validation/sketch-rotate.md) accepts one closed four-Line XY
profile rotated around its authored +Z normal construction Line, with signed
angle and eligible axis edits on one retained owner. Native actual-hit input,
143-call real-app API, exact source/derived vertices, history/reopen and
independent watertight 1,952 mm³ STL readback pass. Independent Astra read-only
review accepted `34f36c6`; its review did not rerun the app. SKT-180/#703 is
closed/Done. Source-size composition is accepted above; Scale remains
SKT-181/#704. General 3D axes, physical input and Windows/Linux runtime remain
unqualified. No CI/release; private editors closed, user PID72618 preserved.

## 2026-10-20 SKT-187 normal-axis acceptance

[Bounded receipt](validation/sketch-normal-axis.md) accepts one authored +Z
normal construction Line in an active XY sketch. It has three stable IDs,
dedicated numeric/native canvas authoring, real 3D segment and end-on marker
picking, edit/Delete/refusal, Undo/Redo and save/reopen. PlaneGCS, planar
profiles and downstream Extrude exclude it; an asymmetric 244 mm² profile
exports unchanged as a watertight 1,952 mm³ STL before/after axis edits.
Independent Sol review accepted `e6a8e6c`; external evidence is
`../Alloy-reference-records/validation/skt187-2026-10-20/`. SKT-187/#710 is
closed/Done. Retained planar Rotate is accepted above; source size composition
remains SKT-188/#711. General 3D Sketch and physical/platform
input remain unqualified. No CI/release; private editors closed, user PID72618
preserved.

## 2026-10-20 SKT-180 Rotate prerequisite split

Public research at
`../Alloy-reference-records/knowledge/specs/skt180-sketch-rotate-2026-10-20.md`
found an active-sketch Rotate route requiring a 3D sketch Line axis rather than
a coplanar point. Independent Astra architecture review found Alloy has only
planar sketch Lines. SKT-187/#710 was the prerequisite accepted above.
SKT-180/#703 now depends on #710 and owns retained planar Rotate with angle
and axis edits. SKT-188/#711 preserves later source-size composition. General
3D Sketch remains broader roadmap scope; no Rotate implementation or parity is
claimed yet. The canonical backlog has 708 issues.

## 2026-10-20 SKT-185 Create Copy acceptance

[Bounded receipt](validation/sketch-create-copy.md) accepts the general Solid
Modify > Move/Copy > Sketch Objects > Create Copy route in an active sketch.
It shares the independent Copy geometry authority, preserves the user's
clipboard and keeps Create Copy/Retain Move mutually exclusive. Native menu,
four actual viewport picks, mode transitions, preview/Cancel/OK, history/reopen
and real-app API pass. Independent binary STL readback yields two watertight
solids totaling 3,840 mm³ initially and 4,224 mm³ after independent edits.
Independent Sol review accepted `ff93014`; external evidence is
`../Alloy-reference-records/validation/skt185-2026-10-20/`. SKT-185/#708 is
closed/Done. Sketch-specific retained-history semantics remain unknown under
SKT-186/#709; Rotate/Scale #703–704 remain Todo. Physical input, matched
styling and packaged Windows/Linux runtime remain unqualified. No CI/release;
private editors closed and user PID72618 preserved.

## 2026-10-20 SKT-185 Create Copy evidence boundary

SKT-185/#708 was the issue for a second active-sketch Move/Copy
Create Copy route over the accepted independent Copy geometry authority. Public
research in `../Alloy-reference-records/knowledge/specs/skt185-create-copy-2026-10-20.md`
confirms Sketch Objects and Create Copy/Translate, but does not establish
sketch-specific retained Timeline/constraint/source semantics. Independent Astra
architecture review recommends a command-local source capture and stable preview
IDs, mutually exclusive with Retain Move; no new retained Copy owner without
evidence. The unresolved retained-history question is linked Todo SKT-186/#709.
The canonical backlog now has 706 issues. The bounded route is accepted above;
no parity is claimed for the unknown history behavior.

## 2026-10-20 SKT-179 sketch Copy/Paste acceptance

[Bounded receipt](validation/sketch-copy.md) accepts selected four-Line
Copy/Paste through context and platform keyboard routes. The copy gets disjoint
stable points/curves/constraints/dimensions with internal references remapped;
unsupported external relations and profile contact refuse atomically. Native
actual-hit preview, click/Enter/Cancel/Escape, focus, Undo/Redo/reopen and local
API pass. A two-Extrude STL has two watertight components totaling 3,840 mm³;
independent source width and copy placement edits yield 4,224 mm³ with stable
IDs. Independent Sol review accepted `498bc3a`; Astra's bounded cross-transform
compatibility pass found no combined blocker across atomic Move, retained Move
and Copy/Paste. External evidence is
`../Alloy-reference-records/validation/skt179-2026-10-20/`. SKT-179/#702 is
closed/Done. General Create Copy #708 and Rotate/Scale #703–704 remain Todo.
Exact reference visual/Timeline parity, physical input and packaged platforms
remain unqualified. No CI/release; private editors closed, user PID72618 kept.

## 2026-10-20 sketch Copy route split

SKT-179/#702 was the active-sketch Copy/Paste slice for one closed profile and
is accepted above. Public research is external at
`../Alloy-reference-records/knowledge/specs/skt179-sketch-copy-2026-10-20.md`:
the documented selection/context or Cmd/Ctrl+C, Paste and placement route is
the bounded slice. Public sources conflict or omit copied constraints, identity,
history and preview defaults, so the spec labels independent-copy semantics as
Alloy policy. The separate Move/Copy > Sketch Objects > Create Copy route is
preserved in Todo SKT-185/#708 with a dependency on #702 and #707. The canonical
backlog has since grown to 706 issues; the general route and Rotate/Scale remain open.

## 2026-10-20 SKT-184 retained Move acceptance

[Bounded receipt](validation/sketch-retained-move.md) accepts one retained XY
Move owner on the isolated four-Line profile. Source points remain authoritative;
same-ID output points are derived once and validated at public boundaries.
Visible Edit Move changes the existing (+5,-3) definition to (+7,-2), and a
later source width edit 20→24 recomputes the existing Extrude from 1,920 to
2,304 mm³. Native actual-hit picks/editing, core idempotence and owner guards,
real-app API/history/reopen and independent watertight STL bounds/volumes pass.
Independent Astra review accepted `cd0e682` plus API input correction `5143acb`;
external evidence is `../Alloy-reference-records/validation/skt184-2026-10-20/`.
SKT-184/#707 is closed/Done. Copy/Rotate/Scale #702–704 and broader Move modes
remain Todo. Exact reference Timeline/visual parity, physical input and packaged
Windows/Linux runtime remain unqualified. No CI or release was run; private
editors closed and user PID 72618 preserved.

## 2026-10-20 SKT-098 atomic Move acceptance

[Bounded receipt](validation/sketch-move.md) accepts native and local-control
atomic XY translation of one isolated closed four-Line sketch profile. Two
independent relative Moves preserve curve/profile/body IDs through an 8 mm
Extrude and an independently read watertight 1,920 mm³ STL. Exact rigid targets,
unchanged unselected geometry/definitions and atomic constraint refusals pass
core checks. Native actual-hit input, two finish-route guards, API terminal-panel
cleanup, Undo/Redo and save/reopen pass focused fixtures. Independent Sol review
accepted `9952f07` and corrective `55628da`; external evidence is at
`../Alloy-reference-records/validation/skt098-2026-10-20/`. SKT-098/#503 is
closed/Done. Retained edit SKT-184/#707 is accepted above; Copy/Rotate/Scale
#702–704 and broader multi-group/3D modes remain Todo. The #707 source/output architecture record is
external at `../Alloy-reference-records/knowledge/specs/skt184-retained-sketch-move-design-2026-10-20.md`.
No CI, release or platform/physical qualification was run. Private editors
closed; user PID 72618 preserved.

## 2026-10-20 SKT-098 Move architecture split

Independent Astra review of `DocumentEdit`, sketch solver point intent and
profile identity supports an atomic authored Move of one isolated closed
four-Line loop under SKT-098/#503, with a separate retained editable definition
under SKT-184/#707. The atomic command must translate all selected unique
points together, solve once, verify exact requested targets and unchanged
unselected geometry/relations, and preserve IDs through downstream Extrude.
Two sequential relative moves are not editing the first definition. A retained
definition requires explicit source/evaluated coordinates and later-edit
composition rules; attaching a delta to today's mutable point coordinates is
not sound. The engineering note is external at
`../Alloy-reference-records/knowledge/specs/skt098-move-architecture-2026-10-20.md`;
the dated public command record is alongside it. This was the planning split
before the bounded atomic Move acceptance above.

## 2026-10-20 SKT-183 skewed rectangular Pattern acceptance

[Bounded receipt](validation/sketch-skew-pattern.md) accepts two nonparallel,
nonperpendicular authored construction Lines for the retained rectangular
circle Pattern. Centers follow `c + i*s1*a + j*s2*b`; positive spacing follows
each line's start→end orientation. Parallel/antiparallel, degenerate and
overlapping results refuse atomically. Native orthogonal/skew actual-hit input
passes 4/4. A real executable reopens its native-created skew grid, edits the
source and both directions through a six-hole Cut, verifies indexed IDs,
history/reopen/refusals and independently reads a watertight STL. Circular
regression passes 3/3. Independent Sol review accepted `c778124` after
recomputing vector sums, UUIDs, mesh topology and the final D2 leader capture.
External evidence `../Alloy-reference-records/validation/skt183-2026-10-20/`,
build `build-preview-release`, `-j4`. SKT-183/#706 is closed/Done. Sketch
pattern proposed-v1 bounded minimum is accepted; cumulative package/physical
qualification remains open. Extent, Symmetric, broader seeds and suppression
stay ADV-005. No CI/release; private editors closed, user PID 72618 preserved.

## 2026-10-20 SKT-182 circular Pattern canvas acceptance

[Bounded receipt](validation/sketch-circular-pattern-canvas.md) accepts real
Create-menu activation, Circle/standalone-Point viewport hover and picks,
Full Quantity and Angle sweep handles, numeric preview, invalid count/angle
rollback, Cancel/Escape/OK and center-glyph editing. The native actual-hit
gate passes 3/3. A real-executable run reopens its native-produced six-hole
model, edits source/pivot, refuses a missing downstream Cut profile, preserves
history/reopen and independently reads a watertight STL. Independent Sol review
accepted commit `f93ad4c` after recomputing identities/centers, STL topology and
the final arc-attached Quantity grip. External evidence is under
`../Alloy-reference-records/validation/skt182-2026-10-20/`; build is
`build-preview-release` with `-j4`. SKT-182/#705 is closed/Done. Skewed
rectangular axes are accepted under SKT-183/#706; broader modes ADV-005. Positive
sketch-UV rotation is an Alloy policy; sign parity with a third-party clockwise
description is unknown for unmatched plane/camera states. A native rectangular
regression close-prompt stall was a test teardown issue and passes after explicit
discard. Physical AppKit, DPR1 and packaged Windows/Linux remain separate;
no CI or release run. Private fixture editors closed; user PID 72618 preserved.

## 2026-10-20 SKT-178 rectangular Pattern canvas acceptance

[Bounded receipt](validation/sketch-pattern-canvas.md) accepts an actual-hit
Create-menu route, Circle and two construction-Line viewport picks, role/hover
feedback, four spacing/quantity GPU handles and shared numeric preview for the
perpendicular 2×3 grid. Invalid directions/drags preserve the accepted state;
Cancel/Escape/OK and center glyph editing pass. The native-produced six-hole
plate is reopened by the real executable, edited upstream, saved/reopened and
independently read as a watertight STL. The focused native gate passes 3/3, as
do rectangular/circular controls regressions. Independent Sol review accepted
commits `e650965` + `df72cef` after checking authored IDs, centers, meshes and
the final D2 label-clearance capture. Build `build-preview-release`, `-j4`;
external evidence `../Alloy-reference-records/validation/skt178-2026-10-20/`.
SKT-178/#701 is closed/Done. Circular canvas input is accepted under SKT-182/#705;
nonorthogonal rectangular axes are accepted under SKT-183/#706. Physical Mac pointer,
DPR1 and packaged Windows/Linux remain separate. No CI or release was run;
private fixture editors are closed and user-owned PID 72618 was preserved.

## 2026-10-20 SKT-177 circular Pattern acceptance

[Bounded receipt](validation/sketch-circular-pattern.md) accepts one ordinary
source Circle patterned around a standalone authored Point in Full/Angle modes,
with explicit total Quantity, Total Angle, source-only solve, stable indexed
instances, owner-safe refusals and a center-glyph edit route. The shared numeric
native/API command passes a 90-call real-executable six-hole plate, source and
center edits, history/reopen, missing-profile refusal and independent watertight
STL readback. Circular Qt actual-hit controls/glyph pass 3/3; its Create action
was triggered by the fixture. Rectangular shared-controls regression passes 3/3.
Independent Astra review recomputed centers, 32 UUIDv5 fields and manifold
mesh/volume without a blocker. Build is `build-preview-release`, Release/private
Qt 6.11.2/OCCT 7.9.3, `-j4`; external evidence is under
`../Alloy-reference-records/validation/skt177-2026-10-20/`. SKT-177/#700 is
closed/Done. Circular canvas picks/manipulators are accepted under SKT-182/#705;
rectangular canvas input is accepted under SKT-178/#701. Broader seeds, Symmetric and
suppression remain ADV-005. Physical/native DPR1 and packaged Windows/Linux
runtime remain separate; no CI or release was run. Private fixture editors
closed; the preexisting user-owned Alloy PID 72618 was preserved.

## 2026-10-20 SKT-097 rectangular Pattern acceptance

[Bounded contract](validation/sketch-rectangular-pattern.md) records one retained
ordinary circle, two explicit perpendicular construction Lines, 2 × 3 total
instances and Spacing / One Direction. Source-only PlaneGCS, UUIDv5 `(i,j)` result
identities, source edits, owner-safe mutation refusal and explicit downstream
profile protection are implemented. Core and real-entrypoint six-hole Cut/STL
checks pass. Native actual-hit controls/glyph edit, numeric spacing, distinct
Create/Edit history and Apple M4 DPR2 resize/captures pass (3/3 Qt functions,
2.617 s); the final 73-call API run and independent six-hole STL readback pass.
Independent Astra review accepted the bounded source, saved-ID and six-hole STL
evidence. External evidence is under
`validation/skt097-2026-10-20/`; accepted configuration is
`build-preview-release`, Release/private Qt 6.11.2/OCCT 7.9.3, `-j4`.
Final external runs: `native-candidate/` and `api-candidate/`; prior failed
qualification receipts remain separate. Both private editors are closed.
SKT-097/#502 is closed/Done. Circular Full/Angle is accepted under SKT-177/#700;
rectangular canvas picking/manipulators are SKT-178/#701 Todo. Wider
seeds/modes/suppression remain ADV-005. No CI or release was run.

## 2026-10-20 SKT-176 centered-list acceptance

[Menu receipt](validation/canvas-marking-menu.md#2026-10-20-skt-176-centered-list-acceptance)
records a content-sized native QMenu list centered below an independent radial
panel. Selected/unselected rows measure 180/165 logical pixels within 356/355
pixel popups, retaining icon/label/shortcut columns and real action hit bounds.
The focused real-app Qt fixture passes 4/4 with ordinary 1500×950 and narrow
1050×750 window captures, canvas-corner bounds/hit checks, direct Backtab and
full construction/circle/history/reopen coverage. External final evidence:
`validation/skt176-2026-10-20/final2/`. Independent Sol review accepted the
bounded menu layout. Failed resize keyboard-focus probes are preserved separately without
interstep focus repair; narrow checks use actual center-button dismissal.
Apple M4 DPR2 only; physical/native activation remains SKT-171/#694. No CI or
release. Private editor closed after validation.

## 2026-10-20 SKT-175 bounded visual/access acceptance

[Menu receipt](validation/canvas-marking-menu.md#2026-10-20-skt-175-bounded-visualaccess-acceptance)
records original icons, transparent compact radial targets, screen clamping and
explicit radial/list keyboard traversal. `build-preview-release` real-app fixture
passes 4/4 in 19.3s after the review-requested direct Qt Key_Backtab correction; component actual-hit checks cover all command buttons/eight
gesture octants/accessibility interfaces, while real Alloy covers keyboard Line,
five tool clicks and the full retained bracket route. External captures/receipt:
`validation/skt175-2026-10-20/backtab/`. DPR2 Apple M4 and actual focus-state
captures inspected; physical hover/input/VoiceOver and native DPR1 remain
unqualified. Lower list still spans radial width, unlike the narrower centered
public screenshot: SKT-176/#699 retains that Todo remainder. Independent Sol
review accepted the Backtab correction; no CI/release. Private windows closed.

## 2026-10-20 SKT-173 active-sketch menu acceptance

[Canvas marking-menu receipt](validation/canvas-marking-menu.md) records a
canvas-owned radial/context surface for active-sketch Select: existing geometry
actions plus eight supported Sketch controls, click and held-right Line/Circle
gestures, accessible IDs and live command/bounds diagnostics. The focused real-app
Qt fixture passes without test-injected context events or interstep focus repair;
original bracket geometry/history/reopen remain covered. Native AppKit activation
was false and remains SKT-171/#694, with the initial failed receipt preserved.
Build is `build-preview-release`; external evidence is
`validation/skt173-2026-10-20/qt-final/`. Independent Astra review accepted
the bounded Qt component. Broader contexts remain SKT-174/#697 Todo and
visual/access refinement SKT-175/#698 Todo; the illustrated TUT-003 gate depends
on SKT-175 and SKT-171. No CI or release.

## 2026-10-20 SKT-172 bounded visual acceptance

[Bracket A2 visual receipt](validation/bracket-visual.md) records supported
linear context-menu grouping, blue authored sketch-curve selection and local
midpoint/tangent badge anchors with shared hit bounds. Independent Sol review
accepted the Qt actual-hit, focused core and Apple M4 GPU evidence from
`build-preview-release` and external `validation/skt172-2026-10-20/qt-final2/`.
Full public marking-menu shell/layout remains mismatch SKT-173/#696; native physical
activation/right-click remains SKT-171/#694. Neither is silently accepted.

## 2026-10-20 SKT-170 bounded bracket end-circle acceptance

The bounded [construction/midpoint receipt](validation/bracket-midpoint.md)
records a passing compiled Qt actual-hit A2 conversion/circle route and a
separate real-executable cap100-to120 edit, exact Undo/Redo and save/reopen.
The sole product change exposes Normal/Construction in context diagnostics.
Strict native mode lost AppKit activation before the upper tangent input and
stopped without repair; its cause is unproven, so native/physical acceptance
remain unqualified under SKT-171/#694, while SKT-172/#695 owns the observed
menu, selection-color and badge-placement differences. Independent Sol review
accepted the Qt/API/diagnostic increment; TUT-003 depends on both Todo children.
Build is
`build-preview-release` Release/private Qt; evidence is external under
`validation/skt170-2026-10-20/`. Both private editors were closed.

## Typed Angle/Cylinder Height accepted (2026-10-20)

PAR-009/#497 adds numeric degree user parameters (`valueDeg`, stable UUID),
driving line-pair Angle and primitive Cylinder Height binding through shared
native/API commands and the grouped model table. Existing length helpers exclude
angle rows; length expressions refuse them. Exact combined R10 cylinder/sector
Cut source edits30→60° and height10→15, two-way type refusal, rename, numeric
unbinding, history/reopen and one-component watertight STL readback pass.
Focused core/length/model regressions and Qt native controls pass4/4 targets.
Independent Astra review found one native User Parameters edit defect: the
displayed `30 deg` literal could not be edited as `60 deg`. The focused fix and
actual-hit cell regression pass, including `2 in` length-cell preservation;
Astra accepted the corrected bounded result.
Apple M4 DPR2 GPU solid/table captures are inspected; physical OS input,
other fields/angle formulas and packaged-platform runtime remain separate.
See [receipt](validation/typed-angle-cylinder-parameters.md); evidence and bound
model/STL are external at
`../Alloy-reference-records/validation/typed-angle-cylinder-2026-10-20/`.
No paid CI or tag was run.

## Analytic sketch Break accepted (2026-10-20 UTC)

SKT-169/#617 accepts one-crossing line/finite-arc
splits and two-intersection circle-to-two-arcs splitting with complete locus
coverage. Shared native/API commands provide red-X hover preview, repeat picks,
Enter/Escape, atomic refusal and explicit annotation-retirement diagnostics.
Core, real-app API/profile/history/reopen and actual-hit Qt native checks pass;
independent Astra source/evidence review found no blocker. The circle's endpoint pairs are
independent IDs to avoid redundant radial equations; no live relation to the
crossing boundary is added. See [receipt](validation/sketch-break.md).

# Continuation context

## Current v1 delivery direction (2026-10-19)

SOL-066/#511 accepts one retained symmetric To Next Rib from a straight open sketch Line into a closed solid with a full-cover perpendicular planar stop. The native Create/Profile/Target/typed thickness/preview/Cancel/OK/Timeline/Flip/Escape test, focused exact core/refusal tests and independent real-app U-channel thickness/source-plane/wall edit, Undo/Redo/reopen and one-component watertight STL readback pass. Astra's earlier-Rib repin finding was fixed with a passing two-Rib regression. Source 5,600 → rib 7,472 → edited stop 7,328 mm³; right orthographic Apple M4 DPR2 GPU capture shows the shelf. See [receipt](validation/solid-rib.md). Web and broader Rib options stay ADV-006; canvas pick/drag is SOL-097/#693, and physical input and packaged-platform runtime remain release gates. No CI run.

PRT-004/#166 accepts one explicit committed open surface body as binary/ASCII STL with persistent non-solid warning, shared serialized preview/units/refinement and independent quarter-cylinder mm/in triangle/area/boundary readback. Native Qt Preview and GPU captures pass; authored model/Undo/saved bytes remain unchanged. See [receipt](validation/open-surface-stl.md). Arbitrary surface mesh connectivity, mixed batches, native Save chooser and packaged-platform input remain separate gates. No CI run.

DOC-037/#521 accepts a transient two-closed-solid Interference/clearance inspector with shared local API and native Inspect panel. Exact 32 mm³ overlap, 2 mm clearance, touching, edited 48 mm³ overlap, Undo/Redo/reopen and no authored mutation pass independent real-app and focused core checks; compiled actual-hit Inspect menu/two canvas picks/Compute/Clear/Close pass. Apple M4 DPR2 whole-app capture shows GPU orange overlap and a readable panel. See [receipt](validation/interference.md). Wider group/fit analysis stays DOC-014; physical input, matched reference layout and Windows/Linux packages stay release gates. No CI run.

SOL-061/#506 accepts one retained hollow circular Pipe along an authored tangent Line/Arc path. The native Create-menu/path/typed-diameter/preview/Cancel test, focused Pipe/Sweep core tests and independent real-app diameter/wall/source edit, refusal, history/reopen and watertight STL readback pass. Apple M4 DPR2 captures show two annular openings and a readable dock. See [receipt](validation/solid-pipe.md). Other sections, extents, operations and path families remain ADV-005; physical OS and Windows/Linux packaged input/GPU qualification remain GATE-050. No CI run.

PRT-016/#692 accepts Parent Folder Browse, safe new-folder name and wrapped exact destination/filename preview in per-body Save As Mesh. Compiled actual-hit Browse/chooser request/Qt Cancel and displayed-path-to-shared-Save checks pass 11/11; independent 52-response real-app STL/3MF two-file readback, destination refusal, model/history/Undo and saved-byte preservation pass. See [receipt](validation/batch-destination.md). Physical native chooser, focus and sheet-over-viewport polish remain PRT-014/#689 while the Mac console is locked; Replace All remains PRT-015/#691. No CI run.

PRT-012/#644 accepts explicit selected-body One File/One Body Per File binary/ASCII STL and Core 3MF export. Per-body output is staged off-thread and published as one new directory; existing destinations refuse unchanged. Independent real-app four-mode readback checks selected-set isolation, 3MF placement, deterministic names and preserved document/files; focused core tests cover preapproval Cancel, late failure and racing destination. Final local `-j4` build and Qt controls 10/10 pass. See [receipt](validation/batch-mesh-export.md). Native Browse/new-folder UI remains PRT-016/#692, physical sheet and viewport presentation PRT-014/#689, existing-directory Replace All/platform transaction breadth PRT-015/#691. No CI run.

PRT-011/#643 accepts an explicit selected-solid STL/Core 3MF mesh preview with Alloy Low/Medium/High/Custom tolerances, serialized triangle count, GPU facet overlay and atomic byte-identical Save. Independent six-case real-app readback checks 112→956 triangles and decreasing chord error on a translated curved part, plus ASCII inches, Core 3MF placement, stale-token refusal, Cancel and retry. Local `-j4` Release build and focused Qt 9/9 pass; see [receipt](validation/mesh-refinement.md). The centered native sheet obscures much of a part; physical chooser/pointer and 1×/2× presentation remain PRT-014/#689 while the console is locked. Maximum Edge Length/Aspect Ratio remain PRT-013/#690, and selected-body batch structure remains PRT-012/#644. No CI run.

GEO-027/#641 accepts a retained construction Point from one semantic Box edge endpoint or an in-range Proportional/Physical location on an authored straight sketch Line. Focused C++/Qt and independent 186-call real-app checks confirm exact source identity, analytic edit behavior, atomic refusal, Cancel/OK, history/reopen and Box-only STL readback. Apple M4 DPR2 captures show visible blue/orange GPU markers after a display fix; see [receipt](validation/datum-point.md). Physical actual-hit selection/distance handle and 1×/2× native gate remain first-v1 Todo GEO-035 while the Mac console is locked; curved paths GEO-036, broader vertices/snaps GEO-037 and public past-end conflict GEO-038 stay Todo. GATE-050 includes GEO-035; no CI run.

GEO-026/#640 accepts a retained datum Axis from two authored standalone sketch Points or one straight authored sketch Line, with exact source IDs, origin/direction/span inspection and compiled source dock. Focused C++/Qt plus independent 133-response real-app checks confirm analytic geometry, source edit/refusal, Cancel/OK, visibility, history and reopen. Apple M4 DPR2 captures show readable axes; preview/committed Axis overlaps its blue source Line, so distinct source cue and physical actual-hit selection remain first-v1 Todo GEO-031 while the Mac console is locked. B-rep edge GEO-032, Extent Type/Resize GEO-033 and vertex/snap GEO-034 remain Todo. See [receipt](validation/datum-axis.md). GATE-050 includes GEO-031; no CI run.

GEO-025/#639 accepts a retained authored straight-Line Plane At Angle core/API and compiled numeric dock. Independent 72-response real-app and focused C++/Qt checks confirm analytic ±30°/45° frames, ordered source identity, source edits, downstream sketch, atomic refusals, Cancel/OK, Undo/Redo and save/reopen. Apple M4 DPR2 captures show readable numeric controls and distinct oblique planes; the pivot line is not distinctly highlighted. See [receipt](validation/angled-plane.md). Physical native canvas selection/rotation handle and 1×/2× qualification remain first-v1 Todo GEO-028 while the Mac console is locked; B-rep edge/face references GEO-029 and Offset/Extent/Resize GEO-030 remain Todo. GATE-050 includes GEO-028; no CI run.

SRF-035/#638 accepts nine-sample numeric G0/G1 inspection of two retained planar faces sharing one exact straight edge. Focused C++ and independent 75-response real-app checks confirm analytic 0 mm/0° coplanar and 0 mm/45° kinked values, retained refs, explicit Alloy 0.001 mm/0.1° thresholds, mm/in presentation, passive model/history, edit/Undo/Redo and exact numeric reopen. Initial panel capture and centered placement defects were fixed in ea82da0/e7d9b15; targeted independent Apple M4 DPR2 captures keep the seam visible at 1280×900 and 1100×760, though the narrow view partly covers one face. See [receipt](validation/surface-continuity.md). Physical actual-hit selection/panel and 1×/2× native qualification remain first-v1 Todo SRF-054 while the Mac console is locked; distinct-edge gap SRF-052 and curved boundaries SRF-053 stay Todo. GATE-050 includes SRF-054; no CI run.

SRF-034/#637 accepts transient single-selected-face Zebra Analysis as an Alloy first slice. Implementation 8c78eef and source-edit occurrence repair 59a132b pass an independent 114-response real-app fixture, exact on/off framebuffer restoration, authored/history and reopened-BRep invariance, source edit/Undo/Redo/reopen and inspected Apple M4 OpenGL DPR2 captures. The shader gives distinct curved bands; a flat orthographic plane has one uniform reflection band. See [receipt](validation/zebra.md). Public instructions describe a saved body-level Analysis with settings, retained in SRF-051. Physical actual-hit Inspect and 1×/2× qualification remain first-v1 Todo SRF-050 while the Mac console is locked; numeric continuity is separate SRF-035. GATE-050 includes SRF-050; no CI run.

SRF-033/#636 accepts transient selected-face normal inspection for one planar or cylindrical face. Focused core plus independent 86-call real-app and reopened-BRep D1 checks confirm exact oriented samples, source recompute, passive history/save/reopen behavior and readable Apple M4 OpenGL DPR2 captures after resize. The display is an explicit Alloy inspection proposal; public evidence does not establish exact native vector-glyph appearance. See [receipt](validation/face-normals.md). Physical actual-hit Inspect input and 1×/2× visual qualification remain first-v1 Todo SRF-049 while the Mac console is locked; wider inspection remains SRF-008. GATE-050 includes SRF-049; no CI run.

SRF-032/#635 accepts one-face External Edges Untrim of a retained planar crossing-sheet Trim. Independent 81-response real-app and saved-BRep checks confirm 800→400→800 mm² and source-width 1000→500→1000 mm², identical restored/pretrim BRep hashes, original face/four-edge IDs, unchanged tool, atomic refusal/retry, Undo/Redo/reopen and inspected Apple M4 DPR2 captures. This finite restoration is an Alloy history policy, not an exact reference-product claim. See [receipt](validation/surface-untrim.md). Physical native selection/controls remain first-v1 Todo SRF-048 while the Mac console is locked; broader modes stay SRF-005. GATE-050 includes SRF-048; no CI run.

SRF-031/#634 accepts a one-edge Natural distance Extend core/API on a rectangular planar Patch sheet. Independent 87-response real-app and saved-BRep checks confirm an in-place 40×30→40×50 mm sheet at 20 mm distance, exact 2000 mm² area, width40→45 mm source edit to 2250 mm², unchanged prior-stage source BRep hash, retained body/face/four-edge IDs, atomic refusal/retry, Undo/Redo/reopen and inspected Apple M4 DPR2 captures. See [receipt](validation/surface-extend.md). Native actual-hit selection/drag and a visible-source profile overdraw artifact remain first-v1 Todo SRF-047 while the Mac console is locked; target termination SRF-045, curved self-intersection refusal SRF-046 and broader variants SRF-005 stay Todo. GATE-050 includes SRF-047; no CI run.

SRF-030/#633 accepts one retained straight planar-face boundary edge as a Normal Ruled flange core/API with numeric dock and a bounded canvas-pick API route. Independent 96-response actual-app and seven saved-BRep checks confirm exact 40×20 mm 800 mm² sheet at 0/±30/90/120/210°, stable source/face/edge identity, 40→45 mm source edit to 900 mm², atomic refusal/retry, Undo/Redo/reopen and inspected Apple M4 DPR2 captures. Public evidence establishes Normal 120° usability, but positive-angle side/defaults remain Alloy policy/unknown parity. See [receipt](validation/surface-ruled.md). Physical native pick/fields and distance/angle handles remain first-v1 Todo SRF-044 while the Mac console is locked; wider Ruled families stay SRF-004. GATE-050 includes SRF-044; no CI run.

SRF-042/#665 accepts one retained partial-cylinder signed Surface Offset core/API and compiled numeric dock. Independent 71-response actual-app and separate saved-BRep probes confirm R10/H30/120° source to R12/R6/R1 for +2/−4/−9 mm, exact area/lineage, R12/H40 source recompute, zero/near-collapse, structured atomic -10/-11 refusal/retry, Undo/Redo/reopen and inspected Apple M4 DPR2 captures. See [receipt](validation/surface-offset-cylinder.md). Curved native face pick/normal drag/actual-hit/1×/2× acceptance remains first-v1 Todo SRF-043 while the Mac console is locked; planar native SRF-041 and Ruled native SRF-044 are also open. GATE-050 includes these native gates; no CI run.

SRF-029/#632 accepts one retained planar-face signed Surface Offset core/API and compiled basic dock. Independent 84-response real-app checks +5/−2/zero separation from a z20 Patch, exact area/bounds/source preservation, source width edit through a downstream sheet, atomic invalid/non-retained curved source refusal, Undo/Redo/reopen and inspected Apple M4 DPR2 captures; a focused 8-response follow-up checks exact source IDs on stale errors after 06a44ac. See [receipt](validation/surface-offset.md). Native planar face pick/normal drag/actual-hit/1×/2× acceptance remains first-v1 Todo SRF-041 while the Mac console is locked. Cylindrical collapse is accepted under SRF-042; broader families remain SRF-004. GATE-050 includes native SRF-041/043; no CI run.

SRF-028/#631 accepts a bounded retained nonplanar four-Line G0 Patch core/API with one bilinear face and actual output boundary/face-iso certificates. Independent 64-response real-app verification checks a skew XY/YZ loop with determinant 12000→18000 mm³, whole-boundary deviation bound 2.22×10⁻¹⁵ mm versus the declared 0.01 mm gate, area 667.266502→727.170867 mm² against independent integration, stable sources, atomic refusal, Undo/Redo, save/reopen and inspected Apple M4 DPR2 captures; see [receipt](validation/surface-nonplanar-patch.md). Native canvas four-edge/1×/2× acceptance remains first-v1 Todo SRF-040 while the Mac console is locked. General B-rep edge networks and higher continuity remain SRF-003. GATE-050 includes SRF-040; no CI run.

SRF-027/#630 accepts one retained simple ordinary-Line loop as a planar Patch sheet New Body. Independent 93-response real-app verification checks one face/closed wire/zero solids, exact 1200→1500 mm² area and 140→160 mm perimeter, +Z normal and zero source-boundary deviation, stable IDs, invalid atomic refusal/retry, Undo/Redo, save/reopen and inspected Apple M4 DPR2 captures; see [receipt](validation/surface-patch.md). Public canvas closed-profile picking and actual-hit native/1×/2× acceptance remain first-v1 Todo SRF-039 while the Mac console is locked. Nonplanar fill remains SRF-028; wider loops/continuity remain SRF-003. GATE-050 includes SRF-039; no CI run.

SRF-026/#629 accepts a bounded open-line Surface Sweep core/API and compiled basic selector dock on retained Line or Arc paths. Independent 194-response real-app verification checks exact 400→700 mm² line and 100π→280π/3 mm² arc areas, source/path/G0 endpoint retention, atomic invalid refusal/retry, Undo/Redo, save/reopen and inspected Apple M4 DPR2 captures; see [receipt](validation/surface-sweep.md). The home isometric view is partly edge-on for the Arc fixture. Public canvas Profile/Path picking and actual-hit native/1×/2× acceptance remain first-v1 Todo SRF-038 while the Mac console is locked. Guides, multi-entity paths and wider extent/continuity modes remain SRF-002. GATE-050 includes SRF-038; no CI run.

SRF-025/#628 accepts a bounded open-line Surface Revolve core/API and compiled basic dock: one retained section and distinct coplanar construction axis generate a valid uncapped non-solid sheet with exact 200π→100π→150π mm² fixture areas. Independent 66-response real-app verification covers retained body/face/boundary IDs, angle/Reverse/source edits, atomic invalid refusal/retry, Undo/Redo, save/reopen and inspected Apple M4 DPR2 captures; see [receipt](validation/surface-revolve.md). Publicly documented canvas curve/axis picking and angular drag are not yet implemented, so native actual-hit/1×/2× acceptance remains first-v1 Todo SRF-036 while the Mac console is locked. Connected multi-curve sections remain first-v1 Todo SRF-037; wider axes/extents and combinations remain SRF-001. GATE-050 includes SRF-036; no CI run.

SOL-089/#627 accepts the bounded Distance And Angle solid Chamfer core/API and compiled dock on one retained straight extrusion endRim. Explicit incident reference face fixes direction; the declared Alloy convention yields 2 mm / 30° setbacks of 2 and 3.464101615 mm. Independent 133-call actual-app verification covers reversed face, downstream blind Cut, separate distance/angle edits, invalid refusal/retry, upstream 30→36 mm source edit, Undo/Redo, save/reopen and independent closed STL. Focused Equal/Two/Distance Angle C++ tests pass. See [receipt](validation/solid-distance-angle-chamfer.md). Native actual-hit/1×/2× GPU acceptance remains first-v1 Todo SOL-096/#658 while the Mac console is locked; current reference UI angle datum/face gesture remain unknown. GATE-050 includes SOL-096. No CI was started.

SOL-088/#626 accepts Two Distance Chamfer on one retained straight extrusion rim with explicit adjacent reference face, independent millimeter setbacks and Flip. Exact cross-section, downstream unchanged-face blind Cut, edge-change side reset, atomic invalid refusal, Undo/Redo/reopen and independent STL pass focused core and 89-call real-app API; unchanged Equal Distance core/API regressions also pass independent Sol review. See [receipt](validation/solid-two-distance-chamfer.md). The native dock is implemented but actual-hit/1×/2× GPU acceptance remains first-v1 Todo SOL-095/#657 while the Mac console is locked. Generated-edge naming/tangent chains remain SOL-005.

SOL-087/#625 accepts one-body uniform Scale about explicit document origin with positive factor, exact preserved body/face/edge identities, normalized frames and factor-cubed volume. A downstream Offset Face retains its exact scaled-face reference and incompatible shrink edit refuses atomically. Focused core plus Align/Draft regressions and 65-response real-app/independent STL checks pass independent Sol review; see [receipt](validation/scale.md). Native Body/Origin controls, Timeline edit and GPU visual acceptance remain first-v1 Todo SOL-094/#656 while the Mac console is locked; nonuniform, custom-pivot and multi-body modes stay SOL-013.

SOL-086/#624 accepts a retained one-face signed Draft core/API on a supported rectangular prism about a cap-coincident construction plane. Exact face lineage and rotated support frame carry a downstream sketch/Cut through angle edit; ±angle/pull, hinge, analytic volume, 80° atomic refusal, Undo/Redo/reopen and independent watertight STL pass focused core plus a 70-response real-app fixture. Astra source review found no geometry blocker; see [receipt](validation/solid-draft.md). Changed edge references are explicitly unavailable. Native controls, Timeline edit and GPU visual acceptance remain first-v1 Todo SOL-093/#655 while the Mac console is locked; broader Draft families stay SOL-013.

SOL-085/#623 accepts the retained rectangular Body Pattern core/API: one root body, one/two nonparallel retained straight-edge directions, positive spacing, index-stable row/grid copies, independent selection, source-edit recompute, atomic dependent-instance refusal, Undo/Redo/reopen and independently read eight-component STL. Focused core, existing feature-pattern regression and 53-call real-app API pass independent Sol review; see [receipt](validation/rectangular-body-pattern.md). Native Bodies mode, actual-hit axis/quantity/spacing controls, visible grid preview, Timeline edit and 1×/2× GPU acceptance remain first-v1 Todo SOL-092/#654 while the Mac console is locked. Broader distributions and reference families remain SOL-009.

SOL-084/#622 accepts a retained planar-face Align core/API with exact source/target face references, a declared retained-UV frame rule, Flip, disposable preview, source-ID/target-body preservation, source-edit recompute, transported face/edge lineage, Undo/Redo and reopen. Focused core and 38-call real-app tests pass independent Sol review; see [receipt](validation/align.md). Native Modify/From/To controls, visible frame cue, Timeline edit and 1×/2× GPU acceptance remain first-v1 Todo SOL-091/#653 after the Mac console locked. Broader references, Angle modes and assembly placement stay SOL-008.

SOL-083/#621 accepts a frozen `bodyCopy` root-solid core and Browser Copy → root Bodies Paste. Explicit XYZ preview/Cancel/OK shares the local API; no live source dependency or copied face/edge lineage is retained. Focused core and an 80-response real-app geometry/history/reopen/STEP/STL fixture pass independent Sol review; see [receipt](validation/body-copy.md). Copy freezes before subsequent source edits, and copied solids accept independent body-targeted Cut/Move. The native actual-hit test builds but strict initial activation is blocked by a saved `consoleLocked=true` preflight; required child SOL-090/#652 stays Todo for unlocked native selection/typing/Cancel/OK and 1×/2× GPU captures. Rotation/manipulators, initial placement edit UI and broader copy ownership/transfer remain SOL-008.

SOL-080/#618 accepts symmetric Half/Whole Length and independent Two Sides New Body Extrude. Shared preview/evaluation use the shifted prism and preserve positive-cap identity; focused core and 125-call actual-app/API lifecycle with STL readback pass. Native actual-hit panel selection/typing/Cancel/OK/edit and Apple M4 GPU captures pass independent Sol review; see the [receipt](validation/extrude-symmetric-two-sides.md). SOL-081/#619 accepts bounded tapered New Body Extrude with retained signed degrees, convex/concave OCCT draft geometry and stable far-cap support. Focused core, real-entrypoint preview/edit/downstream/history/reopen/STL and actual-hit native controls pass independent Astra review; its clipped panel hint was corrected and recaptured. See [receipt](validation/extrude-taper.md). SOL-082/#620 accepts one open straight-Line Thin Extrude with Center/Side1/Side2 wall placement, canvas selection, same-authority preview, downstream cap Cut, Undo/Redo, reopen and independent STL readback. A 79-call real-app fixture, actual-hit Thin/taper/symmetric native regressions and inspected Apple M4 captures pass independent Sol review. Hidden form rows were collapsed so Operation and hints remain visible; see [receipt](validation/extrude-thin.md). Chains/arcs, side-wall lineage and broader profile/start/extent/operation combinations remain SOL-010.

REL-030/#469 is closed/Done for its [2026-10-20 planning freeze](v1/scope-reconciliation-2026-10-20.md), which selects first-v1 minimums/owners for all 134 proposed rows, including the 27 previously open-owner rows and explicit outside-closure later modes; planning closure does not qualify a build. Its independent all-150-row audit found a transitive 3MF-import contradiction in the first-v1 release graph; the corrected GATE-040/#85, XCH-008/#75 and PRT-002/#76 dependencies require accepted Core 3MF export XCH-011/#524, while later 3MF import/extensions remain XCH-005/#73. The freeze adds omitted native sketch/export, linked-source, Rib, responsiveness and packaged recovery obligations. The [release execution ledger](v1/release-execution-ledger.md) covers all 150 rows and keeps package/platform gates open; REL-054/#615 gates stable v1 promotion on post-package REL-049/#529 intrinsic-cost audit. The [private review site](https://alloy-v1-review.rjevans1.chatgpt.site) is owner-private; browser visual QA remains open. Preview gates are independent. No paid CI was started.

The initial follow-up split published 32 bounded children #616–#647 across document/history, advanced solid, surface, datum, inspection and mesh-export minima. Three Measure children #648–#650, native Timeline child HIS-013/#651, Body Copy native child SOL-090/#652 and Align native child SOL-091/#653 and rectangular Body Pattern native child SOL-092/#654 and Draft native child SOL-093/#655 and Scale native child SOL-094/#656 and Two Distance Chamfer native child SOL-095/#657 bring the canonical backlog to 654. The inventory maps each proposed-v1 row to a reachable owner; its 397-command discovery count remains a dated 2026-10-17 runtime snapshot, not a live count. SOL-064/#509 names single-edge Start/End Variable-radius Fillet as the proposed first advanced mode; Ruled Normal edge-distance/angle and Zebra inspection have dated public documentation in the external knowledge archive. REL-055/#616, DOC-039/#642, DOC-042/#649 and HIS-012/#646 accept bounded cores with explicit native and broader-feature remainders; other new children remain Todo contracts.

REL-055/#616 accepts the existing atomic Save behavior through a deterministic writable-prior-file replacement failure. A focused core regression and 22-response real-app local-control run preserve the prior bytes and authored/history/undo state, then retry and reopen both revisions; independent Sol review found no blocker. See [receipt](validation/failed-save.md). Physical Save chooser input, interrupted large writes, Windows/Linux runtime and migration remain open under their existing owners. No CI was started.

DOC-039/#642 accepts the bounded analytic Measure core: exact edge length, point/finite-line distance, unambiguous right angle, planar/cylindrical face area and active mm/in results through the real application API and an Apple M4 GPU panel. The 52-call fixture and saved-authoring comparison passed independent Sol review. DOC-042/#649 accepts deterministic overlap/duplicate refusal and source-edit/occurrence/reopen integrity through a 147-call real-app fixture; see [receipt](validation/measure-references.md). DOC-040/#648 now accepts native actual-hit Inspect/shortcut/filter/pick/copy/clear/close controls at 1×/2×, unchanged authored saves and GPU captures. Independent Sol review found clipped angle results; a layout fix and fresh captures at both scales resolved it. Physical OS input, Windows/Linux runtime and exact panel parity remain unqualified; DOC-041/#650 preserves wider result and hover-display compatibility. See [analytic](validation/measure.md) and [native](validation/measure-native.md) receipts.

HIS-012/#646 accepts the bounded transient read-only history prefix and latest restoration: Box 20×15×10 followed by top Offset Face +5 preserves one body ID, 3000→4500 mm³ full volume, authored data, revision and Undo state; Save while rolled reopens at latest. Focused core and a 35-call real-app GPU/API fixture passed. HIS-013/#651 now also accepts actual-hit native Timeline menu and Move to End input, with popup activation/closure, camera/read-only checks and clean Apple M4 captures; the earlier standalone probe crash was a stale test-widget pointer after the tile rebuilt. Physical device/other-platform runtime and exact pixel parity remain unqualified, while assembly-history rollback and broader historical authoring remain HIS-003. See [receipt](validation/history-rollback.md). No CI was started.

HIS-011/#645 now accepts retained suppression for Box and planar Offset Face chains: one explicitly suppressed Box makes its dependent Offset unavailable while an independent later Box remains, with dimmed Timeline tiles and separate visibility state. Focused core, a 55-call real-app API fixture, actual-hit right-click Suppress/Unsuppress, Undo/Redo, source editing after restore and suppressed/restored save/reopen passed independent Sol source review and inspected Apple M4 GPU captures. Unsupported feature families/joints/Pin refuse atomically in a suppression-bearing document; broader dependency-family parity remains HIS-003. See [receipt](validation/history-suppression.md). Popup warnings remain a documented native-input limit. A local `ninja -t recompact` cleared the repeated dependency-log recovery warning; no full incremental-build performance claim follows. No CI was started.

SKT-100/#519 has a bounded direct same-line Horizontal/Vertical conflict candidate: a certified existing relation/line/positive length diagnostic, last-valid draft preservation, Select Existing Relation/Delete repair, Vertical retry and length edit. Focused core 2/2 and a 63-call real-app API/GPU/reopen workflow passed independent Astra geometry review; native actual-hit input remains pending while the Mac console is locked. SKT-168/#614 retains diagnosis of rejected edits to accepted driving dimensions and any explicit dimension-only recovery. See [the candidate receipt](validation/sketch-constraint-conflict.md).

SKT-164/#610 accepts retained finite line-intersection inference through native/API sketch-to-Extrude, source-edit and independent Astra review. SKT-165/#611 has a committed direct Toggle Driven/Driving candidate with core, 57-call real-app sketch-to-Extrude, independent Sol and GPU evidence, but remains Todo because a locked macOS console blocked its native actual-hit menu gate before first input. See [its receipt](validation/sketch-dimension-driving.md). SKT-166/#612 separately retains the proposed relation-add conflict recovery. The public evidence distinguishes direct existing-dimension toggle from an offer for a newly added redundant dimension. A fresh audit found SKT-034/#205 had already independently accepted point-to-straight-edge D, two-offset 3.9 mm placement, source-following Cut and native Qt/GPU. SKT-167/#613 now has independent core/API/GPU acceptance for the point-edge-specific Driven offer, but remains Todo for stricter native actual-hit/focus qualification; see [its receipt](validation/point-edge-driven-offer.md). The dated public records are outside git under `../Alloy-reference-records/knowledge/`. Native Save As Mesh OS chooser remains REL-053/#607 Todo after a bounded diagnosis; no CI run is authorized without a fresh user request.

XCH-010/#522 now accepts one-file OBJ export for selected committed closed solids, with mm/in world Z-up groups, atomic background publication and independent parser/Blender 5.1.1 readback after source edit. Native dialog state and publication cancellation pass Qt tests, but physical/native system chooser input is XCH-012/#604: FND-028 now qualifies initial unlocked-session native activation and first input; the export sheet keyboard/native return is accepted in REL-052; the actual OS chooser remains REL-053. XCH-013/#605 retains broader units, one-body-per-file and other body/material options. See [OBJ receipt](validation/obj-export.md). No CI was started.

SOL-067/#512 accepts [retained planar Offset Face](validation/face-offset.md) for one face on one closed source solid. A true OCCT local offset retrims angled adjacent walls; the native Modify menu and dock support actual-hit selection, signed mm/in preview and cancel/commit. SOL-072/#598 adds an original-face anchored GPU arrow: a held native pointer drag crosses positive, zero and negative distance through the same session and numeric field; aligned views present a readable rotate-to-drag fallback. Independent Sol review corrected stale source-face highlighting after zero/failed previews. SOL-073/#599 now preserves qualified exact straight planar edge and boundary identities through the offset. A sloped-wall fixture verifies true retrimmed endpoints; a 59-call real-app replay checks linked Project and downstream Fillet after editing the earlier Offset, Undo/Redo and save/reopen. Native Qt clicks the actual offset edge and previews Fillet on Apple M4 GPU. Astra independently found no core code blocker. GATE-120/#601 now exercises deleted-edge refusal on an exact saved stepped prism: +5 mm ledge offset remains one valid solid, deleted IDs are unavailable, a Fillet refuses atomically, an unaffected edge stays available, and source edit/reopen pass. Split/merge refusals remain source-reviewed without runtime fixtures. SOL-074/#600 now retains exact full circular cap rims on constant-radius cylinders; core +3/−2 and real-app +5→+8 linked Project/Fillet, Undo/Redo, reopen and GPU pass. Local circular Project references now validate without an imported occurrence ID. Noncircular/retrimmed curves remain SOL-075/#602. SOL-076/#603 now qualifies native actual-hit circular-rim selection and R1 preview; the earlier test stall was its unsaved-document cleanup prompt, not the viewport event path. The edit dock accepts a negative distance through `feature.editDialog`; physical timeline-button activation was not demonstrated. The earlier real-app 40-call replay checks a downstream Hole, collapse refusal and STL bounds. Multi-face/tangent modes remain broader roadmap scope. Physical OS input and Windows/Linux runtime are unqualified.

DOC-034/#496 accepts the [active design-unit workflow](validation/display-units.md): an actual-hit Browser Units control, mm/in sketch and Extrude values, converted profile area, unchanged canonical geometry, committed inch-valued upstream edits, Undo/Redo, save/reopen and independently parsed STL bounds. A 63-call real-app replay, Qt control test and inspected Apple M4 captures support this bounded result. DOC-038/#597 accepts [selected-solid Properties](validation/body-properties.md): native Browser entry, active-unit bounds/area/volume, exact copied-instance identity, live refresh and stale-instance feedback. A 38-call real-app replay, Qt actual-hit controls and independent Sol recheck support the bounded result. Physical OS input, other-platform runtime and exact visual parity remain separate.

GEO-006/#309 now fixes the circular-seam provenance collision that blocked both retained bracket checkpoints. Exact OCCT history canonicalizes artificial seams, preserving authored contacts and legacy IDs; 20 original actual-app Opens, edits/reopen, focused core 4/4 and Apple M4 captures passed independent Astra review. See [the receipt](validation/prism-provenance.md). REL-051/#586 now accepts the full-size bracket native sketch→Extrude commit/cancel→immediate Undo→Browser popup Escape prefix, with app-owned Extrude viewport focus restoration and separate API STL/readback after independent Sol replay; see [its receipt](validation/ordinary-bracket-input-prefix.md). REL-012/#415 is Todo/blocked on REL-053/#607 for the actual Save As Mesh chooser and same-fixture export. Strict native diagnosis found the private test application inactive with no key window before modal input, so the earlier Browser-tree focus observations do not yet isolate a product defect; FND-028/#589 now owns genuine initial native activation. FND-028/#589 now passes the unlocked-session strict first-pointer/key prerequisite with direct AppKit key-window/Qt focus agreement and an inspected native capture; independent Sol review found no blocker. The preflight handles macOS omitting session lock keys after unlock. REL-052/#587 now accepts the export sheet keyboard/reopen/Cancel and native editor return after local combo StrongFocus and guarded parent activation; the unchanged strict route passes 3/3. The actual OS chooser Cancel/Save/readback remains REL-053/#607. A bounded native chooser probe reached an actual NSSavePanel, but after Cancel the owning sheet remained Qt-modal with no AppKit key window or Qt focus. An immediate guarded activation did not fix return ownership; the draft was reverted, Save/readback is untested, and the build directory test executable must be rebuilt from accepted source before reuse. See [the diagnosis](validation/native-mesh-chooser-investigation.md). No product activation workaround was needed. Two focus workarounds remain discarded. Exact before/after evidence and an opt-in failing driver are external under ../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/.

CI-009/#427's Windows-only manual retry [39142087387](https://github.com/rje/Alloy/actions/runs/39142087387) at `aa635f6` completed pinned Qt/FreeType/OCCT and linked the adapter/client under MSVC Ninja, then failed at `ClosedBlendShell.cpp`'s private `near` declarations. A first rerun was canceled early when another `near`/`far` pair was found in SketchSolver and renamed at `089bd55`. The user chose to let Windows-only [run 39195713105](https://github.com/rje/Alloy/actions/runs/39195713105) finish; it reached `AlloyWindow.cpp` at Ninja 904/935, where MSVC C1128 requires `/bigobj`. CMake now sets that option for the application target under MSVC, but Windows compile/link still need a future approved run. CI costs money: ask before every further manual dispatch or tag push that triggers CI; no automatic per-commit CI is enabled. Packaging and loaded runtime remain pending; see [receipt](validation/cross-platform-runtime-recipe.md). CI-012/#588 accepts the per-component package/source/load inventory join for the unchanged local macOS arm64 archive after independent Sol review: 265 entries, 55 Mach-O receipt records, 43 observed loaded images and 13 focused tests; see [receipt](validation/runtime-inventory.md). Legacy current-build provenance, Windows/Linux package/load evidence and CI-004 remain open.
The [Linux package preparation](validation/linux-package-preparation.md) confirms the successful CI compile artifact omits runtime library bytes, so CI-007 must stage a relocatable archive inside the build job after CI-004 input admission; no Linux package/runtime claim follows from that artifact.

The independent REL-030/#469 v1 inventory pass found 27 accepted bounded subsets, 32 open owners, 62 broad/undecided proposed-v1 rows and 13 release-qualification rows; these are ownership classes, not a completion percentage. The command inventory now records export-only Core 3MF as the first-v1 minimum, the later intrinsic-cost audit REL-049 after functionality/packaging, and stable Metal/Vulkan gates separately from the disclosed OpenGL preview. GEO-024/#590 owns ordinary native Tangent Plane input after the accepted GEO-004 constructor; FND-028's initial native-session prerequisite is now accepted, while GEO-024's own workflow remains Todo. REL-030 stays Todo until all required rows have finishable owners or explicit later dispositions.

The public support-redefinition review and independent architecture audit found that an upstream deleted-face edit is atomically refused before an invalid sketch can be committed, and generated loop Offset identity also depends on the original boundary. HIS-009/#591 accepts transient failed-edit inspection; HIS-010/#594 adds changed-boundary inspection; SKT-154/#580 supplies actionable stale-Delete wording. SKT-160/#592 accepts ordinary root face-sketch repair through explicit earlier parallel face/plane selection, candidate frame/downstream preview, nested Cancel, and one aggregate valid commit with Undo/Redo/reopen; see [receipt](validation/sketch-plane-redefinition.md). SKT-161/#593 accepts the generated loop Offset identity policy and five real-app saved fixtures. SKT-162/#595 now accepts [distinct-face generated Offset recreation](validation/generated-loop-replacement.md): explicit face/distance, fresh generated IDs, one aggregate history step, old-output consumer refusal and stale-preview invalidation. Core 2/2, Qt 4/4, real-app generated API 108 calls, ordinary API 85 calls and Apple M4 GPU captures passed independent Sol review. SKT-163/#596 now accepts [same-face changed-valid recreation](validation/generated-loop-same-face.md) after a real async source edit: exact surviving support, complete changed boundary, explicit signed distance, fresh generated identities and one aggregate commit. Core 2/2, Qt 5/5 plus details 3/3, a 95-call real-app replay and Apple M4 GPU captures passed independent Astra architecture review. Classic/multiple owners, projected relink, arbitrary frames, old-output consumer repair and physical native input remain open. Public behavior/unknowns remain outside Git at ../Alloy-reference-records/knowledge/specs/skt161-generated-loop-offset-repair-policy-2026-10-18.md.

The SKT-154/#580 reachability audit established a distinct changed-boundary failure on a saved valid model: the prior Combine edit returned `ambiguous_inset_lineage` before HIS-009 opened, so deleting the profile in the accepted sketch would test the wrong boundary. HIS-010/#594 accepts narrow changed-boundary inspection with exact failed-prefix Delete refusal; see [receipt](validation/failed-boundary-inspection.md). SKT-154/#580 accepts actionable recovery text in shared Delete for both failures while preserving stable codes, diagnostics, session and document; see [receipt](validation/sketch-loop-offset-recovery.md). Ordinary plane reassignment is now accepted under SKT-160; generated-boundary repair remains open. The [external audit](../../Alloy-reference-records/knowledge/specs/skt154-reachability-after-his009-2026-10-18.md) retains the original reachability evidence. Physical native input remains unqualified.

The user reviewed the private printable-part v1 design as a reasonable working
direction and asked that partial work stop accumulating. REL-030 remains open
for final scope reconciliation after a [150-row inventory audit](v1/inventory-reconciliation.md)
identified broad owners and release-gate gaps. GATE-050/#86 and REL-018/#421
now explicitly require packaged STL/STEP/3MF from an edited, reopened modeled
external-thread fixture; actual package evidence remains open. Work one bounded
active issue to its stated acceptance, create linked Todo children for any
explicit remainder, and move the board item to Done only after real-app
evidence. DOC-013 local New/Open/Save As/dirty-close now has real API and
native-dialog evidence, including API inspection/answering of an open native
decision; see [the receipt](validation/document-local-files.md).
The 2026-10-17 ownership pass added 20 bounded Todo implementation issues
[#496–#515](backlog/github.md) to the v1 inventory and GitHub project board.
They cover proposed tools previously linked only to broad capability families
or completed partial children. Their contracts/fixtures and final v1 release
disposition remain open under REL-030. Six additional Todo owners
[#518–#523](backlog/github.md) cover previously unowned sketch
deletion/diagnosis/intersection, interference inspection, OBJ output and
active-preview visibility responsiveness. XCH-011/#524 adds a bounded required
3MF export owner. SKT-102/#525 accepts authored ellipse/conic/fit/control-spline Delete with native/API, exact history/reopen and a downstream solid; see [its receipt](validation/sketch-authored-curve-delete.md). SKT-141/#567 now accepts whole-owner Slot, Arc Slot, Polygon and Chamfer Delete through core, real-app API and Qt-delivered native routes, with exact Undo/Redo/reopen and shared-identity preservation; see [its receipt](validation/sketch-constructor-group-delete.md). Chamfer left-click selection remains unqualified; context and list Delete pass. SKT-142/#568 now accepts line/arc Mirror and open-chain Offset Delete with core, real-app API, native Qt, Undo/Redo/reopen and Apple M4 captures; see [its receipt](validation/skt142-mirror-openchain-delete.md). SKT-144/#570 now accepts whole-owner projection-backed rectangle Offset Delete, exact prior source-profile restoration, adjacent-owner/support retention and native/API/history/reopen/GPU evidence; see [its receipt](validation/sketch-closed-rect-offset-delete.md). SKT-145/#571 accepts single-owner support-face loop Offset Delete: both schemas pass focused core; the classic annulus passes real-app API/native selection and Delete, support edit, independent-sketch preservation, exact history/reopen, generated-profile consumer refusal and Apple M4 captures. Independent Sol review found no blocker; see [its receipt](validation/sketch-loop-offset-delete.md). SKT-152/#578 now accepts planar-inset real-app/native qualification on the retained 18-edge enclosure support, exact source-edit/profile identity, history/reopen and generated-profile consumer refusal with Apple M4 captures; see [its receipt](validation/sketch-planar-inset-delete.md). SKT-153/#579 now accepts two independent classic annulus owners in one face sketch, with exact sibling/source/support preservation, both-order native/API Delete, consumed-profile refusal, history/reopen and Apple M4 captures after independent Astra review; both schemas pass core. The classic multi-owner receipt is [here](validation/sketch-multiple-loop-offset-delete.md); SKT-158/#584 now accepts two same-sketch planar-inset owners with real-app/native both-order Delete, source edit, consumed refusal, history/reopen and GPU evidence after independent Sol review; see [its receipt](validation/sketch-multiple-planar-inset-delete.md). SKT-154/#580 actionable stale-support/lineage recovery guidance, now Todo/blocked on SKT-025 support repair after the attempted stale-file fixture was rejected during Open before Delete could run; no source change was retained. SKT-143/#569 now accepts selected retained linked line/circle/point Delete through native canvas/list/context and the shared API: source/support preservation, virtual-point dependency and downstream-profile refusal, restored-context Undo/Redo, save/reopen and Apple M4 captures. Independent Astra functional review found no blocker; its observed palette diagnostic/helper overlap was corrected and checked in final native/ordinary captures. See [its receipt](validation/sketch-linked-projection-delete.md). SKT-155/#581 accepts bounded authored/projected line and circle overlap selection via native hold-to-choose, stable role/ID cues, shared Delete, exact history/reopen and Apple M4 captures after independent Sol review; see [its receipt](validation/sketch-overlap-selection.md). SKT-157/#583 accepts transient hover preview over overlapping authored/projected lines and circles with no selection/history mutation, final-binary native/API checks and Apple M4 captures after independent Sol review; see [its receipt](validation/sketch-select-other-hover.md). Actual Depth/Parents model ancestry remains blocked Todo SKT-159/#585 on DOC-006/#18 after public evidence ruled out Authored/Projected buckets; SKT-156/#582 accepts explicit face-projection groups with exact selected-edge Delete, shared-edge membership, native/API history/reopen and Apple M4 captures after independent Astra review; see [its receipt](validation/sketch-face-projection-delete.md); broad source repair/independent-copy work remains SKT-026/#160. Circle and fit-spline Offset owners are accepted separately below. SKT-148/#574 now accepts whole-owner fit-spline Mirror Delete with native canvas/list/context, exact source/tangent/axis retention, history/reopen and Apple M4 captures; see [its receipt](validation/sketch-fit-spline-mirror-delete.md). SKT-147/#573 accepts Circle Offset result/source/association Delete through shared native/API routes, exact history/reopen and Apple M4 captures; see [its receipt](validation/sketch-circle-offset-delete.md). SKT-150/#576 now refuses historical sketch Delete before publishing when a removed curve invalidates an explicitly selected downstream Extrude profile, with stable owner/profile guidance and preserved session/document; unrelated Delete passes native/API, history/reopen and the same solid. See [its receipt](validation/sketch-delete-downstream-preflight.md). SKT-146/#572 accepts fit-spline Offset result Delete with exact endpoint retirement, source/sibling retention, native/API/history/reopen and GPU captures; see [its receipt](validation/sketch-fit-spline-offset-delete.md). SKT-151/#577 now accepts direct result-curve selection through visible source grips while retaining deliberate source-grip dragging, with baseline failure, native/API and GPU evidence; see [its receipt](validation/derived-curve-hit-priority.md). SKT-149/#575 now accepts explicit Break Link for fit-spline Offset: it keeps source/result IDs and endpoints, converts the result to independently editable persisted control poles/knots within a sampled 0.001 mm tolerance, and passes native/API/history/reopen/GPU plus final palette readability review; see [its receipt](validation/sketch-fit-spline-offset-association.md). Exact glyph-input parity remains unclaimed. SKT-103/#526 owns the Polygon's first retained
placement anchor, SKT-104/#527 its canvas annotations, and SKT-105/#528 the
second placement driver and size/orientation conflict contract. REL-049/#529 records the user's sequencing: finish v1
functionality and packaging gates first, then audit intrinsic cost across
supported commands, including simple corrected-iron Cuts, and create/complete
measured optimization children. Off-thread execution alone is insufficient.
The working partial-row v1 disposition is in
[docs/v1/partial-row-disposition.md](v1/partial-row-disposition.md).
SOL-071/#566 now accepts named Combine target/tool consumption and readable
invalid-result recovery through native input, real-app API, history/reopen and
Apple M4 captures; see [its receipt](validation/combine-naming-recovery.md).
SKT-094/#499 now accepts the documented Three-Point Rectangle workflow:
three native points, typed Tab-lock lengths, retained Parallel/Perpendicular
relations, a source-driven corner, downstream extrusion and independently read
STL, with real-app Undo/Redo/reopen and GPU captures. See
[the receipt](validation/sketch-three-point-rectangle.md).
The extended external dated source review found no separate documented
Parallelogram creation mode. SKT-106/#530 disposed that inventory assumption;
ordinary Line and Parallel constraints are the documented primitives, while a
dedicated Alloy constructor would be an optional Alloy-specific proposal.
See [the command record](research/commands/sketch-three-point-rectangle.md).
SKT-095/#500 now accepts the documented Three-Point Circle: three native
circumference picks, disposable preview, retained source-point relations,
source-edit recompute, downstream extrusion, history/reopen and independently
read STL. See [the receipt](validation/sketch-three-point-circle.md). The dated
public command review identified two distinct line-tangent input workflows;
SKT-107/#531 now accepts the bounded 2-Tangent Circle on two authored straight
lines: a native two-line/radius/click workflow, two retained Tangent relations,
source-line and radius edits, history/reopen and actual GPU captures. See
[the receipt](validation/sketch-two-tangent-circle.md). SKT-108/#532 now also
accepts 3-Tangent Circle on a three-edge rectangle fixture including a parallel
pair: native third-pick commit, unique finite-solution policy, three retained
Tangents, source edit, history/reopen and GPU captures. See
[its receipt](validation/sketch-three-tangent-circle.md). SKT-109/#533 now
accepts switching all five Circle modes through the actual Sketch Palette:
unfinished picks reset, committed circles persist, and the next canvas pick
works without focus repair. See [its receipt](validation/sketch-circle-feature-options.md).
The backlog contains 590 issues. SKT-023/#164 has sketch-presentation children SKT-125–128/#550–553 and Fix/UnFix children SKT-133–135/#558–560; later curve/ownership families are SKT-136–138/#561–563. Two-Point Circle Fix is accepted as SKT-139/#564, and the other v1 authored circle modes are qualified by SKT-140/#565; SKT-023/#164 is accepted for its declared v1 analytic scope. SKT-125/#550 accepts per-entity sketch roles and a live Find Under-Constrained list with native selection/zoom, mixed actual-app source edit, history/reopen and Apple M4 captures; see [its receipt](validation/sketch-entity-roles.md). SKT-126/#551 accepts independent sketch visibility, hidden-target pick refusal and session-local display state through actual-app API, native input, reopen and Apple M4 captures; the reopened palette capture was accepted only after its layout settled. See [its receipt](validation/sketch-visibility.md). SKT-127/#552 accepts contextual reveal with Show Constraints off and stable-ID point Delete Coincident through native clicks, exact draft Undo/Redo, save/reopen and Apple M4 captures; see [its receipt](validation/sketch-point-relations.md). SKT-128/#553 accepts the supported glyph/edit inventory through native badge picks, representative deletion/Undo, saved reopen and Apple M4 captures, including a corrected Vertical mark offset; see [its receipt](validation/sketch-glyph-inventory.md). SKT-133/#558 accepts native/API point and authored straight-line Fix/UnFix with retained endpoint locks, atomic refusal, one-step authored history and Apple M4 captures; see [its receipt](validation/sketch-fix-unfix.md). SKT-134/#559 accepts bounded Center Circle Fix/UnFix with retained center/radius locks, native repeat-click and radius conflict/recovery, exact reopen and Apple M4 captures; see [its receipt](validation/sketch-fix-circle.md). SKT-135/#560 accepts ordinary finite Arc Fix/UnFix with retained center/radius/start/sweep locks, native positive/negative sweeps, atomic edit refusal, exact reopen and an edited rounded solid with independently checked volume; see [its receipt](validation/sketch-fix-arc.md). SKT-139/#564 accepts Two-Point Circle Fix/UnFix with retained diameter source/direction, native repeat-click, atomic conflict/recovery, exact reopen and Apple M4 captures; see [its receipt](validation/sketch-fix-two-point-circle.md). Other authored circle modes pass native/API Fix qualification under SKT-140/#565; see [its receipt](validation/sketch-fix-circle-modes.md). Later curve/ownership work is SKT-136–138/#561–563. REL-015/#418 now accepts a packaged macOS
real-app/API/GPU failure-and-recovery slice and fixes a no-op over-thick open
Shell. Five saved fixtures check atomic refusal or unavailable analysis,
unchanged original files, valid edits, Undo/Redo and reopen; see
[the receipt](validation/actionable-failures-packaged.md). Native physical
error discoverability remains Todo REL-050/#539 after computer control failed
to attach to the staged window. This release gate does not establish
clean-host or Windows/Linux behavior.
SKT-022/#163 is split into four bounded children SKT-115–118/#540–543 for rounded-slot arc Symmetry, circle-circle Tangent, interior arc-line Tangent and unsupported-pair feedback; the external dated gap contract separates these from accepted families. SKT-115 accepts the two-arc rounded-slot relation, diameter/spacing edits, downstream solid, native Qt picks and badge, history/reopen and inspected GPU captures; see [its receipt](validation/sketch-arc-symmetry.md). SKT-116 accepts internal/external circle-pair Tangent with radius/center edits, native picks and badge, history/reopen and inspected GPU captures; see [its receipt](validation/sketch-circle-circle-tangent.md). SKT-117 accepts strictly interior arc-line Tangent through source edits, downstream rounded extrusion, native Qt picks/badge and history/reopen; the independent contact segment does not form a single simple wire with the arc. See [its receipt](validation/sketch-interior-arc-line-tangent.md). SKT-118 accepts readable native pair-eligibility feedback, preservation of valid pending picks, and recovery into Concentric with badge, history and reopen; see [its receipt](validation/sketch-constraint-pair-feedback.md). SKT-121 accepts two-circle Symmetry with one remaining translation DOF, diameter edit, conflict refusal, native badge, history/reopen and GPU capture; see [its receipt](validation/sketch-circle-symmetry.md). SKT-119 accepts finite arc-circle Tangent through radius/center edits, out-of-sweep refusal, native picks/badge and history/reopen; see [its receipt](validation/sketch-arc-circle-tangent.md). SKT-120 now accepts two finite arcs through radius/center edits, independently checked common contact, both sweep refusals, native badge and history/reopen; see [its receipt](validation/sketch-arc-arc-tangent.md). The first independent cumulative review found the curved-arc endpoint gap, now accepted under SKT-123/#548 for Arc Start/End to Circle Tangent and SKT-124/#549 for a smooth directed join between two explicit arc endpoints. Both pass source edits, atomic refusal, native picks, history/reopen and Apple M4 captures; see the [circle endpoint](validation/sketch-arc-endpoint-circle-tangent.md) and [arc join](validation/sketch-arc-endpoint-arc-tangent.md) receipts. A second independent Astra audit found a distinct-ID arc/line endpoint Tangent gap, now accepted under SKT-129/#554. The [receipt](validation/sketch-distinct-arc-line-tangent.md) includes a closed edited plate/solid, retained separate IDs, native controls, history/reopen and independent Sol review. SKT-022/#163 is closed for its declared v1 line/circle/arc analytic scope. Later multi-pair Symmetry is SKT-122/#547; spline Tangent/Symmetry and explicit branch-choice research are SKT-130–132/#555–557.
SKT-096/#501 now accepts one Equal Distance
sketch Chamfer on an authored line corner, with native preview/typed edit,
retained source dimensions, extrusion, history/reopen and independently checked
STL. See [the receipt](validation/sketch-equal-chamfer.md). The dated public
review split the other parameter modes into SKT-110/#534 and SKT-111/#535,
both now accepted. SKT-112/#536 accepts adjacent retained Equal Distance
corners sharing a source side, independent values and overlap refusal; see
[its receipt](validation/sketch-multiple-chamfers.md). SKT-113/#537 now accepts
a live on-canvas drag handle and direct first-distance label editing with a
visibly driven second value; see
[its receipt](validation/sketch-chamfer-canvas-edit.md). Provenance is outside the repository with a
[compact command record](research/commands/sketch-chamfer.md).
SKT-110/#534 now accepts Two Distance sketch Chamfer with stable ordered side
assignment, independent retained values, native Feature Options switching,
canvas handles/annotations and checked extrusion/reopen. See
[its receipt](validation/sketch-two-distance-chamfer.md). The dated public
Distance And Angle addendum is outside the repository.
SKT-111/#535 now accepts Distance And Angle for finite connected corners,
including native distance/angle handles, source edit, invalid-angle refusal,
history/reopen and an independently checked solid. See
[its receipt](validation/sketch-distance-angle-chamfer.md). Crossing lines and
ambiguous quadrants remain outside this accepted bounded case and are Todo
SKT-114/#538; all three documented Chamfer modes are now available in Feature
Options.
SKT-093/#498 now accepts Inscribed, Circumscribed and Edge Polygon creation,
retained edits, native numeric refusal, three closed regions, downstream
extrusion, history/reopen and independently read STL. Astra reviewed the
final app captures and accepted the bounded slice; see
[the receipt](validation/sketch-regular-polygons.md). Actual placement relations
were split between SKT-103/#526 and SKT-105/#528. Physical input and
other-platform qualification remain open.
SKT-104/#527 now accepts true circumradius, inradius and fixed-base-edge canvas
annotations with native double-click editing, unit conversion, visibility,
refusal, downstream extrusion and independently read STL. Astra accepted the
actual capture and shared-command route; see
[the receipt](validation/sketch-polygon-annotations.md). Label drag and exact
styling remain unqualified; second-driver relations are accepted below under
SKT-105.
SKT-103/#526 now accepts a genuine first-point origin, authored-point or
retained-line-midpoint relation. Its solver anchor moves the same Polygon and
downstream extrusion under an upstream edit; stable IDs, external constraints,
history/reopen and independent STL passed. Astra reviewed the solver and
native cue evidence; see [the receipt](validation/sketch-polygon-first-anchor.md).
Second-point size/orientation ownership was tracked under SKT-105/#528.
SKT-105/#528 now accepts the retained second point/midpoint in all three
Polygon modes: moving an upstream source changes size and rotation through
the solver, and incompatible numeric values refuse without detaching either
relation. Native cues and source-owned reference annotations, downstream
extrusion, undo/reopen and independently read STL passed; see
[the receipt](validation/sketch-polygon-second-driver.md). Exact modern input
focus/styling, physical hand input and other-platform runtime remain open.
Broad parent splits and the 13 partial rows with accepted subsets are recorded
in [the ownership audit](v1/ownership-audit.md). Final v1 option decisions and
release disposition remain open under REL-030; do not infer readiness from the
larger issue count.
The user confirmed 3MF export and modeled printable threads as required first-v1
capabilities on 2026-10-17. XCH-011/#524 now accepts a bounded Core 3MF export
through Save As Mesh and the shared command: one/two placed objects, millimeter
units, independent mesh/placement readback, source edit/re-export, native chooser
Cancel/Save and separate PrusaSlicer CLI import passed. The [receipt](validation/3mf-export.md)
keeps other platforms, package qualification and extensions separate under
XCH-005. SOL-059/#504 now accepts a bounded retained external modeled Thread
on M6/M8/M10 nominal bosses: native side picking and Escape/OK, full/partial
length, source/edit/history/reopen and independent watertight STL passed. The
[receipt](validation/modeled-external-thread.md) records the measured worker
preparation and preserved-mesh display path. Broader standards, internal modes,
print fit and other-platform qualification remain open under ADV-005 and release
gates.
SKT-099/#518 now accepts ordinary line/circle/arc/Point deletion through the
shared command, canvas Delete and sketch context menu. A real-app retained
profile/extrude/save/reopen gate and a slot-group transactional refusal passed;
see [the bounded receipt](validation/sketch-geometry-delete.md). SKT-102/#525
retains spline/conic/ellipse and grouped/derived Delete. Physical hand input,
other platforms and exact reference cascade/visual parity remain open.
DOC-016/#167 now passes the bounded actual STEP Open and export-sheet controls:
inch STEP, nondefault STL, native Save/Cancel, independent readback and exact
authored/undo preservation. Export sheets were launched through the local API;
DOC-035/#516 now passes physical File-menu entry for STEP and mesh sheets and
one full native-menu STEP export with independent readback; see
[the File-menu receipt](validation/native-exchange-menu.md).
DOC-036/#517 now passes the body Browser right-click > Save As Mesh > native
chooser path with an independently valid STL and unchanged authored/undo state.
The body menu uses a nonblocking popup, matching the viewport body menu; see
[the Browser receipt](validation/native-browser-mesh.md) and
[the native-sheet receipt](validation/native-exchange-sheet.md).
TUT-030/#442 research is independently accepted: the public connector
dimensions remain unknown, while the user-approved nominal-fixture adaptation
under TUT-035 passes measured 0.5 mm clearance to each half. The governing
[final receipt](validation/enclosure-connector-clearance.md) records the actual
−1.5 mm Cut offset and rebuilt R0.4 rim. TUT-016 keeps the full tutorial gate;
physical fit and exact source connector parity remain unclaimed.
DOC-009/#21 now passes its bounded two-body Browser gate: names and IDs,
visibility policy, Browser/viewport body-selection synchronization, exact
undo/redo and native-file reopen. A real-app API workflow plus native captures
received independent review; see [the receipt](validation/browser-document-workflow.md).
Broader assembly Browser and physical interaction parity remain separate work.
REL-037/#480 now has independently accepted saved-iron final Open presentation
evidence. Cached exact body bounds cut the GUI Fit phase from about 70 ms to
9–12 ms; the current-build terminal poll fell from 520–578 ms to about 470 ms.
Concurrent API heartbeat, tab/history identity, exact authored state and native
capture passed. A roughly 0.42 s final UI pause remains measured, while
broader responsiveness stays under REL-016; see
[the receipt](validation/open-final-presentation.md).

GitHub Actions already compiles tag/manual candidates for macOS, Windows and
Linux. [The build/distribution plan](build-distribution.md) distinguishes
temporary build artifacts, relocatable test archives, signing/notarization,
clean-host runtime qualification and promoted GitHub Release assets. CI-010/011
track bounded Mac and Windows signing follow-ups; CI-002–009/GATE-005 retain
bundling and cross-platform runtime gates. No signed public release is claimed.

## Active user order (2026-10-17)

REL-046/#492 is independently accepted for the corrected-iron cumulative
44-stage from-blank replay after REL-047/#493 moved the bounded STL export off
the UI thread. The accepted candidate16 recording has a 686 ms maximum actual
frame gap, responsive pending status/navigation/heartbeat, all 44 captions,
exact authored/evaluated geometry and ancestry, byte-identical STL, checked
STEP, and a postmovie source edit with Undo/Redo and reopen. See
[the cumulative receipt](validation/iron-responsive-cumulative.md) and
[the export receipt](validation/responsive-stl-export.md). The failed candidate15
2.088-second export gap is retained as evidence; the accepted candidate14
functional recording is retained. Physical input, other platforms/models and
broader REL-016 readiness remain open.

REL-045/#491 is independently accepted for the corrected-iron interactive
Surface Trim target click. The original camera/ray/viewport/face intent stays
fixed while private partitioning runs for 18.829 s; after pan and orbit, the
same inner region is red and selected. Status/navigation/GUI heartbeat stayed
responsive, and exact committed geometry/ancestry, one history entry,
Undo/Redo, save and reopen passed. Final publication took 847.838 ms
separately. See [the receipt](validation/responsive-interactive-trim.md).
The actual target pick used the shared API/native handler after one native
Change Target setup click; physical target-pointer delivery and a full movie
remain outside this gate.

REL-043/#489 is independently accepted: Surface Trim preview bounds now ignore
incidental viewport triangulation and match the accepted committed panel
projection/regions/selected geometry. The live meshed and private fresh app
previews, Cancel, full committed authored/bodies, one history operation,
Undo/Redo and native reopen passed. See
[the receipt](validation/mesh-independent-surface-trim.md). Interactive
last-tool selection had blocked for 18.412 s; REL-045/#491 subsequently moved
that interactive partition off-thread. The REL-043 geometry fix alone did not
claim responsiveness for that click.

REL-039/#482 is independently accepted for the corrected-iron Inner handle
no-guide New Body Loft. Third-boundary preview evaluated for 19.149 s on the
worker while status/pan/GUI heartbeat stayed responsive; cached OK took
338.034 ms without a second kernel run. Exact accepted committed geometry,
ordered sources and generated ancestry, one history entry, Undo/Redo and
native reopen passed. See [the receipt](validation/responsive-inner-loft.md).
Guided, new-component and ordinary Loft responsiveness remain Todo under
REL-044/#490; changed historical suffix commit and physical input remain
outside this result. REL-016 stays open.

REL-038/#481 is independently accepted for fully specified Surface Trim panel
partition and final commits on the corrected iron. Panel partition/commit and
connected-rear partition/commit all ran off-thread with responsive status,
pan and GUI heartbeat; exact accepted **committed** geometry/source ancestry,
within-run IDs, one history entry, Undo/Redo and reopen passed. The old live
preview used viewport-meshed bounds and differed from commit; REL-043/#489
subsequently fixed that discrepancy. Interactive target-click cache misses
were outside this bounded gate and later addressed by REL-045; direct Trim API
responsiveness remains separate. See
[the receipt](validation/responsive-surface-trim.md). Cumulative REL-016
remains open.

REL-042/#488 is independently accepted on the saved corrected-iron pre-bottom
checkpoint. One macOS LaunchServices bundle launch produced an active native
window; Sketch Edit itself produced viewport focus without inter-step repair.
The exact bottom line was added, Edit/Finish each evaluated for about 19 s on
the worker with responsive status/pan/GUI heartbeat, and the resulting
three-tool bodies/references, one history entry, Undo/Redo and native reopen
matched the accepted stage. See [the receipt](validation/responsive-historical-sketch-finish.md).
Physical input and the later Extrude/Fill suffix remain outside this bounded
gate; REL-038/039 later passed their bounded paths, while cumulative REL-016
remains open.

REL-041/#484 is independently accepted for the direct no-session source edit
that previously blocked the window for 55.825 s. On the saved corrected-iron
candidate14 model, the same fit-point edit evaluated for 29.598 s off-thread;
status/pan/GUI heartbeat stayed responsive. Exact authored/full evaluated
bodies, stable face IDs, one history operation, Undo/Redo and native reopen
passed. See [the bounded receipt](validation/responsive-sketch-source-edit.md).
Physical input and the other Trim/Loft stalls remain open. REL-042 later
verified historical Sketch Finish on the saved checkpoint.

REL-040/#483 now has an Astra-reviewed historical Sketch Edit worker path and
focused Finish command foundation. On the saved corrected-iron pre-bottom
checkpoint, Edit evaluated for 53.331 s off-thread while status, pan and GUI
timer remained responsive; the published draft matched the source. A stale
full-document operation result initially exceeded the control client's 1 MiB
bound and was corrected. Initial native editor activation failed in those
attempts; [the bounded receipt](validation/responsive-historical-sketch.md)
preserves that limit. REL-042 subsequently passed the exact saved-fixture
Finish through a LaunchServices bundle launch. REL-016 remains open for
cumulative interaction readiness. No private editor remains open.

2026-10-17 latest: TUT-035/#472 is independently accepted and closed for the
approved enclosure connector-clearance adaptation. The 33-feature native model
reopens exactly, both exported halves independently read back as valid solids,
all 24 reference/half pairs have zero positive overlap, and the unchanged
nominal Ethernet connector is 0.5000000000000071 mm from each half. The R0.4
rim after the changed port is explicitly adapted; physical fit and general
upstream reference repair remain unverified (HIS-008). See
[the receipt](validation/enclosure-connector-clearance.md) and the external
gallery's `#enclosure-clearance-fit` stage continuation.

TUT-034 candidate11 commits a valid closed iron solid with exact undo/redo and
native reopen. Independent STL and STEP readback supports a through-handle and
rear web. Astra accepts this bounded solid trial but rejects full shape
acceptance: public plan views show a narrower pointed nose than the current
broad rounded one. Source-edit and cumulative recording gates also remain.
External `deliverables/iron-reassessment-2026-10-17/candidate-11-connected-solid-trial/`
preserves captures, native model and exports. TUT-034/TUT-018 remain open.

Candidate12 changes only the source lower-profile tip tangent weight 200→40,
with the opposite side derived by Mirror. Astra independently accepts the
sharper plan-view nose, full solid, meaningful source edit, exact undo/redo and
native reopen, watertight genus-1 STL and independent STEP web/aperture
witnesses. See [the receipt](validation/iron-corrected-shape.md) and external
`candidate-12-tip-weight40-{admission,solid}/`. Exact source dimensions/camera
and separate-tool parity remain unknown. No further geometry search is needed;
the cumulative from-blank recording, stage ledger and gallery replacement are
the remaining TUT-034/TUT-018 gates.

The iron recording exposed a separate human-readiness blocker: editing the
same per-profile Surface Extrude to append its bottom sheet produced a
119.4-second synchronous preview on the Qt UI thread, with an earlier edit
opening/preview around 81 seconds. REL-033/#476 now has an independently
accepted bounded asynchronous fix. The saved-iron edit opened with a 40.32 s
background evaluation and appended the bottom sheet with a 21.23 s background
evaluation, while status, navigation and GUI timer checks stayed responsive.
Cached Commit took 291 ms; exact geometry, history and reopen passed. See
[the receipt](validation/surface-extrude-responsive-preview.md). This covers
Surface Extrude command sessions, not every modeling operation. Candidate13
did reach the same accepted solid and
save native/STL outputs, but `recording.stop` refused because a UI-thread stall
exceeded 300 frame slots. It exited nonzero and closed its private editor; no
accepted movie or gallery replacement exists. Preserve external
`candidate-13-from-blank-recording/` evidence and
[the failure receipt](validation/iron-creation-recording-blocked.md). Do not repeat the full run merely
to reproduce the stall; implement and verify REL-033 first.
TUT-034/#470 and parent TUT-018/#302 are Todo on the project board while the
remaining UI-thread gate is active; reopen TUT-034 as In Progress for one corrected
recording after both gates pass.
The same failed run also measured a 94.46-second final Boundary Fill commit.
REL-034/#477 now passes independent bounded acceptance: the saved four-tool
commit evaluated for 32.36 s off-thread, with responsive status/pan/GUI timer,
exact accepted solid, undo/redo and reopen. See
[the receipt](validation/boundary-fill-responsive-commit.md). The initial
Boundary Fill cell enumeration now passes independent REL-035/#478 acceptance:
the saved four-tool Select Cells command submitted in 92 ms, ran 29.79 s in
the background, preserved exact cell identity and final solid/history/reopen,
and kept status/pan/GUI heartbeat responsive. See
[the receipt](validation/boundary-fill-responsive-cells.md). REL-036/#479 now
independently accepts background evaluation for interactive native Open on the
same iron fixture: 30.607 s background work with responsive status/navigation,
exact authored/geometry/history/save, and a disclosed 1.279 s final presentation
pause. See [the receipt](validation/responsive-native-open.md).
REL-037/#480 later accepted a bounded final-presentation reduction; its measured
remaining pause is recorded above. TUT-034/#470 now has an
independently accepted corrected from-blank recording: 44 stages, 562 seconds,
2,810 frames, exact final native solid, checked closed genus-1 STL and
independent STEP readback. Meaningful source edit, Undo/Redo and reopen pass.
See [the receipt](validation/iron-corrected-cumulative.md) and external gallery
`#surface-iron-corrected`. TUT-018's adapted tutorial result is restored within
the disclosed source/native-input limits; seven 20–36 s synchronous Trim/Loft/
sketch stalls and a post-movie 55.825 s source edit remain REL-016 work.
REL-038–041/#481–484 split these measured Trim, Loft, sketch and source-edit
stalls into finishable Todo children; recording completeness does not close
the cumulative responsiveness gate.

Finish the current tutorials and prepare a comprehensive web-reviewable v1 plan for dependable 3D-printing creation workflows. TUT-018/#302 is reopened: the user rejects the final iron shape. Prior API/mesh checks prove only the adapted fixture, not visual/tutorial acceptance. Correct the silhouette and sculpted handle against public final tutorial evidence. Full enclosure TUT-016 remains open. The proposed v1 inventory and backlog mapping live in docs/v1; scope is pending user review.

The private proposed-v1 review is published at https://alloy-v1-review.rjevans1.chatgpt.site (REL-030/#469). Versioned docs/v1 includes 150 tool rows and 397 commands cross-checked with runtime discovery. Scope remains pending user review. TUT-034/#470 owns the corrected iron. Its earlier OCCT three-section Loft crash is independently fixed under GEO-023/#471 at b7d88d6; the corrected bridge passes preview, commit, save/reopen and winding checks. Candidate07 also passes a bounded independent panel/bridge shape review. Final rear/underside closure, full silhouette and export remain unaccepted. REL-031 is accepted at e555907: iron hide/show drops from 42–48 s to about 0.34 s by reusing geometry; full geometry edits/reopen remain blocking. See docs/validation/asymmetric-handle-bridge.md and visibility-only-evaluation.md.

User approved a documented minimum 0.5 mm clearance adaptation around the nominal enclosure Ethernet connector on 2026-10-17. TUT-035 owns implementation; leave the reference part unchanged and distinguish measured model clearance from physical fit.

2026-10-17 continuation: candidate07 corrected iron panel and bridge passed independent visual/geometry review, but final closure and silhouette remain unaccepted. Candidate08 preserved that nine-feature source and added fixed rear closing sheets; its first Outer Trim preview safely refused with `trim_region_ambiguous` (finite imprint, no separable region). No Trim, closure, or movie was committed. See external `deliverables/iron-reassessment-2026-10-17/candidate-08-fixed-closing-trials/README.md`; await independent contact/tool diagnosis before another geometry trial.

The approved enclosure clearance edit on the accepted 29-feature pre-rim checkpoint initially failed real-app preview with `Renamed local body is no longer active` because its temporary edit preview retained final `Top` body organization before Split created it. REL-032/#475 fixes that prefix; the focused regression and the real enclosure Cut now pass preview, commit, exact undo/redo and save/reopen (external `deliverables/enclosure-cumulative-2026-10-17/clearance-prefix-03/`). A separate read-only boundary diagnostic found a genuine 18-to-16-edge Split-cap change; the old rim inset guard correctly rejects its old boundary identity. HIS-008/#474 tracks explicit repair. TUT-035/#472 now rebuilds the four rim-tail operations from the accepted pre-rim checkpoint. The inherited R0.5 paired fillet refused on the changed mouth; one disclosed R0.4 trial passed preview and commit but needs clear terminal capture, mating groove, export and measured clearance. Do not claim a printable clearance until the actual app chain, resulting model, exports, and BRep measurement pass.

## Historical evidence (superseded where noted)

The acceptance claim below was withdrawn for TUT-018 on 2026-10-17; shoehorn acceptance remains valid.

### Previous order (2026-10-15)

**User order complete:** TUT-017 shoehorn and TUT-018 surface iron are
independently accepted. See [iron receipt](validation/surface-iron-complete.md):
complete-03 passes224calls/25stages, full243.8s creation movie, enclosed genus1
handle/STL, sourceedit/stablefaces/history/reopen. Gallery entry
`surface-iron-complete` includes both models and STL; local links verified.
No private editor remains open. Existing macOS build contains the implementation;
no package, release tag or CI build was triggered. Native full-workflow gesture
parity, exact reference silhouette and broader Boundary Fill operations remain
qualified/deferred as recorded. Earlier failure chronology below is preserved
for evidence and is superseded by this accepted final result.

TUT-033/#468 corrected the cumulative blocker: cumulative `closure-check-02` produces a
valid watertight solid but cuts the rear handle web, giving a C-shaped slot.
Independent review confirms mesh Euler characteristic2/genus0, not the required
enclosed tunnel. TUT-032 qualifies a bounded cell only; its OUT witnesses do not
prove an enclosed handle. Correct the retained rear closing curves by exact
contact reasoning behind the opening (X>90), then require one closed connected
mesh with Euler characteristic0/genus1 plus actual front/oblique captures.
The corrected rear-wall recipe is independently accepted: one valid8-face solid,
connected closed genus1 mesh, visible rearweb, sourceedit/stablefaces/history/reopen.
See [receipt](validation/iron-enclosed-handle.md). Complete-03 final recording
is underway; do not publish partial complete-01/02 as accepted.

SRF-011 Boundary Fill is independently accepted (`f8c00b4`, `0ecfd85`):
actual iron core,62-call API,native3/3 and corrected preview3/3 pass.
The full TUT-018 recording `complete-01` reached the handle Loft with public
Canvas references, then hit the client response-size bound in `app.state`.
REL-029/#467 is independently accepted at `27c13e5`: only public preview
drafts omit duplicate image bytes, with explicit metadata. Focused API/internal/
authored/save checks pass. The full `complete-02` recording is running. The failed movie/transcript are preserved;
full TUT-018 acceptance and final gallery publication remain pending.

The actual iron fixture found an on-axis shared spline endpoint omission in
Mirror creation. The focused correction `c205138` is independently accepted:
ordered shared IDs, reflected curve, actual iron fixture and22-call API/reopen
pass. SKT-092 is reaccepted; earlier native controls are unchanged.

Finish TUT-017 shoehorn, then pursue TUT-018 surface iron. The complete shoehorn
is now independently accepted; see [receipt](validation/shoehorn-complete.md).
GEO-019 exact R12/R16 corners and GEO-022 exact R1.5 perimeter are integrated at
`b51fc5f` and `498b165`. GEO-018 retained final endpoints and GEO-004 tangent
plane now pass the actual final constrained hole/Cut chain. Source length130→140,
exact history, reopening and strict component STL pass. Full177-second API
creation movie/model/STL are linked in the external gallery. Native creation
through Thicken remains TUT-031; physical body/Sketches Browser drag remains
ASM-012 and is explicitly separate from functional cumulative acceptance.

The complete creation run succeeded through export/recording. A later test-only
support-face assertion failed after Cut; a corrected bounded17-call saved-model
replay passes edit/history/reopen. Both evidence histories are preserved under
external `shoehorn-workflow-2026-10-15/complete-01`. No repeated full recording
was needed. Final runner guard is corrected. The source dimension-placement
contract uses horizontal50/vertical16; exact public spline interpolation remains
unknown. Bend16→18 still refuses atomically in the bounded corner construction.

TUT-018 now leads. SRF-017/#361 is independently accepted at `fc3ba9c`:
actual mirrored solved sketches→bilateral rail Loft→paired Trim gives2validclosed
side openings, stable source edits/history/reopen and rigid-frame invariance.
The exact source-local tensor uses real curves, with narrow symmetry/contact
admission; boundary naming is unique sampled correspondence against exact
restrictions, not builder history. See [receipt](validation/surface-bilateral-rail-loft.md).
The default-opacity GPU artifact was forced surface alpha, not geometry;
committed surfaces now honor authored opacity and Loft preview is opaque.
Final native/GPU captures are smooth. Explicit transparency remains REL-027/#459.

SKT-092/#457 is accepted through its actual shared-tip correction `c205138`;
see [Mirror receipt](validation/fit-spline-mirror.md). Public operation video
observations now confirm first Loft2profiles/1rail and handle Loft3profiles,
Chain Selection/noRails/NewBody. Exact curve classes/contacts remain qualified.
External knowledge is `surface-iron-2026-10-15-public-frames/`, including the
read-only retained bridge implementation map.

REL-026/#458 is independently accepted at `499d580` plus teardown correction
`e90e1a4`. Immutable session partition reuse gives194–566ms warm region/preview/
name actions in the actual40-call iron workflow; exact input/revision invalidation,
geometry/IDs, history, reopening and native teardown pass. Cold splitting/commit
remain about11–12s. See [receipt](validation/trim-session-reuse.md).

SRF-015/#359 is independently accepted: actual retained Trim chain→ellipse→
opposite chain New Body bridge passes core,43-call API and3/3 native controls,
source/ellipse edits, stable IDs, history/reopen and sampled support contact.
The intended active-component owner and component-owned native ellipse are
verified. Clean GPU views show an open passage and smooth surfaces. See
[receipt](validation/surface-handle-bridge.md). Cold operations take about14s;
edit opening41s is tracked separately as REL-028/#460. No responsiveness claim.

SRF-020/#461 is independently accepted at `e690833`, `6e0be0c` and `fc3d475`:
one Two Sides Extrude creates separate retained profile bodies while preserving
legacy compound features. Actual iron core,45-call API/native3/3 and smooth GPU
captures pass; the same sketch/Extrude can add a third output without losing old
IDs. See [receipt](validation/per-profile-surface-extrude.md). Public frames
establish2 profiles and2 new tool bodies, but exact curve grouping remains an
explicit Alloy fixture choice. Cold latency is recorded under REL-016.

Independently accepted SRF-016/#360 uses
`surface-iron-continuation-2026-10-15/per-profile-extrude-01/native/native-two-profile-sheets.alloy`
as the actual4-body source: separate upper/lower Extrude bodies are TOOLS;
existing Inner handle bridge is the declared TARGET. Exact-history diagnostics
show the unchanged tools only slit the exterior annulus, but partition the Inner
bridge (upper3 faces, then lower4 after the first Trim). Source curves are unchanged; an
exploratory endpoint extension was not adopted. This agrees with the public Inner
highlight but does not establish the complete reference target inventory.
The external revised curved Trim plan
supersedes the earlier compound-as-target assumption. Exact-history signatures
retain regions and adjacent edge fragments; duplicate ancestry refuses.
Both Trim trials, Remove/revert, source edits and reopening pass; see
[receipt](validation/surface-curved-trim.md). SRF-011 Boundary Fill awaits bounded OCCT
MakerVolume feasibility check on the actual sheets with a declared bottomZ=0
variant; no cell/solid acceptance yet.
Both bounded MakerVolume attempts produced no solids. Exact sections found
the lower cutter crosses Z0 at X124.906, beyond heel X120; moving its free end
Z−8→−16 still crosses beyond the heel at X121.828. Neither closes the low escape.
No further endpoint sweep is planned. TUT-032/#462 independently qualifies a deterministic
retained fit point on the bottom inside the rear: baseline endpoint retained,
middle point (110,8)→(110,0), bottom Z0. Exact contact and escape interception
were checked before the new volume attempt: one valid98122.002mm³ solid,
seven uniquely source-traced faces, one unique cell signature and five clear
handle witnesses. Two collapsed-tip orientation warnings remain recorded.
Evidence is external `deterministic-bottom-contact-01/`. Accepted precursors
remain unchanged; these are independent dimensions, not exact reference values.
SRF-011 kernel/document and native/API controls are now being implemented in
parallel. Product closure/edit/native acceptance remains pending.
SRF-016's core,62-call API and3/3 native checks are independently accepted.
The from-blank runner caught same-owner sketch Trim admission; SRF-021/#463
fix `3c7dda8` is independently accepted through core and actual component-owned
lens Trim/handle bridge; see [receipt](validation/owner-local-surface-trim.md).
The next step exposed coincident-point chaining omitted by shared-ID prepared
fixtures. SRF-022/#464 makes Surface Extrude follow explicit retained Coincident
endpoint equivalence; no proximity weld or runner reparenting workaround.
SRF-022 is independently accepted at `30b27d7`/`744217e`; see
[receipt](validation/coincident-surface-chains.md). Actual from-blank-03 passes
127calls/16stages through surfaces, corrected closing tools, both Trim trials,
Remove and exact undo. Source model is saved externally as `iron-surfaces.alloy`.
The final cumulative runner now includes bottom editing, Boundary Fill and
checked STL/source editing; these stages await the product build and evidence.
SRF-023/#465 solid/mixed tools and SRF-024/#466 multiple selected cell outputs
preserve broader Boundary Fill scope as Todo/GATE-050 dependencies.
The five-body precursor's bottomZ=−4 does not meet the exteriorZ=0 boundary;
final closure needs a declared, verified bottom geometry choice, not a body-count
assertion. TUT-018 remains open, including final full movie/model/solid export.

REL-028/#460 is independently accepted at `91add7c` (test correction `c3a6f1e`, metadata correction `3af41a1`):
exact source/session reuse avoids duplicate final Loft evaluation while retaining
non-tail downstream validation and authoritative commit. Same-fixture edit
opening40.319→5.226s; another owner-context session8.481s, repeated previews
118–247ms. Native tail/non-tail/source-stale checks and25-call API pass. See
[receipt](validation/loft-edit-prefix-reuse.md). Remaining cold cost is explicit.
No new package/CI run; Windows/Linux runtime testing remains external.

## Timeline visual polish (2026-10-15)

User-requested REL-025/#454 is independently accepted. The timeline now uses
original vector feature icons, scoped container borders, vertically centered
fixed-size tiles and restrained menu arrows with clear hover/focus states.
The focused Qt check passes existing edit/Cancel/overflow/Show latest and actual
split-arrow popup/Escape; idle/focus menu hit geometry preserves the drag exclusion.
Astra reviewed source and final normal/minimum app captures. No new playback,
rollback or selection semantics were introduced. See
[receipt](validation/timeline-visual-polish.md); external before/final captures are
in `deliverables/timeline-polish-2026-10-15/`. `build-preview-release/alloy.app`
contains the update; previously packaged archives and tutorial videos predate it.

## Tutorial continuation (2026-10-15)

TUT-031/#453 is independently accepted: native shoehorn creation from a blank
document through the dimensioned spline, endpoint planes, section profiles,
guided Loft and signed +3 mm Thicken. The complete 76-second creation video,
eight stage captures and editable model are in external
`deliverables/shoehorn-native-prefix-2026-10-15/native-08/` and linked first in
`demos/2026-10-13/index.html#shoehorn-native-prefix`. The cumulative Qt input test
passes, with actual native dimension/thickness entry, preview/Cancel/commit,
source-face identity and correct retained-sheet/root-solid ownership. Astra
reviewed source, saved state and film samples; root reviewed captures and linked
artifacts. API camera/visibility/save and assisted menu opening remain explicit
adaptations. See [receipt](validation/shoehorn-native-prefix.md).
The full TUT-017 gate retains exact grouped corner/perimeter finishing, hole
support, organization and output. Existing core geometry/history evidence was
reused; no broad test suite or new package/CI build was run. Further tutorial
work now prioritizes a materially new GEO-019 strategy to finish this chain;
do not repeat the unchanged failed fillet or iron boundary investigations.

Astra's bounded saved-result finite-cap investigation is complete. Boundary
mismatch up to 0.355 mm and sampled points outside the original solid rule out
claiming a same-surface/wire orientation repair as exact recovery. No new
Fillet run or product fallback followed. That earlier investigation returned GEO-019/#352 to Todo; the new active
construction candidate above supersedes that status. See the
[diagnostic receipt](validation/shoehorn-corner-diagnostic.md).
Current public source recheck confirms three spine points and 130/50/16
dimensions but does not resolve interpolation/tangent conditions; see external
advanced-audit-2026-10-15/shoehorn-spine-curvature-qualification.md.


## Initial-build readiness priority (2026-10-14)

REL-017/#420 is independently reviewed in [the preview guide](development-preview.md):
current capabilities, local package/start instructions, examples and explicit
input/platform/tutorial limits. REL-014/#417 committed-state recovery is now
independently accepted: per-tab durable snapshots, real forced-kill recovery,
exact authored identity and safe save/close guards. See
[receipt](validation/document-recovery.md); active drafts and native macOS
file-panel cancellation remain outside the evidence. REL-011/#374 bounded
assembly drag profiling is accepted at `c1d9395`, but all six moves exceed the
100 ms target (median 159.925 ms). REL-020/#446 owns the failed performance budget;
see [measurement](validation/assembly-native-drag-profile.md). The measured
build is Debug. A matched Release configuration improves median dispatch to
140.571 ms but still misses the target. Phase probes identify refresh/presentation
as the dominant cost; a bounded pose-refresh experiment failed and was reverted.
REL-020 returns to Todo with diagnostics preserved. REL-013/#416 visual/control
audit found severe ribbon/navigation clipping at 1040×640; bounded REL-021/#447
responsive controls is accepted at `c562c85`: readable representative icons and
complete menus, active-tool labeling and actual Qt menu/Pan/Escape checks pass.
See [receipt](validation/responsive-controls.md). REL-022/#448 is accepted at
`e8bc743`: shared Box/Cylinder scrolling fields and persistent actions pass
keyboard/Qt button and visual review; pointer scrolling remains unqualified.
See [receipt](validation/primitive-dock-height.md). REL-023/#449 is accepted at
`3055456`: concise status and persistent warnings, normal/minimum captures,
genuine recovery-init failure metadata and startup GPU-state consistency pass
focused Qt/Astra review. See [receipt](validation/status-strip-readability.md).
REL-024/#450 is independently accepted: scrolling Extrude form, fixed actions
and clear missing-end-face guidance pass Qt footer/Cancel/valid-commit and
normal/minimum visual review. See [receipt](validation/extrude-dock-height.md).
REL-013 has current Fillet empty-edge and Combine empty-tool captures without
a demonstrated clipping defect. The dimensioned sketch palette and actions are
readable. NAV-021/#452 is accepted at `da11b31`: hover/keyboard disclosure
clears the accessory footprint while preserving the active sketch and cube
controls. See [receipt](validation/cube-accessory-disclosure.md). Exact reference
disclosure remains unknown; this is explicit Alloy policy. The parent audit
remains open: final captures also show clipped wide navigation labels and sketch
summary overlap requiring a settled-layout check. The old blank viewport band
matches dock allocation; actual Qt/native/framebuffer dimensions agree.
Use `build-preview-release`
for optimized interactive previews and `build-tutorial-polish` for Debug checks;
both use the coherent private Qt SDK. Older packaged archives predate these changes.

REL-019/#440 adds pre-painted synchronous geometry-busy feedback and reliable
cursor/status cleanup, including initial solid previews. A cheap real-app Box /
Fillet-preview / refused-edit fixture passes 1/1; source and evidence passed
independent kernel-agent review. See [receipt](validation/geometry-busy-feedback.md). This is
honest wait feedback, not asynchronous evaluation or cancellation; REL-016 remains
open for responsiveness.

NAV-016/#437 is accepted at `7d95732`: retained custom named views, Browser
create/rename/restore, metadata history, save/reopen and atomic malformed-file
refusal pass focused Qt checks and Astra review. See
[receipt](validation/navigation-named-views.md). NAV-017/#438 saved/reset Home
is accepted at `6e38eb6` with metadata history, save/reopen and focused Qt/GPU
evidence. See [receipt](validation/navigation-home-saved.md). NAV-018/#439
application navigation preferences is accepted at `049b0d5`: all three alternate
mouse chords, isolated settings restart/reset and unchanged history pass Qt
checks; interruption fixes are source-reviewed and compiled. See
[receipt](validation/navigation-input-presets.md). NAV-019/#443 configurable
Front/Top/up conventions is accepted at `a355c50`; parent NAV-007/#413
is closed after independent cumulative receipt review. Physical NAV-008 remains
open. REL-014/#417 committed-work crash recovery is accepted. NAV-020/#444 is accepted at `53507a1`: the
cube remains orthographic under perspective model projection; matched GPU
captures and actual cube picking pass. See
[receipt](validation/navigation-cube-projection.md). The normal app is relinked.
REL-019/#440 is accepted at `6ad76be`; REL-016 remains open.

TUT-029/#441 diagnostic completed at `636c8b5`: final case/reference intersection
checks found Top 0.732240293455 mm³ and Bottom 0.888291503536 mm³ overlap with
SIDE-ETH-001; the remaining 22 pairs are zero-volume. See
[receipt](validation/enclosure-reference-clearance.md). TUT-030/#442 owns source
qualification and explicit correction, blocking full TUT-016. Do not silently
change stock, shell, radius or imported reference. Public-source qualification
confirmed the connector width/depth are unspecified; TUT-030 returns to Todo
until qualified input or an explicit design adaptation resolves the nominal
fixture. Physical fit remains unclaimed. GEO-019 exact grouped shoehorn diagnostics are
accepted as blocker evidence at `db0bc01`; orientation-only saved repair remains
invalid and material preservation unproved. See
[receipt](validation/shoehorn-corner-diagnostic.md). The issue returns to Todo;
no generic healer or unchanged retry. Surface-iron SRF-017 lacks a separate
authored main spine for auxiliary-guide contact; no unsupported one is invented.
SRF-018/#445 is independently accepted: one planar convex target sheet trimmed
by one retained crossing sheet using actual surface intersection. Core, 37-call
API and native ribbon/tool/region/Cancel checks pass; source edits preserve
retained identities through history and reopen. See
[receipt](validation/surface-planar-sheet-trim.md).
Full SRF-016 iron trial/revert and TUT-018 remain open.
SRF-019/#451 is independently accepted at `e0ef76e`: retained no-rail Loft
across three closed sketch sections, split from SRF-015. The supported fixture
uses two fit splines, a full ellipse, then two fit splines; other arrangements
refuse explicitly. Generic ordered references leave room for future expansion.
Core, real-app API and actual Qt section picks/Cancel/OK at 1040×640 pass, with
source edits, hidden-source replay, stable references and history/reopen. The
Loft form scrolls above fixed actions. See [receipt](validation/surface-closed-loft.md).
The numerical fixture is independently authored; source-exact topology remains
unknown. SRF-015 retains actual bilateral Trim-boundary integration and SRF-017
exterior quality. No implicit transport spine or unchanged failed exterior retry
is planned; full TUT-018 remains open.

CI-005/#423 local macOS arm64 relocation is accepted at `3c88127`: extracted
archive, 43 observed bundled non-system libraries/zero external, Apple M4 GPU
and API/save/STL evidence. See [receipt](validation/macos-relocatable-preview.md).
The older archive predates GEO-020. A refreshed local navigation candidate now
contains accepted sources through `a355c50`; its extracted app passed bundled
runtime, GPU, new view metadata save/reopen, STL and packaged-client checks.
Archive `navigation-candidate-01/Alloy-macos-arm64-preview.zip` SHA-256
`c5eedaabacf504c845cf42973f536afa0af21c6bf97e909b5cc2130122ca2fa9`
is independently reviewed local evidence, not a public release.
NAV-014/#434 explicit Pan/Zoom/Zoom Window is accepted at `73a578c`, with
actual Qt controls/drag/Escape/post-tool selection and GPU captures. Final signed
rectangle schema is included in the next coordinated app relink. See
[receipt](validation/navigation-pan-zoom-window.md). NAV-015/#435 selected framing
and planar Look At is accepted at `140f8a7`; visible sketch strokes now determine
framing, with center/scale and GPU evidence. See
[receipt](validation/navigation-selection-look-at.md). NAV-006/#412 projection
modes is accepted at `fee6a19`: actual GPU projection, hybrid face resolution,
projected picking and per-tab restoration pass focused Qt validation and Astra
review. See [receipt](validation/navigation-projection.md). Named views and
navigation preferences NAV-007 are now accepted;
Windows/Linux CI-009 and external GPU/input gates remain separate. No CI dispatch,
version tag or release was triggered. The user has not yet tried physical Mac
trackpad input; keep that qualification pending.

The user reprioritized polish/quality, familiar viewport navigation and an initial
build, while retaining advanced tutorial completion. See the actionable
[readiness burndown](initial-build-readiness.md). Twenty-two bounded children
NAV-001–009, REL-012–018 and CI-004–009 are published as #407–428; existing
file/physical-input/platform gates and advanced chain dependencies are reused.
Orientation-cube and navigation-control scope is further split into finishable
NAV-010–015 children (#430–435), without closing the parent scopes.
NAV-001/#407 is independently accepted at `2d1a699` and closed: free orbit,
a visible checked tool for users without a middle mouse button, custom-camera
tab restoration and navigation gesture priority. Focused Qt tests passed 3/3,
and actual GPU captures were inspected. Physical devices, trackpads and
constrained/pivot modes remain separate. The main app was relinked. See
[the receipt](validation/free-orbit-navigation.md).

GEO-017/#347 is independently accepted at `6d9b8dc`: an exact analytic planar
inset helper produces the actual 17-edge U contour with identities derived from
the owning feature, source edges and incident vertices. Changing upstream stock
width from 64 to 65 mm changes area from 233.8577 to 236.289 mm² while preserving
IDs through source history and reopen. This is a kernel ingredient; authored
inset/UI remains SKT-076, now unblocked. See
[the receipt](validation/enclosure-inset-diagnostic.md).

CI-008/#426 is independently accepted at `098e485`: clean patched Qt build,
5/5 offscreen regression, cache reuse/refusal, Alloy build and static dependency
inventory. See [receipt](validation/mac-runtime-recipe.md). This does not qualify
loaded runtime, relocation or packages. The existing SDK remains intact; CI-009
retains Windows/Linux alignment and parent CI-004 stays Todo. The user chose
external Windows/Linux testers; the [checklist](validation/external-package-checklist.md)
keeps qualification pending until results arrive.

SKT-076/#346 is independently accepted at `c625649`: retained trimmed-seam
inset, legacy annulus compatibility, stable profile/edge/contact IDs, exact Join
2 mm and selected end-rim Fillet R0.5 through width edits/history/reopen. Native09
passes actual edge/field/Escape/Return with corrected instructions and visible U
preview. See [receipt](validation/trimmed-seam-inset.md). The approximately 30 s
collapse refusal is tracked in REL-016. TUT-023 still owns the full ridge,
retained-tool Cut and exports; this ingredient does not close the tutorial.

NAV-002/#408 is accepted at `b6237b5` / `ee66d96` for constrained orbit,
custom pivot, panned pivot stationarity, fixed-view reset, per-tab camera state
and unchanged authored/history state. API05 passed 3/3 and actual GPU captures
were reviewed. Native input remains explicitly unverified: Qt native01/02 stopped
at initial activation; root's bundled-app CUA route could not bind the live
window (`cgWindowNotFound`). The root-owned empty app was closed through its API.
[NAV-009/#428](https://github.com/rje/Alloy/issues/428) preserves that native gate
and blocks cumulative NAV-008. Do not repeat activation attempts without changed
foreground/binding evidence. See [receipt](validation/constrained-orbit-pivot.md).

TUT-023/#324 returned to Todo after the exact paired R0.5 operation refused
atomically on the final 29-feature snap fixture. The opposed rear rails span
54 mm across a 1 mm ridge at z=17; this remains an explicit source-selection
adaptation. [GEO-020/#429](https://github.com/rje/Alloy/issues/429) now owns the
bounded paired-rounding blocker and is a dependency of the cumulative gate.
No reduced radius, alternate pair sweep or single-edge substitute was accepted.
Broader TUT-016 retains the complete source tutorial; zero overlap cannot
establish numerical clearance or physical fit.

NAV-003/#409 is independently accepted at `794613a`: cursor-anchored zoom,
separate wheel/pixel/native gesture routes, Shift-native-pan orbit and interrupted
stream reset. Qt05 passed the focused checks including an actual Body-priority
viewport click after navigation; GPU output was inspected. See
[receipt](validation/trackpad-navigation.md). Physical Mac trackpad and platform
input qualification remain NAV-008; preferences remain NAV-007. Final nonfinite
anchor guard is source/build reviewed; no unchanged rerun was needed.

GEO-020/#429 is independently accepted and closed at `82b0d7c`. The existing
Fillet builder remains first; a geometrically bounded paired-ridge composition
handles the diagnosed full-round case with finite-chain/terminal guards and
conservative composed kernel history. Three existing fillet regressions pass.
The exact authored R0.5 gate and real-app width64→65, same Fillet/seed IDs,
undo/redo and exact reopen pass; Apple M4 GPU captures were independently reviewed.
See [receipt](validation/paired-ridge-fillet.md). GEO-021/#436 is also accepted
and closed at `95d4e56`: an isolated cleanup preflight vetoes invalid simplification
without shipping copied topology. Exact retained-tool Cut, width edit, history,
reopen, retained-support use and separate STL/STEP readbacks pass. New genuinely
split edge references remain unavailable; no ambiguous rebinding is allowed.
See [receipt](validation/boolean-cleanup-veto.md). TUT-023/#324 is accepted at
`1e87919`: the 267-second final-stage API movie, rim/tool captures and both halves’
actual STL/STEP readbacks pass independent review. The external gallery links
this continuation to earlier creation footage. See
[receipt](validation/enclosure-rim-fit-stage.md).
Physical clearance and full-source cumulative TUT-016 remain separate.
NAV-010/#430 is accepted at `75695f7`: GPU cube, readable blue hover and
face/edge/corner picks through authoritative camera state. Qt06 proves 26 API
orientations and three representative Qt clicks; physical/DPI/sketch routes
remain unqualified. See [receipt](validation/orientation-cube-regions.md).
NAV-011/#431 is accepted at `66d38cd` for Qt cube dragging around the view target
while retaining the custom pivot for ordinary orbit; natural-focus Escape remains
NAV-008. [Receipt](validation/orientation-cube-drag.md). NAV-012/#432 is accepted at `a1b3e88` / `82beb1c`: Home/adjacent/roll
controls, live-widget capture composition and Qt03 3/3 with readable GPU output.
Adjacent/roll source support is related-product guidance, not established exact
target behavior. See [receipt](validation/orientation-cube-accessories.md). NAV-013/#433 is
accepted at `057026b`: visible Fit preserves orientation while Home resets it;
current valid preview geometry participates in framing. Focused Qt checks pass
3/3 with GPU captures; F6 binding is inspected, natural key delivery remains
unverified. See [receipt](validation/navigation-fit.md). NAV-014/#434 and NAV-015/#435
retain explicit pan/zoom/window tools and selected framing/planar Look At. Parent NAV-004
stays Todo behind those children; existing projection/DPI/physical gates remain. Newly
archived public 1080p tutorial frames show two tangent chains, 14 edges total,
Tangent Chain enabled and a typed 0.5 radius. This supersedes the former unknown
about chain versus individual-edge selection, but does not establish our adapted
edge IDs, exact source topology, per-chain counts or the second Offset value.
Keep source images outside Git. Diagnostic commit `b4c9f35` preserves inputs and
exceptions in JSON; it was rebuilt without repeating the unchanged failing
geometry. The original refusal evidence remains source/log/rail enumeration,
not the then-empty two-rear-rails JSON or an explicit atomic snapshot assertion.

Current CUA documentation (refreshed 2026-10-14) now advertises held modifiers
for macOS coordinate clicks via `click(point, {key: "super"})`. The older
SOL-058 limitation may therefore be testable; this is tool capability evidence,
not a passed physical-input gate. Try it only in an isolated validation editor.

Use only the main checkout and one private GUI lease; no new worktrees are
needed. Alternate readiness slices with concrete advanced-chain blockers.
Packages can progress independently. No version, CI dispatch or release has
been triggered. Initial OpenGL preview limitations remain explicit; stable RHI
requirements stay. Earlier accepted milestones remain valid; this section
records the current priority.

## Prior accepted queue (2026-10-14)

Continue autonomously across accepted milestones; a status report is not a stop.
RES-021/#383 source preflight is accepted: the short expression exercise uses
Center to Center Slot (first cap center, second cap center, width), distinct
from the midpoint-first Center Point Slot. SKT-083/#385 is independently accepted
at `e620987`: core passes, native01 passes actual three-click/H/V/D45/persistent-D
R5 input, and API02 passes 28 calls through D45→D50, analytic area
528.539816→578.539816 mm², history and reopen. The GPU capture is readable;
typed slot input remains source-reviewed only. See [receipt](validation/center-to-center-slot.md).
PAR-007/#384 is independently accepted at `8934749`: shared same-sketch
expressions retain local dN names, UUID dependencies and entry units. Focused
core plus three relevant prior core checks pass; added formula-edit/dependent
delete-refusal checks pass. API02 passes 45 calls with polygon area
12500→17400 mm²; native02 passes actual inline `d3 + 50`, driver edit100→120,
dependent150→170 and formula reopening with readable captures. See
[receipt](validation/sketch-dimension-expression.md).
SKT-084/#387 is independently accepted at `b260850`: focused origin-snapped
core regression and native13 full tutorial pass retained bottom Horizontal/right
Vertical, aligned 120/170 polygon area17400, unchanged45/R5 slot and history/reopen.
Native14 shows readable H preview. Ctrl/Command routing is source-reviewed only.
TUT-024/#386 and FND-026/#391 are independently accepted with recording
presentation `4d09f0b`. The full from-blank Qt workflow passes; recording02 is
110.133s, with visible H preview (58s) and literal formula entry (97s). The fixed
recorder provenance header avoids covering modeling controls. Video, editable
model and receipt are in the external gallery at
`demos/2026-10-13/index.html#sketch-expressions`; all134file links resolve.
One transient pre-Line pointer-preview refusal, raw repeated frames, the
nonblocking menu-opener adaptation and unknown public screenshot metadata remain
explicitly qualified. Shared API viewport/inspection/history/storage steps are
captioned separately. See [receipt](validation/sketch-expression-tutorial.md)
and [contract](research/sketch-expression-exercise-contract.md).
SKT-085/#388 is independently accepted at `466eb7e`: new/legacy tangent cores
pass2/2; native04 passes3/3 with actual Shift-from-circle and ordinary onto-circle
clicks, visible Tangent previews, retained contacts, Finish/history/reopen. Radius
edit and missing-contact refusal pass; acquired point/origin snaps keep priority.
Broader Ctrl/Command inference suppression remains with SKT-021 (H/V only today).
See [receipt](validation/sketch-pointer-circle-tangent.md).
SKT-086/#389 is independently accepted at `96060ef`: focused core1/1 and
native05 3/3 cover held tangent-Arc preview, no-drag/Escape cancellation, release
commit and continued Line, history/reopen. See [receipt](validation/sketch-line-tangent-arc.md).
SKT-087/#392 and TUT-025/#390 are accepted at `1d476bc`. Single-junction Fillet
now previews/focuses/acceptsR10 while two-corner batching remains native-verified.
The full from-blank construction recording01 passes3/3 (65.103s): source-order
circles, tangent chains, R10 and held-drag arc create three valid cells including
the central bridge. Small circle edit preserves valid bridge and all retained
constraint records; undo restores the undimensioned source checkpoint for exact
save/reopen. Astra independently reviewed source, code, original visuals, movie
and authored geometry. [Receipt](validation/base-sketch-construction.md).
The64.133s movie/model are in the external gallery at
`demos/2026-10-13/index.html#base-sketch-construction`; all139 local links resolve.
Menu opener adaptation/Qt-not-OS input, rough proportions, raw repeated frames
and two transient pre-Line pointer-preview refusals remain qualified.
TUT-026/#393 is independently accepted at `40db961`: native02 passes3/3
through the eighth visible Tangent, circle/arc dimensions and six ordered center
edits. Ten dimensions, eight Tangents, three valid cells and observed0DOF; exact
authored/evaluated history/reopen pass. Existing R10 is reused. The19.866s native
continuation movie/model are alongside the construction movie in the gallery at
`#base-sketch-dimensions`; all146 local links resolve. [Receipt](validation/base-sketch-dimensions.md).
VIS-005/#396 and TUT-027/#394 are independently accepted at ac299a6 and
10a9e03. Native07 passes3/3: three blue selected regions before depth, 48.25mm
preview, Cancel clears selection without authored/history changes, and one valid
closed solid at126221.660324mm³. Depth edit, undo/redo and exact reopen pass.
See [receipt](validation/base-sketch-first-solid.md). The18.7s continuation and
losslessly joined1m43s from-blank-to-solid film are in the external gallery.
The joined film contains three recorded application sessions, not one continuous
session. Qt/menu adaptation, zero-depth preview refusals and missing taper field
remain qualified; the broader SOL-010 and SOL-016 tasks remain open.
TUT-028/#395 is independently accepted at f9620ab: native cap-face sketch, referenced
circle centers, diameters10/8/8, then three-profile Cut/All/Flip. Sol owns the
single build/GUI lease; Astra independently verifies. Public references stay external.
SKT-088/#397 is accepted at1ba977e: native ingredient3/3, all three retained
curve-center refs including the origin overlap, right center follows +1mm source
edit/undo. Left binding is verified; movement exercises the right/shared replay.
See [receipt](validation/face-circle-center-snap.md).
SOL-056/#398, DOC-033/#399 and SOL-057/#400 are independently accepted at179b2e7
with receipt efe7f68. Native-cut-preflight10 passes3/3: automatic unique support-body
target, All/Flip, valid closed117581.495129mm³ body with three independently checked
through-openings, reopened preview/Cancel, idle selection then edit, source+1mm,
Ø8→9 change, undo/redo and exact reopen. Explicit target/source overrides remain
preserved; linked/placed target ambiguity is refused. Last different-sketch override
guard is source/build verified. See [receipt](validation/face-supported-cut-all.md).
Recording03 passes3/3 including both first-circle Returns with natural focus.
Its21.133s film and editable model are in the external gallery, alongside a123.8s
joined four-session film from blank; all165 local links resolve. Five intermediate
preview refusals, Qt/menu adaptation and separately performed API checks remain
qualified. See [full receipt](validation/base-sketch-face-holes.md).
No additional ingredient reruns are needed. TUT-019/#305 is independently accepted at acc8616 after VIS-006/#402 and FND-027/#401: recover
the existing cumulative native piston harness and its source ledger, retaining
accepted API/geometry/native-child evidence. Sol owns the single build/GUI lease;
Astra verifies. FND-027 is independently accepted at af828d8: native15 passively verifies main
activation after conversion, New Component and Rename; no inter-step harness
focus repair. Shared one-shot callback is guarded by lifetime, active app,
current tab/document and no competing modal/popup/window. Native15 is still an
overall failed cumulative run at the fourth joint's upper mounting-bore hover.
The harness had reversed source participants and targeted arm instead of piston
bores; source review corrected all four external Revolute rows. Native18/19
confirm fixture occlusion from negative-Y isometric, not wrong-body picking.
VIS-006 is independently accepted at55b04f4: shared rearIsometric (+X,+Y,+Z)
and Browser entry expose the exact Inner finite bore wall at250%; actual GPU
capture/retained occurrence+origin pass. This is a declared camera adaptation.
Native21 full supplied-fixture Qt smoke passes3/3 in34.106s: corrected source
participants/bore refs, seven joint rows, linked Paste−180, three/five moving leaf
poses, full-pose Revert and exact save/reopen. Copy's queued focus completion is
observed passively before Paste. Recording04 passes3/3 in63.150s;62s movie930frames801repeats0capturefailures,
207commands with no failures. Astra reviewed source, geometry and movie. Native
film/model are in external gallery #piston-native-workflow, all172 links resolve.
Supplied-model, camera/numeric supplement, local-storage and Qt-not-OS limits stay
explicit; no collision/global-rank/smooth-FPS claim. No ingredient reruns needed.
ASM-012/#343 returned Todo after bounded physical-control recheck. Normal app
launch/full-path CUA now acquires AX and screenshots, but two body drags (label
and row margin) did not change ownership. Logs show native press/release10/14ms
apart without recorded held motion; move throttling makes this inconclusive.
No successful QDrag/drop or product failure asserted; folder drag not attempted.
Source review flags mutable currentItem drag source and embedded label routing
for a future focused trace. Evidence/receipt are external under
component-reparent-2026-10-14/asm012-prepared/. Private editor closed; no user
model altered. FND-015/#158 returned Todo after the next bounded recheck:
normal build-tutorial-polish launch loaded coherent private Qt libraries/plugins.
Physical Perpendicular and accessibility-index Equal activation both produced
shared authoritative tools/relations and matching AX row labels. API-prepared
outline and 500mm dimension remain explicitly setup, not native evidence.
After the dimension, CUA observation/rebind failed cgWindowNotFound while the
same app process and control API stayed healthy. Selected-row OS AX edit/history
remains unverified; no application crash or complete accessibility pass asserted.
Evidence: external fnd015-accessibility-2026-10-14/normal-launch-01. Avoid another
control-acquisition loop; continue a bounded feature with Qt/native/API evidence.
SOL-016/#154 returned Todo: combined5-to4-to5 Qt selection and signed arrow
produced correct one-body geometry, but the operation popup stayed visible and
the initial direct OK delivery bypassed its actual recipient. Independent review
rejected cumulative UI acceptance. Two bounded real-row/readiness hypotheses
failed; external sol016-five-profile-qt-2026-10-14/native-01 preserves evidence
and harness, with no product fix or failing CMake target retained. Stop retries.
SOL-058/#403 retains physical held-modifier macOS/Windows checks; TUT-003 depends
on both. SKT-089/#404 is independently accepted at c6a87c9: numeric redundant
new dimensions offer explicit Add Driven/Cancel; conflict41 refuses, matching40
becomes read-only(40), driver50 updates(50), history/reopen preserve intent.
Core/native03 pass2/2. API setup/placement and Qt typing/choice clicks stay distinct.
Root then found the new Qt harness omitted production AlloyTheme::apply, explaining
dark dock titles. Theme-only followup ec16303 aligns the test entrypoint;
native04 controls pass and the actual light-title/offer capture is readable. The external SOL016 production-theme native04 still leaves its popup visible
and stops before OK; receipt02b0726 preserves this. Theme mismatch does not
explain the remaining popup gate. It is Todo, with no failing test target
retained and no more retries absent concrete event diagnosis.
Broader SKT-021 and physical403 remain Todo. Only one build/GUI lease is active.
PAR-008/#405 is independently accepted at `137bd16` and closed. Retained
One Side Distance arithmetic preserves stable parameter IDs and entry units;
API07 checks fixed-profile depth80→90, signed values, atomic refusal, history
and reopen. Production-theme native08 checks actual formula typing, preview,
OK/edit/Cancel and readable Distance (mm). See
[receipt](validation/extrude-distance-expression.md). Broader expressions remain
PAR-001. Backlog is406 issues,266 closed; dependencies remain acyclic.
SOL-016 remains Todo. Qt source explains the missing popup dismissal: the
saved harness clicked within the runtime release guard without the pointer
movement that clears it. Corrected native05 stopped before the workflow because
the initial window never became active; no popup or product acceptance follows.
Exact log and corrected harness remain external under the existing Cut delivery
folder/native-05. No failed test target remains; no further retry planned.
SKT-090/#406 is independently accepted at81a363e and closed. Straight-line
Extend previews the nearest supported intersection in green, preserves endpoint
IDs and publishes one solved edit. Core/native03 cover the 2mm corner gap,
200mm² profile and shared-command5mm extrusion to a valid1000mm³ solid.
Actual Qt hover/click/Enter, API setup/Finish/Extrude and committed history/reopen
are distinguished in [receipt](validation/sketch-line-extend.md). Draft-level
history routing is source-reviewed. Broader curve tools remain SKT-011; physical
SOL-058 remains Todo. No private editor or active candidate remains at this
accepted milestone. Pull the next bounded tutorial-enabling child from the live
queue; do not repeat unchanged physical/startup gates without new evidence.
Only main and its existing build are used; no inactive worktrees accumulate.
DOC-031/#348 is independently accepted at `89782cf`: unchanged-product native09
passes modal line focus, rename commit, Browser label and unchanged geometry.
Earlier button-focus failure remains unexplained, not claimed fixed.
SKT-080/#380 is independently accepted at `c730f97`. Explicit shared-endpoint
arc-line Tangent supports either pick order, retained contact/side and visible
badges. Core passes; API05 passes 50 calls with a rounded plate resize, atomic
refusal, history/reopen; native02 passes actual picks/recovery/badge deletion.
Broader tangent pairs remain SKT-022. See [receipt](validation/sketch-arc-line-tangent.md).
SKT-081/#381 is accepted at `d0d7197`, with display-only near-zero label polish
at `d9fb3a2`. Core passes; API01 passes the 12000→13000 mm³ split-top plate
edit/history/reopen/refusal workflow; native02 passes focused picks/badge deletion.
See [receipt](validation/sketch-line-collinear.md). Separate Coincident rank
refusal and exact OS/visual parity remain qualified.
SKT-082/#382 is accepted at `d436bd1`: explicit existing-circle/line Tangent
preserves finite-contact solver/schema/badge semantics. Core/API01 (40 calls)
and native02 (3/3) pass radius/datum edits, both pick orders and badge deletion.
See [receipt](validation/sketch-circle-line-tangent.md).
SKT-077 remains Todo after native03/04: initial activation-before-projection
did not fix picking. Actual hit widget/frame/projected center and before/after
captures are preserved externally in the face-origin deliverable native04.
The click selects neither face nor body; no native origin acceptance or product
change. A concrete picker/render diagnosis is required before another attempt.

Worktree cleanup removed 180 inactive checkouts, retaining branch refs and a
verified external artifact archive under
`maintenance/worktree-cleanup-2026-10-14/`. The subsequently completed `sketch-offset-union` checkout was also retired.
Only main now remains. The final `native-exchange` and `sketch-badges` checkouts
were retired after confirming all commits have integrated equivalents and
archiving their ten untracked files with SHA-256 verification under the cleanup
record's `final-two/` directory. Both branch refs remain. Old worktree build paths no longer exist:
recreate a branch checkout if needed and recover evidence using that archive
manifest. Canonical external deliverables and the video gallery are unchanged.

SKT-074/#330 is independently accepted and integrated at `8241b66`, with
owned test-ID/API provenance correction at `33ea70a`. Main application build,
closed Offset and extrusion document checks pass. Real-app clearance edits,
exact coverage, history/reopen and visual preview are accepted; native aggregate
selection is separately accepted under SKT-078/#377 below. The completed worktree was retired after
preserving evidence externally. See [receipt](validation/closed-offset-union.md).
DOC-029/#334 is implemented on main at `eddc86e`: Browser Edit can change a
retained section without replacing the active sketch. Actual-app API04 passes
59 calls through Project, retained Offset and Finish; native Qt Browser/type/
Escape/OK and natural focus return pass, with readable captures. Cancel adds no
history entry; OK adds one independent analysis action. Independent Astra review accepts this bounded scope; #334 is closed.
Public nested-interaction mechanics remain unknown; this is an
explicit Alloy policy. Nonempty selected-source preservation is not separately
proved by the pending-Project fixture. See [receipt](validation/section-during-sketch.md).
SKT-078/#377 is independently accepted at `0c49262` + `b01084a`. Native03
verifies focused Qt keyboard selection, ordinary replacement, aggregate Clear,
Cancel, commit and undo/redo. Pointer-row clicking remains unverified. The Clear
row now removes the staged aggregate and disables preview/OK correctly.
SKT-077/#375 is back in Todo: initial activation then a face-pick failure
stopped the focused native gate before sketch creation. Draft and logs are
preserved externally under `deliverables/face-sketch-origin-2026-10-14/native-draft/`.
No native origin acceptance is claimed; accepted core/API checks are unchanged.
Read-only review rules out required cached hover: picking ray-tests click coordinates.
Pre-activation projection versus queued viewport sync is plausible but unproven.
DOC-031 earlier native06/07 reached the modal dialog but focus stayed on a
button. The ineffective activation change was reverted; native09 now passes
unchanged product. The regression test and qualified receipt are accepted;
prior failure evidence remains in the external body-rename deliverable.
PAR-006/#378 is independently accepted and closed at `187f262`. User length
formulas such as `Length=Width*2` retain reference UUIDs through rename and drive
existing model bindings through evaluated values. Core named/model checks pass;
API01 changes an 80×40×5 rectangle extrusion to 90×45×5 (16000→20250 mm³),
with refusal, history and reopen checks. Native04 passes focused Qt Expression
cell typing and synchronized form display. Functions, broader unit algebra and
anonymous model-field formulas remain PAR-001. See [receipt](validation/length-expressions.md).
Implementation and validation reused main and its build; no new worktrees.
SKT-079/#379 is independently accepted and closed at `d4195da` plus receipt
clarification `5c0ad5a`. Symmetry constrains two existing straight lines about a
construction axis, retaining endpoint correspondence and one selectable/deletable
badge. Core passes; API11 passes 42 calls through a 60×30×5→70×30×5 plate
(9000→10500 mm³), conflict refusal, history and reopen. Native03 passes actual
Qt button/three picks/wrong-axis recovery/badge deletion with focus assertions.
Broader symmetry families remain SKT-022; exact OS and visual parity are not
claimed. See [receipt](validation/sketch-line-symmetry.md). All private validation
editors are closed; only main remains and no additional worktree was created.
FND-001 comparison policy is accepted in docs/compatibility.md; FND-025/#376
retains unknown public screenshot metadata and unproven layout generations.

SKT-075/#332 is independently accepted and integrated at `62b97d7`. Fresh
supported planar-face sketches record `originConvention: projectedOwnerV1`
and project the authored owning-component origin onto the face, retaining its
axes. Legacy saved sketches without the marker retain their original frames.
Main application build and focused fresh-origin/legacy-face core checks pass;
actual-app API03 passes 11 calls with a reviewed GPU capture. Later copying
preserves existing sketches; new ambiguous support authoring refuses atomically.
Known enclosure coordinate adaptations use resolved sketch frames now; the full
enclosure sequence was not replayed. Native pointer-origin snapping remains
SKT-077/#375 Todo. See [acceptance](validation/face-sketch-origin.md).

Canvas DOC-024, retained Remove HIS-007 and Surface workspace VIS-004 are
independently accepted and integrated at 645497f. The combined build, focused
core checks and actual Surface Modify > Remove native route pass. SRF-012 exterior endpoint-contact guide-rail Loft is independently accepted at
`a8ed9c4`, including focused core, 37-call actual-app API and native3/3 evidence.
SRF-013 paired-sketch surface Trim is independently accepted and integrated
at `415550d`/`9ccfe66`. Core, 41-call actual-app API and refreshed native3/3
evidence pass, including readable labels, Change Tools and Cancel. See
[bounded Trim acceptance](validation/surface-trim.md). SRF-015 completed a separate headless handle-bridge feasibility probe: an
independent closed two-spline/full-ellipse/two-spline input creates a valid
open shell with sampled section agreement. This is not implementation acceptance;
the issue is back in Todo pending actual curved Trim boundary inputs. Both lanes
keep independently chosen prepared fixture topology explicit. SRF-017/#361 now
retains the missing exterior topology for two side boundaries: public source
describes mirrored lower splines, whereas accepted SRF-012 uses straight lines
and its current Trim gives a central seam window. Exact source rail contacts
remain unknown. A bounded C0 probe supports bilateral Trim/bridge/closed-cell
topology but has an unacceptable center ridge. The single G1 curtain correction
fails endpoint tangent compatibility and unchanged geometric limits; Astra
verified the blocker. SRF-017 remains Todo: a single endpoint-compatible tangent-handle follow-up
resolved tangency incompatibility but still failed positional accuracy at
0.002358/0.002609 mm against the unchanged 0.0003 mm ceiling. Independent Astra
review verified this bounded evaluator/fixture blocker. No tolerance relaxation,
retained implementation or more kernel experiments in this increment.
RES-019 piston public source research is accepted; exact public neutral sample
is not established; the independently authored adapted fixture is now accepted
within its documented component scope. ASM-013/#362 native fixture and identity component hierarchy are independently
accepted and integrated at `7bbf456`: four valid solids, native conversion/nesting,
source radius edit, history/reopen and parent STL pass. See
[acceptance](validation/piston-components.md). Main build and focused core pass. The external gallery now links a 69.8-second
from-blank component creation checkpoint under `#piston-components`, with
editable model, two-body checked STL and separate native hierarchy close-up.
ASM-014/#363 is independently accepted and integrated at `96c032c`: one
retained Y-axis circular-origin Revolute, signed angle preview/commit and root
identity Pin pass core, 29-call actual-app API and bounded native3/3 checks.
Main application build and focused core pass. See
[acceptance and native setup limits](validation/as-built-revolute.md). The shared
solver backend at `59782f1` independently checks original joint frames, full Pin
pose and signed targets. Pin follows evaluated timeline prefixes; typed joint
metadata does not close HIS-003 broader timeline scope. ASM-008/#304 is independently accepted and integrated at `bcc5493`, with
retained circular source references, exact simultaneous rotation/slide, per-island
gauges and explicit passive-coordinate refusal. Main core checks pass 3/3;
actual-app API 24 calls, native 1/1 and a focused wrong-type refusal check pass.
See [acceptance and native setup limits](validation/as-built-cylindrical.md).
The native Cylinder rim prerequisite at `04a4b7d` preserves top/bottom edge IDs
through radius/height edits. ASM-015/#364 is independently accepted and integrated at `105961e`: coupled
3R+1C+Pin authority, analytical API motion, additional Revolute creation and
bounded Qt Upper Arm drag/Revert pass. Main app builds; focused core, 32-call API
and native 3/3 evidence remain distinct. See [workflow](validation/piston-coupled.md)
and [native qualifications](validation/piston-coupled-ui.md). The gallery now has
a 188.5-second from-blank creation-through-coupled-motion checkpoint. A measured
223 ms preview event is tracked separately in REL-010/#367, not a frame-rate claim.
REL-010 is independently accepted and integrated at `00b0990`: one coupled
solve supplies inspection and placed bodies within a preview/refresh, with
unchanged solver checks. Main application build and coupled core pass. The
[acceptance receipt](validation/assembly-preview-reuse.md) records same-process
core parity and actual-app seven-joint API counts: one solve per preview, zero
additional refresh solves. Remaining native drag profiling is REL-011/#374 Todo;
API evidence does not establish continuous interactive responsiveness.
ASM-017/#366 is independently accepted at `de30222`: linked local occurrence
subtrees share source definitions and retain composed placement, paths and exact
selection. Native 3/3 and actual-app API02 31 calls (three intended refusals) pass;
shared height edits update both copies, history/reopen preserve exact state.
Main app builds and focused core1/1 passes. See
[bounded acceptance](validation/linked-local-occurrences.md). ASM-016/#365 internal
joint remapping/copied-piston motion is independently accepted at `560ccf1`:
one internal Cylindrical clone, original external joints/Pin retained, seven rows
and six leaves with analytically checked opposing piston travel. Actual-app API
36 calls, native3/3 and core pass; see [acceptance](validation/copied-piston.md).
ASM-018/#368 is independently accepted through `20aa7a4` plus STEP dialog capture
support in `a005a3a`: whole-six and selected-two STEP/STL geometry readback,
committed seven-joint pose output and staged-pose refusal pass a separate36-call
actual-app gate. See [export acceptance](validation/linked-instance-export.md).
Main application builds and focused copied/export core2/2 passes. TUT-019/#305 is
now the cumulative gate: one from-blank creation-through-copied-motion and
placed-output movie finished (351.667 seconds, all four stage receipts pass,
zero capture failures). The external gallery links it at `#piston-full-output`;
independent artifact review accepted the semantic/API recording. Exact native
TUT-019 remains open: a separate cumulative native
harness now contains the bounded source operations; cumulative native acceptance is still pending.
Its broad ASM-001/002/003 dependencies were
replaced with the accepted bounded children after independent review; broad
parents remain Todo with their full remaining scope. A final source-flow audit
identified required native gaps: ASM-019/#369 Copy then Cmd/Ctrl+V and placement
Enter is independently accepted at `0b931a3`/`ca7d1fd` with native3/3 evidence;
see [clipboard acceptance](validation/linked-local-occurrences.md).
ASM-020/#370 cylindrical-face origin inference and modifier hover locking are
independently accepted through `c177550`: native10 passes3/3 with valid preview,
Cancel and Joints visibility; retained authority is separately accepted at
`3751ff5`. The final local-axis triad correction is source/build verified. See
[joint-origin acceptance](validation/joint-origin-inference.md) for deliberate
midpoint/release mappings and input/capture limits.
ASM-021/#371 paste drag and contextual Position/Revert are independently
accepted at `31516f5`/`e5128fc`: native06 passes 3/3, both captures were reviewed,
and the integrated application builds. Stable copied preview identities, signed
arrow movement, one Enter commit and actual-focus Escape pass. See
[placement acceptance](validation/linked-local-occurrences.md).
ASM-022/#372 Browser organization is independently accepted through `3436a44`,
with native3/3 and focused core evidence; see
[Browser acceptance](validation/piston-components.md).
ASM-023/#373 shared As-Built Type and component picks are independently accepted
through `16d6e9c`. Native17 passes3/3 in6.61 seconds using actual embedded label
widgets, preserving exact occurrence IDs and the active authoring owner. It
supersedes the earlier test that bypassed those children through tree-viewport
clicks. Queued session-guarded routing prevents callback lifetime hazards.
Position setup remains API-qualified in this focused check; the cumulative
sequence uses the separately accepted viewport-origin route. The combined main
app builds. See [shared dialog acceptance](validation/as-built-native-selection.md).
All bounded input children are closed. TUT-019 returns to Todo while its
cumulative native gate awaits usable desktop input. The preserved harness
connects them in source order from the declared four-solid adaptation.
The cumulative native10 attempt stops after body-conversion OK: the dialog
becomes active, but the main window does not regain activation. No inter-step
focus repair was introduced. An OS-click check in the actual bundled app was
blocked before input by desktop control reporting `cgWindowNotFound`; the API
remained responsive with no focused window. This does not establish a product
focus defect. Evidence remains external under
`deliverables/piston-native-workflow-2026-10-14/modal-os-check-01/`.
REL-010/#367 command-scoped evaluation reuse is independently accepted;
REL-011/#374 preserves the remaining native drag profiling and responsiveness
gate in Todo. The original broader requirement was split rather than silently
closed with API-only evidence.
Direct Copy Component/API routes alone are not exact native source input proof.
The cumulative API movie remains valid qualified geometry/semantic evidence.
Its native continuation also checks a small recorder correction: delivered
nonspontaneous Qt mouse events now move the visible pointer/ring with a `Qt input`
label, retaining event-log provenance. Source review and main build pass; actual
overlay alignment is pending the first cumulative capture.
The observed blank
command-dock band and native drag latency remain REL-005/REL-011.
The bounded implementation plan is external under
`knowledge/tutorials/piston-subassemblies-2026-10-14/assembly-continuation/asm015-plan.md`.
SRF-016/#360 preserves later surface-as-tool Trim; accepted
SRF-013 is scoped to sketch-normal paired-spline Trim only. Exact shoehorn R12 and enclosure rim
remain blocked Todo work; full surface-iron acceptance is still ahead.

## Accepted retained surface Remove (2026-10-14)

HIS-007 adds one-body `removeBody` history for a current local open
sheet. Browser Remove commits directly; `body.removeDialog` selects, previews,
commits or cancels through the same CadDocument authority. Source features,
authored names and Browser ownership survive removal, and deleting Remove
restores the edited source. Focused core 2/2, a 26-call real-app API run and a
Qt-delivered Browser/dock test pass. The Modify ribbon entry is integrated through VIS-004;
native Escape and multi-body Remove are unverified/out of scope. Independent
compatibility review accepted the bounded ingredient. Main Surface Modify
routing and native selection/Cancel/Browser check pass. See [the receipt](validation/remove-surface.md).

## Accepted local reference canvas (2026-10-14)

DOC-024/#294 now has a separate local-image reference authority, retained real
plane GPU texture, position/angle/uniform-width/opacity controls, Browser
visibility and two-point dimension calibration. Core retained-plane edit and
portable embedded reopen,46-call real-entrypoint API and bounded native
placement/calibration/history checks pass. Independent Astra review accepted
the corrected Browser visibility and completed-pair calibration guards; focused
native regression passes. Integrated into main. See
[the receipt](validation/local-reference-canvas.md) and
[the source contract](research/local-reference-canvas-contract.md). Public
flags/flips/nonuniform controls and exact manipulator/layout parity stay explicit
broader compatibility work. No reference images are bundled as app assets.

## Accepted Two Sides surface extrusion (2026-10-14)

SRF-014/#355 is independently accepted and integrated. Selected open lines and
fit splines produce real surface sheets with editable side reaches and retained
IDs; planar outputs have valid support frames. Core, 48-call API and Qt native
picker/typing/Escape checks pass. Output is a per-curve multi-face compound, not
a sewn shell or completed iron fixture. VIS-004/#358 separately accepts Surface workspace
navigation; Trim/Boundary Fill integration stays in TUT-018. See
[the receipt](validation/surface-extrude-two-sides.md).

## Accepted retained Split section (2026-10-14)

DOC-032/#357 is independently accepted. The enclosure original Top section now
follows its exact named face into the unique Split descendant, without changing
the authored support. Actual 30→31 mm source height edit moves the plane by1 mm;
history/reopen and GPU clipping pass. A bisected side face refuses ambiguity.
See [the receipt](validation/section-split-support.md). Historical movie still
shows the old unavailable section; new separate capture records the fix.

## Cumulative visible navigation workflow (2026-10-19)

NAV-005/#411 now passes a 110-call actual-app cumulative Fit/Home,
Pan/Zoom/Window, selected body/face/small-sketch and hidden-geometry workflow.
The selected planar face of a +90° placed assembly child exposed a false
unsupported-face refusal; `view.lookAt` now checks the uniquely matched
displayed world face for planarity and uses its placed frame. Curved-face and
ambiguous-instance refusals remain. See
[the receipt](validation/navigation-cumulative.md). NAV-008/NAV-009 retain
physical/native input, while GATE-005 retains packaged multi-OS GPU/runtime.

## Retained sketch intersection slice (2026-10-19)

SKT-164/#610 now accepts a finite line-line intersection candidate while authoring a Line endpoint, represented by one retained two-source relation. Both source-line edits recompute a closed profile and downstream Extrude (2000→2500 mm³); finite/parallel/ambiguous sources refuse or omit atomically. Core/HV 2/2, an 84-call real-app workflow, native Qt pointer/badge Delete/Undo 3/3, exact reopen and Apple M4 GPU captures passed independent Astra review. See [receipt](validation/sketch-line-intersection.md). Exact public glyph styling, physical input, linked projected curves and large-sketch performance remain open.

## Simple Hole slice (2026-10-19)

SOL-078/#608 now accepts retained sketch-point Simple Hole on an explicit target with Distance or All extent, editable history, source-point reassignment and upstream width follow. Focused core 2/2 including legacy Countersink, a 73-call real-app workflow, native Qt Point/Create/target/numeric/Timeline input, exact Undo/Redo/reopen, downstream Chamfer and independent closed STL readback passed. Independent Sol review found no blocker; see [receipt](validation/simple-hole.md). SOL-079/#609 now accepts retained Counterbore recess diameter/depth with Distance/All, 3/3 focused core including direct BRep step/depth probes, an 83-call real-app replay, native Qt controls/GPU and independent closed STL; independent Sol review found no blocker. See [receipt](validation/counterbore-hole.md). Direct face placement, broader Hole types, physical-device and packaged runtime remain separate.

## Cone and torus primitive slice (2026-10-19)

SOL-062/#507 adds retained circular Cone/Frustum and ring Torus New Bodies to
the shared primitive API and panels. A 60-call actual-app replay passes edit,
downstream Move, refusal, Undo/Redo, reopen, GPU captures and independent STL
readback; see [the receipt](validation/cone-torus.md). The earlier native Qt probe stopped before first input in a locked macOS
session. SOL-077/#606 now accepts actual-hit Create, numeric, Preview, Cancel,
OK and Timeline edit with exact Undo/Redo/reopen at DPR 1 and 2, readable panel
borders and Apple M4 GPU captures after FND-028 unlocked-window readiness;
independent Sol review found no blocker. Physical/packaged input remains open. Public Torus diameter semantics
and a matching dedicated Cone command remain unproven in the source evidence.

## Cumulative enclosure tutorial gate (2026-10-19)

TUT-016/#299 has fresh from-blank real-app creation footage through the saved
29-feature snapped Case and a separate continuation from that exact model to
the final 33-feature rim/fit Case. The final adapted model reopens; both halves
are valid closed solids, 24 independent half/reference BRep pairs show no
positive intersection, the nominal connector is 0.5 mm clear of each half,
and separate STL/STEP exports pass independent readback. The creation runner's
teardown timed out after completed save/exports; its process then exited and the
later continuation/reopen runners exited normally. The full stage ledger,
gallery links and explicit parity/physical-fit limits are in
[the gate receipt](validation/enclosure-cumulative-gate.md). Historical context
below describes earlier partial stages and must not override this accepted
cumulative result.

## Accepted enclosure recompute improvement (2026-10-14)

REL-009/#356 uses invocation-local solved-history prefixes to remove repeated
OCCT replay inside one candidate recompute. Exact final source and R0.5→R0.45
Fillet edit retain identical authored/body/sketch summaries and fall from
12.950/13.596s to4.701/4.706s headless; append/organization regressions pass2/2.
Independent review and the 36-call actual-app tail pass: opens about5.1s,
Fillet edit4.9s and undo/redo0.2s. No GUI before/after ratio is claimed. See
[the bounded receipt](validation/document-recompute-prefix.md). REL-004 remains
open for broader product performance budgets.

## Accepted enclosure snap checkpoint (2026-10-14)

TUT-022 has independently accepted adapted API workflow evidence through both
Mirrors and the reordered Bottom Fillet groups. The 23-minute creation recording
saved the final model before a late undo-order assertion; separate tail-02 passes
36 calls covering actual edit/history/reopen, adapted section clipping, separate
closed STL meshes and STEP export. Actual Top/Bottom overlap is zero; the known
0.73224 mm³ nominal-board connector collision remains. The gallery now links the
431-second idle-trimmed creation movie, raw trace and separate post-save section.
Full TUT-016 retains DOC-031 native naming, DOC-032 face-linked section support,
and TUT-023 alignment rim. See [the receipt](validation/enclosure-snaps-workflow.md).

## Accepted component printing selection (2026-10-14)

PRT-010/#351 is independently accepted and integrated. Component STL export
selects visible bodies, so the hidden retained source sheet can coexist with its
printable solid. Explicit or visible-sheet selections still refuse strict solid
export. The actual source solid now moves into its sheet component and survives
save/reopen: temporary sketch-solving prefixes exclude final Browser ownership,
matching the history evaluator. The 17-call actual-app check and inspected dialog
capture pass; a closed 2308-triangle STL was independently checked. Visibility
undo is covered, not reparent undo/redo in this run. See
[the receipt](validation/component-mesh-visibility.md). Full source finishing,
native Browser drag and intentional open-surface export remain separate.

## Surface corner Fillet geometry blocker (2026-10-14)

The accepted guided-sheet/Thicken source has a bounded positive-solid
orientation correction independently accepted and integrated, with the focused
core receipt in [shoehorn-thicken-orientation.md](validation/shoehorn-thicken-orientation.md).
It preserves material and retained IDs but does not enable the exact wide-end
R12 corner result: the actual 16 mm-bend fixture still returns an invalid OCCT
shell even from an equivalent-material forward solid. GEO-019/#352 now owns
that exact geometry qualification; GEO-018/#350 remains Todo for generated
curved-face/endpoint identities and the later R1.5 perimeter round. The
external `shoehorn-fillet-radius-diagnostic.md` records source uncertainty,
fixture hash, selected edges and status detail. The complete TUT-017 workflow
remains open.

## Accepted retained surface Thicken (2026-10-14)

SRF-010 is independently accepted and integrated through `224b98e` (plus
comment/evidence qualifications through `8c4213c`). One signed open-sheet offset
creates a separate solid in the active component. The source sheet remains in
its original component and is hidden in the authored creation transaction;
undo and later visibility changes preserve it. Six faces and twelve edges have
stable source-derived identities. Core, 34-call real-app API, Qt face/Flip/Cancel,
STL readback and 71-call from-blank dimensioned source through Thicken pass.
See [the receipt](validation/surface-thicken.md). VIS-003's yellow surface,
blue selection and gray solid distinction is independently accepted from actual
captures; this is not exact palette parity.

The [guided-sheet prefix](validation/shoehorn-guided-prefix.md) is independently
accepted: driving130/50/16 spine, midpoint50/10/10 arc and30 line sections,
endpoint planes, guided Loft, bend16→18 edit and history/reopen. The cumulative
runner `tests/shoehorn_workflow_api.py` is now integrated with optional Thicken
and grouped corner checkpoints. `--through-thicken` passed in external
`deliverables/shoehorn-workflow-2026-10-14/thicken-01/`; `--through-corners`
has run and remains blocked at the exact R12 corner operation described above. Full recording, finishing, hole, final organization
and component export remain open under TUT-017.

GEO-004 tangent-plane core/API foundation is integrated at `2431184` through
`8dbe6a3` and independently accepted for the current curved Loft endpoint.
Native initial focus could not be established, so native Escape/selection stays
unverified; final source Fillet endpoint acceptance remains open. GEO-018/#350
remains Todo for exact generated corner/perimeter Fillet face and endpoint
identities, pending GEO-019. PRT-010/#351 is now accepted: implicit component
printing selection excludes hidden retained sheets, as recorded above.
The public material/ownership and mesh-selection research stays external under
`knowledge/tutorials/advanced-audit-2026-10-14/`. No source workflow steps are
silently replaced by the bounded foundation claims.

## Accepted local object organization (2026-10-14)

ASM-007 is independently accepted for separate body and sketch-folder moves
through shared commands and Browser context actions. The object-owner map keeps
original modeling feature ownership intact, including earlier-feature previews,
source edits, history/reopen and destination-scoped export. Integration is at
`55b93af`; main application build and focused component test pass. Actual pointer drag remains unverified
under ASM-012/#343: synthetic input did not deliver a drop, and the desktop
computer-use backend could not find the running app window. This gap remains a
dependency of the complete surface tutorial. See
[the receipt](validation/component-reparent.md).

## Accepted retained Cut All (2026-10-14)

SOL-047/#287 is independently accepted and integrated at `d9a0003`.
It retains an explicit target body and All/Flip extent through the existing
Extrude Cut authority, deriving signed reach from current OCCT target bounds on
each rebuild. Focused core, 29-call real-app API and native Qt chooser/Flip
checks pass; [the receipt](validation/extrude-through-all.md) distinguishes the
single-target limitation. Main app and focused core checks pass. The full shoehorn
surface/plane/cut sequence remains separate.

## Accepted arc midpoint on attached path sections (2026-10-14)

SKT-068/#282 is independently accepted and integrated, extending the existing Midpoint
constraint to ordinary arc half-sweeps. The native tool selects the arc then
the attached sketch origin; the same sketch/API authority retains a fixed
origin point and rejects the antipodal branch. Source-sized front 50/10/10
and rear 30 sections follow GEO-003 endpoint planes after a spine edit.
Focused core, a 44-call real-app API workflow, and Qt-native arc/origin clicks
with inspected dimension/badge captures pass. See [the receipt](validation/arc-midpoint-endpoint.md).
Main application build and focused arc test pass; broader midpoint,
projection and surface scope stays with their parent issues.


## Accepted endpoint Plane Along Path (2026-10-13)

GEO-003/#281 is independently accepted and integrated for one retained fit
spline at proportional Start/End. It uses the evaluated, dimension-solved
OCCT edge and ordered fit-point identities, with a deterministic perpendicular
plane and dependent sketch support. Focused core, a 38-call real-app API
workflow and Qt-native canvas selection/End chooser/Cancel/accept pass. See
[the receipt](validation/endpoint-path-planes.md). Main application build and
focused endpoint core test pass. Solved-curve picking has separate shared-command
proof; native05 covers basic gestures. Broader path extents stay with the parent.
SKT-068/#282 is accepted and closed after independent review, with
core/API/native evidence and readable badges.
The second Sol lane now audits ASM-007/#300 component reparenting while
the geometry lane completes GEO-014 and SOL-055 reflected-root replay before SRF-009 surface Loft.

## Accepted Selection Priority (2026-10-13)

DOC-025 native Select-menu Body/Face/Edge priority and its selection-transition
fix are independently accepted and integrated. Public toggle behavior and the
explicitly chosen transient Auto default are recorded in [the receipt](validation/selection-priority.md).
Focused Qt-native Box-edge selection, shared API selection and edge replacement
through sketch/profile/plane/tab transitions pass. Face mode remains bounded
to retained planar supports; general curved-face coverage is not claimed.

## Enclosure continuation (2026-10-14)

TUT-021/#322 is accepted and closed. Its 236.2-second from-blank movie covers
shell, mounting pegs, projected connector clearances, long slot and centered
card Cut with R2 lip. Exact authored reopen and Case-only STEP/STL readback
pass. See [the receipt](validation/enclosure-port-card-workflow.md); the movie,
model and exports are first in the external gallery (82 links checked).

TUT-022 now has a candidate from-blank 23-minute creation recording and saved
Top/Bottom model through both feature Mirrors, Bottom Fillet reorder, and two
opposite R0.5 groups. A late undo assertion stopped the original run after
save; the saved model passed a separate real-app geometric edit/undo/redo,
authored reopen, individual manifold STL and STEP export tail. Actual
Top/Bottom positive-volume overlap is zero. The original Top face-linked
section becomes unavailable after Split, so the tail explicitly uses a
same-position origin-plane inspection section; native typed body rename still
belongs to DOC-031. See [the candidate receipt](validation/enclosure-snaps-workflow.md)
and its external partial-run/tail evidence. TUT-022 awaits independent
cumulative review and any remaining source gate; TUT-023 alignment rim follows.
Actual final seam inspection found one closed
U-shaped wire with18 edges, all18 now named through accepted GEO-016/#345 (`87b5ee5`),
including12 anchored-chain identities. Source height edit/history/reopen and
legacy Split regression pass. SKT-076/#346 remains Todo for
retained inset-interior profile selection, pending GEO-017/#347: both Arc and
Intersection OCCT offset probes failed the actual inward1.25 result. No UI
or ideal-outline substitute is accepted. Source-to-offset shoulder bands
would be the wrong region. The actual port opening must remain unbridged. HIS-006 is independently accepted at `6684dac`:
actual Bottom Fillet reorder after both Mirrors, separate opposite R0.5 group,
24-call API/history/reopen and Qt native cancellation/drop pass. The final
Top/Bottom positive-volume intersection is zero; nominal connector overlap
remains qualified. See [receipt](validation/timeline-reorder.md). Accepted prerequisites include
SOL-052 owned Split, GEO-012 exact inner Cut rims, GEO-013 retained Cut side
supports, and SOL-054 connected selected-region Join replay. SOL-054 technical
YZ replay/edit/history/reopen is accepted at 9179015, but its technical finish
choices do not qualify the source attachment. GEO-014/#340 is independently accepted and integrated at `6eca141` for the
actual wall/stem junction edge: unique OCCT Modified tool-edge history and
incident target face can identify the current vertical root at
X=-28.5,Y=17.5,Z15..20.5. The actual root accepts smaller chamfers but refuses
the required 2 mm at the inferred 2 mm stem width, even on fresh Join.
GEO-015/#341 separately owns this source-geometry/exact-size qualification.
The bounded fallback is independently accepted and integrated through `1f4b4ea`.
Focused exact-size, rigid rotation/reflection, oversized refusal, assembly
interference, source edit/history/reopen and 23-call real-app checks pass.
Reflected OCCT planes require parametric U × V normals; that correction is
accepted without weakening the geometric certificate. Main application builds.
The actual five-feature reflected group is independently accepted at `b92db08`;
GEO-015 is closed. Source-sized Mirror edit/history/reopen passes with the
exact 2 mm root, and final paired Bottom fillets resolve intermediate overlap.
SOL-055/#342 is independently accepted and integrated at `ad23a81` for
reflected attachment-root lineage and supported technical finish replay.
Its core source edit/history/reopen and 13-call real-app inspection pass;
the broad capture does not visibly expose the root. Exact-sized source finish
and actual five-feature group remain GEO-015/SOL-053/TUT-022.
Do not substitute the hook elbow or a transverse
free edge merely because its Chamfer succeeds. SOL-053/#337 is independently
accepted and closed for the source-sized YZ snap group. TUT-022 still owns the
full source-step runner/movie; DOC-030/#344 is accepted at `cc52857` for
retained body naming and actual Split API/history/reopen. DOC-031/#348 retains
the unresolved native modal focus/typing gate; the context action opens and
Escape cancels. Named body consumption currently refuses atomically. SRF-009 is active in parallel: the actual
arc-to-line guided surface is now independently accepted and integrated.
SRF-010 Thicken viability is confirmed on that sheet; retained solid
application integration remains active.

The accepted nominal board is rotated 90 degrees about Z. Source opposite-wall
reflection therefore maps explicitly to Alloy origin YZ, not XZ. Actual opposed
Cut API/native proof is integrated at `62d164a`: each closed 6 × 3 catch removes 63 mm³
at X=+/-32,Y20 and clears the long slot by 6.75 mm. Earlier same-wall XZ fixtures
qualify only their bounded lineage ingredients. The inferred hook starts from
the exact Cut side at Y17, offset to Y17.5; retained Project lines partition its
outline into selected 20 + 7 + 1 mm² cells and an excluded 3.5 mm² cell. Public source
instructions explicitly select multiple profiles for Join 5; exact source
support/sign/corner and clicked cells remain uncertain. The actual Join adds
no measured interference; nominal fixture already has 0.73224 mm³ Top overlap
with SIDE-ETH-001, unchanged by the snap. Physical fit is not certified.

HIS-006 candidate is rebased at `5ec4b64` in the timeline-reorder worktree, with
original backup retained. App/native target build and prior one-Mirror core
pass. Final API/native checks require Bottom R0.5 before BOTH Cut and snap
Mirrors, then move it after both and edit the opposite retained edge. Current
technical snap diagnostics do not yet author that source order. Root is now running the actual two-Mirror API gate in the rebased
timeline-reorder worktree. The fixture has full Top five-feature finishes,
Bottom R0.5, Cut Mirror then snap Mirror; the final edit adds the opposite
inner edge as a separate R0.5 group. No extra one-Mirror rerun qualifies it. Sol lanes currently own GEO-014 and ASM-007 reparenting. Coordinate the sole GUI lease live rather than assuming a stored owner.

## Accepted snap-group feature Mirror (2026-10-13)

SOL-051/#327 is independently accepted and integrated. The retained source
Join, three Chamfers and Fillet replay across origin XZ, including source edit,
history/reopen and an unrelated Bottom Fillet interlude. The real-app API,
five-Timeline-tile native selection/Cancel and clear both-snap captures pass.
See [the bounded receipt](validation/feature-mirror-snap.md). Cumulative source
placement, Cut Mirror interaction and Fillet reorder remain TUT-022 work.

## Accepted section updates during sketch (2026-10-13)

DOC-028/#333 preserves active sketch drafts through retained section visibility
and shared numeric edits. Native Browser visibility, Project continuation,
Finish/Cancel and atomic invalid edits are independently accepted. Native
nested numeric section dialogs remain DOC-029/#334. See
[the accepted receipt](validation/section-during-sketch.md).

## Accepted Join/Chamfer edge lineage (2026-10-13)

GEO-010/#329 is independently accepted and integrated as a backend prerequisite. Extrude Join
publishes current retained prism edges, and Chamfer preserves unique OCCT
Modified descendants. An accepted rounded-Shell/Split fixture now supports the
source-sized Join5→three Chamfers→Fillet0.5 sequence, meaningful Join edit,
atomic invalid edge, history and reopen. Independent review and main integration
are complete. See [the accepted receipt](validation/join-edge-lineage.md).

## Accepted retained Cut feature Mirror (2026-10-13)

SOL-050/#326 is independently accepted and integrated for the first
undercut Extrude Cut feature Mirror across origin XZ. Core, real-app API and Qt
Timeline-selection checks pass with inspected two-pocket bottom-view captures;
independent review and main integration are complete. The later selected
Join/Chamfer/Fillet group is SOL-051/#327. Subsequent accepted source-size,
reorder and cumulative gates now satisfy SOL-049/#293; the independent parent
audit closes it without repeating those checks. See
[the candidate receipt](validation/feature-mirror-cut.md).
## Accepted closed projected rectangle Offset (2026-10-13)

SKT-073/#328 is independently accepted and integrated:
four retained projected rectangle edges create one stable signed Offset group.
The three actual adapted connector faces produce four disjoint profile cells,
exact 973.5 mm² union area, and one Cut To Object through the Case wall.
Real-app API and native Qt evidence, retained plane edit, atomic refusal,
history/reopen and the limitation on topology-changing individual clearance
edits are recorded in [the candidate receipt](validation/closed-rectangle-offset.md).
Independent review and main integration are complete. SKT-074 separately owns
explicit aggregate selection across changed cell contacts.

## Latest checkpoint and next work (2026-10-13)

TUT-020/#320 is independently accepted and integrated: from blank, the nominal
board reference, rounded closed case, three named sections and four linked
mounting pegs pass 110 real-app API calls, a 145.2-second creation movie,
placement edit/undo/reopen and independent Case-only STEP/STL readback.
A separate neutral top capture shows all four pegs. See
[the accepted prefix receipt](validation/enclosure-prefix-workflow.md).
GEO-008, SKT-071 and GEO-009 are accepted, integrated and closed.
Current work: SKT-072/#325 construction-plane Project, SKT-073/#328 closed
rectangle Offset and SOL-050/#326 Cut feature Mirror are independently accepted,
integrated and closed. TUT-021 is now accepted; Sol is preparing TUT-022 from its saved model. SOL-051/#327 mirrored snap-group replay is accepted and integrated after
GEO-010/#329 proved the ordinary five-operation chain. The second Sol lane is
implementing HIS-006/#280 Timeline reorder headlessly; the actual two-Mirror
source proof remains required before acceptance.
GEO-011/#331 current Cut-rim catalog publication is independently accepted and
integrated; the correctly centered card opening now supports its R2 fillet. An initial origin-snapped diagnostic was
misplaced and is explicitly failed evidence; SKT-075/#332 tracks fresh face-sketch
origin compatibility without rebasing old sketches. The accepted SKT-075 implementation now
versions fresh direct-face sketches and preserves absent-field legacy frames;
focused core and API evidence are in [the accepted record](validation/face-sketch-origin.md).
Native pointer-origin input remains SKT-077; transformed-instance authoring remains outside this bounded implementation.
SOL-049 is now closed
after an independent audit of the subsequently accepted source-group and
Timeline-reorder gates. The following lane notes describe that earlier checkpoint. The first source-order TUT-021 run exposed a retained
section update blocked during sketch authoring: DOC-028/#333 is accepted and integrated for
analysis-only shared updates that preserve the sketch draft. DOC-029/#334 keeps
the native nested-editor follow-up Todo; no native parity is claimed from API
updates. SKT-074/#330 separately tracks
explicit union-profile selection across topology-changing clearance edits.

Next source stages are TUT-021/#322 (ports and card opening), TUT-022/#323
(Split and mirrored snap groups, depending on SOL-049 and HIS-006), and
TUT-023/#324 (alignment rim and Keep Tools). TUT-016 remains the full enclosure
gate. Broad sketch support, Shell, Fillet, Chamfer and pattern family issues
remain open; accepted bounded foundations and these source stages now carry
the enclosure dependencies. External evidence remains under
`../Alloy-reference-records/deliverables/enclosure-prefix-2026-10-13/final/`.

## Accepted Overall Slot and origin-section offsets (2026-10-13)

SKT-069 true end-to-end slot length/width/angle and DOC-027 origin-plane
Distance/Flip are independently accepted, integrated and closed. Main app
build and three focused slot/section checks pass. Native and API evidence
remain distinct; exact OS input and reference styling are not claimed.
See [Overall Slot](validation/overall-slot.md) and
[origin-section offsets](validation/origin-section-distance.md).
## Accepted closed rounded-prism Shell (2026-10-13)

GEO-008/#316 is independently accepted and integrated. The corrected source-shaped centered Rectangle93×64 →
Extrude30 → complete grouped R5/R2.5 Fillet → closed Shell3.5 passes an analytic
26-patch convex-envelope certificate and yields a valid exact cavity, six inner
planar references, retained Midplane Split, edit/history/reopen and two checked
printing meshes. The earlier Box50×80×40 remains an engineering regression.
See [the bounded candidate receipt](validation/closed-blend-shell.md). The
cumulative enclosure tutorial is still pending.

## Retained-plane Split Body (2026-10-13)

SOL-045/#279 independently accepted: two closed solids from an active body and
retained construction plane, stable signed-side IDs and cap references, closed
cavity partition, edits/history/reopen, named native selectors and separate STL
readback. See [Split Body receipt](validation/split-body-plane.md).
## Accepted local reference insertion (2026-10-13)

ASM-010/#314 in `feat/local-reference-insert` extends the accepted embedded
XDE graph with a local source locator and one outer rigid placement owned by
the import feature. A new local component owns its imported bodies; component
opacity is a separate presentation value. Move previews a pending pose;
Capture Position commits it and Revert discards it. Modeling, save and export
refuse while a pose is pending. An explicit Update reports missing or changed
source; byte-identical input remains current. Changed-byte topology
correspondence is tracked separately by ASM-011/#315, rather than silently
reusing face/edge IDs. The bounded core test and real-app workflow pass on the
qualified twelve-solid board: moved face and edge ray picks, sibling Case
Project following Capture, opacity, history/reopen, and source-state refusal.
Native Qt typed Move preview/Cancel passes with inspected captures. Astra's
independent review accepts the final Escape-safe candidate, now integrated. See
[local reference candidate](validation/local-reference-insert.md).

## Accepted STEP assembly import (2026-10-13)

XCH-009 in the isolated `feat/step-assembly-import` worktree implemented
one retained XDE solid assembly import transaction. It embeds the source and
source-derived graph, preserves nested/repeated identities, evaluates placed
leaf solids for rendering/inspection/export, and exposes a read-only Browser
hierarchy. Focused core tests pass on a synthetic nested rotated repeat and
the external twelve-solid board fixture. A 30-call real-app API run and a
physical Qt Browser leaf-click test pass, with inspected GPU captures;
independent bounded review accepted the candidate and main integrated it.
Viewport ray-picking of a moved imported occurrence is covered by ASM-010's
candidate gate. See
[STEP assembly candidate](validation/step-assembly-import.md). XCH-003 and
ASM-006 retain broader exchange fidelity and editable insertion.

## Current implementation lanes (2026-10-13)

XCH-009/#313 assembly STEP import and SOL-046/#285 grouped Fillet are
independently accepted, integrated and closed. Combined main app build and
focused STEP/grouped-Fillet integration checks pass (2/2). Respective receipts
record API/native/geometry scope and later transformed-pick/Thicken limits.

GEO-008 exact recognized convex closed Shell is independently accepted and integrated.
SKT-071 retained circular model-edge Project is independently accepted and integrated.
GEO-009/#321 fixes automatic center ancestry for generated shell arcs; the actual
cumulative inner-floor sketch/Project/Offset/Join/edit/reopen/export sequence passes.
TUT-020 full-creation movie and independent cumulative review are accepted. RES-020
independently qualifies the external v2 nominal card envelope for correct
source-side placement while preserving v1 receipts. TUT-020/#320 is now active:
a bounded from-blank cumulative mounting-peg workflow and creation video, with
ports/snaps/reorder and the remaining full enclosure stages still in TUT-016. DOC-023 selected-face Section Analysis is independently accepted
and integrated; combined app build and three focused integration checks pass. Root completed SOL-048 supported closed inward Shell on main, independently
accepted at 5b1bb29 with native and 28-call API evidence. Plain Box cavity, six inner planes, edit/history/reopen and
atomic failures pass core. The original grouped R5/R2.5 followed by Shell3.5 exposed
OCCT offset collapse; accepted GEO-008/#316 now handles the certified convex
case through an exact cavity construction. The actual tutorial stock is Center Rectangle93x64
followed by Extrude30; Box50x80x40 is only a regression fixture.
Do not replace the source with a plain/open box or reduce its thickness.
ASM-011/#315 separately owns changed-byte reference Update correspondence;
ASM-010 only supports unchanged-byte update/status and atomic changed refusal.
ASM-006 stays Todo as broader parent. The enclosure gate depends on the actual
source-required bounded children, not optional changed-file update breadth.

## Current advanced tutorial work (2026-10-13)

TUT-015 is independently accepted and closed: 228-call actual-app full creation
run, 97-second API movie plus labelled 21.8-second focused thumb replay,
parameter resize, correct front-opening Slider, history/reopen and separate
Box/Lid printing exports. Artifacts are external under
`deliverables/sliding-box-2026-10-13/creation-final` and `thumb-visible-final`.
See [the cumulative receipt](validation/sliding-lid-cumulative.md). ASM-009 is
also accepted and closed. The main movie retains the offscreen thumb segment;
the separately labelled focused replay provides readable creation evidence.

GEO-005 Midplane is independently accepted and integrated: exact parallel face
references, sequential canvas selection, preview/Cancel/edit and shared API.
Core/native checks and a 33-call real-app workflow passed; main application
build and focused core pass. The issue is closed. See [Midplane receipt](validation/retained-midplane.md).
RES-017 independently qualifies an MIT independently authored board assembly
reference externally. One root, five groups and twelve solids preserve relevant
mounting/port geometry. Official dimensions and nominal envelopes are explicitly
separate; no exact source-file or physical-fit claim. Hierarchical STEP import
and local insertion remain the next enclosure application prerequisites.
XCH-009/#313 now owns the bounded authoritative STEP solid-assembly import
child, in parallel with SOL-046/#285 grouped Fillet. Both Sol lanes work in
isolated worktrees; Astra reviews schema/source and independently verifies.
The broad XCH-003 parent remains Todo for full attribute fidelity and export.

## Accepted advanced tutorial ingredients (2026-10-13)

ASM-005/#296 Slider is integrated and independently accepted. SOL-043/#274
To Object and SKT-070/#306 typed Line length/angle are independently accepted
and integrated; the combined app build and six focused core checks pass. Their receipts are
[Slider](validation/as-built-slider.md), [To Object](validation/extrude-to-object.md)
and [typed Line](validation/typed-line-angle.md). These are bounded ingredients;
the cumulative sliding-lid tutorial gate remains open.

## Accepted primitive Box topology (2026-10-13)

The integrated implementation adds six stable planar supports and twelve straight
edges to the real primitive Box, plus uniquely mapped straight inner Shell
boundaries. The Box50×80×40 → top-face Shell4 core gate, edits, exact
undo/redo and reopen pass. A 28-call real-app shared-API run verifies viewport
face picking, preview/Cancel, Shell, GPU captures and edits; the private editor
is closed. Physical native input remains unverified. See [the bounded candidate
receipt](validation/primitive-box-topology.md).
Linked Project and To Object are also independently accepted and integrated.

Updated 2026-10-13. This is the project handoff for a fresh session. Read the root
[AGENTS.md](../AGENTS.md) first; its reference restrictions apply to all work.

The original overnight queue and ornament milestone are accepted. On2026-10-13
user authorized the advanced tutorial queue: sliding-lid box, snap-fit electronics
enclosure, surface shoehorn, then surface iron and piston subassemblies. Five
cumulative gates TUT-015–019 now track the required existing owners and bounded
new children. Detailed public research stays external at
`knowledge/tutorials/advanced-audit-2026-10-13/independent-spec.md`.

PAR-004/#270 and DOC-022/#271 are independently accepted and integrated:
Box/Shell direct named lengths and primitive/inner-Shell face/edge references.
Main builds; the combined `tests/sliding_box_prefix_api.py` passes real primitive
Box→Shell, source-size edits and thickness3, stable back-wall references,
undo/redo and exact reopen with an inspected hardware GPU capture. Receipt in
`docs/validation/box-shell-parameters.md`; artifacts external at
`deliverables/sliding-box-2026-10-13/prefix-01`. Both issues are closed.
SKT-066/#272 linked Project and SOL-044/#275 Offset-start Extrude are also
independently accepted and integrated. The full lid workflow/movie remains open.
SKT-067/#273 open-chain Offset and PAR-005/#276 model parameter table are
independently accepted, integrated and closed. Sol owns the cumulative TUT-015
runner; a second Sol lane is starting GEO-005/#278 retained Midplane for the
next enclosure gate, with Astra independent verification. The Offset must retain the five-segment open dovetail chain,
include top shoulders and leave the original top closure separately authored.
The parameter table must rebind existing numeric modeling inputs, preserving
stable parameter identity. Root integrates accepted ingredients and prepares the
cumulative source-order gate. The new `tests/sliding_lid_workflow_api.py` passes
the numeric Box-component/Shell prefix, projected 4 mm/55 degree dovetail,
source-style short Midpoint axis and endpoint Horizontal constraint, Mirror,
actual trapezoid To Object Cut, undo/redo and exact reopen. Captures/ledger are
external at `deliverables/sliding-box-2026-10-13/dovetail-prefix-03`; independent
prefix review accepted the source, ledger, analytic cut volume and GPU capture. It uses the mirrored left half rather than asserting
the source right-half screen sequence. Lid, parameter-table and thumb-divot
stages are now present; see the current cumulative status above. To Object derives extent from current retained face
geometry, never a frozen authored distance. REL-008/#308 is independently source-accepted: a non-mutating optional-origin
read repairs general sketch Fillet, and existing face-support diagnostics are
preserved. Existing focused Fillet/Project checks pass. The larger face_support
fixture remains intermittently blocked by separately tracked GEO-006 edge
provenance ambiguity; no clean-suite claim is made.
The integrated Project app builds and its focused core test passes. Root owns the
sole private-editor lease; no private editor is currently running. Later source/input
qualification remains explicit. Five gates have 38 new prerequisite/research/gate
tasks; two separately tracked regressions and a newly exposed Cut-edge dependency make
311 total canonical issues. GEO-007/#310 is independently accepted and integrated: the actual dovetail Cut omitted
new slant/bottom and split shoulder references from the Project catalog. Exact-history straight Cut boundary naming now exposes the five required notch
edges. Its 27-call real-app continuation verifies actual sibling Project, width
and thickness edits, semantic shoulder IDs, merged-bottom contributors, undo
and reopen. See `docs/validation/linear-cut-project.md`.

## Remaining lid gate integration (2026-10-13)

GEO-007 is closed; its actual edited Cut→Project continuation passes. PAR-005 model parameter table is independently accepted and integrated. SKT-067
Offset is independently accepted and integrated, including final native palette
layout and actual projected chain selection/Flip/Cancel. Root owns DOC-026/#311, an additional
TUT-015 dependency exposed by the front-face thumb midpoint requirement.
DOC-026 is independently accepted and integrated. Straight extrusion base
boundaries have exact FirstShape lineage; signed/start-offset and stable-ID
core checks pass. The 40-call `tests/base_cap_midpoint_api.py` verifies retained
front top-midpoint ellipse, source width edit, profile creation and actual thumb
Cut. Projected-profile extraction retains intrinsic ellipse axis relations.
See `docs/validation/base-cap-midpoint.md`. PAR-005 is integrated with the Offset candidate; four focused combined core
checks pass. The cumulative-01 real-app run now passes actual Box/Shell/dovetail
Cut, sibling Project/Offset/closure/Lid To Object, post-authoring seven required
bindings and the source 74×40×100 resize. Thumb Cut, actual Lid Slider and
final save/export checks remain in progress in the same runner.
The full tutorial still needs the combined lid Offset/closure, late model-table
rebinding, source resize, 15×2 thumb Cut and actual assembled Slider demonstration.
Keep one private editor leased at a time; agents coordinate the handoff with root.

## Accepted ornament gate — 2026-10-13

The complete bounded ornament tutorial264 and binary normal fix269 are now
independently accepted and closed, as are all prerequisites260–268. Receipt:
[ornament tutorial acceptance](validation/ornament-tutorial-acceptance.md).
Cumulative05 supplies the82.6s creation movie, editable original/offset4 models,
count/source edits, history/reopen and accepted actual GPU captures. The separate
geometry followup passes both saved models, allnine exact input containment,
closed apex and through bore. Reexport01 supplies both corrected strict closed
millimeter STLs; initial failed05report/mesh remain unchanged. Export correction
changes only normal bytes, preserving all vertex/attribute bytes and facet counts.
Main app builds; independent source/core/runtime reviews integrated6dc450c.
The external gallery now contains11 complete creation videos,9 solid exports and
2 sketch lessons. All private editors and root validation processes are closed.
Native-input/exact pixel parity and source-free spline coordinates remain qualified.
The checkpoints below retain historical attempts; this accepted status supersedes
their active/pending/failure descriptions.

Latest: corrected cumulative-05 is terminal. Creation visuals independently pass,
including closed pointed apex;82.6s5fps movie is valid. Original strict STL,
source/count edit/history and both reopens pass. Edited binary STL then fails
facet-normal agreement with emitted vertex winding. PRT-009/#269 is the bounded
active export fix;264 returns to Todo with that dependency. Sol diagnoses the
preserved mesh; no relaxed checker or geometry policy. A separate headless
saved-model geometry audit is running because the API runner stopped before it.
All private editors are closed. Do not rerun creation for an export-only fix.

Current: cumulative-04 finished with complete API/history/reopen, both strict
millimeter STLs and saved-model geometry oracle passing. Its85.6s recording is
valid, but independent visual review rejects the truncated crown. Append timings
are plane4.73s/sketchfinish4.41s/Cut10.89s (previous129.86/126.39/134.50s).
Full open129–142s and upstream edit146.54s still rebuild as required.
The bounded tip-fixture correction22edff7 moves only the upper free fitpoint to
(17,89), a disclosed choice, not a public numeric specification. Exact−5/−4
Intersect seeds preservez80; original retained Join has solid near-tip material.
Cumulative-05 is now running with strengthened apex checks and a5fps real-time
recording (long exact geometry waits remain visible, not time-compressed).
Main app is unchanged; saved-model verifier rebuilt. No05 acceptance yet.
Issues265 and268 are independently accepted and closed;264 is the sole active
ornament gate. Corrected05 creation has saved the original model/STL and a valid
82.6s5fps movie with the pointed apex visible in the actual GPU capture. It is
currently validating the upstream offset edit; final corrected geometry and
independent visual acceptance remain pending. Earlier04 artifacts are preserved.

Latest: #268 bounded append reuse is integrated0088e64 with independent
source/core acceptance9d6e20a. Four focused checks pass; main app and saved-model
geometry verifier build. Fresh cumulative-04 is running from blank with creation
recording, source edits/history/reopen and strict STL/oracle checks. No result is
claimed yet. Root owns the sole private editor. See
validation/document-prefix-append.md; upstream edits/open still fully rebuild.

Cumulative-04 creation now completes with a valid85.6s recording, original
editable model and original millimeter STL; edit/reopen/oracle checks remain live.
Independent visual review found a material fixture mismatch: the current strip
truncates the upper crown near z70.48, whereas the public source has a closed
pointed apex. Full tutorial visual acceptance is blocked on source-supported
strip placement correction; exact free spline points are unknown. Sol and Astra
are investigating the fixture without changing kernel/tolerance policy. Preserve
this run as useful functional evidence, not complete visual parity.

Shell correction #266 is independently accepted and closed, integrated
0499f83/d3d3fbe. Main app and saved-model geometry verifier build; generic Shell
and full-Revolve support core2/2 pass1.50s. Real-app source/control edits,
history/reopen, corrected section captures and retained strip Intersect pass.
The exact circular closed-cap policy is bounded; source topology parity remains
unknown. #265 is now In Progress for cumulative Pattern/Join/bore/edit/STL;
#264 full movie remains Todo. The strengthened saved-model verifier is integrated
a919696 but has not evaluated a complete ornament yet. This current status
supersedes earlier pending #266 descriptions below.

The first cumulative run (`deliverables/ornament-2026-10-13/cumulative-01`
external) passes source Shell/Intersect and retained Pattern8, then refuses
the explicit eight-body hub Join with `join_lineage_ambiguous`. Report, full
transcript, partial recording and editable Pattern8 checkpoint are preserved.
No final hole/model/STL pass is claimed. Sol investigates ownership coverage
versus actual material loss under #265; the private editor exited. Main source
b94fbf2 includes the prepared original creation recording and two-STL recipe.

The numerical Join candidate is integrated a22c676; main app and latest strict
geometry verifier build. Its explicit consistent adaptive integration and combined
estimated-error budget retain the original material threshold. Independent raw
BRep review classifies shared-edge changes as added support representations; all
original geometry/tolerances/topology remain, but byte identity is not claimed.
The full `cumulative-02` run has started with a180-second command timeout and
creation recording. Its outcome is pending; do not treat a running gate as passed.

`cumulative-02` is terminal: actual hub Join passes, then construction-plane
creation times out180s during repeated coverage Common computation. Recorder
cleanup refuses its long frame gap; no full movie/bore/STL is accepted. The
private editor is closed. #265 numerical correction has independent acceptance
(c508f3a), but a narrow single-output ownership optimization is under review to
resolve the observed recomputation cost. Primary timeout and secondary recording
failure are preserved; runner cleanup now records them separately.

Performance follow-up SOL-042/#268 is now active; #265 returns to Todo with
that explicit dependency. The independently accepted single-result optimization
8700806/f15b469 removes coverage Common overhead, but cumulative-03 still spends
over60s in actual Fuse Build while appending a construction plane. This live
run remains observed; no restart or success claim. Root profiles are archived
under external validation/ornament-feasibility/cumulative-{02,03}-plane-cpu-sample.txt.
Sol is comparing repeated evaluation inputs before selecting a bounded fix;
geometry approximation and tolerance changes remain excluded.

Cumulative-03 is terminal with a complete saved original ornament (one valid
closed solid,17695.726643306665 mm³), including the bore. Initial Join23.67s;
plane129.86s, sketch Finish126.39s, Cut134.50s. No command timed out; the sole
terminal failure was recording.stop frame-gap refusal, before edit/reopen/STL.
The private editor is closed. Source solves and rebuilt prefix BReps are exactly
stable; #268 now implements independently reviewed one-feature append reuse
through the same evaluator, guarded by exact prefix/metadata equality.

## Latest accepted checkpoint — 2026-10-13

Full slots/splines/conic/ellipse lesson #257 and color follow-up #259 are accepted
and closed. The complete31-step API sequence passes247calls, saves three editable
sketches with first two hidden, and verifies analytic geometry plus meaningful
edits/exact undo/redo/reopen. The64.5s full creation movie predates the final
solved-color correction; four corrected real-app captures independently verify
black defined ellipse, blue free conic, orange selection and blue ellipse after
removing a dimension. See validation/slots-splines-tutorial.md and
validation/entity-solved-colors.md. No solid/STL is required by this sketch lesson.
Main app builds and combined solved-color/ellipse/arc-slot core3/3 pass1.44s.
All private editors are closed. Original failed attempts are preserved externally.
The local gallery now includes ten complete creations (eight solid meshes and
two sketch lessons), explicitly qualifying movie versus corrected-capture timing.
Older checkpoint entries below retain historical state; this paragraph supersedes
their pending or active claims for #251–257 and #259. Slot drag preview #258 is
now independently accepted and closed (79fa11b/e805afb); main app builds.

## Join lineage immutability accepted — 2026-10-13

SOL-041/#267 changes the coverage Common probe to configure non-destructive
execution before Build, avoiding the eager two-shape constructor. Multi-party
Join source BRep/profile/authored-state preservation and repeated ownership/
lineage/validity checks pass with the existing successive Join regression.
Only OCCT's documented container Free flag is baselined in the test; geometry,
tolerances and topology remain exact. See
[receipt](validation/join-lineage-immutable.md). Independent Astra review and main integration are complete; the main app build
and focused two-test check pass. No UI or cumulative ornament policy is changed.

## Circular Body Pattern accepted — 2026-10-13

#263 is independently accepted and integrated with shared Circular Pattern controls,
retained Origin Z, Full distribution and total quantity including seed. Original
seed ownership is retained; generated bodies/faces/edges have deterministic UUIDs
and exact OCCT transform lineage. Count/source edits, downstream eight-body Join,
atomic removed-reference refusal and history/storage pass core checks; native
selection/count/Cancel/OK/edit Escape and actual readable GPU captures pass.
See [contract](research/circular-body-pattern.md) and
[receipt](validation/circular-body-pattern.md). Main app build and the focused
Pattern/Mirror checks pass; complex ornament Join #265 and complete gate #264
remain separate.

## Current work — ornament tutorial (2026-10-13)

The complete ornament lesson is tracked by #264, with prerequisites #260–263 and
Boolean correction #265. #260 is accepted and closed; main app rebuild passes.
Retained open fit-spline Offset (#261) is independently accepted and closed,
integrated03de12f/0564a19. Main app build and focused core pass. Native preview
shows both curves through Cancel/accept; API closure/Intersect and exact history
evidence remain separate. See validation/fit-spline-offset.md. Section Analysis (#262)
is independently accepted and closed: core, 79-call actual-app API, native3/3,
actual GPU captures and integrated main build/core pass. Circular Body Pattern (#263) is independently accepted and closed, integrated
as ebd5647/d0ba161/b1be3bf/51d79df. Main app build and Pattern/Mirror core2/2
pass1.51s; native05 settled captures and exact-offset location rotation are
reviewed. See validation/circular-body-pattern.md.
#266 owns the localized inner Shell seam correction. Join lineage correction
#267 is independently accepted and closed (b36fbac/5498152): kernel configuration
precedes execution, repeated evaluation preserves geometry, and main app plus
focused core2/2 pass1.59s. UI and numerical policy are unchanged. #265 is back in Todo pending those
inputs; its cumulative Join investigation remains recorded.
Acceptance still requires retained Pattern/Join reproduction. Public sources/visuals stay external in
the ornament-pattern knowledge record.

Full-Revolve planar support #260 is independently accepted: actual 80/5/R50
profile, generated bottom-face lineage, Inside Shell 5 mm, measured wall/opening,
upstream edits and exact history/reopen pass. Native face selection and Shell
preview/Cancel/commit pass with inspected GPU captures. See
validation/revolve-planar-support.md. The full ornament has not run; all source
steps and the cumulative Join risk remain required.

Section Analysis is integrated as28fba0d/4fa3b7c/ffa489f. See
validation/section-analysis.md for saved analyses, exact geometry reuse, UI and
qualified native/API/GPU evidence. The default main app is rebuilt.
The external `validation/ornament-feasibility/` kernel probe exposes an unresolved
cumulative risk: the exact-offset strip produces a valid shell Intersect member
and first-member/hub Join, but eight rotated members fail to unite validly under
both sequential and simultaneous Boolean strategies. These probes predate the
retained Circular Pattern candidate and do not establish its cumulative result.
Do not claim #264 feasible or complete from the separate ingredient tests. The
chosen image-like spline coordinates and failed logs are retained externally;
Sol is reviewing Boolean robustness without replacing the source geometry.
#265 (SOL-039) tracks cumulative Join after the active #266 Shell correction;
#264 depends on both through this chain.
A second external probe using the actual candidate SketchRegions profile and
GeometryKernel extrusion/Intersect reproduces invalid fifteen-solid array Join.
Actual retained Circular Pattern/application reproduction is still required.

The actual blank-document prefix now reaches the source shell, retained Section
and closed offset strip but refuses the 60 mm Intersect with
`boolean_empty_result`. This differs from the headless fixture and is the current
investigation priority before array Join. Runner22d8237 and
validation/exact-offset-join-investigation.md identify preserved46-call evidence
and exact pre-Intersect authoring. The private editor exited; no seed or full
tutorial pass is claimed. Shared-transform, SameParameter and bounded offset
approximation probes all failed to cure the raw array Join; no fallback changed
production geometry. Clean, pre-Boolean checks now localize a self-interfering
inner Shell seam: curve/surface deviation about0.7657mm under0.8039mm tolerance,
despite passing BRep validity and regular off-seam thickness checks. #266 owns
that correction. A separate deep-copy rotation makes the exact-offset prism
invalid; preserving geometry under a rigid location transform passes the same
checks and is accepted in #263. Astra's shared-partition proposal has not
run because its prerequisite shell is not Boolean-valid. Earlier diagnostic
metrics taken after an eager Boolean constructor were confounded; subsequent
before/after-hash checks independently establish the untouched-input defects.

Actual cumulative-01 now passes exact Shell/Intersect/Pattern8 but hub Join
refuses `join_lineage_ambiguous`. Headless retained Fuse is one Boolean-valid
solid with no participant-minus-result residual solids; mixed fixed/adaptive
coverage estimators caused false ownership refusal. The bounded #265 candidate
uses consistent explicit GK1e-11 and both reported error estimates against the
unchanged material threshold. Simple Join regressions pass; source-specific
retained Join now returns, but adds located pcurve representations to shared
inputs; independent raw-diff reconstruction confirms additive support caches
with exact original geometry/topology/ranges/tolerances under index remapping,
not byte identity. Containment and cumulative acceptance remain pending.
See validation/exact-offset-join-investigation.md and external retained coverage
reports. No dimensions, kernel tolerance, authored evaluator or Fuse changed.

The numerical correction permits actual cumulative hub Join, but downstream
construction-plane ordered recompute times out180s in repeated coverage Common.
The bounded performance candidate uses completed valid single-solid Fuse union
ownership without repeating Common per input; multi-output grouping, actual
history, addition/ledger checks remain unchanged. Focused single/multi Join core
passes; retained headless Join is21.774s with same result geometry/ownership.
Full application latency and cumulative gate remain pending; no broad caching.

#268 candidate exact one-feature append reuse now builds. Repeated source solves
and preJoin input BReps are byte-identical; full ordered re-evaluation nevertheless
repeats an expensive Fuse. A member current-document path reuses the complete
accepted evaluation only for exact authored/resolved/global/order prefix matches
and one appended plane/sketch/Cut; automatic Cut source-hide is the sole metadata
exception. Same kernel suffix authority, full upstream/open fallback and focused
snapshot/failure/history core checks pass. See validation/document-prefix-append.md.
Actual cumulative04 and independent final acceptance remain pending.

Independent cumulative04 visual review identifies an open crown at70.483mm
against the source closed pointed80mm apex. Free upper fit placement(3,89)
excluded the axis; bounded runner correction(17,89) preserves all dimensions and
otherpoints. Exact profiles/Intersect−5/−4 now include the apex and reach80mm;
runner bounds and saved-model tip-material assertions prevent recurrence. See
validation/ornament-apex-fixture.md. Actual corrected final visual/full gate
remains pending; current04 is not a visual pass.

#269 binary STL candidate fixes one edited-ornament normal mismatch caused by
float32 vertex rounding. It recomputes only staged facet normal bytes from actual
serialized vertices; coordinates/winding/counts/checker remain unchanged. Actual
export regression reproduces the old failure and passes alongside pole/multibody
checks. Root owns targeted saved original+edited re-export, reusing creation and
history evidence. See validation/stl-serialized-normals.md; no full replay required.

Source boundary reread: the tutorial explicitly selects the bottom face for
Inside5 Shell, but its section image does not prove an annular opening. That
earlier requirement was inferred. The exact inward meridian closes on the axis
above the base; #266 implements a bounded exact axis-trimmed cavity candidate
without changing authored dimensions. The disclosed closed-cap policy is
authorized; exact source boundary parity and the complete workflow remain unknown.

The exact circular-meridian cavity proof now passes source80/R50 and control85/R55:
BRep/Boolean-input validity, normal wall5, independent disk-integral volume and
the same60mm strip Intersect all pass. A bounded production candidate is authorized
only for an unmodified exact circular Full360 two-face Revolve, with true axis,
two simple cavity roots strictly above the selected plane and actual lineage.
This self-closing offset case uses a disclosed closed-cap policy; source boundary
parity remains unknown. No general Shell replacement or full workflow is accepted.
Evidence: external validation/ornament-feasibility/exact-cavity-{source,control}.
The candidate now builds and passes explicit source/control policy, Boolean
validity, independent volumes and edit/history/reopen core checks. Actual app
source-prefix Intersect passes, with source/control API history/reopen and
readable section captures. See validation/revolved-shell-closed-cap.md. Independent
review and integration are complete; #265 cumulative Join remains separate.

The cumulative runner is prepared in `tests/ornament_cumulative_api.py`
(3567dba/5b65cc9). It reuses the source prefix, verifies retained Pattern count
edits, explicit eight-participant Join, framed transverse Cut, upstream Offset
edit/history/reopen and strict millimeter STL. API schema and syntax checks pass;
it has not launched an editor or established cumulative acceptance. #265 is now
active after independent #266 acceptance. See
[runner scope](validation/ornament-cumulative-runner.md).

## Earlier accepted checkpoints and development history

The dated entries below preserve historical evidence and may describe pending
states that the latest accepted checkpoint above supersedes.

**Prior accepted milestone: wall-outlet sketch #242 and snap correction #243.**
Astra independently accepted source-order creation, final zero-DOF geometry,
readable dimensions/constraint symbols, 29.4-second full API movie and editable
sketch, plus the separate exact edit/history/reopen tail. The original incomplete
attempt report remains preserved. See [validation/wall-outlet-tutorial.md](validation/wall-outlet-tutorial.md).
Implementation625610e/baf143a and review78e8412/620e156 are integrated; the default
application rebuilt successfully and matches the reviewed candidate's source.
All private validation editors are closed. The gallery now has eight creations:
seven solid/STL lessons and this sketch-only lesson. Both issues and their board
items are Done. Older paragraphs below retain development history.

**Plate prerequisites #244–246 are independently accepted and closed.**
Named length bindings, current shortened rim lineage and standalone sketch Point
are integrated. Point implementation41158eb/review526240b passed the main default
app build and combined Point/named-length core2/2 in1.05s. Native Point creation,
two retained edge distances and finished selection have scoped independent
acceptance in validation/standalone-sketch-point.md. Named bindings and ordinary
Distance controls remain intact. All private editors are closed.

**Countersink Hole #247 is independently accepted and closed.**
Combined implementation20c5a15/reviewec275ed integrated; default app
rebuilt. Retained point/target, target-derived All reach, native fields and
preview/Cancel/commit/timeline edit pass. A genuine target-pick defect was fixed
by restoring selectable stock before picking; red overlay depth bias removes
preview striping without altering geometry. Core/API/native evidence remains
separate; see validation/countersink-hole-controls.md and backend receipt.
All private validation editors are closed.

**Parameter dialog recording support #250 is accepted and closed.**
Implementationf3d46af/reviewdc79583 integrated and default app rebuilt. Actual
owned dialog open/close/reuse, live table refresh preserving pending text and
selection, standalone capture and existing movie composition are verified.
See validation/parameter-dialog-capture.md for separate state/API/native limits
and the corrected first-run test-cleanup/process-overlap qualification. All
private editors are closed. Full gate249 can record the actual three-row table.

**Hole Spacing/Adjust pattern #248 is independently accepted and closed.**
Backendf407aa6/native82da8e1/reviewbba0ae3 integrated. Default app rebuilt with
accepted parameter-dialog support. Actual oriented signed axes,9-hole geometry,
thicker underside Adjust, retained edits/history/reopen and readable native
controls/captures pass. Legacy additive Extent/Identical stays unchanged. See
validation/hole-spacing-pattern-controls.md and backend receipt for finite
seed-specific mode and original seed-cut limitations.

**Newest accepted milestone: complete countersunk plate #249.**
Integrated runner f3bf3f9/cabb05c and receipt a9aca83/review5cbb02b pass the full
source-order gate: actual parameter table, named rectangle/extrude, rounded plate,
Point/two edge distances, Countersink All and nine-instance Spacing/Adjust.
The 21.1-second full creation movie, editable file, independently checked closed
7760-triangle STL, saved-BRep openings and exact edit/history/reopen are accepted.
See validation/countersunk-plate-tutorial.md for source adaptations, recording
limits and separate native evidence. Issue/board Done. All private editors closed.
The external gallery has nine creations: eight checked solid meshes and one sketch.
**Active lesson: slots, splines, conic and ellipse (#251–257).**
The full 31-step source gate #257 is now In Progress; its runner is being prepared. Its source-order plan and geometry
oracles are in [validation/slots-splines-tutorial-prep.md](validation/slots-splines-tutorial-prep.md).
The external contract/design records are under
`knowledge/tutorials/after-countersunk-plate/`; the 20-record knowledge corpus
includes six archived visual references for this lesson.

Straight slot #251 is independently accepted and integrated as e3f1843/46dbae1.
Its native creation/dimension/cancellation and 37-call API evidence are recorded
in [validation/center-point-slot.md](validation/center-point-slot.md). The live
solved-shape drag presentation remains Todo #258; current dragging shows a guide
line and solves on release. Fit spline #253 is independently accepted and
integrated as 375ee76/f708e11, with reviewed point-move integration fix 43e0d27.
Main application builds and both core tests pass (2/2, 0.79 s). Native Qt, framed
GPU capture, 38-call geometry/storage API and final stale-drag evidence remain
separate in [validation/fit-point-spline.md](validation/fit-point-spline.md).
The degree-five C2 evaluator is real; its baseline acceleration, knot and handle
mapping policies are disclosed Alloy choices.

Arc slot #252 is independently accepted and integrated as 5beec9c/1e62c33.
Main builds and three slot/spline core tests pass; native creation and 44-call
combined source API evidence are separate in validation/center-point-arc-slot.md.
A reviewed integration correction explicitly chooses the intended preview group.
Control spline/curvature comb #254 is independently accepted and integrated as
e17f0d5/c3a6638. Main builds; four slot/spline core checks pass in 1.79s.
Native and 32-call API evidence remain separate in validation/control-point-spline.md.
Conic #255 is independently accepted and integrated through 8addcb5. Its
analytic core, 39-call API and native continuous Rho drag/context OK/edit/cancel
have separate evidence in validation/conic.md. Ellipse #256 is independently
accepted and integrated78a4cb7/68ddbce, with reviewed dedicated-center validation
correction df5b98d. Its native creation/dimensions and 35-call API verify retained
axes, full275x100 geometry, edits and exact history/reopen. Main app builds;
combined ellipse/conic/control core3/3 passed, followed by ellipse0.42s after
the merge correction. All private validation editors are closed.

Full lesson #257 is In Progress, with all six prerequisites accepted and closed.
Sol prepares the full source-order runner in the ellipse worktree. Root owns
integration, single-editor execution and GitHub updates; Astra reviews the full
gate. The runner has not yet executed and no full creation movie is accepted.
Accepted root API ingredients include arc44calls, control32calls, conic39calls
and ellipse35calls. Their scripts and receipts retain scope and original failure
qualifications; do not rerun unchanged ingredients merely to duplicate evidence.

The first slot starts at its center: source prose conflicts with the pictured
command and official public UI. Full centerline 150/R25 yields 200×50 bounds.
Arc path R150/end R15 plus the 135-degree radial relation yields an upper
90-degree sweep. Three control points yield actual degree two under the public
maximum-degree policy. The final ellipse is horizontal, origin-centered and
275×100; its inferred constraining steps remain explicit. Conic Rho mapping and ellipse typed-size conventions are explicit implementation
policies with independently reviewed public-reference limits. No full lesson or exact visual/input parity claim is made.

User requested private repository visibility on 2026-10-13, then requested
public visibility on 2026-10-21. `rje/Alloy` is now PUBLIC, verified through
GitHub CLI. The MIT license and long-term open-source product goal are unchanged.

The nested cubes #172, egg cup #173, toy block #146, die #147 and subsequent
Centerline/Revolve #217, paper-clip #225 and initials-keyring #233 tutorial gates are accepted and
closed. Their receipts
separate full API creation movies/editable files/checked STL from bounded Qt
input evidence and explicit source adaptations. Physical OS input and exact
visual parity remain qualified. The external
`../Alloy-reference-records/demos/2026-10-13/index.html` links all seven movies,
editable models and STL deliverables. The Revolve receipt is
[validation/revolve-tutorial.md](validation/revolve-tutorial.md).

**139 of 264 issues are closed.** Recent accepted work includes compact ribbon
icons #218, retained Extrude Intersect #219, nested profile/island diagnostics
#165, portable local controls #220, shared-line fillets #221, typed Line length
locks #222, retained Sweep #223 and native Sweep controls #224. Each has a scoped
receipt in `docs/validation/`; broad parents remain Todo for their actual
remaining requirements. Main's default application is rebuilt through all accepted #235–241, alongside
#227–232, #234 and #236:
`build-tutorial-polish/alloy.app/Contents/MacOS/alloy`.

**Paper-clip gate #225 and tangent-symbol correction #234 are closed.** The
complete source chain uses real typed Line/Tab input, six actual fillet arcs,
Circle Tab/Enter and Sweep selectors. Astra independently checked its movie,
authored source, analytic geometry, strict one-component STL and edit/history/
reopen evidence. See validation/paper-clip-tutorial.md. The 160-second movie
predates the tangent-symbol correction; the focused corrected GPU capture and
validation/tangent-badges.md establish current readability. The external gallery
links that capture alongside the full movie/model/STL. Native ingredient tests
remain distinct from API movie evidence; no physical OS or exact pixel claim.
The source's post-fillet dimension drivenness is unknown; Alloy's explicit
reference-span conversion is disclosed. No full tutorial replay was needed for
the symbol-only change. All private validation editors are closed after the focused saved-model history
check. The default application is rebuilt with the #243 candidate correction.
Keep only one private validation editor open at a time.

**Keyring #233 and driving fillet dimensions #235 are independently accepted and closed.**
Luna identified the illustrated initials-keyring lesson externally; Astra
independently checked all six source pages. Seven bounded issues #227–233 now
queue typed two-point Rectangle, overall fillet annotations, rounded-loop Offset,
two-point Circle, editable text geometry, native text placement and the full gate.
Rectangle #227 and text geometry #231 are independently accepted, integrated
and closed; their receipts are validation/two-point-rectangle.md and
validation/sketch-text.md. The active children have separate Sol worktrees. Native text placement #232
is independently accepted and closed; see validation/sketch-text-controls.md.
Rounded fillets #228 are independently accepted and closed; see
validation/rounded-rectangle-fillets.md. Offset #229 is independently accepted, integrated and closed. Its preview
support-resolution failure was fixed and native/API checks pass. Circle #230 is
integrated and builds; geometry/input review passed and the settled palette
capture is readable. Independent visual review accepted and closed #230; the earlier
overlap was capture-before-queued-layout, requiring no production layout fix. Full gate #233
runner preparation is committed (3d465da), with bore-clear text placement fixed
at14b789e after independent preflight. All prerequisites including Cut rim #236
are accepted, integrated and closed. The cumulative blank-to-Save/movie/STL chain is independently accepted and closed
by receipt validation/keyring-tutorial.md (59f94eb, integrated9da32a9). External
`deliverables/initials-keyring-2026-10-13` has the source, 26398-facet one-component
STL and report; `demos/2026-10-13/initials-keyring` has the 63.8-second movie.
Text clearance, same-slot edit/history and exact reopen pass. The seven-card
gallery links all final artifacts; earlier failed runner attempts remain separate.
Driving fillet intent #235 is integrated (`23c0015`, `0dccbb0`, review `8750aea`)
and closed. New eligible origin rectangles retain driving size IDs/values and
supporting-line origin incidence. The independent review caught and verified a
fix allowing unrelated circles/arcs in the same sketch. Finite point-on-curve
semantics and legacy driven sketches are unchanged; the old keyring movie is
historical, while new runner assertions use driving intent. See
validation/rounded-driving-intent.md. Luna researched the next self-contained
wall-outlet sketch externally; the sample-dependent doorstop was not promoted.
The independent audit is complete; #237–242 now track rough Center Rectangle,
source constraint gestures, parallel-line dimensions, associative sketch Mirror,
native Mirror selection and the full sketch-only gate. Rough rectangle #237 and
Mirror foundation #240 are independently accepted and closed. Mirror integration
`aad481b` plus `6f5dc29` (review `2a0455d`) retains solver symmetry and explicit
edit policy; pending input is cancelled on history traversal and corrected GPU
captures show smooth reflected arcs. See validation/sketch-mirror-foundation.md.
Sol owners implement source constraint gestures #238, parallel-line dimensions
#239 and native Mirror collection #241. Full gate #242 remains Todo. Root has an
isolated `../Alloy-worktrees/wall-outlet-gate` with prepared source-order API
recording runner (`521fde8`) and independent geometry/relation oracle
(`5288517`, `3835dfe`). Only syntax is checked; no completed workflow is implied.
#239 is independently accepted, integrated (`e16f0ab`, `2537bc4`, review
`811f6a9`) and closed. Its settled palette capture resolved the immediate-layout
overlap without a production change. #238 is independently accepted, integrated (`a6fae44`, `fbec737`, review
`e0b779e`) and closed. Concentric rings are recognizable and source-order
relations retain meaningful solver diagnostics. Native Mirror #241 is independently accepted, integrated (`804db7d`,
`dff7790`, review `40b78e7`) and closed. All five children #237–241 are now
accepted. Root owns the active full source-order gate #242: build, blank-to-finish
API movie, exact final geometry/relation checks, editable file and history/reopen.
Integrated build and three affected core tests passed (3/3, 3.14 s). First full recorded attempt failed at the origin Circle click after rough Center
Rectangle and midpoint axis: retained coincident origin/center points were
incorrectly treated as ambiguous snap targets. New #243/SKT-055 tracks the
bounded fix, owned by Sol. External deliverables/wall-outlet-sketch-2026-10-13/attempt-01
and its movie remain explicitly incomplete; the editor exited. No full success
is claimed, and #242 now depends on #243.
Attempt02 completed the source creation, Finish and Save using the #243 candidate
(a117be8), with a 29.4-second API movie and zero-DOF final sketch. Astra independently
checked the source, geometry, readable dimensions/constraint symbols and movie.
Its subsequent history check compared against the earlier pre-Finish draft and
failed on six reflected coordinates differing by at most 1.07e-14 mm after edit
entry reevaluation. The runner now compares exact undo against the actual
edit-entry baseline, after verifying that baseline with the geometry oracle;
`tests/wall_outlet_history.py` checks the saved artifact without replaying creation.
The original attempt report remains incomplete and preserved. The focused tail passed exact undo/redo/reopen (history-tail-01); acceptance
awaits independent final review and #243's projected-support precedence correction, identified
by Astra and covered by Sol's supported-face-origin Qt check. No full gate pass
yet. The final post-Finish capture has minor transient footer clipping; final
sketch dimensions and geometry are readable.
The source ends at a sketch, so there is no STL claim for this lesson. Shared schema/solver edits and private GUI
checks remain coordinated. The integrated application build and both Mirror /
rounded-intent core tests passed (2/2, 2.23 s); no full tutorial replay was needed. After #239 integration, Mirror and parallel-line
core checks passed together (2/2, 2.11 s); default app rebuild will include the
remaining reviewed children before the full gate.
New typed rectangle fillets now retain driving size/origin through #235. Legacy
#228 files retain their explicit measured-span policy. The integrated #235/#237
application build and both affected core tests passed (2/2, 2.55 s). Text retains authored parameters and uses mature font
outlines; generated glyph curves are not individual solver entities. Detailed
source choices and uncertainties are external in
`knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.

**Native compile milestone #132 is closed.** All four clean jobs in manually
triggered run38396495940 passed at source d072534. Downloaded artifacts, compiler
and dependency/source records were independently checked; see
[validation/native-milestone.md](validation/native-milestone.md) and
`build/evidence/native-matrix-38396495940/<target>`. This source predates #220
and Sweep. Windows text-contract hashes reflect CRLF checkout bytes; actual Mac
headers remain15.0/SDK15.5. The separate Qt-only Windows control run38401749556
passed for #220. None of these compile/control receipts establishes remote CAD
runtime, hardware rendering or a redistributable bundle. #131 and packaging/runtime
gates retain that work. No duplicate matrix, per-commit CI, tag or release.

The actual native file-sheet gate #167 remains Todo after two computer-control
binding failures before interaction; no application defect or sheet pass is
claimed. See validation/native-exchange-assessment.md. Keep one private editor
at a time, close it after evidence, and preserve user-owned instances.

The sections below are chronological records; older ownership, pending status,
counts and candidate descriptions are historical.

## Overnight queue authorized (2026-10-12)

User selected autonomous overnight tutorial work: nested cubes #172, egg cup
#173, toy block #146, followed by die #147 and coherent backlog work if these
finish before the user returns. The tutorial sequence file is authoritative.
Sol owns bounded Shell and offset-plane candidates in separate worktrees;
Astra researches the exact public step/visual ledgers externally and independently
verifies integrated work. Root serializes integration and private editor access.
Each full gate needs source-step coverage, complete creation recording, editable
file and verified STL. Native input and API evidence remain separate; do not
close broad feature parents for bounded children. No nightly completion estimate
or release is promised. Keep board updates tied to real active ownership.

## Overnight implementation checkpoint (2026-10-13)

Nested cubes #172 and egg cup #173 are independently accepted as complete
API/artifact gates and CLOSED. Receipts: validation/nested-cubes-independent.md
and validation/egg-cup-independent.md. Each has an editable source-sized model,
strictly checked STL, full blank-to-finish movie and separate bounded Qt input
evidence. Physical OS automation remains unavailable; exact interpolation or
pixel parity is not claimed. Shell185, planes186, named faces187, Mirror188,
Center Rectangle189, Loft190, circle Offset192, multi-body STL193, binary facet
precision fix194, modal quit195, and bounded client deadline197 are CLOSED.

Nested deliverables external deliverables/nested-cubes-2026-10-13; movie53.4sec
external demos/2026-10-13/nested-cubes/nested-cubes-from-blank.mp4. Evidence in
build-tutorial-polish/evidence/nested-cubes-movie. Two closed components; inner
12mm cube centered inside25mm outer, eight collapsed binary pole facets filtered
on detached mesh without weakening strict readback. Named face support uses exact
evaluated producer/body/face lineage, including Boolean survivor ownership.

Egg deliverables external deliverables/egg-cup-2026-10-13; final movie36.7sec
external demos/2026-10-13/egg-cup/egg-cup-complete.mp4. Final evidence in
evidence/egg-cup-final; one closed13450triangle STL, source heights0/18/38/58.
Loft uses blue section outlines and suppresses transient datum fills during
valid preview; authored visibility preserved. Full parameter/history evidence
in evidence/loft-timing. Default client deadline30sec, configurable--timeout,
identity probes2sec. Some redo/reopen took5–9sec; performance followup stays#83.

Toy block #146 is independently accepted as an adapted complete API/artifact
gate and CLOSED, together with Pattern191, roof196, Bottom198, display199 and
shutdown202. Receipt validation/toy-block-independent.md; app/runner f17589e,
independent Qt tests/receipts8c769b3. Final48second movie external
 demos/2026-10-13/toy-block/toy-block-complete.mp4; editable model and STL in
 deliverables/toy-block-2026-10-13. Strict one-component36644triangle mesh,
31.8x15.8x11.3mm. Seed edit/history/exact reopen pass. The original movie used projected-corner3.9 dimensions. The refreshed accepted
movie now uses true retained point-to-edge3.9 dimensions; see the update below.
Physical OS input and exact visual parity are not claimed.

Pattern native-widget timeline/axes/Enter/Cancel/OK, Browser Bottom/roof click,
eight center candidates, two retained references and construction midpoint circle
pass. Mac global shortcut focus was unavailable; actual menu actions plus Qt
mouse events supplied bounded input evidence. Active pending numeric teardown
passes after child signals are blocked before window member destruction.
Display tessellation .0005/.10 leaves exact CAD/export tolerances unchanged;
recorded final fillet5.760seconds/reopen5.415seconds and zero capture failures.
Weaker underside fill makes roof/tube inspection readable. Broader performance83
and navigation22 remain Todo.

The overflow die #147 is also independently accepted and CLOSED. Its full
shared-API run has 230 successful commands, 24 features owned by the named die
component and 21 diameter-3 spherical cuts across six faces. Expected removal
47.25π mm³, upstream rounding edit, undo/redo and exact authored reopen pass.
Final BRep volume is 7801.394541 mm³. The binary STL has 53,414 facets, one closed
oriented component, 20 mm bounds and mesh volume 7801.733045 mm³. The complete
323.3-second movie has zero capture failures. Evidence is in
`build-tutorial-polish/evidence/die-final/`; external deliverables are in
`deliverables/die-2026-10-13/`, with movie `demos/2026-10-13/die/die-complete.mp4`.
Independent receipt: `validation/die-independent.md`, commit af89e64.

Bounded die prerequisites are accepted: components #200 (704f279/60a774c),
Chamfer #201 (d3ce00d/e927785), Sphere #145 and named views #203 (b30fff1).
Components have real local definition/occurrence ownership, active-owner creation
and component-scoped STL. Nested instances, transforms and broader assemblies
remain in #87. Chamfer supports equal distance; broader variants remain in #50.
The source window-selection gesture is adapted to Select All Edges; named views
replace the view cube and an ordinary menu replaces radial Repeat. Physical OS
input and exact visual parity are not claimed. Actual Qt events verify the
bounded controls; OK commits, while Enter commit remains unqualified in the
inactive test window. The source permits either.

Sphere uses retained planar support, local u/v, diameter and Cut/New Body, with
Repeat binding the newly selected face and clearing the old center. Last accepted
diameter carry-over and checked component activation are explicit Alloy choices.
The 20 mm cube, 1 mm rounding and 21-pip placement are deterministic fixture
choices, not source dimensions. Sphere poles align with the support normal to
avoid the observed side-face Boolean failure. Full downstream history validates
before historical edit previews; new/last previews reuse the same evaluation.
Neutral stock, a blue support outline and red cutter are independently accepted.
Origin arrow cones are suppressed because an end-on cone masked small pips;
colored axes and labels remain. Oblique cavities read clearly; head-on lighting
can be ambiguous. Broader rendering and performance work remains separate.

The first die run exposed three collapsed serialized binary pole facets.
PRT-007 #204 is fixed and accepted (c942a9c/5e60bf7): remove exact collapsed
records without changing retained bytes, CAD or mesh tolerances; preserve atomic
output and accurate counts. The unchanged strict reader passes. First-attempt
artifacts and movie remain archived separately. PRT-008 #206 also fixes the
confirmed curved ASCII counterpart (66322f7). Actual-app component exports in
mm/in pass strict readback with 53,414 facets and 29 removed ASCII records;
authored state, undo and revision remain unchanged. Focused pole and existing
multi-body/atomic-refusal tests pass. No universal all-settings mesh claim.
See `validation/stl-pole-facets.md`.

All four overnight tutorial gates are Done. SKT-034 #205 is independently accepted and CLOSED (5846877/0fa5eed): direct
circle-center to retained straight model-edge dimensions.
Core/API and independent Qt input pass, including D, center/edge selection,
separate placement, 3.9/Enter twice, Escape, label drag, upstream edge movement,
Cut volumes and exact reopen. Receipt: validation/point-edge-dimensions-independent.md. The external contract is
`knowledge/specs/point-edge-dimension-next-slice.md`. Root serializes private GUI
access. The accepted tutorial, ASCII, point-to-edge and Timeline checkpoints through
c328566 were pushed at that checkpoint. The refreshed toy recording and deliverables use true edge
dimensions; earlier artifacts remain archived.
SKT-036/#212 independently accepts side-face boss and inclined-support coverage.
Parent #150 and native recovery #213 are now independently accepted and CLOSED;
ordinary Qt row selection/Space verifies visible refusal, Cancel and correction. Broader #43 retains history behavior beyond accepted Timeline readability;
#83 retains performance work. RHI-003/#208 is independently accepted research (1e0d0fe); see
dependencies/rhi-shortlist.md. No replacement renderer is selected; #137 retains cross-platform qualification
and adoption decisions. SKT-035/#209 is independently accepted (b6b1f00): affected-sketch context in
atomic support failures. See validation/multi-face-independent.md. The isolated Metal investigation #210 is independently accepted and CLOSED
(5b29523/916daff); production remains OpenGL. The next illustrated Revolve foundation #214 is independently accepted and
CLOSED (b504efd/1d9e235); integrated application build and focused core pass.
Centerline #215 is independently accepted and CLOSED (6d9ed55/783f6ce); the
integrated app build and Centerline/Revolve core checks pass. Active work is
#216 native selection/Partial angular dragging and #217 full tutorial runner,
in separate Sol worktrees. The runner prepares against the frozen pointer API;
its final cumulative run waits for accepted #216. Astra verifies both. Public source steps 39–50 and visual evidence stay
external in tutorial.next-revolve-feature-checkpoint. Centerline can form profile
boundaries; it is not Construction. The detached lesson axis must also coexist
with a separate closed profile. No release,
tag or per-commit CI is triggered. Continue authorized backlog work while the user
is asleep; do not stop just because the first three gates are complete.

HIS-005 #207 is independently accepted and CLOSED (4e34866/8f87058): fixed-size readable Timeline
targets, complete name/type tooltips, horizontal overflow and a view-only route
to the latest feature. Implementation and independent review are integrated.
External research is knowledge/specs/feature-timeline-readability-slice.md. Public
Move to End changes model history; do not label a view-only scroll that way or
add a fake rollback control. Existing edit/context authority remains unchanged.
Broader HIS-003 stays Todo. Reuse the saved die and a focused Qt/capture check.

## Complete support demo (2026-10-12)

Requested full creation movie delivered at
`../Alloy-reference-records/demos/2026-10-12/support-complete/support-from-blank.mp4`.
78.1seconds, from blank through sketch/Trim/Join/Fillet/cutter/Combine/export,
actual API command overlays and captions, no EDITED banner. Full runner passes;
blank/sketch/final frames inspected. Native continuation remains separate.
The subsequent overnight queue above supersedes the discussion pause.

## Current accepted support checkpoint (2026-10-12)

The cumulative functional original-video support tutorial #171 is accepted.
The complete shared-API run covers the dimensioned sketch, Trim, five real
regions, successive Join20/4/24/22, two outer vertical-corner1mm fillets, original
disk-cap circle5.2, cutter47, local MoveY-10deg/Z-31/X-2, Combine Cut, upstream
edits, Undo/Redo, exact reopen and independently read-back closed millimeter STL.
Final outer size32×15×24mm, one valid solid,1,550 mesh triangles. The Move frame
composition remains declared source-informed inference; no exact internal parity.

Astra independently reviewed geometry, source mapping, dimensions/captures and
native evidence. Native continuation from the accepted filleted fixture reaches
the exact API solid/STL. The366.5-second raw and145.6-second edited continuation
retain provenance, visible inputs and recorded recoveries; neither is a blank
native tutorial movie. Prior native Trim/region and Join/Fillet receipts complete
the cumulative gate. The PDF six-edge0.5mm variant and optional text/chamfer stay
with existing broader issues. See `validation/support-tutorial-independent.md`.

#176/#182 and the two native follow-ups #183/#184 are closed. OCCT7.9.3 whole-
cylinder hit testing had inverted circle-prism bounds; supported face selection
fixes actual body/context/Combine clicks while retaining stable body IDs and blue
whole-body cues. Move now scrolls with fixed visible actions at normal/minimum
heights. Astra rechecked the exact native failures successfully. Early Join
history previews and Fillet post-finish focus were also fixed and verified.

Current binary: `build-tutorial-polish/alloy.app/Contents/MacOS/alloy`.
API evidence: `build-tutorial-polish/evidence/support-tutorial/`.
Deliverables: `../Alloy-reference-records/deliverables/straw-support-2026-10-12/`.
Native media: `../Alloy-reference-records/demos/2026-10-12/straw-support-native/`.
All private editors are closed. Preserve the one-editor-at-a-time rule.
Backlog after closures:184 issues,56 closed,128 open. No release/tag/CI run.
Next tutorial is nested cubes #172: research/implement its bounded prerequisites
from the live dependency board before any complete-workflow claim.

## Current integrated checkpoint (2026-10-12)

The functional straw-bung original-video variant #170 is accepted and closed.
Blank API and native flows, editable history, closed oriented millimeter STL,
independent geometry and input evidence, raw recording and a 208-second edited
movie meet the gate cumulatively. PDF60/X-10 numeric variant is recorded but
unexecuted. Original footage retains its recoveries; no flawless-input claim.
See `validation/straw-bung-native.md`. The transient Home-to-C recovery could not
be reproduced and remains noted under #7. No speculative focus fix was made.

Accepted and closed: #174 real overlapping regions, #177 bounded line Trim,
#178 responsive recording, #179 correctly composed native popup/Move route,
#181 accessible Combine target/tool mode synchronization, #180 readable fitted
captions. Each has independent Astra evidence. Build/API/native checks remained
proportional; no full modeling rerun was used for small input/layout fixes.
Current main binary is `build-tutorial-polish/alloy.app/Contents/MacOS/alloy`,
using the private coherent Qt runtime. Separate support build/evidence remains
under `build-support-integrated`. All private review editors are closed; keep
only one open during subsequent validation and preserve any user-owned app.

The earlier Join/Fillet candidate work is now accepted and closed as recorded
above. Broader Fillet options remain Todo under #41; the PDF variant remains
separate. Current implementation ownership is recorded in the overnight queue above.

## Historical checkpoints

The sections below preserve earlier evidence and decisions. Their active/pending
status descriptions are historical; use the current accepted checkpoint above
and the live project board for present status.

## Resumed active work (2026-10-12)

User requested continued autonomous work. #170 is In Progress: Astra owns the
full native video-variant straw-bung walkthrough, recording, step ledger and
independent comparison. #174 is In Progress in isolated `support-regions`:
Sol implements the next tutorial's overlapping line/circle profile partitions.
A separate Sol worktree `bung-native-fixes` is reserved for concrete issues
found by the native walkthrough; it does not imply an extra active issue.
Desktop ownership is serialized through Astra. Root maintains tracking and
integrates accepted work. Previous closed slices and delivered artifacts remain
accepted; the complete native gate is not yet closed.

## Recorder responsiveness and next tutorial prerequisites (2026-10-12)

#178 fixes capture timer starvation: single-shot scheduling gives native input
an interval after GPU readback. Build passes; Astra measured recorder-on AX at
0.35 seconds versus 67–220 seconds before, and restarted the full native #170
walkthrough. Preserve the interrupted raw recording as failed-attempt evidence.
Wall-clock repeats remain explicit; target 15 fps does not promise 15 unique
captures per second. Full stopped-video acceptance is still pending.

#177 adds bounded line Trim, discovered in the original support video but omitted
from its PDF. Sol owns `sketch-trim`, while #174 proceeds in `support-regions`.
#176 remains Todo: source step 7 requires a transient disconnected Join result;
its scope now explicitly includes multi-result body identity. Do not substitute
New Body or unusual composite profile selection for these source operations.

The idle-edit helper `tools/trim_tutorial_idle.py` retains action neighborhoods,
normal playback speed, an explicit EDITED banner and a raw-to-edited frame map.
Its first real interrupted-video check retained all 45 relevant recorded actions
in 27.27 seconds from 510.33 seconds. This is tool qualification, not a tutorial
pass. Media and receipts remain outside Git.

## Sketch timeline research (2026-10-12)

User requested a parallel architectural check. Public evidence supports one
sketch feature in the main timeline with action-level editing undo, not a nested
parametric timeline. Keep current intent/draft-journal separation. Exact current
post-Finish undo grouping remains an evidence gap; no code change was made for
this question. See [the compact contract](research/sketch-history.md) and its
external source record.

## Active tutorial delivery: straw bung (2026-10-12)

Nine published issues (#168-176) add four illustrated tutorial gates and bounded
Join, Move, command-first Circle, overlapping-profile partition and successive
Join-result work. The three first features are integrated on main. Join, Move and Circle have independent bounded native/geometry acceptance.
Move handles use solid GPU shapes; its selected preview is blue and repeated
session Escape cancellation is fixed and natively verified. No release was cut.

The combined real-app video-variant chain passed: Circle -> base extrusion ->
face Circle -> Join -> Join-cap Circle -> cutter extrusion -> local Move ->
Combine Cut -> edit/undo/reopen -> STL. Astra independently accepted geometry,
actual captures and mesh. See [the checkpoint](validation/straw-bung.md) and
[independent review](validation/straw-bung-independent-review.md). The result is
one valid solid and a closed 26 x 26 x 11 mm print mesh. Files are outside Git in
`../Alloy-reference-records/deliverables/straw-bung-2026-10-12/`.

The full native step-by-step/movie gate #170 remains open. Source PDF and video
values differ; preserve the variants and inferred foot pivot/local composition
rather than claim exact PDF input parity. The [source contract](research/straw-bung-gate.md)
links the external 14-step ledger and public video provenance. Bulk research
remains in `../Alloy-reference-records/knowledge/tutorials/warwick/`.

Next: straw support #171 (overlap partitions #174, successive Join #176, solid
fillets #41); nested cubes #172 (shell #51, mirror #54, planes #56, faces #159,
center rectangle #162); egg cup #173 (planes, loft #58, shell). Broader Copy/Align
stays #53 and Intersect stays #161. Existing bracket native follow-ups remain
Todo. The three bounded implementation slices are complete; #170 remains
Todo for uninterrupted native input, source-step ledger completion and movie.
Backlog after their closure: 176 total, 44 closed, 132 open. No implementation
issue remains In Progress at this checkpoint. The prior bracket API chain passed
on the combined candidate. Final UI-only fixes do not change modeling geometry.

## User slicer milestone (2026-10-12)

The user confirmed the delivered STL imports successfully and "slice looks great".
Their screenshot shows the five-hole bracket on an Original Prusa MK3.5 bed.
Record this as successful user import/slicing evidence, with no physical-print
claim. See validation/bracket-exchange.md. Exact native tutorial and accessibility
follow-ups remain open; slicer success does not close those unrelated gaps.

## Active delivery: tutorial through slicer export (2026-10-12)

User explicitly requested complete native tutorial interaction, bounded reference
visual review, and STEP/STL exchange now, ending with an STL they can open in a
slicer. STEP/STL and bounded UI fixes are integrated on main; combined
build and bracket export check passed. The bounded #158 dimension/Undo/Redo AX
fix is integrated; Equal-tool activation still omits some OS accessibility labels. Astra
accepted profile fill, badge hit priority, independent mesh/STEP checks and the
bounded native AX improvement. Private review apps are closed; models retained.
#23/#44 are closed and Done. #155/#158 remain open and Todo with explicit
remaining evidence/AX work. #167 preserves native STEP Open/unit dropdown and
nondefault mesh settings qualification. Backlog: 167 total, 41 closed, 126 open. Open-surface STL scope from #44 is
explicitly preserved in #166 (PRT-004), Todo and attached to the surface gate.

Slicer files are already delivered under
`../Alloy-reference-records/deliverables/illustrated-bracket-2026-10-12/`:
full-size mm STEP and STL (918.198 × 438.909 × 50 mm) and a clearly labeled
0.2× STL sample (183.640 × 87.782 × 10 mm). Source is the completed API tutorial
artifact, not the separate native walkthrough. Astra independently confirmed
one valid STEP solid and 3,636-triangle connected closed oriented meshes with
five holes. No slicer/printer profile supplied; no G-code or print-success claim.
Native held-modifier coverage remains a separate evidence limitation; do not
rename synthetic Qt input as physical input or claim an exact gate without it.

## Tutorial recording delivered (2026-10-12)

#157 is closed and Done on the project board. Backlog snapshot: 165 total,
39 closed, 126 open. Astra accepted the 89.8-second explicitly edited native
highlight and final source redaction fixes f9a2d8b/3376ab5 with build evidence.

Recorder and paced bracket driver are integrated from Sol commits 8b17d71 and
eaaa4e1 (main bb386b9/e733e85). Astra independently accepted the short API smoke,
full 6:07 API bracket movie, and raw 12:30 native circle-to-solid input evidence.
See [tutorial-video.md](validation/tutorial-video.md) for receipts, limitations,
and repeatable commands. Native C, origin click, diameter entry, E and distance
entry produced a valid 20 mm diameter × 10 mm cylinder. Native toolbar AX actions
are UI command evidence, not fabricated pointer events. The full bracket movie
is explicitly API-driven; exact all-native tutorial parity remains #155/#154.

Local media lives under `../Alloy-reference-records/demos/2026-10-12/`, outside git.
FFmpeg is an external developer dependency installed through Homebrew, not bundled.
Capture preserves wall-clock duration with logged repeated frames; effective
unique capture is currently 3.7–4.9 fps in long runs, not smooth 15 fps motion.
Desktop automation causes long idle pauses in the native raw clip. Any edited
highlight must retain an explicit label and a raw-time edit decision list.
Exchange #23/#44 work remains preserved in the exchange-finish worktree and Todo.
All private recording applications were quit after saving; no human work discarded.
No release, tag, external media upload or CI dispatch was performed.

## Editor shutdown and demonstration preference (2026-10-12)

At the user's request, all five remaining Alloy editor instances were closed
through `app.quit` after checking every tab was saved and no draft/session was
active. No discard was required. Earlier instructions below reserving running
human windows are historical: those windows are now closed; saved models remain.
The user clarified that "run the bracket test and show me" means watching the
workflow progress step by step, not only seeing its final model. Future demos
should present the actual construction sequence at a watchable pace.

## Work tracking and scope checkpoint (2026-10-12)

Implementation issues must be small enough to finish and close. When a family is
larger than the current increment, publish linked Todo follow-ups for every
remaining requirement before closing the verified subset. Preserve cumulative
gate dependencies. In Progress means current work, not partially implemented work.
Astra verified publication: 165 total issues, 38 closed and 127 open; seven
follow-ups (#159–165) are Todo. Astra reviewed the splits; original descriptions are retained in
[the split audit](backlog/scope-splits-2026-10-12.json). Exact tutorial and physical
modifier gates remain open. Constraint interaction #152 is closed with its final
hover capture accepted. Arc input #33 is also closed: both native arc variants,
placed R12, exact native undo/redo and save/reopen passed Astra review. See
[the native arc supplement](validation/curves-combine.md). Combine #40 is also
closed after native Cut, both Keep Tools states, retained identity/volume and
exact save/reopen passed independent review in the same receipt.

Unfinished exchange work is preserved locally on
`wip/exchange-preserved-2026-10-12` at `88b50fd`, including its two STEP core commits.
Main remains on the previously qualified application implementation. STL API
checks passed on the private ExchangeCandidate, but native export and integrated
STEP qualification are incomplete. Sol now owns #23 in the isolated
`../Alloy-worktrees/exchange-finish` worktree, with Astra independently verifying;
#23 is In Progress and #44 remains Todo. Do not claim these
features ship on main. The private `build-exchange` binary is older than that
branch's latest source; it can exercise unchanged sketch behavior, not prove STEP.
Its API ready file is `build-exchange/evidence/closure/ready.json`. Preserve human
candidate windows described below.

## Latest human bracket window (2026-10-12)

The user requested the angle-bracket test and display. The registered
`tutorial_bracket_api` passed on the current exchange candidate at
`../Alloy-worktrees/exchange-finish/build-step/evidence/bracket-current/report.json`.
A separate copy is open in `build-bracket-demo/Alloy Bracket.app`, bundle
`org.alloycad.BracketDemo`, ready file `build-bracket-demo/ready.json`, using
`build-bracket-demo/human/angle-bracket.alloy`. It has one valid five-hole body,
four editable features and a larger isometric view. This window is now reserved
for human use: do not operate, quit or rebuild it. Earlier human windows remain
reserved too. This is an API workflow rerun, not exact all-native tutorial closure.

Exchange work remains isolated, not integrated to main: STEP source commits
`6eda8bd`/`fb71037` now have Astra's bounded acceptance after core/API/independent
mm/in reader and native mm export checks. Native unit switching and File Open
remain unqualified. Root's STL fresh API suite and native default Save As Mesh
also pass; added independent stored-normal checks pass on the same exported files.
Before #44 closure, preserve open-surface export/labeling as a new linked Todo
issue. Do not add it to GATE-020 if it depends on SRF-001 (which itself depends on
GATE-020); move that explicit future acceptance to the surface gate with an audit
entry. Root modified only `tests/stl_export_api.py` in the exchange worktree after
Sol released it. #23 and #44 are still open pending final docs/split/integration.
Private StepCandidate remains available for agent testing; it is not the human
BracketDemo. No release or CI was triggered.

## Current checkpoint: complete bracket chain

The registered `tutorial_bracket_api` case now passes from blank document through
all three illustrated activities, exact history and save/reopen. Astra accepted
source/geometry and representative visuals. A separate native walkthrough also
reached the final five-hole solid; see [its receipt](validation/illustrated-bracket.md)
for the extra A1 Finish/reopen, two API multi-selection substitutions, physical
modifier gap and remaining visual mismatch. Exact TUT-003 stays open.

Root owns `build-full-tutorial/alloy.app` (TutorialCandidate) and
`build-tutorial-api/alloy.app` (API runner candidate). The human CutCandidate in
`../Alloy-worktrees/profile-cut/build-cut/alloy.app` and older build-input app
remain untouched. Constrained construction color is corrected in `324fb74` and
Astra accepted actual captures. `e84703d` restores visible historical support
stock when editing the hole sketch after Cut; the focused visible/hidden stock,
Cancel/Finish restoration and exact authored-state probe passed Astra review.
Grouped sketch menus are implemented in `5c58d36`; [their receipt](validation/sketch-menus.md)
records focused Qt action tests and
native nested Circle/Fillet/Parallel/construction/Press Pull dispatch passed.
Native C works after menu closure without an extra viewport click. Popup pixel
fidelity remains unqualified because main-window captures exclude separate popups.
Constraint interaction is integrated through `bf87e99` with a test-only correction
in `ab4f7bb`; [functional verification](validation/constraint-interaction.md) passes
focused Qt and native deletion/undo/visibility checks. Astra's visual review found
an origin-Z-label collision; `0b74676` corrects it, and actual corrected/selected
captures plus shifted native picking passed independent review. A core Sol
capacity failure was recovered by root without losing the unfinished work.
`7af56b9` fits populated sketch content on entry/Look At; source review passed,
runtime framing now passes: the complete supported bracket sketch and stock fit
without cropping, with an unchanged authored draft. Luna researches the remaining Fixed/Coincident
glyph reference details outside the repository. Astra verifies each coherent slice. Preserve
accepted evidence; do not repeat unchanged geometry suites for a color-only fix.
No release/tag/CI was triggered. Status requests do not pause autonomous work.

Human testing handoff: `build-tutorial-polish/alloy.app` (Alloy Tutorial Polish,
source `0b74676`) is now reserved for the user. Its separate file is
`build-tutorial-polish/human/constraint-bracket-preview.alloy`. The user was given
an edit-third-timeline-sketch, delete/undo Equal badge, visibility and Ø50→60
checklist. Do not operate, quit or rebuild this app while it is a human window.
Use a new private candidate for subsequent work. The older CutCandidate and
build-input apps also remain untouched. `build-human-tutorial/alloy.app` is an
older separately copied preview, not the latest handoff. TutorialCandidate is
stopped. The 100 mm origin-centered edit framing reset is corrected and verified.

## Decisions from the conversation

- The initial candidates were standalone open-source Maya and Houdini equivalents.
  Parametric CAD became the selected direction. The earlier alternatives are
  preserved in [target-comparison.md](target-comparison.md).
- Alloy must eventually be a functional feature-for-feature CAD replacement,
  open source and modifiable. It is a standalone app, not a Blender extension.
- Users should find the same feel: familiar visuals, layout, styling, command
  behavior, keyboard controls, navigation, and workflows. A small initial release
  does not reduce that long-term goal.
- Features that currently require cloud access may be deferred. This exception
  is feature-specific, not a reason to discard entire feature families.
- Initial emphasis is shape creation, sketches, solids/surfaces, common imports,
  and exports suitable for 3D printing.
- This is a clean-room implementation. Public docs, screenshots, and instructions
  are the reference material. Never run, inspect, or capture proprietary reference software or use
  non-public information. Public API descriptions are references only.
- Agentic control must be built in from the start for operation, self-testing,
  state inspection, and captures. UI and agents use the same application commands.
- Verification must be proportionate: useful unit tests and pragmatic integration
  tests for core behavior and material risks. Stop after reasonable assurance;
  do not repeatedly run unchanged workflows or chase hypothetical edge cases.
- The user selected C++ and dropped Rust from the planned stack after reviewing
  the CAD ecosystem rationale. The scaffold uses C++20/Qt 6, with OCCT as the
  proposed CAD kernel and Python for the initial agent client.
- The user selected MIT for Alloy's original source and documentation.
  Third-party libraries retain their own licenses.
- Before modeling operations accumulate, settle authored operation history,
  transaction authority, undo/redo and recompute boundaries. FND-003/FND-004
  lead this work; see architecture.md. The FND-003 comparison and DOC-002 authored
  integration are complete. See decisions/001-authored-document.md.
- The user requires a GPU-accelerated 3D viewport from the foundation, with
  actual renderer evidence. OCCT/OpenGL replaced the QPainter reference grid;
  software rendering does not satisfy viewport acceptance.
- macOS, Windows, and Linux are required product targets. Milestone releases
  must offer downloadable native builds with platform-specific runtime and
  cumulative workflow evidence. Currently only macOS runtime is verified.
- Native CI runs only for version tags or explicit manual dispatch. Cut coherent
  reviewed workflow versions; ordinary commits/PR updates do not run the matrix.
- The accepted renderer destination is Alloy CAD presentation on an established
  RHI with native Metal on macOS and Vulkan on Windows/Linux. OCCT remains the
  kernel; current visualization is still AIS/V3d/OpenGL. See decision 003.
- The next main modeling milestone is origin-plane sketch → rectangle →
  constraints/dimensions → extrusion → upstream edit, undo/redo and save/reopen.
  Primitives established geometry/history/storage; broad STEP/platform/face/region
  work stays tracked without blocking that first sketch workflow.
- The user authorized concurrent agents for independent feature work, with shared
  command/document contracts and one reviewed integrated app.
- Continue working autonomously through the ready queue after reporting useful
  human checkpoints; a status update or request to show the app does not pause
  implementation. The user explicitly permits Sol implementation agents with
  Astra as independent verifier. Keep human test windows untouched while building
  and validating separate candidates.
- Luna maintains an external searchable workflow/visual knowledge base using
  public web research; Astra independently checks high-risk behavior and the
  integrated implementation. Detailed records, source URLs, images/video frames
  and derived databases belong in `../Alloy-reference-records/knowledge`, with a
  lightweight query helper and usage guide in the repo. Keep relevant defaults,
  selection, visual state, commit/cancel/history and material edge cases sourced.
- Face selection → face-supported sketch → referenced model-edge dimensions →
  Extrude Cut/Join → another face sketch is an early core workflow. DOC-006/012,
  SKT-013/010/016 and SOL-015 are P0; SKT-017 verifies the joined local workflow.
  The overall UI still falls short of the target: FND-011 tracks concrete icon,
  density, contextual-toolbar, palette, theme and stale-label corrections.
- The first manual sketch coordinate controls are functional helpers. User review
  accepts continuing modeling now and a later public-reference compatibility pass
  on numeric entry, drawing inputs, dimensions, selection and palette behavior.
- The user wants compatibility corrected through bounded independent milestone
  reviews, preferably Astra on work it did not implement. Compare public
  documentation, screenshots, original community articles and video tutorials
  against actual Alloy visual/input behavior and outputs. Record source IDs/dates
  and video timestamps, matched/mismatch/unknown findings and specific fix issues;
  use the API for deterministic fixtures and native input for physical behavior.
  Another reviewer checks Astra-authored code. Follow compatibility.md and the
  compact sketch-controls record; do not sweep every command after every commit
  or claim overall exact parity from one state.
- Public-reference research must lead each user-facing implementation, including
  its controls and presentation. Gather public examples independently and produce
  a bounded specification with verification steps; do not wait for the user to
  supply screenshots or identify missing behavior. In particular, bare dimension
  value labels do not satisfy sketch annotation presentation: research witness
  lines, arrows, placement/editing, glyphs, colors and profiles proactively.
- The user requires the researched visual quality throughout implementation, not
  a later cosmetic pass. Treat actual readable annotations/controls, selection,
  profile/solid presentation and native interaction as feature acceptance, with
  bounded independent review and honest limitations.
- The project lives at `~/src/cpp/Alloy`. Alloy is the working name. Astra was
  requested for the initial public-interface research and project foundation,
  and completed that assignment.

## Actual implementation

A native Qt Widgets shell exists in `src/`, built by CMake. It has command search,
Browser/Timeline/navigation layout controls, and an independently drawn reference
grid with named orientations, pan, and zoom. A private local control socket and
`tools/alloyctl.py` expose command discovery, execution, state, and capture.

OCCT 7.9.3 is linked for native AIS/V3d/OpenGL visualization; the live Apple M4
hardware driver and Retina framebuffer/startup/resize were verified. See the
[GPU foundation receipt](validation/gpu-foundation.md). `kernel.visualizationIntegrated`
is true. DOC-001 links the thin OCCT modeling boundary; `kernel.probe`
evaluates a real diagnostic solid without adding authored geometry. Its real-app
case and box/cylinder/open-face unit checks passed and Astra reviewed the slice.
See [kernel receipt](validation/kernel-boundary.json). The document now evaluates
authored boxes and cylinders through that same boundary and renders their real
AIS shapes; `kernel.integrated` is true.
The cumulative runner in `tests/workflows/run.py` now wraps the real shell case,
with fixture/candidate/artifact hashes, capability checks and live gate prerequisites.
Its real-app success and focused refusal checks passed, and Astra reviewed the
bounded implementation. See [runner receipt](validation/workflow-runner.md). Physical
and visual acceptance remain separate. DOC-002, FND-006, the shared `primitives`
case and DOC-003/004/005/007/008 aliases are registered. `primitive-regression`
runs shell, document, sessions and primitives once each on the same candidate.
The DOC-002 authored layer now has a root UUID, typed owned primitive intent,
canonical millimeters and mm/in display units. A bounded unit fixture and the
real-app DOC-002 case passed; see [authored receipt](validation/authored-document.md).
Box/cylinder creation, dimension editing, deletion, undo/redo and native `.alloy`
save/open are implemented. Candidate authored state and evaluated geometry are
validated before one journal entry is committed. Undo restores both snapshots;
redo preserves IDs, and divergent edits discard the abandoned redo branch.
Files retain editable intent; evaluated geometry is rebuilt on open. Reopening
starts a clean session journal. Invalid edits/files and failed saves preserve
the live document. Preview is disposable, with no authored edit on cancel.

The [primitive receipt](validation/primitives.md) records the passing cumulative
real-app regression, three focused CTest checks and Astra's independent review.
Separate native UI checks covered toolbar creation, dimension typing, keyboard
undo/redo, native Save and Timeline edit/cancel. An editable sample lives in
`examples/gauge-block.alloy`. Body picking and geometry inspection use the actual
OCCT scene and evaluated document. Face/edge selection, persistent-reference
integration, arbitrary orbit, import and export remain pending.
Visual/interaction parity and the broader product gates are not accepted.

SKT-001/002 now implement origin-plane sketches, connected lines and two-point
rectangles through the shared application commands. XY/XZ/YZ planes have explicit
frames; stable sketch, point and line IDs live in authored JSON. A rectangle is
four authored edges with shared endpoints; new rectangles also author four automatic
horizontal/vertical constraints. Previously saved sketches preserve their intent. Sketches have separate evaluated OCCT
edge caches and never claim solid body IDs or volume. Create/edit sessions retain
a disposable draft; Finish creates one QUndoStack entry, while Cancel and palette
close preserve the prior document. The UI has an origin-plane chooser, contextual
tools and a docked Sketch Palette. Canvas clicks and numeric coordinates use the
same commands; L/R select drawing tools and canvas Escape stops a pending tool
without cancelling the sketch. The later extrusion slice is recorded below.

The [sketch receipt](validation/sketches.md) records the integrated build and
three focused CTest checks, the previous shell/document/session/primitive cases,
and the new sketch workflow on the same candidate. Native checks separately
covered choosing XY, two-click rectangle drawing, L/Escape, Finish, Timeline
re-entry, adding a line and palette-close cancellation. The independently authored
`examples/rectangle-sketch.alloy` was opened in the real application. Three-plane
world transforms, draft preservation, mixed primitive/sketch undo and native
save/reopen are covered by actual core and application checks.

FND-005 selects PlaneGCS at a pinned public revision through a replaceable shared
library. SKT-003/004 now integrate it in the actual application: coincident,
horizontal, vertical, parallel, perpendicular and fixed constraints; linear,
horizontal and vertical driving dimensions; driven measurements and solve/DOF
inspection. Point/line selection is shared by UI and automation, with orange
highlights, green fixed points and whole-sketch black/blue constraint status.
Failed solves preserve the usable draft and report conflict identities. Width
changes update real OCCT edges and driven measurements before one Finish undo
entry. Authored constraints/dimensions survive save/reopen and backward-compatible
unconstrained files remain valid. See [constraints receipt](validation/sketch-constraints.md).
The native adapter/document fixtures passed. Existing application workflows and
the new constraint workflow passed on the initial integrated binary; the focused
new workflow was repeated after the explicit line-ID fix and native palette
readability fix. Keep these candidate boundaries explicit in the receipt.
Application and library operation are qualified only on macOS arm64; portable
exports are implemented but Windows/Linux compilation and runtime/package closure
remain unqualified. The later [annotation/profile receipt](validation/sketch-annotations.md)
qualifies D/placement/inline numeric input, driving label double-click editing,
placement-only label dragging and a selectable real single-loop profile. Native
point dragging, inference and per-entity constraint badges remain linked follow-ups.
This is bounded acceptance, not full interaction parity.

HIS-001/SOL-001 now implement ordered sketch/profile recompute and editable
signed-distance extrusion into a new body. Finished profiles are selectable;
E opens the right dock with a disposable real OCCT solid preview. Commit uses one
shared transaction and hides the source sketch; Browser visibility is reversible.
Editing the source temporarily hides dependent bodies. Exact profile references
resolve against newly evaluated preceding sketches. Invalid dependencies or
failed geometry reject the complete candidate without replacing live paired
authored/evaluated state. Width 40→50 at height 25 and depth 5 rebuilds volume
5000→6250 mm3 with stable profile/feature/body IDs; undo/redo and reopen preserve
the chain. The bounded local SKT-015 checkpoint passed the primitive, constraint,
annotation and extrusion cases on one binary, five focused core tests, separate
native drawing/dimension/extrusion/upstream edit/cancel, and Astra's independent
source/capture/state review. See [the extrusion receipt](validation/sketch-extrusion.md).
MSAA is capability-selected (four samples on Apple M4); world lighting gives
distinct face tones and dark outlines. Preview is neutral/opaque with a blue
normal cue. Broader profile/operation/extent modes and interactive manipulators
remain absent; the opaque Home preview occludes its selected base. This accepts
the declared local workflow, not exact parity, tutorial or product/release gates.

A reported viewport mouse regression is fixed and independently reviewed.
Camera setters now defer redraw until a completed view is ready; Retina picking
and pan use native window coordinates rather than framebuffer DPR. Named views
retain direction, resize retains aspect, and fit keeps tall geometry visible.
Actual camera diagnostics, the fresh regression and separate native mouse checks
are recorded in [viewport input receipt](validation/viewport-input.md).
The user's three-body model was preserved in a local recovery file and reopened.

The subsequent [Mac editing receipt](validation/mac-editing.md) records fractional
JSON-to-native picking, distinct blue selection, Option/Alt-left pan and a real
Qt QDockWidget primitive panel. Field completion updates disposable previews;
editing temporarily hides the target presentation, with restoration on cancel,
failure or stale sessions. Dock/float state and bounds are inspectable through
the API. Final cumulative regression and Astra review passed; native typing→OK
and title-close checks passed on the docked candidate. Physical Option-drag
remains unverified because the exposed computer-use API cannot hold a modifier.

FND-006 adds verified live-instance discovery, shared command sessions, observable
units/search dialogs, draft preview, cancel, stale-commit refusal and transaction
IDs. Its real two-app workflow and focused selector regression passed; Astra's
independent review found no remaining blocker. See
[session receipt](validation/command-sessions.md). Primitive sessions extend this
same flow with real geometry previews; asynchronous jobs await a long-running
feature. Windows local-control adaptation remains FND-012.

The native Debug build and CTest real-app smoke passed on Apple silicon macOS
with Qt 6.11.2. The smoke exercised real commands, state changes, errors, search
open/close, screenshots, and clean exit. See [development.md](development.md) for
exact commands, dependencies, evidence paths, and limitations; see
[agent-control.md](agent-control.md) for the protocol and client.

Local evidence is in ignored `build/evidence/`. Dated reference URLs and three
public documentation images are stored separately in `../Alloy-reference-records/`,
with stable IDs in [the source registry](backlog/sources.json) and
[the image provenance manifest](research/reference-images/provenance.json).
They are research references, never app assets. A fresh checkout can build and
verify Alloy without the private reference records. Product-specific reference
names and URLs must not be reintroduced in public files or planning records.
The user authorized a complete reachable-history rewrite and public planning
cleanup. Older screenshots and source hashes remain historical observations;
they do not establish that a transformed historical commit was tested unchanged.

The user authorized a public GitHub repository/project and an issue backlog with
cumulative validation gates. The repository is [rje/Alloy](https://github.com/rje/Alloy)
and the public board is [Alloy](https://github.com/users/rje/projects/1).
The MIT-licensed foundation is published on `main`.
Astra's deep dive and backlog are published: **150 issues** across ten phases,
comprising **95 implementation tasks, 15 research tasks, 22 epics, and 18
validation tasks**. All remote issue bodies, dependency links,
labels, milestones, and live readiness were verified through the API. The shell
baseline [`GATE-000` #12](https://github.com/rje/Alloy/issues/12) is closed;
FND-010 #10 is also closed with independently reviewed runner evidence;
FND-003, FND-002, DOC-002, FND-006, FND-004, FND-009 and DOC-001 are closed after
independently reviewed evidence. DOC-003/004/005/007/008 are closed with reviewed
primitive evidence. UX-001 is closed with independently reviewed input/editor evidence;
FND-005 and SKT-001/002 are closed with their bounded solver/sketch evidence;
SKT-003/004 are closed with independently reviewed integrated constraints/dimension
evidence; SKT-014 is closed with reviewed real-profile/annotation evidence.
HIS-001/SOL-001 and SKT-015 are closed with reviewed extrusion and local checkpoint
evidence; their board items are Done. There are 25 tasks closed, 125 open and
13 dependency-ready at this handoff. Live counts
are obtained with tools/backlog.py rather than this earlier publication snapshot. Later epics need
bounded child issues before implementation; this count does not exhaust parity.
Read [the backlog](backlog/README.md), [publication receipt](backlog/publication.json),
and [deep dive](research/cad-workflows.md) before pulling work. Run
`python3 tools/backlog.py ready` for the current queue and
`python3 tools/backlog.py ready --apply-labels` after changing issue states.
The later cross-platform requirement added five issues: FND-012, CI-001,
CI-002, GATE-005 and CI-003. Read [releases.md](releases.md) and the
[release research](research/cross-platform-releases.md). Native CI is seeded
for macOS arm64/Intel, Windows x64 and Ubuntu24.04 x64; its initial run compiled Windows and both macOS architectures. Linux failed
on a missing Fontconfig development header; that run and its artifacts were
removed with the old-history cleanup. The corrected
[matrix run](https://github.com/rje/Alloy/actions/runs/38192218283) compiled all
four targets successfully at source 12fe534. FND-009's independently reviewed
toolchain/artifact audit passed; subsequent candidates remain separate.
It retains unbundled compile artifacts, not yet independently runnable packages.
A later uniform Python pin failed Mac/Windows setup while Linux compiled;
platform-specific archive pins are corrected. The new matrix passed Python
setup on all four targets; Linux and macOS arm64 compiled, Windows Qt SDK archive
extraction failed before compilation, and
macOS Intel also compiled successfully. CI-001 remains open. The later automatic
run 38199898011 was cancelled under the accepted tag/manual build policy. See
dependencies/native-builds.md.
Windows named-pipe control, Linux X11/GLX runtime qualification, relocation,
dependency notices and tagged milestone publishing remain open.
GitHub operations must use `gh` and APIs; the user explicitly rejected browser
computer control for GitHub. The CLI token now supports Projects; the current board items are tracked through the API.
The board auto-add workflow targets open issues from rje/Alloy. Update Status
through the CLI/API as tasks start and finish.

## Where to resume

FND-003 is complete with a permitted Qt/OCCT comparison and independently reviewed
[decision 001](decisions/001-authored-document.md). It selects authored JSON and one
QUndoStack, with no persisted session journal. DOC-002 integration and its focused evidence are complete;
FND-006 shared sessions and the first exposed modeling/undo/storage slice are
complete. FND-004 resolves stable geometric references independently.
Its bounded OCCT fixture now passed generator role rebinding, fillet history,
split/deletion/merge cases; [decision 002](decisions/002-geometric-references.md)
defines conservative unresolved behavior. DOC-012 still implements the real resolver.

The latest local application binary includes placed dimensions, real closed
profiles and editable dependent extrusions.
Use `python3 tools/alloyctl.py instances` for authoritative live identity; old
ready files may be stale. Preserve live authored work and any unfinished draft
before restarting. Recovery documents remain under the local Alloy Application
Support directory. The new sample `examples/dimensioned-rectangle.alloy` has a
50 × 25 mm anchored rectangle, driving width/height and a driven diagonal.
The last human test used `examples/sketch-extrusion.alloy`, an
independently authored 50 × 25 × 5 mm part with driving width/height and editable
Sketch → Extrude history. Click either Timeline item to edit, try undo/redo and
save/reopen. That historical launch record is `build/evidence/human-ready-extrusion.json`
(private instance registry; verify liveness before selecting it). The prior annotation
ready file is stale. Preserve any later human changes before replacement. The
native test part was saved separately in `build/evidence/extrusion-native-part.alloy`.
Detailed transcripts/build/test/native artifacts are retained under
`build/evidence/constraints-*`; the [constraints receipt](validation/sketch-constraints.json)
identifies which checks ran against each binary. Repeat checks only for material
changes, new failures or unresolved risks.

**Active correction:** SKT-005 #32 remains In Progress. Its D/placement/inline
value, label editing/dragging and proper line graphics now have focused API/native
evidence and independent Astra acceptance. Broader drawing inputs/inference and
center-rectangle entry are still open. SKT-008 #35 retains glyphs/per-entity DOF
and visibility controls. The annotation correction's earlier binary SHA256 is
`1610e1ed19378ff5c0ec2d1d4b133559df2d089028598e5c94caa175c9d4bb56`;
`build/evidence/annotations-final/report.json` identifies source/fixture/artifacts,
and `annotations-native-final-*` retains physical checks. Earlier visually failed
captures are not accepted. No unchanged full regression, tutorial gate or CI matrix
was repeated for this bounded correction.

**Latest human-test checkpoint:** HIS-001/SOL-001/SKT-015 have independently
accepted implementation and API/native evidence. The final executable SHA256 is
`5ddb024941d36c7497d8f89b3a40ef0f0004f4b0b4ed06f2a11a3c804d171594`;
`build/evidence/extrusion-final/report.json` records the four same-candidate cases
and checkpoint alias, with 37 matching source/fixture hashes. Native evidence is
`build/evidence/extrusion-native-checks.json`; its origin has a 0.12605042 mm offset,
so it establishes physical drawing/dimensioning without a snapping claim.
[The extrusion specification](research/commands/extrude-first-slice.md) records
public sources and boundaries. No unchanged full suite, tutorial, CI matrix or
release was run. Pull the live ready queue before selecting the next bounded
modeling increment: boolean extrusion and circles/arcs are important remaining
parts of the sketch-led path. SKT-005 #32 owns inline width/height, canvas dimension placement/editing
and drawing inference; SKT-008 #35 owns per-entity diagnostic colors/badges and
visibility controls. Keep functional modeling and these concrete fidelity
corrections moving in bounded increments.

SKT-001 no longer waits for GATE-010 or face-reference resolution. SKT-013 retains
face-attached sketches; SKT-009 retains circles/holes/complex regions; PAR-001 and
HIS-004 retain named expressions and broader recompute. Full bracket and product
release gates remain unchanged in meaning. STEP, face/edge selection, full camera
navigation and qualified platform packages remain separate required work.
RHI-001/RHI-002 and GATE-045 record the accepted native graphics migration.
The [RHI comparison](research/rhi-candidates.md) researches QRhiWidget and bgfx and
recommends a bounded pinned-Qt spike first. No native RHI spike or migration has
run yet, so the live application remains hardware-accelerated OCCT OpenGL.

The user authorized simultaneous feature agents. The completed tracks separated
authored sketch/history, UI/GPU interactions and Astra's solver research/spike;
they agreed the command/state contract before editing disjoint files. Root then
built and validated one integrated candidate. Reuse that ownership pattern for
independent next increments, with one integration authority and bounded verifier
review; do not launch conflicting app/build runs from several agents.

FND-002 admission supports shared Qt/OCCT and FTL FreeType. The pinned component
record, retained notices and local absolute-path link audit are in docs/dependencies.
It explicitly leaves package closure, sources and replacement instructions to
CI-002/CI-003. Solver selection and its passing native fixture are recorded in
decision 004; the extracted shared adapter is now integrated and portable exports are implemented,
while actual other-platform builds and distributable runtime closure remain open.

The exact dated reference release/platform/theme/navigation baseline,
minimum OS versions/architectures and portable solver runtime qualification
remain unresolved. They are recorded gaps, not forgotten decisions. Current
public-interface/build research now covers the major feature families with 157 dated
reference entries, including original community tutorials, contradictions, and open questions. It does not yet specify
every command, default, error, or visual state.

## Future tutorial exercises

Public beginner-project research is recorded in
[the tutorial recipes](research/tutorial-gates.md). TUT-001 #146 follows the full
toy-block lesson; TUT-002 #147 follows the library die/component/STL lesson.
Neither has been executed. Every source step must be available through shared
real-app commands and the human UI; skipped modeling steps mean incomplete,
and visual expectations need inspected public images before matched claims.
The recipes separate source dimensions from Alloy-chosen deterministic fixtures.
SKT-016 #144 and SOL-014 #145 explicitly cover missing construction/distance and
spherical-cut capabilities. The full die waits on component organization; early
adaptations must name that deviation. These additions bring the backlog to
150 tasks: 95 implementation, 15 research, 22 epics and 18 validation tasks.
The nearest sketch/extrude checkpoint SKT-015 and version/manual CI policy remain
unchanged. Pull readiness and issue state remain authoritative through gh/API.


## Active external knowledge and implementation work

The external corpus is at `../Alloy-reference-records/knowledge`: portable JSON
records and source/image provenance are authoritative, with a rebuildable SQLite
FTS index. The repository keeps only [the usage guide](knowledge/README.md),
`tools/knowledge.py`, and neutral planning/source pointers. Initial topics are
face-supported sketches, projected geometry, edge-relative dimensions, curves,
selection/constraint status, and solid operations. Visuals are research evidence,
never shipped assets; unknown version/theme/scale and unplayed video material
are explicitly qualified. FND-013 #148 accepts the initial six-record, nine-image library after Luna research
and Astra independent review. Ongoing expansion remains part of implementation.

The next implementation candidate is in progress: analytic circles/arcs and
Combine Join/Cut/Intersect cores have source review, but are not yet accepted as
working application features. UI integration and real-app verification remain.
Build this candidate separately in `build-next` while preserving the live human
extrusion window. SOL-015 #149 tracks direct Extrude operations; SKT-017 #150
tracks the subsequent face-sketch/projected-distance/Cut/Join local checkpoint.

The external visual contact sheet is `knowledge/visuals/index.html` beneath the
sibling archive. A second batch adds fillet/chamfer and an explicitly separated
beginner face-sketch/Cut/Join recipe, bringing the corpus to eight records and
12 images; Astra accepted the corrected expansion after independent source review. No research
image or bulk record is committed. The initial six-record acceptance remains
separate from later expansions.

Implementation handoff: Combine core CTest passed in `build-next`; its first
integrated app compile passed before a later multi-tool union correction.
Astra identified missing history-prefix display/picking in Combine edits; the
helper correction is written and the curves agent is integrating renderer stage
support. Circle/arc UI and renderer integration remain active. Neither new UI
feature has real-app/native acceptance yet. Focused cases are prepared in
`tests/boolean_api.py` and `tests/sketch_curves_api.py`, registered in the workflow
ledger. Relink and test this candidate only after source ownership is released.
Do not overwrite or operate the existing human extrusion window in `build/`.


## Latest candidate: curves and Combine

The new implementation in `build-next` compiles, passes focused core/API cases
and has independent Astra source/visual acceptance for its bounded behavior.
Read [curves/Combine evidence](validation/curves-combine.md) for exact candidate
boundaries. Final renderer candidate SHA256 is
`8f5a9f2e74f8c8c5b334410992036c6a2d33b2be03b037262d43082aee732a9b`.
Circle/arc controls and dimensions, circle-profile extrusion and Combine history
are implemented. The final visual fixes make diameter labels readable and curved
solids smooth; Combine edits display the correct history prefix and consumed tools.
Native verification remains incomplete because the desktop controller could not
bind the separate candidate window (`cgWindowNotFound`). Temporary test instances
were closed; instance discovery showed no remaining original human process at
that point. Old ready files are historical, not proof of a live app. No human
model was changed. SOL-002 and SKT-006 remain In Progress; full angular dimensions
and further sketch input compatibility are not complete. Next: bounded native
C/D/arc and Combine selection/Keep Tools checks, then face-selection/naming/sketch
support work already prioritized in the queue. Research library expansion is
accepted at eight records/twelve images, fully external.

## Human feedback and active input correction

The next human test found: C needed an extra viewport click after XY; circle
selection/D was unclear; extrusion flickered; Combine OK was difficult to find;
Browser cylinder selection during Combine crashed. The retained crash report is
`~/Library/Logs/DiagnosticReports/alloy-2026-10-12-100835.ips`; its top frame is
Browser itemClicked after a checkbox visibility rebuild invalidated the item.
Working-tree fixes defer tree/list checkbox commands, focus the viewport after
sketch entry, pin Combine OK/Cancel below scrollable fields, replace previews
without intermediate removal frames, and hide coplanar source profile fill during
extrusion preview. The user asked to relaunch before further verification.

`build-next` was rebuilt and launched for that retest: PID 95777, instance
`40ef488a-9608-45b1-94d8-f24b46d94a47`, ready file
`build-next/evidence/human-ready-input-fixes.json`. Verify liveness; preserve any
new user work. No tests or API mutations should use that human instance.

Astra's source review then identified checkbox selection/session-staleness as a
remaining issue. A further source-only fix suppresses checkbox-as-body-selection
and lets a currently valid Combine session follow a visibility-only revision.
That final change is NOT in the running human binary yet. New
`tests/widget_interactions.cpp` exercises actual synthesized Qt events against the
shared `alloy_application` core. It compiled before the final source correction,
but has not run; coordinate its visible test windows with the human retest.
Physical OS input and flicker acceptance remain pending. Do not describe these
as verified fixes yet. The constrained-value field is awaiting identification
from the user; public research is recorded externally in
`knowledge/specs/circle-dimension-edit-semantics.md`. Existing driving dimensions
are editable, driven measurements read-only; the exact control the user meant
has not been confirmed. Keep source edits, tests and build ownership centralized.


## Input/theme correction candidate (2026-10-12)

The human retest exposed a black Combine scroll container and continuing sketch
focus trouble. Application startup now applies a shared explicit light QPalette
(active/inactive/disabled roles), including unstyled scroll descendants. Palette
creation and existing-dimension editing are separately labeled; selecting a
circle/arc or a dimensioned line chooses a matching existing dimension. Driving
values require Apply; driven measurements and missing dimensions disable editing.
Line dimensions match endpoint pairs in either order.

The sketch-entry focus handoff now repeats after panel/toolbar layout and requests
native viewport activation. Actual native coordinate mouse input on macOS passed
XY -> C without a viewport click, circle creation, Escape, circle-edge selection
-> D, placement, typing 20 and Return. Authored state confirms radius 10 mm and
one driving diameter of 20 mm. An earlier apparent failure used an accessibility
button action followed immediately by C: logging showed the key arrived while
still in sketch.plane, before its animated button activation completed. Do not
confuse that test-order artifact with the user's original report. Root-cause
certainty for the user's original focus failure remains limited; retain human
retest coverage.

Final focused widget tests pass, including circle Apply/read-only measurement,
line endpoint dimension selection, Browser visibility during Combine and pinned
OK/Cancel. Real-app extrusion and Combine API/history/reopen regressions pass.
Evidence is local under build-input/evidence: widget-input-final.txt,
native-circle-d-complete.json, native-circle.png, combine-light.png,
extrude-final/sketch-extrude-api.json and combine-final/boolean-api.json.
Astra independently reviewed final source/captures and found no blocker within
this scope. Temporal extrusion flicker still requires human acceptance; static
captures cannot prove its absence.

Use build-input/alloy.app for the updated retest candidate (separate local bundle
identity Alloy Input Candidate). The earlier build-next human window may contain
unsaved sketch work: preserve it and verify live discovery before any operation.
The candidate includes Browser/Combine crash prevention and preview changes from
the preceding section. No milestone release or CI build was triggered.

## Exact illustrated tutorial requirement (2026-10-12)

The user requires tutorial steps to match exactly, including visible constraint
icons and presentation. The earlier suggestion to skip missing operations is not
an acceptance path. All three illustrated bracket activities were audited against
current source and published screenshots with Astra review. See
[the gate contract](research/illustrated-bracket-gate.md). Source-located detailed
instructions, uncertainties and 22 unique visual references are external under
`knowledge/tutorials/illustrated-bracket/`. The exact workflow is blocked, not run.
Five new backlog tasks #151–155 cover document tabs, constraint-first sessions,
sketch fillets, multi-profile Press Pull and TUT-003. Existing owners were tightened for
inference, two-line angles, Equal, construction, region extraction and model-edge
references. TUT-003 feeds GATE-020; geometry alone cannot close its visual gates.

## Tutorial-first continuation (2026-10-12)

User requested reprioritization around exact tutorial completion and automated
end-to-end validation of each increment. `docs/backlog/tutorial-sequence.json`
is the active ranked sequence; `tools/backlog.py ready` prioritizes its available
owners without waiving dependency closure. GitHub issues have P0 tutorial work
and the project has Delivery order ranks. Start with constraint/input/angle/badge
slices, then exact first activity, rounded profile/Press Pull, face references,
and complete five-hole Cut. Full TUT-003 remains blocked. Short-candidate research
is external `knowledge/tutorials/short-candidates.json`; the supplementary wall
outlet lesson adds mirror/concentric and is not an easier replacement gate.

Sol implemented Equal and persistent constraint-first tools; Astra reviewed the
core and caught a failed-attempt presentation refresh bug that root corrected.
Root built an isolated `build-tutorial` candidate. Solver and existing synthetic
widget regressions pass; `tests/constraint_tools_api.py` passed the real-entrypoint
five-circle diameter/Equal chain and line-relation workflow with glyph identities,
undo/redo, save/reopen/edit. See `docs/validation/constraint-tools.md` for exact
coverage and evidence paths. Native repeated picking and source-matched visual
acceptance remain pending; SKT-007/008/018 stay open. Next resolve these bounded
interaction/visual checks, then origin/inference and two-line angular dimension
for the first illustrated activity. Do not treat API-created coordinates as an
inference or native-input pass. The human `build-input` app was preserved; do not
replace its binary or discard its document. No CI or release was triggered.

## Parallel tutorial work and angular milestone (2026-10-12)

User explicitly requested subagents working through the board and notification at
major testing milestones. Use disjoint implementation ownership, root-owned
build/app/integration and independent Astra verification. Keep tasks/issues/board
current; stop verification after proportional evidence and proceed to the next
visible workflow. Research material remains external.

Sol core and UI implementers delivered real two-line angular dimensions for
SKT-006: stable entity IDs plus raySigns retain quadrant intent, native D supports
two consecutive line picks, placement chooses acute/obtuse wedge, labels follow
the arc tangent, and degree values survive length-unit changes. Root's real-app
`tests/angular_workflow_api.py` passes a constrained triangle -> 5-mm extrusion ->
45-to-60-degree upstream edit -> undo/redo -> reopen, including 45/135-degree
pointer preview checks. Core tests passed. Native 45-degree extension and final
145-degree obtuse entry passed with Astra-reviewed captures. See
`docs/validation/angular-sketch.md` for exact evidence and limitations.

Native testing caught a Qt macOS accessibilitySelectedChildren crash on a sketch
list rebuilt every pick. Selection-only refresh now reuses tree items. Retest
passed; draft-changing refresh still rebuilds so this is targeted mitigation.
The existing circle-widget preview test failed once, then passed in isolation;
FND-007 tracks the unresolved flake. Do not claim general input or full-tutorial
acceptance. The native Equal follow-up also passed; its receipt is updated.

All private native test windows were closed. Human build-input was untouched.
Updated candidate: build-tutorial/alloy.app (local identity Alloy Tutorial
Candidate). Useful saved part: build-tutorial/evidence/angular-workflow/angular-plate.alloy
(current angle60, displayinches, real extrusion). No release/tag/CI run.
Next primary #32 work: origin/endpoint snapping and durable coincidence before
claiming exact first activity. Luna prepared bounded public-source instructions
in ../Alloy-reference-records/knowledge/specs/origin-endpoint-inference.md.
Read it before implementing; documented snap symbols/retained relationships are
separate from proposed pixel threshold and unresolved override behavior.

## Autonomous continuation correction (2026-10-12)

The user clarified that milestone notifications are progress updates, not stopping
points. Continue pulling and implementing the tutorial-first queue with subagents
and independent verification after reporting a milestone. Do not yield merely
because one slice was committed. Stop only for a genuine blocker requiring user
input, explicit user direction, or completion of the requested queue. Preserve
proportional verification and update issues/board as work advances.

## Origin snapping checkpoint and next work (2026-10-12)

Origin/endpoint inference now creates durable coincidence constraints for native
line/rectangle pointer creation. Focused core and real-app profile/extrusion/edit/
history/storage checks pass. Native L and five clicks produced a closed four-edge
outline with a fixed origin and retained chain joints; native Finish and subsequent
API history/reopen passed. Astra reviewed source and captures without a material
blocker. See docs/validation/sketch-snaps.md for exact limits and evidence. The
private candidate was closed; human build-input remains untouched.

The backlog is157 tasks. SKT-020 (#156) tracks accepted-edit undo inside unfinished
sketches, the next implementation slice. FND-014 (#157) tracks shareable real-app
tutorial videos with cursor/click/key and step overlays. macOS capture capability
was inspected, but recording permissions, framing and output have not been tested;
no video exists yet. Videos and research remain outside git. Label API-driven
actions distinctly from native inputs in eventual demonstrations.

## Active sketch history milestone (2026-10-12)

SKT-020 (#156) now has a small accepted-draft journal. Undo cancels an inline editor,
then the active sketch tool, then traverses geometry. Redo requires Select. Public
product-team evidence resolving this precedence is recorded externally in
knowledge/specs/sketch-draft-history.md. Accepted geometry/constraint/dimension/name
edits participate; hover, rejected/no-op edits do not. Annotation drag is one entry;
Finish is still one document transaction and Cancel preserves committed work.

Build and real-app ledger workflow pass. Actual native Cmd+Z/Cmd+Shift+Z removes
and restores a snapped line with exact IDs; native label drag undoes in one step.
See docs/validation/sketch-history.md. Do not overlap same-bundle API runners with
native CUA checks: that confused process/window bindings during this checkpoint.
Private test app closed. No release or CI run.

Next bounded tutorial fix is automatic aligned dimension placement on the short
sloped edge for Activity1's100-mm dimension. Astra found current cursor bounding-box
logic chooses horizontal/vertical at the source placement. Core linear length and
tangent rendering exist; adjust selection policy then qualify the actual six-edge
outline with repeated Perpendicular/HV/Equal and constraint-stage captures. Document
+tab remains separate DOC-015; full tutorial remains blocked.

## First activity modeling checkpoint (2026-10-12)

The actual six-edge first-activity modeling sequence passes through the real app:
three Perpendicular pairs, two H/V relations, two Equal pairs, then500mm,45degrees
and aligned100mm; fully constrained closed profile, exact relations/glyph targets,
area, Finish, undo/redo and save/reopen. tests/bracket_activity1_api.py is registered
as bracket-activity1 with covers[]: this does not close TUT-003. Native isolated
D/pick/place/type100 proves the new automatic aligned policy on an API-seeded edge.
See docs/validation/bracket-activity1.md for receipts and limits.

The workflow exposed actual defects fixed in this slice: a1-mm projection could
quantize to zero and hide constraint badges at10% zoom; the default PlaneGCS
particular DogLeg step collapsed a free cap while adding45degrees. Badge scale now
samples100mm, and the solver uses its supported least-norm step without inventing
constraints. Existing independent residual/collapse checks remain. Five focused
sketch/solver/profile/extrusion/curve regressions pass. An adaptive fine grid now
covers the drawable across zoom/pan/resize; initial OCCT grid-center sign bug was
corrected and final captures independently reviewed by Astra. No CI/release ran.

Next DOC-015 (#151): genuine independent document tabs required by exact Activity1.
Plan: heap-owned CadDocument per stable tab because QUndoStack is noncopyable;
one shared viewport/palette rebound centrally, with all revision/draft/widget
caches invalidated even for equal-revision documents. Preserve accepted sketch
session+journal/camera/selection across switching. Guard raw inline input, partial
gestures and solid dialogs rather than silently discarding them. Deferred callbacks
must capture tabId/sessionId. Close/quit considers every dirty document and draft;
Save must require Finish/Cancel for uncommitted drafts. No reorder/detach scope.
Sol has staged the implementation and a coherent real-app workflow test. Root is
compiling and Astra is reviewing ownership/lifecycle before acceptance. After tabs, perform the
complete native first activity with constraint-stage visual comparison. Full
three-activity tutorial remains blocked by later modeling capabilities.

## Document tabs checkpoint (2026-10-12)

DOC-015 now has independent heap-owned documents and per-tab sketch draft/history,
selection/camera retention. Build passes; 85-command real-app workflow passes at
build-tutorial/evidence/document-tabs-ledger/report.json. Astra reviewed ownership,
cache resets, deferred callback guards and dirty lifecycle. Native plus, A/B/A,
Cmd+N, inactive dirty-close Cancel/Discard passed with authoritative state receipts.
The native pass caught canceled closure changing active tab; fixed and retested.
See docs/validation/document-tabs.md for scope and remaining Save-close qualification.
No private test app remains running. Next: complete native illustrated Activity1,
then construction and inferred circles for Activity2. Video capture remains #157,
planned but unqualified; no recording was made. No release or CI run.

## Native Activity1 interruption (2026-10-12)

After 6fabc42, root native input created plus tab, XY, six edges, three Perpendicular,
two H/V, two Equal, and500mm dimension. API navigation configured zoom/pan; all
modeling was native. Constraint AX toggles change checked appearance without the
clicked handler, so physical clicks were used. Repeated physical tools worked.
Native angle45 entry plus AX observation crashed PID10635. Matching report is
alloy-2026-10-12-125610.ips (older112454 is another PID); fault is Qt macOS
accessibilitySelectedChildren. Fullnative gate remains FAILED/incomplete.
FND-015 #158 tracks this; board In Progress. Sol staged stable sketch-tree items
for ordinary dimension changes; Astra reviewed source, native retest pending.
Construction core/UI work is also staged separately by Sol agents, unverified.
Current private app exited in crash. Evidence activity1-native-* records stages.

Native recovery retest passed: root reconstructed the previously observed native
500-mm authored draft as activity1-native-length500-recovered.alloy, opened and
edited through API, then used native D/picks/type45/Return/Escape and AX query,
followed by native100 aligned and AX query, Finish. Final DOF0 and500/45/100 with
12 visible glyph records; Astra inspected captures and accepted bounded recovery.
This is not an uninterrupted native Activity1 pass. Specific accessibility crash
path no longer reproduced; checkbox activation remains open under#158.
Construction core sketch_curves test passed; full application constructionflow
still pending. Private app quit. DOC-015#151 closed; snapshot27closed131open/158.

Construction real-app ledger now passes. Native physical cap-edge selection+X
converted exactly that line, retained its constraints, removed profile shading,
and selected dashed capture is underconstruction-native-selected. Accessible
constraint control activation now updates authoritative tool state; clicking off
returns Select. Right-click context menu did not appear onmac at edge or blank
viewport despite valid state; investigate native event routing before claiming
exact source contextmenu step. Private candidate quit; SolUI adding narrow input
diagnostics before next build. No full tutorial/recording pass claimed.

Context-menu probe resolved: native rightclick reached the viewport and opened the
menu; intervening CUA AX inspection dismissed it. Batched rightclick/Down/Return
converted the selected API-seeded line (`context-menu-sequence.json`). No behavior
fix needed. Lightweight viewport diagnostics now expose right/contextmenu counts,
last guard state and popup lifetime for future native automation. Final diagnostic build passed; no app remains active. NativeX + menu conversion + accessibleconstraint
activation accepted as bounded checks; exact two-edgeShift tutorial step remains.

## Persistent circle midpoint checkpoint (2026-10-12)

Core/UI now retain circle-center midpoint relations to stable normal/construction
line IDs. Triangle cue during firstcenter placement, atomic freshcenter/circle/
relation at circumference click; radiusfree. Build and targetedcoretest passed,
realapp sketch-midpoint ledger passed drivingparentchangescenter, conversion,
history/storage. Native C/center/circumference/Esc on API-openedA1leftend passed:
center(0,50), realrelation, DOF1. See docs/validation/sketch-midpoint.md. Astra source
and capture review accepted the bounded circle-center slice. No appactive. Next tangentcircle
inference and mixedregion profiles remain distinct CP03 blockers. ExternalLuna
research is indexed in circles-arcs-dimensions; knowledgevalidate/build passed.

## Uninterrupted native first-activity checkpoint (2026-10-12)

At370c735, native plus/XY/six-edge outline/3Perp/2HV/2Equal/500/45/100/Finish passed
in one app session with accessibility reads throughout. No recovery or crash.
View navigation and final save/inspection were API; modeling native. DOF0 and
exact dimensions, residual1.79e-11. Evidence activity1-uninterrupted-*; private
app quit. Astra review requested for FND-015 closure. Tangentcircle source is
staged and compiling, not yet accepted. See validation/bracket-activity1.md.

## Tangent circle and native accessibility checkpoint (2026-10-12)

Persistent circle/line tangent placement now passes build, targeted core test,
real-app two-end workflow and one native end-circle placement. Parent cap100→120
updates radii50→60 with exact retained IDs/history/storage. Square inference and
persistent glyph captures were accepted by Astra; see validation/sketch-tangent.md.
Private app quit. Mixed circle/line regions and sketch fillets still block CP03.
Astra also accepted uninterrupted native A1 on370c735 for bounded FND-015 closure.
Next: SKT-019 two corner fillets, shared radius100 and obsolete500 dimension removal,
then mixed-region extraction. Public corner pair identification is inferred from
source wording/topology; exact second small arc is not independently legible.

## Fillet staging and accessibility recurrence (2026-10-12)

SKT-019 source is staged: paired radius100 fillets with endpoint tangent relations,
explicit500 dimension removal, shared radius edits and one draft transaction.
Core and real-app sketch-fillet ledger pass. Native two corners/type100/Return
also passed on pre-sync binary. Astra found APIpreview100/editor10 mismatch;
field synchronization fixed and rebuilt. Postbuild bulkAPIsetup→Filletpreview→
firstAX query crashes; #158 reopened, #153 remains open. Matching PID51719 report
alloy-2026-10-12-134028.ips and LLDB PID52946 both accessibilitySelectedChildren+184;
not attributed to sync itself. No private app now alive. Fillet source/current evidence
were committed locally asdf0c363 while investigating the accessibility bridge.
Nextregion architecture experiment is external knowledge/specs/occt-planar-regions.md.


## Region integration and Qt patch experiment (2026-10-12)

Isolated region core commitsab15671/de02fef are integrated locally. OCCT splitting
and planar face extraction now return plural evaluated regions with authored
source/contact signatures, preserving legacy single-profile IDs. Astra reviewed
the bounded implementation; root build and actualA2 workflow checks are pending.
Sol is adding the real-app region test. Mixed-reference intersections without
retained contacts reject explicitly; this is not full SKT-009 closure.

Qt Cocoa plugin experiment is outside the checkout in ../Alloy-dependencies.
Exact6.11.2 source plus proposed upstream765434 ownership guards compiled and
loaded through QT_PLUGIN_PATH (not QT_QPA_PLATFORM_PLUGIN_PATH, which still picked
the system plugin). APIpreview100→AXeditor100→nativeReturn passed, with twoR100
arcs. NativeUndo/Redo plus AX then crashed PID56575 report135311; FND-015 and
SKT-019 remain open. No globalQt change, no shipped patch, no release/CI.
The private app CUA relaunched empty after this crash was identified and quit.
Human build-input stays untouched.


The second patched-runtime crash was QAccessibleTable::modelChange reached by
QComboBox::clear during Undo. Retained/incremental dimension rows remove that
reset. Final native preview100/editor100→Return→Undo→Redo with AX reads passes;
exact accepted/redo authored state confirmed, Astra independently reviewed.
See fillet-combo-* evidence and dependency recipe. FND-015 stays open for default
runtime/deployment qualification; no global dependency replacement. Region core,
legacy profiles and extrusion tests pass. Fillet core's only repeat-solve delta
was7.1e-15degrees; an actual save/reopen check found the same authored drift. Solver serialization
now preserves degree fields when both solved radian parameters are exactly
unchanged; no tolerance hides solver movement, and exact roundtrip checks remain. The first region API run passed geometry and all3 picks,
but stopped at a save/reopen expected-state comparison currently being corrected.


## Three-region workflow checkpoint (2026-10-12)

The final sketch-regions ledger passes actualA2 outline/endcircles/fillets, three
OCCTfaces with analyticareas, stableIDs radius100→90/cap100→120, eachprofilepick,
history and exactsave/reopen. See validation/sketch-regions.md. Solver now retains
authoreddegrees onlywhen both solvedradianparams exactlyequal theirseeds; this
removes observed7.1e-15degree reopen drift. Region/filletcore pass; olderprofile
and extrusion regressions passed. No fullSKT-009closure or generalintersectionclaim.
SKT-019#153closed bounded; snapshot28closed130open/158. FND-015 remains open for
defaultpatchedruntime deployment; its tested native crashpath passes.

NextSOL-016 underway: Solcore owns CadDocument/GeometryKernel and focusedextrusion
test; SolUI owns AlloyWindow/ReferenceView/ExtrudeControls. Exclusive legacyprofileId
or pluralprofileIds, sameearliersketch, contiguousselectedfaceunion→one50mmprism,
transactional rejectionofinvalid/missing/disjointinputs. UI Shiftselection and
PressPullQ shareauthoritative extrusion. Rootowns builds/app; Astra verifies.


## Multi-profile typed extrusion checkpoint (2026-10-12)

The connected three-region core, actual 123-command sketch-multiprofile ledger,
and native Q → three ordinary clicks → Distance 50 → OK pass. The actual solid
is valid and closed: one body/feature, depth 50 mm, volume 5,185,592.30050822 mm³.
Parent width edits, history and reopen retain IDs. Public staff research corrected
initial selection to collect ordinary clicks before depth entry.

See validation/multi-profile-extrusion.md. The saved rounded-bracket-50.alloy is
under build-tutorial/evidence. The private app quit; human build-input is untouched.
SOL-016 remains open for arrow dragging, five-profile Cut and native platform
toggling. Astra verified the bounded typed route but flagged gray preview, a tiny
arrow and missing canvas label. Those were corrected, rebuilt and independently
accepted in presspull-preview-final.json/.png; preview Cancel preserves exact saved
authored state. Arrow dragging remains unsupported. Sol core is
planning the planar face reference/frame foundation for CP04, read-only; no face
implementation is accepted. Root owns builds/app/GitHub. No release or CI ran.

## Active face-support worktree (2026-10-12)

The accepted typed extrusion milestone is committed/pushed as f97c9ba. Its
build-tutorial app and saved rounded-bracket-50.alloy are preserved. New CP04 work
is isolated in ../Alloy-worktrees/face-support on feat/face-supported-sketches.
Sol core owns geometry/document/frame helpers and core tests; Sol UI owns
AlloyWindow/ReferenceView. Root owns builds and application operation; Astra
independently reviews. See decisions/005-supported-sketch-frames.md for the bounded
positive extrusion end-cap contract. No face-support runtime evidence exists yet.


Private bundle launch now loads the patched Cocoa plugin from the candidate's
Contents/PlugIns/platforms directory with plugin-path environment variables unset.
Native fillet Return/Undo/Redo with accessibility reads passed and accepted/redo
sessions match exactly; see dependency recipe and qt-bundled evidence. This is a
local bundle qualification, not portable packaging. The private test app quit.

CP04 core and UI source is staged in the face-support worktree. The first build
passes after a test-local variable rename; focused face-support, legacy extrusion
and profile tests pass. Astra found and the UI agent fixed selection through an
occluding solid face. Real-app and native face workflow qualification remains
pending; no implementation acceptance or issue closure yet.


## Positive end-cap sketch checkpoint (2026-10-12)

CP04's bounded face foundation passed combined build, three focused core tests,
the 147-command sketch-face-support ledger and native face selection/Create Sketch/
rectangle/Finish. Astra independently accepted source, state and captures. Parent
50→60 depth changes transport the evaluated frame while local authored coordinates,
profile identity and support references remain stable. History and reopen pass.
See validation/face-supported-sketches.md. General faces, Boolean-modified supports
and projection remain open; DOC-006/012 and SKT-013 are not fully closed.

Next SKT-010: retained cap-edge projection so construction lines can use Parallel
and distance constraints against model geometry. Core proposal requires real OCCT
edge lineage through region union and prism, not indices or nearest-edge guesses.
Public evidence and design review precede implementation. No app remains running;
human build-input stays unchanged. No release or CI run.


SKT-010 implementation is now active in the same isolated worktree, after the
accepted face checkpoint. Current public documentation resolves automatic planar
face-edge availability; the tutorial must not gain an extra Project step. The
external spec linked-support-edge-projection.md records evidence and uncertainty.
Sol core owns retained OCCT boundary lineage, projection resolution/solver and
focused tests; Sol UI owns direct support-edge selection and reference display.
The bounded first slice supports locked straight-edge references and does not
claim complete projected-region/curve behavior. Root owns builds and integration;
Astra reviews independently. Board #46 is In Progress, full issue remains open.


## Linked straight cap-edge checkpoint (2026-10-12)

Implementation c8327ee is integrated. Build, focused core checks and 182-command
sketch-linked-projection ledger pass. Native Parallel control + two consecutive
construction-line/model-edge pairs + Escape/Finish pass (API-seeded guides).
Astra accepted actual reference colors/glyph pairs and authoritative constraints.
See validation/linked-cap-edges.md for evidence and strict retained-reference limits.
Projected endpoint dimensions are API-qualified; native endpoint/center inference
and full tutorial construction drawing remain next. Curved projection, repair and
general profile participation remain unsupported. SKT-010 stays open/In Progress.

Luna researched exact five-profile Cut into an indexed external record,
bracket-five-profile-cut.json (knowledge now9 records). Public screenshot uses
Distance -200mm, One Side, Cut and five red previews; that value is an observed
example, not a default, and Through All is not required by these source steps.
Further research is enumerating exact construction and hole-center inferences
before implementing the next tutorial slice. No private app remains active.


## Native projected endpoint checkpoint (2026-10-12)

Implementation f62ebc7 is integrated and Astra accepted. Native endpoint picking,
read-only refusal, circle-center Coincident snap and D-created 100mm horizontal
distance all pass. Source edit/history/storage are covered by focused core
regression; native Finish/save also passed. See validation/linked-cap-edges.md.
The private candidate is closed; human build-input remains untouched.

Luna completed the exact construction/five-center contract in external
knowledge/specs/bracket-construction-snap-path.md; knowledge now has 10 records.
Next active slice is lazy retained center points of actual circular cap edges,
using stable source curve lineage. Two anchored ends plus two independent
Parallel directions should fully constrain the connected guide chain. This is an
implementation/geometry inference, to be verified, not a claim about reference
internals. Sol core/UI implement in the isolated face-support worktree; root owns
builds/apps and Astra verifies. Then implement the tutorial's inline 50mm circle
entry, five attached centers and four Equal pairs, followed by five-profile Cut.
SKT-010 remains open/In Progress. No release/tag/CI is triggered.


## Anchored construction guide checkpoint (2026-10-12)

Implementation 62e3efd is integrated and Astra accepted. Actual circular cap-edge
centers support lazy retained snaps. Native two-line chain, X construction and
two guide/model-edge Parallel pairs reach zero DOF. Source-edit/history/storage
core regression passes. A Browser bulk-row-removal Qt accessibility crash on
Undo was fixed with persistent ID-keyed rows and hidden/disabled retired rows.
Post-fix native selected-row Undo/Redo plus AX inspection passes exact 3→2→3
feature restoration; save/reopen passes. Qt cached AX names remain inconsistent
after hide/reveal, tracked in #158; actual visible Browser rows are correct.
See validation/linked-cap-edges.md. Private candidate closed; human app unchanged.

Next active slice: inline circle diameter (C, center, type50, Enter), one accepted
transaction retaining circle + driving diameter. Sol core/UI implement in the
face-support worktree. Root then validates five centers endpoint/midpoint/bend/
midpoint/endpoint plus four Equal pairs. Cut follows: disconnected ephemeral
profile prisms, one explicit retained target body, signed Distance, native red
preview/arrow drag. Astra identified a required prefix-history fix for editing
the earlier supported sketch after Cut; recorded on #154. No new issue count,
release/tag or CI run.


## Five-circle and Cut work in progress (2026-10-12)

In the isolated face-support worktree, native C/center/type50/Return, four
endpoint/midpoint circle placements and four successive Equal pairs now reach
five radius25 circles at zero DOF. Finish, diameter50→60, history and exact
save/reopen pass; Astra independently reviewed the bounded source/state/captures.
Evidence is build-face/evidence/five-circle-native-constrained.json/.png and
native-five-equal-circles.alloy. This resumes an existing edited bracket fixture;
it is not an uninterrupted exact full-tutorial pass. Inline circle source is committed locally as9719c08 and cherry-picked into the
Cut candidate as7e1b69d; final runtime qualification remains pending.

A separate two-dimension insertion check exposed another Qt accessibility crash:
QAccessibleTable::modelChange dereferences an expired child interface during
QComboBox row insertion (report alloy-2026-10-12-151125.ips, PID97793). Earlier
Alloy row-retention workarounds do not qualify the general path. FND-015 #158
remains In Progress. A coherent private Qtbase build is underway outside the
checkout at ../Alloy-dependencies/qt-private-runtime, using pinned6.11.2, the
previous Cocoa ownership patch and a proposed narrow expired-ID cache fix.
Installed Qt and the human build-input app remain untouched. No private app is
currently running. Do not claim this new patch works until regression and actual
native editing checks pass.

In ../Alloy-worktrees/profile-cut, core81b9cc7 supports five disconnected
profiles as ephemeral cutters against one retained target, with one Cut feature
and one valid closed solid. Focused profile_cut checks pass diameter/depth edits,
prefix evaluation for earlier supported sketches, atomic refusal, history and
storage. This uses rectangular stock, not the full bracket UI. Sol is finishing
Cut controls/red previews/arrow dragging; Astra is reviewing independently.
Root owns integration/builds/apps and the next actual five-hole bracket test.
No release, tag or CI run has been triggered. Continue the queue after status
updates; they do not pause implementation.


Private Qt runtime now builds and passes the cache regression: unpatched
Table/Tree/List fail recreation; patched versions pass (5QtTest results,0failed).
All Qt frameworks/Cocoa/native style loaded exclusively from private install,
GPU AppleM4 remains active. Native firstØ60/freecenter and secondØ50/origin entry,
selected-row draftUndo/Redo, Finish and BrowserUndo/Redo pass; Astra accepted
bounded evidence. AX row-label inconsistencies remain under#158, so no general
accessibility closure. No globalQt changes.

Integrated profile-cut candidate7e1b69d builds against that runtime; focused
face_support/profile_cut checks pass. tests/workflows/check_profile_cut.py passes
actual bracket5toolpreview, one retained validclosedCut solid, earlier supported
sketchdiameter50→60, Cutdepthedit, exacthistory and reopen. NativeQ/fiveordinary
profileclicks pass. First arrowdrag found missing coordinate schema bounds;
Sol fixing that plus stale panel error (75c120b) before native retry. Private
app intentionally quit for rebuild; human app untouched. Full tutorial gate
remains open and is not replaced by this supplied-fixture checkpoint.


## Native five-hole Cut checkpoint (2026-10-12)

Integrated candidate749a667 includes inline circle entry, retained one-target
profile Cut, prefix-history support while editing earlier sketches, native
signed-distance arrow and corrected blue selected-profile/red cutter overlays.
Native Q, five ordinary region clicks, arrow drag to -200 mm, Cut and OK pass.
The real bracket is one valid closed solid with five holes. Final API workflow
passes earlier diameter50→60, Cut depth edit, exact undo/redo and save/reopen.
Hidden retained-target edit preview/cancel passes. The focused Qt widget modifier
test passes remove/re-add after distance entry; physical held Cmd-click is not
verified because the available CUA API does not expose held-modifier clicks.

Astra accepted native geometry/input, final edit-preview visuals and private Qt
runtime evidence. See validation/five-circle-cut.md and dependency recipe. Full
TUT-003 and#154 remain open for remaining exact/cumulative coverage, including
physical platform-modifier evidence. No issue was closed by this bounded slice.
The task count remains158, with28closed/130open at this checkpoint.

A separate Cut Candidate application is available for human testing in
../Alloy-worktrees/profile-cut/build-cut/alloy.app, bundleorg.alloycad.CutCandidate.
It uses the private coherent Qt runtime; preserve it while the user tests. The
saved native bracket is build-cut/evidence/native-five-hole-bracket.alloy. The
older build-input app remains untouched. Continue next with the exact illustrated
tutorial from a clean document, per docs/research/illustrated-bracket-gate.md;
use another private instance for automation. No release, tag or CI run.

## Single-solid STEP candidate (2026-10-12)

In isolated `../Alloy-worktrees/exchange-finish`, DOC-011 now has a compiled
single-solid STEP import/export candidate. Imports author one undoable feature
with verified embedded source bytes, requiring one valid closed solid and one
unambiguous length unit. Export selects one evaluated body and explicit mm/in
units, without editing the design. File Open and File Export have shared-command
UI routes. An independent OCCT fixture program authors the input and reads both
output units in separate processes; the private real-app DOC-011 ledger passes
malformed/multiple-solid atomic refusal, history, and reopen after removal of
the original source. See `docs/validation/step-exchange.md`. Native dialog and
visual qualification are still outstanding; do not close #23 on API evidence
alone. Unrelated STL work is preserved but not qualified by this checkpoint.

## Bounded Extrude Join candidate (2026-10-12)

Isolated `../Alloy-worktrees/bung-join` implements SOL-018 #168: one retained
Extrude Join target, shared dialog/API operation, authored references, recompute
and a supported positive Join cap when the exact cap survives the fuse. The
public straw-bung steps now reach Ø26×3 flange, Ø19×8 neck and a third Ø6.2 tool
on the joined cap. Focused Join/Cut/face-support tests and actual API dialog
preview/cancel/commit, Undo/Redo, Join depth edit/tool recompute and save/reopen
pass. See `validation/profile-join.md` for evidence and limits. Physical input
and independent compatibility review remain pending. Body Move/Copy and the
full tutorial remain separate work; no release or issue closure is implied.
## Bounded body Move candidate (2026-10-12)

SOL-019 #169 adds one-body Free Move with accumulated local rigid deltas,
explicit pivot, GPU arrow/ring handles and one editable authored transaction.
Copy/align remain in parent #53. The public video-backed route is 40 mm cutter,
Y+10°, localZ-20 about foot(0,0,11); PDF60/X-10 is a distinct variant.
Focused core plus real-app API/history/reopen/downstream Combine evidence pass;
native manipulator/context-menu verification and integrated tutorial remain
pending. See [the bounded contract](validation/body-move.md). No push/release or
issue closure performed. Root integrates from isolated `bung-move` candidate.

## Command-first Circle entry candidate (2026-10-12)

In isolated `../Alloy-worktrees/circle-entry`, SKT-028 starts Center Diameter
Circle with `sketch.circle`, global C or Solid Create → Circle, before choosing
XY or a supported cap. The pending session carries `pendingTool: circle`, then
enters the same circle tool with viewport focus. Existing typed diameter/Enter
and Cancel authority are reused. Focused widget events and a real-app API chain
through origin, first extrusion cap and Join cap pass with cancellation and
save/reopen. See `validation/circle-entry.md`. Physical native evidence and
independent review remain pending. No arbitrary face support or other global
shortcut repairs are implied.

Native Circle entry now passes C→XY/original cap/Join cap, pointer origin,
typed26/19/6.2 and Return/Finish, with frames z3/z11 independently inspected.
Native Escape cancels the chooser without a feature. Evidence is in the private
circle-entry candidate's `build-circle/evidence/native-*`. API3mm extrusion and
8mm Join connect the native circle checkpoints; this is not a full native
tutorial run. The inherited Create dropdown could not be activated through the
attempted CUA calls, so only its synthesized widget action is verified. Review
is pending; the desktop is released for root's combined qualification.

The bounded Move visual follow-up uses solid unlit GPU shafts/cones/torus rings
instead of GL-width-clamped wire handles. Build and the actual-app
`move-preview-solid-triad.png` capture passed own inspection. Mixed-step docs now
state `T * R` applies rotation then translation to a point. Astra's native numeric
and manipulator review is pending on the private MoveCandidate; no exact/native
closure claim is made.


Astra independently accepted Move numeric Y10/localZ−20 completion/reset,
physical arrow/ring drag and native OK/Cancel/save. Repeated Move openings left
orphaned Escape shortcuts: a bounded follow-up gives the panel its own shortcut,
reuses one viewport shortcut and focuses the dragged viewport. Build, focused
repeat-open/Escape widget regression and actual native drag→Escape pass with
exact saved authored state restored (`native-escape-fixed.json`). The moved
body now uses opaque selection blue with neutral stock; the fresh capture was
independently accepted. Native context-menu activation remains inconclusive;
there is no uninterrupted exact full-tutorial claim.

## Straw-support region candidate (2026-10-12)

Isolated `../Alloy-worktrees/support-regions` implements SKT-027 #174 with OCCT
source-set edge provenance for coincident straight lines and stable intersection
branch naming. Focused post-Trim geometry reaches five real selectable regions;
raw crossings remain nine atomic cells rather than an invented full disk.
Diameter edits retain the disk profile or explicitly refuse changed bridge
references. Adjacent API rectangles avoid duplicate inferred relations on shared
point pairs. See `validation/overlap-regions.md` and
`research/straw-support-regions.md`. Real-app script is prepared; native/API
capture and independent review remain pending.

Public video170–192s revealed Trim operations omitted by the PDF, now owned by
SKT-029 #177. External `knowledge/tutorials/warwick/straw-support-ledger.json`
records source URL/hash/timestamps and observed versus inferred geometry.
The video247s source preview shows two separated profiles,4mm and Join; Bodies
folder is collapsed, so its intermediate body semantics remain unknown. SOL-020
#176 owns that later multi-result compatibility work; #174 does not change it.
## Bounded Trim candidate (2026-10-12)

Isolated `../Alloy-worktrees/sketch-trim` adds SKT-029 #177: T/Modify Trim for
line targets against finite line/circle boundaries, GPU interval hover cue,
shared API/click draft edits and retained point-on-curve contacts. Source video
170–191s trims circle-internal chords, left-edge halves and the guide while
retaining full circle; outside-left stubs remain uncertain and are preserved.
Build and three focused source/solver/document tests pass. The real-app API
script and native evidence are pending combined #174/#177 qualification after
Astra releases the desktop. See `validation/sketch-trim.md`. No push/closure;
Extend/Break and broader curve trimming remain in parent #47.
## Owned popup capture diagnostic candidate (2026-10-12)

In isolated `bung-native-fixes`, FND-017 #179 corrects top-level owned popup capture
from parent-chain mapping to explicit global-coordinate subtraction. Actual body
menus now report phase/target/actions/result; API state exposes real popup/action
rectangles and old/new mappings. Native#170 delivered right-click/context events
but its popup was missing from composed capture/AX and no native Down/Return
reached Qt during the blocked interval. Build passes; narrow capture widget and
native menu→Move verification await the serialized desktop slot. Source-backed
capture correction is distinct from unproved command/AX defects. No Qt patch or
push/closure. See `validation/popup-capture.md`.


### Successive Join candidate (SOL-020 #176)

Isolated candidate contract: `research/successive-join.md`. New Extrude Join
features author ordered participants and normalized per-connected-solid
`bodyResults` ownership. Detached outputs reserve stable feature/source UUIDs;
later merges retain the first participant and consume the other IDs. Legacy Join
files, Cut, New Body and Combine retain their existing single-solid policies.
Kernel history preserves unique unchanged earlier cap descendants, including
original disk cap at z20. Preview/API expose all outputs through shared state;
Browser bodies remain individually connected solids. Changed connectivity fails
atomically instead of silently reassigning identities.

Private Qt/OCCT full build and six focused core/regression checks pass, including
source 20/4/24/22 geometry, 1/2/2/1 connected bodies, retained depth edit/history/
reopen and consumed-reference refusal. `tests/successive_join_api.py` awaits the
integrated real-app/native review. This is a candidate, not accepted compatibility;
reference Browser was collapsed and its exact intermediate body count is unknown.

## Equal-distance Chamfer candidate (SOL-028 #201)

Isolated `equal-chamfer` implements real OCCT equal-distance retained-edge Chamfer,
shared `solid.chamfer`/dialog, editable feature history and a readable preview dock.
Focused twelve-edge 20 mm cube, distance/upstream height edits, history/storage
and atomic failure checks pass together with existing Fillet regression.
Actual API runner `tests/solid_chamfer_api.py` is prepared but unrun; root owns
private editor serialization and independent review. Select all edges is an
explicit native button; source drag-window gesture remains mismatch, not parity.
Broader variants remain SOL-005. See `validation/solid-chamfer.md`.
## Local component candidate (2026-10-13)

ASM-004 #200 has an isolated local-components candidate: real authored local
component definitions and distinct root-child identity occurrences, per-design
activation outside CAD history, active-owner creation and retained-owner editing.
Browser radio/grouping and active-owner Timeline use actual evaluation ownership.
Existing STL export offers occurrence scope and resolves actual component material
bodies, excluding root stock. Root-only files remain valid; unsupported nesting,
instances/transforms/external references/joints remain in ASM-001. Explicit
cross-component modeling inputs and dangling ownership fail atomically.
See [the bounded contract](research/local-components.md). Checked activation on
creation and root activation on reopen are recorded Alloy choices, not claimed
reference defaults. Focused kernel ownership/history/storage/STL readback passes;
real-entrypoint runner is prepared. Native/API/visual acceptance awaits Astra's
independent serialized desktop review. No acceptance, push or release is implied.


## Face Sphere controls candidate (SOL-014 #145)

Isolated `face-sphere-controls` layers real face-pick/center-click, diameter/Enter,
red disposable Cut tool, New Body, editing/Cancel/OK and nonmodal context Repeat
on the integrated Sphere core. Stable feature/body IDs are reserved per session.
`solid.sphere`, `solid.sphereDialog`, `solid.repeatSphere` and
`view.projectFacePoint` are the frozen shared API. Repeat binds its new actual
face and clears center; diameter3/default and last-accepted carry-over are explicit
Alloy choices. Build/application and dedicated Qt widget test compile with the
private coherent Qt runtime; core Sphere test passes. Real-app API runner and
focused widget run await root's exclusive desktop assignment and Astra review.
No GUI was launched by the implementer. Root must inject `authoringArguments`
into the candidate wrapper when integrating active component ownership.
See `validation/face-sphere-controls.md`; full die gate remains separate.

## Serialized pole STL correction candidate (2026-10-13)

PRT-007 #204 inspects staged binary STL facet records before atomic publication,
removing exact serialized duplicate-vertex/zero-area facets and correcting header,
triangle, byte and removal metadata. It preserves CAD, mesh tolerances, retained
facet bytes and the existing ASCII path. The complete saved die reexports through
non-GUI authoritative evaluation with 53,414 facets after three serialization
removals; unchanged strict mm/in checks pass one closed component, normals,
winding and 20 mm bounds. Focused four-feature pole fixture and existing placed
binary/ASCII box/atomic-refusal checks pass. See
[the bounded receipt and remaining limits](validation/stl-pole-facets.md).
Root owns integrated app/export and final movie; no GitHub closure or broad
spherical/curved-ASCII qualification is implied by the isolated candidate.

## ASCII spherical-pole STL follow-up (2026-10-13)

PRT-008 #206 has an independently reviewed isolated candidate. The actual ASCII
serialization path now drops exact duplicate-vertex/zero-area pole facets while
preserving retained text/normals, units, CAD, mesh settings and atomic publication.
Four-feature pole and existing binary/ASCII placed-body/atomic checks pass.
One private real-app shared-command export of the saved complete die passes
unchanged strict ASCII mm/in readback: 53,414 facets, 29 serialized removals,
one closed oriented component and scaled 20 mm bounds. Authored/undo/revision
stay unchanged; the app exits cleanly and the desktop is released. Astra accepts
bounded code/artifact evidence; root integration/issue closure remain separate.
See [the updated pole receipt](validation/stl-pole-facets.md).
## Retained point-to-edge dimension candidate (SKT-034 #205)

Isolated `point-edge-dimensions` implements circle-center→retained straight-edge
P2L driving dimensions, native D/point/edge/separate placement/3.9/Enter and live
perpendicular witnesses. Explicit Alloy semantics are positive supporting-line
distance and retained side relative to canonical edge endpoints; source internals
remain unknown. Core DOF/radius/side/conflict and40→50plate Cut/history/reopen pass.
The real-app API runner passes center35→45 and volumes5000−45π→6250−45π.
Qt toy gesture, Escape and annotation-only label drag pass3/3; no physical OS
input claim. A bounded viewport-key fallback repairs delivered D/Escape when the
outer Qt window is inactive. Both stored sides are exercised. Candidate captures
and authored evidence live in `build-point-edge/evidence/{api,widget}`; all private
validation processes closed and desktop released to root for Astra review.
See `validation/point-edge-dimensions.md`. Broader SKT-017/025/026 remain separate;
no issue closure, release or push implied by this candidate.

The updated full toy runner now passes using the actual center-to-top/left-edge
dimensions instead of the earlier corner-coordinate substitute. First stud moves
to (3.9,11.9), row pattern runs -Y8; the source Bottom-view construction diagonal
retains world endpoints (3.9,3.9) and (11.9,11.9). Do not change that diagonal just
because the seed occurrence moved. New evidence/toy-block-point-edge has API-pass,
128 commands, 60.8-second complete movie with zero capture failures, and the same
strict 36,644-facet/2451.548940 mm³ final STL. External movie is
demos/2026-10-13/toy-block/toy-block-with-edge-dimensions.mp4. Astra independently accepted these updated artifacts (3dcdf1a). Deliverables
toy-block-2026-10-13 now contain the new model/STL/summary, with previous files
archived in previous-corner-dimensions; original movie/evidence remain untouched.
No private editor is running. Sol audits remaining SKT-017/#150 prerequisites
read-only before root chooses the next bounded gate; no new task is active yet.

## Timeline overflow candidate (2026-10-13)

HIS-005 #207 replaces compressed Timeline text with fixed 44-pixel glyph tiles,
full name/type tooltips/accessibility names and horizontal overflow. Shared
Show latest/showFeature navigation changes only the viewport; appended/changed
features reveal after layout. Existing single-click edit/Pattern seed/context
bindings and active-owner IDs remain. No marker, rollback, reorder, suppression
or model-history Move to End is implied; HIS-003 retains those requirements.
Focused Qt input passes first/middle/last target + exact Cancel and narrow/wide
width checks. One actual-app run inspects all 24 saved die features and captures
1040/1400/1800-width states; it exits cleanly and releases the desktop. Own
before/after capture inspection confirms icon readability instead of clipped
names. Astra source/capture review reports no blocker; final independent
acceptance/integration remain root-owned. See
[the bounded receipt](validation/timeline-overflow.md). No tutorial replay,
push, release or GitHub closure was performed by this isolated candidate.

## Active multi-face cumulative gate (2026-10-13)

SKT-017 #150 is now pulled after a read-only implementation audit. Dependencies
were replaced with the actual accepted bounded prerequisites (GEO-002, SKT-010,
SKT-034, SOL-015, SOL-018, SOL-020), retaining all stated scope. Broad support
repair, explicit projection policies and Intersect remain separate Todo tasks.
Sol (die_chamfer) owns isolated multi-face-gate; Astra independently verifies.
Reuse the retained edge-distance plate/Cut, add an independently dimensioned
8x3 side rectangle and Join4 (96 mm³), plus a real inclined planar face from an
extruded XZ trapezoid. Move clears support lineage and is not a valid fixture
shortcut. Check actual UI creation/selection and frame mapping, upstream edits,
Cancel, undo/reopen and visible atomic refusal/recovery when support splits.
Recoverability means preserving accepted state and allowing Cancel/correction;
replacement-support/relink UI is not implemented or claimed. Keep private GUI
serialized. No new feature implementation should be invented if existing commands
pass this full gate; split a concrete missing behavior if encountered.

RHI-003 #208 is active parallel research owned by Luna (die_research): a small
primary-source comparison of maintained open C++-usable native Metal/Vulkan
foundations, with Qt integration, licensing, shader/picking/capture feasibility
and a proposed OCCT-data spike. Bulk research stays external; root will write a
compact repository shortlist after Astra review. No dependency is selected or
adopted by this child. RHI-001 #137 retains actual spike/platform/admission work
and remains Todo. This research does not interrupt modeling or change the current
OCCT OpenGL viewport.

## Isolated graphics investigation queued (2026-10-13)

RHI-003/#208 is independently accepted source research in
`dependencies/rhi-shortlist.md`. RHI-004/#210 now investigates an opt-in Qt6.11.2
Metal sample with actual OCCT box/cylinder triangles, depth/edges/selection,
RGBA8 ID readback and native capture/resize evidence. Sol owns an isolated
worktree; Astra verifies. No production renderer changes or adoption are claimed.
Shader Tools is absent; any reused example shader bundles need matching source
and license provenance. The existing app remains OpenGL. Parent#137 retains
Vulkan-host evidence, admission, selection and migration. Root serializes GUI
access behind the active#150 multi-face gate.

## Palette summary correction accepted (2026-10-13)

FND-022/#211 is independently accepted:634b67c updates sketch scroll-layout
constraints so multi-line summary text is fully visible below Name. Main app build,
1400/1100-window captures, longer Offset instruction and exact Cancel preservation
pass; Astra reviewed actual images and source. See validation/sketch-palette-summary.md.
The palette remains fixed312pixels; toolbar truncation and broader layout remain
separate. No repeated full tutorial or physical OS input claim.

## Multi-face candidate evidence (2026-10-13)

Core and real-app API cumulative tests pass retained plate edge offsets/Cut,
side8×3 Join4, genuine inclined support/frame edits and atomic split/deleted
support diagnostics. Separate native side and inclined authoring workflows pass.
Native recovery is incomplete: the bounded checkbox attempt highlighted the tool
without toggling it, leaving the original valid preview. Keep #150's remaining
visible native refusal/Cancel/correction gate; no repair UI is claimed. See
[the evidence and limitations](validation/multi-face-gate.md). SKT035 adds the
exact affected sketch ID while preserving originating diagnostic attribution.

## Multi-face split accepted (2026-10-13)

Integrated b6b1f00/8adebff/6b2f300 independently accept diagnostic#209 and
side/inclined authoring#212. Parent#150 is Todo, requiring native support-recovery
#213: styled checkbox input did not change the selected Combine tools in the
bounded probe. Cause remains unresolved; preserve it rather than claiming native
refusal/Cancel/correction from API evidence alone. The API/core cumulative chain
and actual Qt side/inclined Join pass. Text-distance/Return input qualification
is recorded in broader operation-edit#161; spin arrows/OK pass. No relink/repair
UI is claimed. Immediate Qt Timeline capture layout is qualified in the receipt.
#210 owns the private GUI slot for a bounded Metal sample run; no Alloy editor
is left open. Main builds include the independently accepted palette fix#211.

## Isolated Metal investigation candidate (2026-10-13)

RHI-004 #210 has a compiled opt-in C++20 Qt6.11.2/OCCT7.9.3 sample, independently
buildable without solver configuration. Default Alloy/OpenGL authority is untouched.
Pinned BSD-licensed Qt example shader bundles avoid new shader tooling; source,
hashes and notices are recorded. Actual Metal/AppleM4/DPR2 fixture rendering,
known face IDs/background/depth against OCCT rays, native capture, narrow/wide
resize, resource recreation and pending Qt-result cancellation pass; process
exits cleanly and releases the desktop. Astra source/artifact review reports no
bounded blocker. No RHI adoption or migration follows. Edge IDs are manifest-only,
full-texture picking/coarse cylinder edges remain spike limits, native GPU-in-flight
close and physical display transitions are unqualified, and numeric GPU IDs/driver
version remain unknown. See [the bounded receipt](validation/metal-rhi-spike.md)
and [standalone build/runner](../research/prototypes/metal-rhi/README.md).


## Next illustrated Revolve queue (2026-10-13)

The Metal investigation #210 is closed with independent acceptance; no adoption
or production rendering change is implied. #214 SOL-029 and #215 SKT-038 are
actively owned by the two Sol agents, with Astra checking the source contract
and candidates. Root granted the private GUI slot to #214 for its focused API
run; all other GUI runs must wait. #216 native selection/Partial angular drag
and #217 full blank-to-printable lesson remain Todo. There are 217 tracked
issues; broad Revolve #49 and construction #144 keep their remaining scope.

External lesson pages 14–18, steps 39–50: create Front/XZ sketch and origin-up
line, convert to Centerline, make separate stepped closed profile, add diameter
dimensions from centerline to parallel profile edge, Revolve with automatic
sole-profile/centerline selection, drag Partial, choose Full and OK, then
rotate/inspect. The source leaves geometry values to the learner. Screenshot
diameters 100/165 and partial 70 degrees are observations, not mandatory numeric
inputs. Distinct centerline intent participates in boundaries; Construction
does not. Diameter annotations span the mirrored full diameter and retain both
line references; they are not half-radius distance labels. Unsupported
nonparallel pairs must refuse explicitly. Core #214 initially uses ordinary
retained straight sketch axes; automatic Centerline discovery belongs to #216.

## Retained Revolve foundation candidate (2026-10-13)

SOL029/#214 adds New Body Full/Partial Revolve around a retained same-sketch
straight line through shared authoring/session commands. Core and real-app API
geometry/history/storage pass; native controls remain SOL030/#216. Centerline
intent/profile and diameter-to-axis semantics belong to SKT038/#215. Normal
boundary-line axes are supported without construction conversion. Only exact
partial cap provenance is named; curved support and Boolean modes remain #49.
See [contract](research/solid-revolve.md) and [evidence/limits](validation/solid-revolve.md).


## Revolve foundation integrated (2026-10-13)

#214 is closed after b504efd/1d9e235 and targeted integrated app/core build;
solid_revolve passes 1/1 in 0.49 seconds. Reuse the independent candidate API
evidence; no repeated full tutorial for this unchanged integration. #215 owns
the next private GUI slot. #216 controls preparation runs in parallel against
its frozen Centerline contract, with acceptance blocked until #215 integration.
#217 remains Todo. Ninety of 217 issues are closed. No release or CI run.


## Multi-face cumulative gate complete (2026-10-13)

Test-only 827cf6a and independent af89878 close #213 and parent #150. Root ran
the recovery-only real Qt application once: 3/3 pass in 4050ms, process exit0.
Ordinary row click/Space resolves the native input qualification; the previous
checkbox indicator hit remains unexplained. Both split/deleted supports visibly
refuse with disabled OK; Cancel/correction preserve document and history exactly.
Reuse accepted #205/#209/#212 for the cumulative gate. No production change,
OS input or broad repair/relink claim. There are 92 closed issues of 217; only
#215/#216 remain actively owned. #215 currently owns the private GUI slot.
## Centerline diameter candidate (2026-10-13)

SKT-038 #215 now has a bounded Centerline intent and parallel-edge full diameter
implementation in an isolated candidate. Centerline remains boundary-capable;
a detached open axis preserves a separate closed profile. D selects axis plus
parallel profile edge, places mirrored Ø witnesses and uses the existing numeric
editor; driving edits retain side and refuse lost parallel orientation rather
than adding hidden relations. Geometry/storage checks and real GPU Qt workflow
passed, including typed value/edit/Cancel, driven read-only and undo/reopen.
Astra source/log/capture review reports no bounded blocker. Final acceptance and
integration remain parent-owned; this does not qualify
the full Revolve workflow or automatic axis/angular controls. See
[the contract and bounded receipt](validation/centerline-diameter.md).


## Centerline integrated; native controls and complete runner active (2026-10-13)

#215 is closed after6d9ed55/783f6ce. The integrated app builds; focused
sketch_centerline and solid_revolve tests pass2/2 in0.96s. Candidate recorded
Qt input/captures are independently accepted without repeating unchanged GUI.
#216 owns the next private GUI slot. #217 prepares the complete runner/media
and source-step receipt in parallel, with final execution waiting for accepted
#216. Reference fixture choices and independent Pappus oracle are external in
knowledge/tutorials/revolve-next-slice/alloy-fixture-plan.md. 93 of217 issues
are closed; only216/217 are actively owned. No release or per-commit CI.
## Native Revolve candidate (2026-10-13)

SOL030/#216 builds on accepted SOL029 and SKT038 checkpoint783f6ce. Actual Qt
Centerline auto-selection, explicit finished Profile/Axis picks, real angular
press/move/release, Partial→Full, Cancel/fresh session, commit/edit/history/storage
pass. Shared pointer-coordinate API also passes and refuses ambiguous inference.
Native drag yields70.0432° with unchanged ring basis; source fixture values and
ring mapping/style remain Alloy choices. Guarded immutable session IDs prevent
late signals/JSON reads from recreating canceled sessions. Source sketch hides
on commit and is temporarily revealed for edit. See
[contract/evidence and capture limits](validation/revolve-controls.md). Astra
review reports no bounded blocker; root owns independent acceptance/integration.
Full tutorial/movie/print output remains TUT008/#217, broader modes remain #49.

## Native Revolve integrated; complete tutorial and ribbon polish active (2026-10-13)

#216 is CLOSED after0a5bc65/8529a0a and successful main app rebuild.
Its accepted native/API evidence is reused. #217 owns the sole private GUI
slot for the complete from-blank Revolve movie, history, exact reopen and STL
gate. Root implements original vector ribbon icons and compact group spacing
in isolated #218, with GUI verification waiting for #217 to release the slot.
94 of218 issues are closed; only217/218 are actively owned. No release/tag
or per-commit CI. Broader parent49 and11 remain Todo.
## Compact original modeling ribbon candidate (2026-10-13)

#218 provides16 original vector action icons,94-pixel ribbon and unchanged
command routing with full tooltip/accessibility/menu names. Actual1040/1400
Retina blank, dimensioned sketch, preview and solid captures fit all targets;
existing native Revolve input test passes3/3. Actualwidget inspection is in
app.state.ui.ribbon. See validation/ribbon-icons.md for bounded evidence and
physical/cross-platform limits. Independent capture acceptance remains pending.
Broader contextual toolbar and interface work stays in parent11.
## Illustrated Revolve gate candidate complete (2026-10-13)

TUT-008 #217 ran against accepted integrated controls checkpoint 8529a0a from a
blank design: all source steps 39–50, real Centerline/full diameters, automatic
Profile/Axis, coordinate-derived Partial 70° drag, Full commit, named-view exploration
and labeled save/STL extension are recorded. The independently authored rough
polygon matches its Pappus oracle (501961.579795 mm³); sketch remains truthfully
underconstrained at 8 DOF. Strict millimeter STL readback passes 2392 closed/oriented
facets and 165×165×50 bounds. Retained Ø110 edit/history passes; exact original and
edited reopen was completed in an explicitly separate short recovery after fixing
an omitted discard argument, not claimed as uninterrupted success. Movie is 86.5s
with API/step captions and no EDITED banner. Astra source/artifact/strict-reader
review reports no bounded blocker. Private editor exited; GUI slot released.
Final acceptance and board counts are root-owned. See
[the source-step and visual receipt](validation/revolve-tutorial.md) and
[runner](../tests/revolve_tutorial_api.py). No exact geometry/pixel/physical-input
claim or broader Revolve-mode qualification follows.

## Native milestone compile preflight (2026-10-13)

Root patch2aac4a7 enables the six OCCT modules required by the current app,
including ApplicationFramework/DataExchange that the older script disabled.
Draw/DETools remain disabled. Build receipts record module choices and the pinned
solver source/build contracts; historical native-targets.json stays unchanged.
Astra independently source-reviewed the patch and found no blocker. Syntax and
CMake component consistency checks pass. The actual four-target manual matrix
will run once after current integrations; no remote compile/runtime success is
claimed yet. #132 remains In Progress.
## Retained Extrude Intersect candidate (2026-10-13)

SOL-031 #219 adds Profile Plane / One Side / signed Distance Intersect through
existing OCCT Common, Extrude API and native operation/target fields. One exact
same-owner target and its body/producer IDs are retained; the modifier owns no
cutter body. Full containment is valid, empty/touch/disconnected results refuse
atomically, and compound selected profiles share preview/commit authority. Core
signed-face/upstream/edit/history/storage checks, shared API units/discovery and
compound-preview regression, and actual Qt operation selection/Cancel/OK pass.
Astra source/log/capture review reports no blocker; private editors exited and
GUI slot is released. Final integration/acceptance are root-owned. Parent #161
remains Todo for broader operation migration/edit and typed-Return/focus coverage.
See [the compact contract and receipt](validation/extrude-intersect.md).

## Retained Intersect integrated (2026-10-13)

#219 is CLOSED after5ac93d8 and independent4e5be86. Main app rebuild passes;
candidate core/native/API evidence and actual captures are reused. Parent161
remains Todo for its explicit wider scope.97 of219 issues are closed. Active
owners are165 (profile regions) and132 (one manual milestone compile matrix).

## Nested profile-region candidate (2026-10-13)

#165 candidate adds exact nested-cell/void metadata, stable-ID edit/storage
coverage and observable unused/coincident/gap diagnostics. An actual undeclared
OCCT endpoint merge is refused without healing; accepted open-chain cells,
zero-gap contacts, tangent controls, overlapping strips and Centerline behavior
remain supported. Core4/4, real API and bounded native annulus/island selection
gates pass. Independent review and integration remain pending. See
[the contract and receipt](validation/profile-regions.md).

## Native exchange binding assessment (2026-10-13)

#167's remaining actual macOS sheet gate was assessed on the current private
app. Computer-use binding returned cgWindowNotFound even with an unchanged
binary validation copy using a unique bundle identifier. The API ready/capture
path worked, but no native target/sheet input was available. Editors were closed;
accepted exchange/API/readback evidence remains unchanged. No application defect
or native completion is claimed. See
[the bounded assessment](validation/native-exchange-assessment.md).

## Portable control candidate (2026-10-13)

FND-024 / #220 candidate adds explicit Unix/named-pipe discovery, a packaged
Qt byte bridge for Windows Python clients, protected inheritable current-user
registry/ready ACL validation and a manual Qt-only Windows transport milestone.
macOS focused transport and one real-app regression passed; actual Windows
non-GUI evidence is pending. This does not close FND-012 / #131 desktop, GPU,
normal-user workflow or second-user denial qualification. Continue from
[the scoped receipt](validation/local-transport.md).
## Retained Sweep foundation candidate (2026-10-13)

#223 candidate supports an earlier circular profile and retained ordered open
G1 line/arc path, Full Perpendicular New Body, through shared API and actual
OCCT preview/history/storage. Exact evaluated entity-edge mapping, actionable
atomic refusals and geometric self-interference checks are included. Source-shaped
analytic core and real-app API pass; native controls and full source authoring
remain #224/#225 and #221/#222. Independent review/integration are pending.
See [contract](research/solid-sweep.md) and [receipt](validation/solid-sweep.md).


SKT-039 / #221 candidate adds paired sketch fillets that exactly exhaust a
shared horizontal return line, preserving real arc/contact identities and
converting consumed width dimension IDs to disclosed reference spans. Other
affected lengths/relations retire explicitly; unrelated dependencies refuse.
The constrained paper-clip source core/API and native ingredient evidence are
bounded prerequisites, not a full tutorial or typed-line claim. Resume from
[the scoped receipt](validation/shared-line-fillets.md).
## Typed chained Line candidate (2026-10-13)

SKT-040/#222 adds source L, first point, positive typed length, Tab lock,
pointer-dominant H/V preview and click continuation to the existing Line tool.
The pending lock is session-only; each accepted segment retains a driving H/V
length dimension and an H/V relation. Actual Qt events pass the exact seven
source endpoints plus invalid refusal, Escape, undo/redo and exact storage;
the real executable replay passes the same shared commands and saved model.
The viewport cue shows value, units and direction. Circle typed Return and
fillet tool entry remain intact. A separate source Circle Tab then Enter probe
lost editor focus, so SKT-041/#226 owns that bounded follow-up before
the full tutorial gate. Physical OS input, exact reference styling and the
complete object remain unqualified. See [the scoped receipt](validation/typed-line-lock.md).
## Native Sweep controls candidate (2026-10-13)

#224 candidate adds Create/Profile/Path selectors, explicit checked Alloy Chain
Selection policy, actual finished line/arc picking, temporary retained-source
visibility and readable GPU chain/profile highlights plus real solid preview.
Native3/3 and shared API pass Cancel/OK/edit/undo/reopen and both normal signs,
with analytic API-authored fixtures. Full source authoring/tutorial remains
separate; no unsupported Sweep modes are implied. Independent review and
integration pending. See [receipt](validation/sweep-controls.md) and
[shared contract](research/solid-sweep.md).

## Sketch Text foundation candidate (2026-10-13)

SKT-046/#231 adds a pinned, redistributable DejaVu Sans 2.37 regular font and
retained text intent on one named planar face. Qt outlines become exact OCCT
line/Bezier wires and planar glyph faces with holes. Each character index is a
stable profile slot owned by one text UUID; same-count edits retain those IDs,
while count changes are refused. The shared API and existing extrusion authority
joined DSB then DS8 after text/height/placement edit to one floor body with
positive checked volumes, undo/redo and exact save/reopen. A native Apple M4
OpenGL capture shows legible raised outlines/holes. The bounded receipt is
[Sketch Text](validation/sketch-text.md). Independent review, native text
placement, broader shaping and the complete keyring lesson remain open.

## Rounded rectangle overall-span candidate (2026-10-13)

SKT-043/#228 extends the accepted two-point Rectangle and sketch Fillet path
to four successive R6 side-pair selections. Four original lines survive with
four quarter arcs and one closed profile. The original 60/30 dimension IDs
become disclosed driven overall spans, while the retired corner-origin
coincidence leaves the fixed origin marker unanchored to the rounded contour.
Focused core, actual Qt side-pair entry/capture and real-app API history,
radius edit and exact storage pass. The radius edit updates the honest measured
spans to about 59.6999 × 29.6996; preserving driving 60/30 through that edit
is separate future work. Independent review and integration remain pending.
See [the bounded receipt](validation/rounded-rectangle-fillets.md).

## Native Sketch Text controls candidate (2026-10-13)

SKT-047/#232 candidate adds a command-first Text entry, real modeless Qt text
dialog, exact regular/bold DejaVu 2.37 choices, editable height/position/angle,
and GPU move/rotation handles. The preview is disposable; Cancel restores the
sketch draft and OK publishes one existing-authority edit. Focused injected-Qt
typing and pointer interaction and real-app API/Join/capture passed on macOS.
This is pending independent review and integration. See
[the controls receipt](validation/sketch-text-controls.md). Same-count text slot
identity, finite uppercase glyphs and text-only sketch limits still apply.

## Keyring full runner preparation (2026-10-13)

TUT-010/#233 has an isolated `feat/keyring-tutorial` worktree from `add7b2a`.
Its proposed [source-step runner](validation/keyring-tutorial-prep.md) maps the
complete blank-to-Save lesson and separate edit/STL checks without a GUI run or
acceptance claim. Exact rim, hole and text lineage is asserted. Rounded-loop
Offset #229 and 2-Point Circle #230 still need integration, and SOL-034/#236
must retain the upper Cut-hole rim for the source's R1 Fillet plus the broad
floor support for Text. Run the cumulative movie only after those gates land.
## Rounded-loop Offset candidate (2026-10-13)

SKT-044/#229 adds one generated-only retained loop offset on an actual named
rounded top face. The source click selects a real outer edge, checked Chain
previews the entire contour at signed −1.5 mm, and OK creates an exact annulus
with a stable profile identity; Escape discards the preview. The bounded
R6 source on the z=2 cap yields a 57 × 27/R4.5 inner loop and a 1.5 mm joined
raised rim. Core, native Qt and real-app API evidence pass geometry, preview,
edits, atomic refusal, undo and exact reopen. This is a candidate pending
independent review/integration. The source's 60/30 overall spans remain driven
references, and the full lesson remains #233. See
[the scoped receipt](validation/rounded-loop-offset.md).
## Circular Cut rim candidate (2026-10-13)

SOL-034/#236 carries the entry rim of one analytic circle Extrude Cut through
the existing Boolean edge history, so a face-supported Ø4 Cut -10 on a 2 mm
floor exposes a selectable upper edge for R1 solid Fillet. The focused kernel
and actual-app API chains pass diameter edit, undo, save/reopen and retained
broad floor support for Sketch Text; selected-rim and Fillet GPU captures are
in the isolated build evidence. Independent review and the full keyring gate
remain pending. See [the bounded receipt](validation/cut-hole-rim.md).

## Driving rounded rectangle candidate (2026-10-13)

SKT-049/#235 adds an opt-in finite rectangle policy for newly authored,
origin-based typed 60×30 outlines. Four separately created R6 sketch fillets
retain driving overall width/height UUIDs and origin support through explicit
internal supporting-line incidences. A single R5.5 edit leaves outer bounds
unchanged, while 70×35 size edits rebuild the downstream 2 mm solid with
stable profile/body IDs. Oversized edits refuse atomically. Focused core,
injected Qt editor and actual-app API/capture cases pass; independent review
is pending. Existing #228 driven files are not migrated. The historical #228
fixture and the earlier five-field candidate intent load unchanged; unrelated
circle/arc geometry in the same sketch is outside the recorded corner check.
The historical #228
and #233 receipts describe their originally captured policy; the #233 runner
has updated new-authoring assertions but was not replayed for this follow-up.
See [the bounded receipt](validation/rounded-driving-intent.md).

## Sketch Mirror foundation candidate (2026-10-13)

SKT-053/#240 adds a retained straight-axis symmetry group for selected sketch
lines and arcs, including stable entity/point mappings and reversed analytic
arc sweep. Source/axis edits propagate through PlaneGCS without copied source
constraints or extra independent DOF. Shared API preview, Cancel and Accept
are implemented; native selection and controls remain SKT-054/#241. The
six-curve rough-source fixture, focused regressions and real-app API captures
are recorded in [the scoped receipt](validation/sketch-mirror-foundation.md).
This isolated candidate awaits independent review and integration.

## Outlet sketch relations candidate (2026-10-13)

SKT-051/#238 adds retained Concentric for circle/arc pairs, ordered point-pair
Horizontal/Vertical, and midpoint inference at either Line endpoint. The
source-order rough rectangle/two-arc Concentric → circle-center/arc-center
Vertical → Equal core fixture passes, including its bounded rank-redundancy
case without dropping any relation. A focused native Qt sequence passed real
midpoint hover/clicks, shared-API construction conversion, constraint picks
and a settled GPU capture. The corrected Concentric badge uses one two-ring
glyph near the shared center. The rough sketch still has six DOF; the full
reflected and dimensioned lesson is separate. Independent review remains. See
[the candidate receipt](validation/outlet-constraints.md).

## Native sketch Mirror controls candidate (2026-10-13)

SKT-054/#241 adds Modify → Mirror with explicit Objects and Mirror Line roles,
fully enclosed viewport drag-window collection, separate deduplicated point and
curve identity rows, Clear, preview, Cancel and OK through the accepted sketch
Mirror authority. A rough source with outer frame, circle and crossing diagonals
selects exactly its four upper lines and two arcs plus six distinct related
points; the source image's 12-object composition remains an inference. Focused
native Qt input and shared real-app API runs pass, with readable upper selection,
lower preview and settled palette. The sketch remains at 33 DOF; the fully
defined lesson is a separate cumulative gate. Both validation editors exited.
See [the scoped receipt](validation/sketch-mirror-controls.md). Independent
review and integration are pending.

## Coincident snap cluster candidate (2026-10-13)

SKT-055/#243 resolves origin circle placement after a rough centered rectangle
and horizontal midpoint axis by grouping only retained Coincident point intent
and point-named Midpoint inference. Projected support points at the same
location retain lower priority than the origin or an authored point. A focused
real-window circle gesture, an unlinked equal-coordinate ambiguity control,
and an empty supported-face origin check pass. The full outlet lesson
still requires a separate cumulative rerun. See
[the candidate receipt](validation/coincident-snap-clusters.md).

## Shortened straight rim candidate (2026-10-13)

DOC-020/#245 retains unique current straight top-rim descendants after a centered
75×50×10 plate's four vertical R5 Fillets. Actual OCCT history supplies identity;
updated oriented endpoints distinguish current boundary ends from source sketch
corner ancestry. Missing/split references refuse without geometry fallback.
Focused core 3/3 and 59 actual-app API calls pass retained two-edge 10 mm distances,
centered width/radius edits, atomic refusal, exact history and reopen; readable
Apple M4 GPU captures were inspected. The fixture uses a circle center, so
standalone Point #246 and the complete plate remain separate. Final opt-in routing
and later-Boolean metadata refresh use final build/core evidence; preceding API
captures cover the unchanged active fillet branch. Future signed direction
consumers (#248) must use oriented endpoints rather than raw curve parameter
signs. See [contract and receipt](validation/fillet-rim-lineage.md). Independent
review and integration remain pending; the private editor is closed.
## Named length parameter candidate (2026-10-13)

PAR-002/#244 adds a document-owned stable-ID table for direct named lengths,
driving sketch dimension and Extrude distance bindings, one native parameter
table and name entry in the existing dimension and Distance fields. Bounded
core, native Qt and real-entrypoint API checks pass; independent review remains.
See [the candidate receipt](validation/named-length-parameters.md). The full
plate/countersink exercise and arithmetic parameter expressions remain
separate work.

## Standalone sketch Point candidate (2026-10-13)

SKT-056/#246 adds a true `{id,type:point,pointId}` entity using existing solver
coordinates and one actual OCCT vertex, excluded from profile boundaries.
Create→Point/native one-click and shared `sketch.addPoint` use the same authority;
point-only sketches finish/edit/save/reopen with zero profiles. Finished GPU
markers and `sketchPoint` picks expose exact sketch/entity/point UUIDs for future
Hole. The rounded-plate two10mm D workflow passes actual Qt menu/pointer/value
input with no focus repair; width75→85 moves the point x−27.5→−32.5 via retained
shortened rims. Core3/3, final focusednative6.06s and real executable67-call API
pass history/reopen/refusals; actual readable captures were inspected. An empty
body cue no longer clears finished point selection during UI refresh. Final
neutral point-only label uses native capture evidence, while preceding API
behavior is unchanged. See [contract/receipt](validation/standalone-sketch-point.md).
Independent review/integration remain pending. Preserve the parallel #244
parameter binding fields during additive merge. Private editors are closed;
Hole and the complete parameterized plate remain separate work.

## Native Countersink Hole candidate (2026-10-13)

SOL-035/#247 now has native Create→Hole, explicit finished Point/target picks,
From Sketch/Countersink/Simple/All, editable bore/sink/angle and derived depth
through the backend's shared authority. The native Qt 8.03 s ingredient passes
actual field edits, exact Cancel, fresh Point/OK and Timeline edit with one
initial activation after API setup and no in-sequence focus repair. Select Target
now removes the unselectable disposable preview before actual stock picking;
read-only session guards prevent canceled-session insertion. A final focused
GPU capture removes coincident red-cone striping with presentation-only polygon
offset, reusing unchanged native behavior. See
[native receipt](validation/countersink-hole-controls.md) and
[backend/API receipt](validation/countersink-hole-backend.md). Actual Apple M4
captures were inspected; private editors are closed. Independent review and
integration remain pending. Pattern and the full parameterized plate remain
separate children.

## Countersunk plate cumulative gate candidate (2026-10-13)

After #244–248 and #250 were independently accepted on main, #249's first
blank-document shared-API source run completed. The 21.1-second movie includes
the actual Change Parameters table before modeling, rough centered rectangle
with Length/Width bindings, Thickness extrusion, four R5 Fillets, supported
Point with two retained 10 mm edge dimensions, Hole Ø4/Ø10/90°/All preview and
commit, and two-axis 3×3 Spacing/Adjust Pattern through source Save. The saved
one-solid BRep independently passes nine top openings, mid-frustum sections,
bottom exits and through-bore classifier checks. Strict one-component millimeter
STL, Length/Thickness edits, exact undo/redo and source-state reopen pass. See
[the candidate receipt](validation/countersunk-plate-tutorial.md) and external
`deliverables/countersunk-plate-2026-10-13/attempt-01`. The private editor is
closed. Independent cumulative review is pending; the full lesson is not yet
accepted, and the movie is shared-API input rather than physical native input.

Slot live solved-drag preview is explicitly retained as Todo #258; the bounded
#251 candidate currently previews a rubber line and solves on release.
## Center Point Slot candidate (2026-10-13)

#251 adds an origin-snapped center-first straight slot with retained construction
centerline, two semicircular arcs, two sides, live Midpoint/Coincident/Equal/
Tangent relations, native half-length/angle/width entry and pointer preview.
The source sequence rough drag → Horizontal → D150 → R25 closes to a 200 ×
50 mm profile with zero remaining DOF; later length edit, atomic refusal,
history and reopen pass focused core/native and real-app API checks. The settled
native captures are readable and the private editor is closed. See the
[candidate receipt](validation/center-point-slot.md). Independent review is
pending; this is one ingredient of the later full sketch lesson.

## Center Point Arc Slot candidate (2026-10-13)

#252 adds a center-first open arc slot with a retained construction path arc,
two analytic side arcs and two rounded caps. The path center/endpoints and cap
radius stay independently editable; derived contacts and sides regenerate and
cannot take independent constraints. The focused source core reaches R150,
R15, Horizontal and D135 at zero DOF with two selectable profiles alongside
the prior straight slot. Native typed and pointer creation, preview and settled
GPU captures pass; a separate real-app shared-API run passes the complete
source constraint/edit/history/reopen chain. See the
[candidate receipt](validation/center-point-arc-slot.md). Independent review
is pending; the full multi-page lesson remains separate.

## Center Point Ellipse candidate (2026-10-13)

#256 adds a retained analytic ellipse with two full construction axes and live
Midpoint/Perpendicular relations. The documented center, first-axis point, and
contour-point picks produce a closed selectable OCCT profile. Explicit origin
Coincident, Horizontal, and full-axis D275/D100 take the source fixture to zero
DOF and area `6875π mm²`; equality/crossing edits retain authored identities.
Focused core and native creation/dimension tests pass with inspected captures;
the private editor is closed. See the [candidate receipt](validation/center-point-ellipse.md).
Independent review and full sketch-lesson validation remain separate.

Latest integration: ellipse #256 accepted78a4cb7/68ddbce with dedicated center
validation merge correction. Main app builds; focused ellipse0.42s passed.
All private editors closed. Sol prepares full #257 runner in ellipse worktree;
no full-lesson execution or movie acceptance yet.

Full #257 attempt03 passes247calls/all31steps, with64.5s creation movie, three
editable sketches and exact history/reopen. It remains open: actual final capture
shows the fixed ellipse blue beside a free conic because coloring uses globalDOF.
New prerequisite #259 (SKT-064) tracks solver-grounded independent solved color.
Sol implements a bounded fix using PlaneGCS nullspace-dependent parameter
mapping; Astra has reviewed functional evidence but cannot accept the
visual gate yet. Failed attempts01/02 retain offviewport fixture mistakes.

#259 focused root fixture is tests/mixed_sketch_colors_api.py, prepared but not
yet run. It loads the full attempt03 source model, checks entityFreedom for
fixed ellipse/free conic, captures unselected/selected states, removes the first
axis dimension (7→8DOF), captures the underdefined control, then Cancel verifies
exact original authoring and finished-state solved metadata/capture. Candidate
worktree is ../Alloy-worktrees/entity-solved-colors; Sol owns implementation,
Astra source review, root sole editor lease when binary is ready.

Next complete lesson: ornament-pattern-20261013, independently audited public
source under external knowledge/tutorials/after-slots-splines/. Four actual
prerequisites and full gate are queued SOL-037, SKT-065, DOC-021, SOL-038, TUT-014.
First prove FullRevolve80/5/R50 bottom-face Shell5 geometry before broader work.
No source step is dropped; spline connector automation is unknown and explicit
retained Line connectors are disclosed. All five issues remain Todo while #258
finishes. No ornament runtime or implementation acceptance yet.

Latest: #258 solved slot preview accepted and integrated; native evidence in
validation/slot-drag-preview.md includes actual capsule/profile, unchanged draft
until release, exact cancellation and late-release protection. Private editor
closed. #260 full-Revolve planar support/Shell source proof is now In Progress
with Sol; #261 open-spline offset is active in a second isolated Sol lane.
#262–264 remain Todo. First prove the actual80/5/R50→Shell5 source
geometry, then stable generated-face naming and native support selection.

## Origin Section Analysis candidate (2026-10-13)

DOC-021/#262 adds optional retained analyses outside features, document-wide ID/
plane/single-active validation and analysis-only undo transactions reusing exact
evaluated geometry. Shared Inspect commands, native plane/hatch/Cancel/OK controls,
Analysis browser edit/show/hide and GPU texture-capped clipping are implemented.
Focused core and 79-call real-entrypoint API evidence pass, including exact modeling
preservation, replacement undo/redo, save/reopen and document tabs. The full ornament
lesson and independent acceptance remain separate. Final neutral hatch/plain captures and corrected panel synchronize successfully;
native popup input remains pending qualification; see validation/section-analysis.md.

## Open fit-spline Offset candidate (2026-10-13)

#261 adds a retained signed exact OCCT offset of an open fit spline with stable
derived endpoints for explicit Line closure. Focused core checks prove normal
distance, one selectable strip profile, atomic refusals, source/distance edits,
Intersect rebuild and history/reopen. Native selection, side/value preview,
Cancel and accept pass with the renderer showing both curves; a separate real-app
API run passes closure and Intersect. See the [candidate receipt](validation/fit-spline-offset.md).
The full eight-member ornament Join remains a separate #265 gate, and independent
review of this ingredient is pending.

## Exact-offset array Join investigation (2026-10-13)

SOL-039/#265 is actively investigated in isolated `exact-offset-join` at base
ad0e75e. Actual candidate #261 profile/GeometryKernel Intersect inputs pass OCCT
Boolean argument checks, but two-rib/hub Fuse reports pcurve construction failure
and acquired self-intersection; eight ribs produce invalid fifteen-solid geometry.
Shared rigid-transform geometry and immutable forced SameParameter reconstruction
both fail to cure it. No production geometry or tolerance policy has changed.
A bounded 1e-6 mm library offset approximation also fails the eight-rib Join;
no production approximation fallback is justified. Astra reviews the structural
Boolean diagnosis before further experiments; actual retained #263 Pattern/Join reproduction remains
required. See validation/exact-offset-join-investigation.md and external
validation/ornament-feasibility. No editor was launched for these diagnostics.

#265 clean input qualification now blocks the proposed CellsBuilder experiment:
untouched retained and raw Shells are BRep-valid but fail OCCT Boolean
self-interference checks on the inner offset face/seam (~0.804 mm acquired edge
tolerance). A first eager-constructor diagnostic was confounded and is explicitly
labeled externally; clean pre-BOP hashes confirm the finding independently.
Actual source-prefix Intersect also refuses before Pattern. Separate exact-prism
rigid rotation passes with shared locations and fails under deep copy; #263 owns
that bounded transform correction and retained validation. No partition/8-member
expansion or production geometry policy changed. See the updated investigation
receipt; Shell and Join-lineage concerns need separate bounded decisions.
# Box and Shell named-length candidate (2026-10-13)

PAR-004 extends the accepted stable-ID direct length binding authority to
primitive Box width/depth/height and inward Shell thickness. Native fields and
shared API accept direct names, preserve binding on edit/reopen and replace it
on numeric input. Focused Box/Shell core, native Qt input and real-entrypoint API
checks pass, including a shared sketch/Shell Thickness, impossible Shell update
rollback and authored save/reopen. The inspected GPU captures show named fields
and the hollow preview; private editors are closed. See
[the bounded candidate receipt](validation/box-shell-parameters.md) and external
`deliverables/box-shell-parameters-2026-10-13`. Independent review and
integration remain pending. The Shell fixture uses an accepted extrusion stock;
primitive Box face lineage and the full Box→Shell source chain are separate work.

# Extrude Offset start candidate (2026-10-13)

SOL-044 adds Start > Offset to the shared Extrude command. Offset is an
independent signed displacement along the authored sketch normal before
Distance; the original sketch/support is unchanged. Native/API create and edit,
stable direct user-length IDs, preview/Cancel, generated face/edge support,
pattern seed propagation and authored history/storage are covered by focused
core, Qt and real-entrypoint checks. The source front-normal sign case evaluates
at world Y=5..10 for Offset -10 and Distance +5, and an internal ellipse Cut
removes exactly the expected volume. See
[the candidate receipt](validation/extrude-offset-start.md). Independent review
accepted the bounded ingredient; integrated on main. To Object and
expression parsing are separate.

## Linked planar Project candidate (2026-10-13)

SKT-066 adds explicit linked Project of real straight face boundaries and normal-to-plane edges (as points), plus root-sibling identity sketch support and sources. The candidate is in `../Alloy-worktrees/linked-planar-project`, branch `feat/linked-planar-project`. Its bounded core, real-entrypoint API and native Qt input tests pass the Box → Shell source, actual visible inner-edge pick, retained constraint anchor, projected-profile extrusion, Box resize, sibling Lid support, preview/Cancel/commit and reopen. See [the candidate receipt](validation/linked-planar-project.md). Broader silhouettes, curves, detached copies and occurrence transforms are outside this ingredient. Independent source/capture review accepts the bounded ingredient; integrated on main. The neighboring `face_support_test` fails with the same origin-reference error on main `3f7650a` and this branch, separate from Project.

## Typed Line angle candidate (2026-10-13)

SKT-070 adds separate Length/Angle Line fields, retained Euclidean length and
angle dimensions, and an authored horizontal construction reference for a later
Coincident correction. The Box → Shell → Project source ingredient, Midpoint
axis, Horizontal connector, Mirror, named Thickness edit and save/reopen run
through the real application. The existing length-only chained Line remains
supported. See [the bounded receipt](validation/typed-line-angle.md). Physical
OS input remains unqualified; the revised preview layout and final constrained
half were captured through the real application at
`/tmp/alloy-skt070-angle-api-final-cursor/`. The horizontal helper shares the angled
line start, leaving exactly one free endpoint length until Coincident to the
projected outer corner makes the half fully constrained. Focused solver, Qt and
real-app checks pass. Independent final source and capture review accepted the
bounded ingredient. Historical source-video quadrant and physical OS key input
remain unqualified.

## Model parameter table candidate (2026-10-13)

PAR-005 adds a grouped Model Parameters table to Change Parameters and shared
`document.modelParameterSet` editing for already-authored Box dimensions, Shell
thickness, sketch driving lengths, Extrude distance/start/end offsets, and
retained open-chain sketch Offset. Stable authored target IDs, direct user
parameter bindings, numeric replacement, atomic evaluation, history and
save/reopen are covered by focused tests. The seven-row real-app Box → Shell →
slanted Line → To Object → open-chain Offset sequence passes, including
Clearance rescaling with the inward Offset sign retained. Native Qt
Expression-cell editing, invalid refusal and numeric unbinding pass. See
[the candidate receipt](validation/model-parameter-table.md) and the external
`advanced-audit-2026-10-13/model-parameter-table-contract.md` source record.

## Sliding-lid cumulative API gate candidate (2026-10-13)

TUT-015 now has one source-order real-app runner in
`tests/sliding_lid_workflow_api.py` extending the accepted numeric dovetail
Cut prefix through root-sibling linked Lid Project, five-edge Offset and
closure, To Object Lid, after-authoring model bindings, 74 × 40 × 100 resize,
midpoint 15 × 2 thumb ellipse/Offset Start Cut, and Y Slider. The 206-call
final actual-app run passes with two separately inspected rest STL parts, a
posed Lid STL and STEP, undo/redo and save/reopen. Its native framebuffer
captures and exact measured geometry are in
[the candidate receipt](validation/sliding-lid-cumulative.md). Independent
review and a paced full-creation movie remain pending before closing the
cumulative gate; ingredient Qt input evidence is separate from this API run.
Independent physical review of that first cumulative run found the authored
positive-Y Slider pose crossed the Box's closed back wall. ASM-009 adds an
explicit Reverse Direction flag to the shared Slider command and native dock;
existing joints retain forward motion by default. The corrected cumulative
runner uses positive coordinates 0..50 along negative world Y and asserts the
Lid is entirely beyond the front at position 50. Focused core, native Qt and
actual-app API checks pass with save/reopen, positive coordinate limits and
negative world STL bounds. See [the updated cumulative receipt](validation/sliding-lid-cumulative.md).
The corrected full creation run and movie remain pending with root.

## Selected-face Section Analysis candidate (2026-10-13)

DOC-023 extends the accepted origin analysis with an exact current planar face
reference, signed millimeter Distance and Flip. The native panel has canvas
selection, Name, Clear, preview/Cancel/OK; shared inspection reports the
resolved plane and unavailable support. Disappearing faces disable clipping
without blocking a modeling edit; undo recovers the retained identity.
Focused core, 32-call real-entrypoint API, native Qt input and inspected Apple M4
hatched GPU capture passed; the independently accepted correction is integrated.
Physical OS input and other-platform GPU runtime remain pending. See
[the accepted receipt](validation/selected-face-section.md)
and the external `advanced-audit-2026-10-13/selected-face-section-contract.md`.

## Origin Section Distance and Flip candidate (2026-10-13)

DOC-027/#318 adds signed Distance and Flip to XY/XZ/YZ analyses using the
same resolved-plane renderer as selected-face sections. Legacy six-field
origin records remain valid; edited records retain both transform fields.
This enables the later adapted Width section at XZ/Y=32 without a substitute
support face. Focused core, native and real-app evidence are recorded in
[the candidate receipt](validation/origin-section-distance.md). Independent
compatibility review and integration are complete.
## Retained circular model-edge Project candidate (2026-10-13)

SKT-071 extends face-sketch linked Project to true analytic circles from
imported STEP occurrences. Stable body/edge/occurrence identities resolve the
current center and radius after captured placement; virtual solver geometry
stays out of authored data. The adapted PCB's four mounting holes project onto
an identity-sibling Case floor, Offset−0.3 creates four radius1.05 peg profiles,
and Join5 succeeds. Partial arcs refuse full-circle projection. Focused core,
real-app API, and native Qt circular-edge Project and projected-circle Offset
pick/typed preview/Escape pass; the prior straight-edge Project core test passes.
The cumulative case-shell/analysis movie remains TUT-020. See
[the candidate receipt](validation/linked-circular-project.md).

## Enclosure mounting prefix candidate (2026-10-13)

TUT-020 now has a from-blank real-app runner for the independently authored
nominal v2 board assembly. It retains separate Z+90 insertion and later
`[28, -42.5, 6]` placement/Capture, then authors the sibling centered
64 × 93 × 30 Case, grouped R5/R2.5 Fillet, closed Shell3.5, three named
sections, four actual linked mounting-hole Projects/Offset−0.3, and Join5
pegs. The 110-call final run, 145.2-second paced API creation movie,
retained edit/undo/reopen, and strict Case-only STL/STEP readback pass.
GEO-009 corrected automatic face-sketch center enumeration so generated
inner blend arcs do not claim authored circle-center identity; it retains
ambiguity refusal for genuinely conflicting source circles. See
[the cumulative candidate receipt](validation/enclosure-prefix-workflow.md).
Independent compatibility review accepted the bounded prefix. This adapted input and
world-axis placement do not establish creator-file identity or physical fit;
connector/card openings remain later TUT-016 work.

## Current isolated candidate: retained construction-plane Project

An isolated candidate extends the shared linked Project path to sketches on earlier retained construction planes (SKT-072). The exact evaluated plane frame, earlier active body and imported occurrence identities remain authoritative; face-only legacy boundary references stay restricted. `linked_planar_project` core and `tests/construction_plane_project_api.py` passed with the actual imported enclosure connector face, plane edit, undo/redo and reopen. See [validation](validation/construction-plane-project.md). The accepted enclosure prefix remains the cumulative baseline; projected closed connector-loop Offset is a separate pending dependency before ports.

## 2026-10-14 tangent-point plane foundation candidate

GEO-004/#286 has a separate `feat/tangent-point-plane` candidate in
`../Alloy-worktrees/tangent-point-plane`. It retains a named curved face and
oriented current edge endpoint in ordinary construction-plane history; surface
normal evaluation uses the endpoint, never the center. Core/build evidence passes
for the curved Loft endpoint and attached circle with source edit/history/reopen
and unavailable-reference refusal. Native controls/API exist but real-entrypoint,
native/GPU captures and independent review are pending the sole private GUI lease.
See [candidate contract](validation/tangent-point-plane.md). Do not close #286.
GEO-018/#350 explicitly preserves the prerequisite for generated corner-Fillet
endpoint identities through the source separate perimeter round; it is a Todo
dependency of GEO-004 and TUT-017. The actual final source endpoint remains required.


### Tangent foundation actual-app follow-up

After explicit GUI-lease handoff, the bounded local-server API lifecycle on the
curved Loft endpoint passed, with inspected controls and actual Apple M4 GPU
captures. The native Qt test could not establish initial viewport focus and
stopped before Escape/selection; preserve this as unverified, with no inter-step
focus repair or accepted-native claim. State reporting and viewport Escape
shortcut were corrected in e57bd32 after independent source/core review. Runtime
evidence is external under `validation/tangent-point-plane-2026-10-14/`; source
final Fillet endpoint remains pending; independent API/core review is accepted. The sole
GUI lease was handed back to Thicken work after all private tangent processes
exited. See the [bounded receipt](validation/tangent-point-plane.md).

## 2026-10-14 Two Sides surface Extrude candidate

SRF-014/#355 has an isolated `feat/surface-extrude-two-sides` candidate. It
retains open sketch-curve IDs and independently measured positive reaches on
both sides of the solved sketch normal. OCCT evaluates one zero-volume sheet
face per curve in a single surface-body compound; the faces are not sewn.
Source sketch and feature edits, stable face/edge IDs, a planar-face attached
sketch, undo/reopen, real-app API, and focused Qt native selection/typing/Escape
passed. Independent review accepted the bounded ingredient; see the current
acceptance entry at the top of this document. The source
outline is independently authored and downstream Trim/Boundary Fill integration
remains separate. See [candidate validation](validation/surface-extrude-two-sides.md).

## 2026-10-14 accepted Surface navigation

VIS-004/#358 in `feat/surface-workspace` replaces placeholder tabs with shared
Solid/Surface navigation. Surface Create exposes the implemented Two Sides
Extrude and guided surface Loft; Thicken moves to Solid Create. Explicit Alloy
policies preserve active dialogs on tab changes and keep E as Solid Extrude;
public parity for both is unknown. Guided surface Loft locks New Component.
Isolated application/native-test build and focused 4/4 Qt-delivered checks pass,
including actual Surface Extrude preview/Cancel, tab-switch session preservation,
and guided Loft mode locking. The actual app capture was inspected; physical OS
input and matched screenshot parity remain unqualified. Independent Astra source
review found no blocker and tooltip polish is applied. See
[bounded contract](validation/surface-workspace-navigation.md).
Surface Modify is integrated with retained Remove; the combined native route
check passes. Independent Astra review accepted both source and final evidence.


## 2026-10-14 exterior rail surface Loft candidate

SRF-012/#353 has a bounded isolated candidate for an endpoint-contact fit-spline
rail from one solved open arc to an ordered connected two-line lower section.
The OCCT sheet is open, two-face, zero-solid, and has source-derived face/edge
IDs; broader rail/contact and handle-bridge modes remain separate. Focused core,
37-call real-app API, and Qt-delivered viewport selection/Cancel checks passed.
Independent Astra review accepted this bounded scope; main integration build
and rail core check pass. See [validation](validation/surface-rail-loft.md).

## 2026-10-14 paired-sketch surface Trim candidate

SRF-013/#354 has an isolated `feat/surface-iron-trim` candidate: retained two-spline XZ sketch-normal through-target Trim for a current two-face open sheet. The shared API/native dock previews selected inner regions red, then removes one or both with stable source-derived face/edge IDs. Focused core, real-app API, and Qt native checks pass; independent compatibility review and integration are pending. The current authored rail-sheet fixture creates one central seam opening, so it is an ingredient rather than full bilateral iron geometry. A separate OCCT probe against the SRF-017 two-patch exterior produces two closed side loops usable by the handle bridge, but retained SRF-017/SRF-015 integration remains open. Surface-as-tool trials remain SRF-016. See [bounded validation](validation/surface-trim.md).

## 2026-10-14 assembly solver backend

`feat/assembly-solver-adapter` adds a pinned replaceable shared OndselSolver and
original POD/C ABI for fresh serialized Revolute/Cylindrical graphs, one full
root Pin and optional signed principal Revolute angle. The focused macOS core
check passes full initial/nonidentity Pin/positive-negative drive poses and
independent original-joint/finite/refusal checks. Arbitrary current Pin uses
mature FixedJoint because upstream isFixed constrains absolute zero. No general
DOF/rank claim, GUI or actual ASM-014 acceptance. Windows/Linux shared recipe is
explicitly unqualified; no static fallback. See
[decision006](decisions/006-assembly-solver.md). Authoritative retained origin and
native/API integration is separate `feat/piston-revolute` work. Independent Astra
backend review accepted a06b2be, integrated at59782f1; actual-entrypoint workflow
checks and frontend acceptance remain required before closing ASM-014.
