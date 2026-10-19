# Partial capability rows in the printable-part v1 plan

Implementation baseline, 2026-10-18, following the user's review of the v1
direction; the first-15 scope proposal below was added 2026-10-19. These are
the retained minimum **proposed v1** variants, not
acceptance of each full command family or physical-input parity. The
accepted receipts prove only their named subsets. REL-030 still owns the final
inventory, release-gate and site reconciliation.

## First 15 broad partial proposed-v1 rows

The following boundaries are a proposed minimum for a robust creation tool:
users can start and recover a design, author and constrain editable geometry,
and repair or clearly refuse broken dependencies. They are scope proposals,
not feature acceptance. Existing receipts establish only the accepted slices
named in the reconciliation table; unresolved behavior remains proposed until
REL-030 closes the inventory review.

| Inventory row | Proposed minimum for v1 | Later or separately owned remainder |
| --- | --- | --- |
| `document.lifecycle` | New, Open, Save, Save As and multiple design tabs for native authored documents, including dirty-close handling and safe refusal of unsupported newer schemas. | Older-schema migration and interrupted large-save recovery under REL-002. |
| `document.recovery` | Recover the latest committed authored state after interruption; a failed Save must preserve the previous valid file. Active tool/sketch drafts may be excluded. | Schema migration and broader large-save/disk-full recovery under REL-002. |
| `parameter.user` | Named length parameters, arithmetic dependencies, and direct bindings for supported sketch driving dimensions and feature distances; reject invalid dependencies without losing the last valid model. | General functions, all unit types, arbitrary feature fields, and broader rename/dependency semantics. The exact retained contract remains proposed under PAR-001. |
| `history.edit` | Edit an existing feature input and recompute downstream features while preserving references, with undo and save/reopen. | Broader feature edit/reorder coverage; dependency-validated reorder has its own accepted slice in HIS-006. |
| `history.suppression` | Suppress and restore one feature, propagate the dependent state, and retain the authored intent through undo and save/reopen. | General suppression across every feature/dependency combination; behavior remains proposed under HIS-003. |
| `history.rollback` | Temporarily evaluate an earlier history point and return to the latest feature without deleting authored future history. | Persisting a non-latest marker and its save/reopen policy; those choices remain proposed under HIS-003. |
| `history.reference-repair` | Preserve common downstream references through edits; for a missing or ambiguous reference, show an explicit repair path or refuse atomically without arbitrary rebinding. | All topology-change classes, projected-link variants and broader repair UX beyond the accepted named cases. |
| `sketch.lifecycle` | Create, edit and finish origin-plane sketches and sketches on supported retained planar faces; allow explicit redefinition for the qualified ordinary support route. | Broader face/support types, normal-entry preferences and topology-repair cases. |
| `sketch.interaction` | Native line and rectangle creation with typed values, visible supported snapping/inference, and explicit conflict recovery that preserves the draft. | General intersection inference, all inline-key/focus combinations and broader constraint conversion. The final minimum is still proposed under SKT-021. |
| `sketch.line` | Create chained lines with coordinate or length/angle entry and retained intent; support the accepted endpoint, horizontal/vertical and tangent inference slices. | Other inference families and remaining native numeric-entry paths. |
| `sketch.fit-spline` | Editable fit-point splines with retained tangent handles and downstream profile use. | Periodic/closed variants, broader endpoint constraints and other spline modes. |
| `sketch.control-spline` | Editable control-point splines with curvature inspection. | Pole/weight/continuity options beyond the accepted control-point slice; exact scope remains proposed under SKT-012. |
| `sketch.construction` | Construction geometry excluded from profiles, supported midpoint relations, and point-to-edge distance dimensions with driving/driven behavior. | Additional relation families and unsupported curve/edge combinations. Point-to-edge behavior remains open under SKT-016. |
| `sketch.dimensions` | Driving and driven dimensions for supported lengths, angles, radii/diameters and common point/line distances, with unit-aware display and retained intent. | Broader dimension types and combinations. Point-to-edge distance and offered driving-to-driven conflict conversion remain proposed/open. |
| `sketch.project` | Associative projection of supported planar edges, vertices and circular references into a sketch, with source identity retained and safe failure when a source disappears. | Independent-copy choice, broader analytic curves/silhouettes and explicit relink/break-link repair beyond the qualified linked cases. |

The minimums above intentionally overlap where one workflow depends on another:
construction relations support dimensions, projection supplies sketch
references, and history repair preserves downstream features. Release evidence
must still exercise the shared UI/API authority, actual geometry, edit, undo,
save/reopen and native input for the required subset. These proposals do not
close the open owners or cumulative release gates.

## Next 15 broad partial proposed-v1 rows

Scope proposal added 2026-10-19 for the next inventory tranche. These smallest
useful modeling results support an editable shape-creation workflow; existing
receipts establish only the named subsets linked in the reconciliation table.
Each proposed minimum still needs its open owner and cumulative release
evidence. No broad option parity or runtime acceptance is implied.

| Inventory row | Proposed minimum for v1 | Later or separately owned remainder |
| --- | --- | --- |
| `sketch.offset` | Retained signed offsets for supported closed line/arc loops and supported open chains, with selectable resulting profiles and atomic refusal on collapse or ambiguous source identity. | General mixed curves/chains, self-intersections, arbitrary corners, and full option parity. |
| `sketch.trim` | Retained Trim for supported analytic line/circle/arc intersections used to create profiles; preserve safe entity/constraint identity and report removed constraints. | Splines and other curve families, multi-interval gestures, and broader constraint-preservation behavior. |
| `sketch.extend` | Extend a straight authored line to its nearest supported unambiguous straight-line intersection, with preview, cancel, retained endpoint identity and atomic refusal on ties/unsupported cases. | Curved targets, ambiguous intersections, and broader curve families. |
| `solid.extrude` | Editable one-sided Distance extrusion for New Body, Join and Cut on supported closed profiles; retain the accepted planar To Object, Cut All and offset-start variants. | Symmetric/Two Sides, taper and thin wall remain required v1 work under `solid.advanced-extrude`; Intersect and broader combinations retain separate ownership. |
| `solid.revolve` | Editable New Body Full or Partial Revolve around a supported retained sketch axis, with an ordered closed profile and explicit refusal for invalid/crossing axes. | Join/Cut/Intersect, thin variants, broader axes/profiles and wider Boolean operation editing. |
| `solid.sweep` | Editable New Body Sweep using a supported circular profile and line/arc path with perpendicular full-path behavior and retained path/profile identity. | Arbitrary profiles, guide rails, twist/taper, path-distance extents and broader path combinations. |
| `solid.loft` | Editable New Body Loft through at least two ordered closed circular sections, with Free ends, no rails and explicit invalid-order refusal. | General noncircular profiles, continuity controls, guide rails/centerlines, and broader section combinations. |
| `solid.hole` | A simple editable cylindrical Hole from a supported sketch point/face with an explicit target and depth or through-all extent; retain the accepted Countersink subset. | Counterbore has accepted bounded sketch-point evidence under `solid.hole-variants`; broader placement and internal/tapped variants remain separately owned. Modeled external Thread remains required. |
| `solid.fillet` | Editable constant-radius Fillet on supported selected solid edges, including a supported tangent chain, with preview/Cancel and atomic refusal when invalid. The separate proposed-v1 `solid.advanced-fillet` row still requires one specifically chosen advanced mode. | Other variable-radius, chordal, setback and advanced modes, general topology and broader edge/support cases. Do not treat the open `solid.advanced-fillet` choice as deferred by this row. |
| `solid.chamfer` | Editable Equal Distance Chamfer on supported retained solid edges, with preview/Cancel, edit and atomic invalid-distance refusal. | Two Distance and Distance And Angle remain required v1 work under `solid.advanced-chamfer`; tangent-chain variants and broader generated-edge naming remain separately owned. |
| `solid.shell` | Editable constant inward Shell on supported solids, with either one selected planar opening face or the accepted closed-shell cases; impossible thickness must refuse atomically. | Outward/both-side modes, rounded options and arbitrary stock/topology. |
| `solid.move` | Editable Free Move for one body using retained translation/rotation values or manipulator gestures, with one history feature and Cancel/undo. | Multi-body and broader pivot/mode behavior. Body Copy and geometry Align are separate inventory rows but currently share the open SOL-008 owner. |
| `solid.split` | Split one supported solid by a retained plane into two valid independently selectable solids, preserving owner and stable result identities. | Face/surface/curve splitting and broader split-tool choices. |
| `solid.mirror-body` | Reflect one source body across a supported planar face or datum plane into one distinct retained body; do not implicitly Join. | Multi-body/group modes, other object types and broader plane/option coverage. |
| `solid.mirror-feature` | Mirror one retained Cut or one supported ordered feature group across a supported origin plane, preserving source-stage evaluation, target identity and downstream references. | Arbitrary feature sequences, all plane types, general topology and universal downstream naming. |

The closed receipts cited in the reconciliation table support bounded ingredients:
they do not prove that the proposed minimum is complete or accepted. In
particular, Simple Hole and Counterbore now have bounded accepted retained sketch-point slices under SOL-078/#608 and SOL-079/#609; broader Hole placement and tapped variants remain open. Loft evidence accepts
the bounded circular workflow but not general profiles/rails/continuity; Revolve
controls are accepted only for the recorded Qt/API fixture, without physical OS
input or full tutorial acceptance; and feature/body transformations remain
limited to their documented fixtures. Those uncertainties remain release work.

## Next 15 broad partial rows after row 049

Scope proposal added 2026-10-19. Each entry keeps a useful v1 capability in its
family and marks broader modes as a proposed remainder, not an accepted deferral
or completion claim. Existing receipts prove only their named fixtures.

| Inventory row | Proposed minimum for v1 | Later or separately owned remainder |
| --- | --- | --- |
| `solid.rectangular-pattern` | Retained one- or two-direction rectangular patterns for supported body and additive/Hole feature seeds, with count/spacing edits, stable occurrence identity and atomic invalid-input refusal. | Broader feature/component kinds, optimized modes and more pattern objects. The required body-versus-feature minimum remains a scope decision under SOL-009. |
| `solid.circular-pattern` | Retained full-circle pattern of one body around a supported axis, plus an editable count and stable occurrence identities. | Arbitrary axes, partial/angular distribution and circular feature/component pattern families remain proposed under SOL-009. |
| `surface.extrude` | Extrude selected open sketch lines/fit splines into separate editable sheet faces, with one- or two-sided extent and retained source identity. | Broader profile/extent combinations, sewn or joined surface output, and automatic inclusion of newly added curves. |
| `surface.loft` | Create an editable G0 sheet through ordered supported sections, including one retained guide/centerline route, with explicit refusal when requested continuity or references are unsupported. | G1/G2 modes, broader rails/sections and other continuity combinations. |
| `surface.trim` | Trim one supported sheet with a retained intersecting planar sheet or paired sketch cutter; make the kept region explicit and preserve the target on cancel/failure. | General curved/multi-cell partitions, arbitrary tool/target classes, Extend and Untrim. |
| `surface.thicken` | Thicken one supported retained sheet into an editable closed solid using signed one-side thickness, stable source references and atomic self-intersection refusal. | Broader curvature/self-intersection cases and general Join/Cut or alternate operation modes. |
| `construction.offset-plane` | Create an editable signed offset from an origin plane, prior construction plane, sketch plane or supported planar face; retain the source and update after edits. | Interactive arrow manipulation, angled/edge/point-derived datum variants and broader source classes. |
| `inspection.section` | One active non-destructive section view on an origin plane or supported retained planar face, with offset/flip, readable hatching and persisted visibility. | Multiple simultaneous sections, arbitrary transformed/curved supports, custom colors and broader inspect tools. |
| `inspection.measure` | User-facing measurements for selected edge length, point/line distance, angle, planar area and solid area/volume, with units and source-selection identity; inspection leaves authored geometry unchanged. | Curvature, minimum-distance/clearance, mass properties and broader multi-object measurement combinations. |
| `selection.general` | Select and inspect stable body/face/edge IDs through viewport and Browser, with clear/add/replace/filter behavior and explicit refusal for stale or ambiguous targets. | Further occlusion/overlap policies, platform modifier routes and other physical-input qualification owned by the release gates. |
| `component.local` | Create and activate local components, assign/reparent bodies and sketches, and preserve component ownership and placement through edit, undo and reopen. | Arbitrary transformed/nested hierarchy, broad Browser drag gestures and assembly joint behavior. |
| `component.linked-copy` | Copy/paste a local component occurrence with its linked subtree and a distinct retained placement/occurrence identity; source edits continue to update linked instances. | Make Unique/unlinked independent copies, cross-document copy and broader nested linked structures. |
| `component.reference` | Insert a local STEP assembly as an external linked component with retained placement/source identity; update an unchanged source and support one changed-source update only where correspondence is proven, otherwise refuse safely. | General changed-file correspondence/relink, broader source formats and multi-file reference management. Common-format import remains a separately required v1 capability. |
| `sketch.break` | Break one supported analytic line, arc or circle at an interior pick into retained editable pieces, with explicit IDs/constraint handling and atomic refusal on unsupported geometry. | Spline/group Break, multiple simultaneous breaks and broader constraint-repair cases. |
| `sketch.transform` | Move, Copy, Rotate or Scale one connected selected sketch group as an editable shared-authority operation; preserve compatible constraints or refuse, with preview/Cancel, undo and save/reopen. | Disconnected/multi-group transforms, broader constraint transformations and unsupported curve families. |

This tranche does not remove or reduce the printable-part release contract.
STL/STEP and Core 3MF export, common-format import, modeled external Thread,
and the macOS/Windows/Linux package/runtime gates remain separately owned and
required v1 work in the inventory and cumulative release gates. These proposals
do not approve a later disposition for any wider noncloud feature family.

The following are the highest-priority bounded ownership gaps exposed by this
tranche. These are proposed child scopes/dependencies, not newly created issues
or accepted v1 cuts; where an existing broad parent is named, split it only
after preserving its remaining requirements.

| Candidate bounded owner | Existing parent/dependency | Finishable acceptance boundary |
| --- | --- | --- |
| Measure analytic baseline | DOC-014/#26 | Separate from Section; measure selected edge length, point/line distance and angle with units/provenance, compare against analytic fixture values, and prove no authored geometry/history change. |
| Analytic sketch Break | SKT-011/#47 | Break one line/arc/circle at an interior pick into editable pieces with stable IDs and explicit constraint disposition; unsupported cases fail atomically; verify undo/save-reopen. |
| Required solid pattern seed matrix | SOL-009/#54; receipts SOL-027/#191, SOL-036/#248, SOL-038/#263 | Declare body and required feature seed kinds for rectangular and circular patterns; cover count/spacing, retained edits and stable instances for each claimed kind. Keep arbitrary axes/partial angular distribution as a separate remainder unless explicitly accepted. |
| Surface Extrude minimum separated from Revolve | SRF-001/#60; receipts SRF-014/#355, SRF-020/#461, SRF-022/#464 | Name required v1 curve/profile and extent cases, then verify editable sheet output, source identity, edit and refusal behavior; keep Revolve separately visible in the parent remainder. |
| Changed external-reference source update | ASM-011/#315; insertion/unchanged-source prerequisite ASM-010/#314 | Use one committed local STEP source-change fixture; update when persistent correspondence is provable, preserve occurrence placement, otherwise refuse without corrupting the prior instance. Keep relink/multi-file behavior explicit. |

The inventory cross-links below point to this section. Existing accepted receipts
remain fixture-bounded; none of the proposed minima claims full interaction,
native-input, package or cross-platform release acceptance.

| Inventory row | Proposed v1 subset | Wider family after v1 or separately owned |
| --- | --- | --- |
| `sketch.arc` | Center Point, Three-Point and accepted line hold-drag tangent continuation | Other tangent modes and construction variants |
| `sketch.slots` | Center Point, Center-to-Center, Overall and Center Point Arc slots | Broader endpoint/constraint editing and other input routes |
| `sketch.ellipse` | Center Point Ellipse with editable axes | Other construction modes and constraint combinations |
| `sketch.conic` | Three-point conic with editable Rho | Broader constraints and downstream combinations |
| `sketch.text` | Editable outline text in the accepted font subset, with placement and rotation | More fonts, path text and layout |
| `sketch.fillet` | Accepted one- and multiple-line-junction fillets, including shared-line and dimension treatment | Analytic/spline junctions and wider options |
| `sketch.mirror` | Associative line/arc and ordinary open fit-spline Mirror | Closed/control/offset splines and transformed support |
| `solid.primitives` | Box, Cylinder and Sphere, including accepted Sphere Cut/Repeat | Other primitive commands retain their separate owners |
| `solid.combine` | Explicit-target Join/Cut/Intersect, Keep Tools and ordinary invalid-result recovery | SOL-071/#566 accepts bounded names/recovery; broad topology remains later work |
| `body.rename` | Current local body via Browser/shared command, retaining identity | Imported/nonlocal organization policy; physical gestures stay in release input gates |
| `construction.midplane` | Retained plane between supported parallel planar faces | Nonparallel/angular-bisector cases |
| `construction.path-plane` | Start/End plane on supported retained fit-spline path | Arbitrary fractions and other curves |
| `construction.tangent-plane` | Tangent plane at a retained point on a supported face | GEO-004's bounded constructor is accepted; GEO-024/#590 retains the ordinary native-input route, and broader face/point cases stay later |

Issue ownership and accepted evidence are in [the ownership audit](ownership-audit.md),
the [inventory](command-inventory.json) and their linked receipts. Release
qualification must also cover actual native interaction, geometry, edit,
undo/reopen/export and packaged runtime on each supported platform. A proposed
subset is not a release promise until REL-030 and the cumulative gates accept it.
The first-v1 3MF requirement means Core **export**; import and optional
extensions remain separately tracked under XCH-005; common-format import remains
required v1 work. Modeled external Thread is required in the first-v1 printable
workflow; internal/tapped Hole variants remain separately owned and are not
implied by that slice.

## Final 18 partial proposed-v1 S rows after the first 45

Scope proposal added 2026-10-19 for the remaining S rows in the 150-row
inventory. These minimums keep each required capability visible in v1 while
bounding a finishable first result. Broader modes remain roadmap work, not
accepted deferrals; issue receipts support only their named fixtures. This
section does not claim implementation or runtime acceptance.

| Inventory row | Proposed minimum for v1 | Later or separately owned remainder |
| --- | --- | --- |
| `solid.advanced-extrude` | Editable symmetric/two-sided distance and offset-start extrusion; supported taper, To Object/All extent, and thin-wall extrusion each have explicit units/targets, preview, edit and atomic refusal on invalid geometry. | Broader target classes and option combinations, with physical-input and package qualification kept in release gates. SOL-010 remains the owner until its option slices are finishable. |
| `solid.hole-variants` | Editable Counterbore and accepted Countersink choices on supported points/faces, with explicit diameter, recess/depth, cut target and through extent; preserve placement through source edits and save/reopen. | Broader placement sets and internal/tapped Hole/thread variants; modeled external Thread remains separately required. |
| `solid.draft` | Apply an editable angle Draft to one supported solid face about a retained neutral plane, with explicit pull direction, preview and atomic refusal when the result is invalid. | Multi-face/group draft, alternate neutral references and broader topology. |
| `solid.scale` | Uniformly scale one retained body about an explicit origin or selected reference, with editable factor/origin, distinct identity when copying first, and undo/reopen. | Nonuniform/multi-body scaling and broader reference/pivot modes. |
| `solid.align` | Align one source body to one target planar face with explicit frame and flip choice, preview/Cancel and retained transform; refuse ambiguous or unsupported references. | Edge/axis/multi-reference alignment and generalized assembly placement. |
| `solid.body-copy` | Create an independently identified retained body copy with a distinct placement and editable downstream features; source and copy remain independently selectable and changes do not alias. | Cross-document copy, linked/unique component semantics and broader dependency transfer. |
| `solid.face-offset` | Offset one supported retained planar face by a signed distance, including the accepted straight-edge and full circular-cap-rim continuation cases; preserve stable references for supported downstream features and refuse unsupported lineage. | Other rebuilt curves, multi-face/tangent cases and broader topology naming, including the explicit SOL-075 remainder. |
| `solid.advanced-chamfer` | Provide editable Two Distance and Distance And Angle chamfers on supported selected edges, with preview, clear invalid-distance refusal and retained downstream references. | Broader tangent chains, generated-edge naming and difficult topology cases. |
| `surface.revolve` | Revolve one supported open sketch section through a retained axis into an editable surface, with angle/direction edit and area/bounds inspection; do not fabricate a closed solid. | Broader section/axis sets, multiple profiles, joins and surface-operation combinations. |
| `surface.sweep` | Sweep one supported open section along one retained line/arc path as an editable G0 sheet, reporting unsupported path or end conditions instead of silently changing them. | Rails, multiple sections, G1/G2 continuity and broader path/section combinations. |
| `surface.patch` | Create both a planar cap from one closed planar loop and one bounded nonplanar four-edge fill with declared boundary continuity; reject disconnected/underdefined loops. | Larger boundary networks, additional continuity classes and broader multi-face repair. The two result types remain separately finishable under SRF-003. |
| `surface.offset-ruled` | Create an editable signed offset of one supported face and a ruled sheet between two supported retained edges, with explicit normal/angle direction and atomic fold/invalid-input refusal. | Broader surface families, multi-edge construction and self-intersection recovery. |
| `surface.extend-untrim` | Extend one supported sheet boundary to a declared distance or retained target, and restore one supported trimmed face to its prior boundary as Untrim; preserve source/history on failure. | Multi-edge and generalized intersection-driven extension, complex trim histories and broad recovery variants. |
| `surface.inspect` | Display evaluated face normals and a simple zebra/curvature view; report sampled positional and tangent continuity with method/units and do not infer continuity from appearance alone. | Curvature combs, broader sampling controls and G2/advanced analysis. |
| `construction.more-planes` | Create one editable angled datum plane about a retained line/edge and source plane, with angle/side controls and update or explicit refusal after source edits. | Other point/edge-derived plane combinations and broader interactive manipulation. |
| `construction.axes-points` | Create retained axes from two supported points or a supported edge, and construction points from a retained vertex or supported curve location, with stable references and edit/refusal behavior. | Other axis/point constructors, inferred references and broader derived-geometry classes. |
| `print.refinement` | Offer coarse/fine/custom export tessellation with actual triangle-count preview, plus one-file/per-body structure and safe filename-collision handling; failed/cancelled batches preserve prior files. | Advanced presets, broader batch naming and platform-specific chooser qualification. This does not reduce required STL/STEP/Core 3MF export. |
| `history.general-expressions` | Evaluate named-parameter arithmetic with explicit units and dependency order in supported sketch dimensions and feature distances; detect cycles/unit mismatch, retain last valid results on failure and verify recompute/undo/save-reopen. | Broader functions, unit domains, arbitrary field bindings and incremental-evaluation optimization. The user-facing named-parameter table remains coupled to PAR-001. |

The v1 printable-part contract remains explicit: STL, STEP and Core 3MF export;
common-format import; modeled external Thread; and macOS, Windows and Linux
package/runtime release qualification are required work. None is moved into a
later remainder by this tranche.

Up to five proposed highest-priority bounded children are listed below. These
are candidate issue scopes under the named existing owners, not newly created
issues or accepted scope reductions.

| Candidate child title | Existing parent/dependency | Finishable acceptance boundary |
| --- | --- | --- |
| Counterbore Hole with retained placement (accepted SOL-079/#609 slice; wider SOL-007 open) | SOL-007/#52; simple Hole SOL-078/#608; Countersink SOL-035/#247 | Reuse accepted point/target/recess source-edit and checked-solid receipts; qualify only additional required placement or tapped variants through new bounded children. |
| Neutral-plane Draft | SOL-013/#59 | Draft one supported side face at a declared angle around a retained neutral plane; verify face direction, dimensions, edit, invalid-angle refusal and history/reopen. |
| Uniform body Scale | SOL-013/#59 | Scale one retained/copy-created body about a declared origin/reference; verify factor, bounds, identity, edit and undo/reopen. |
| Planar cap and nonplanar Patch split | SRF-003/#62; prerequisite SRF-002/#61 | Deliver separately testable planar-loop cap and bounded four-edge nonplanar patch; check boundary deviation/continuity and atomic refusal for disconnected loops. |
| Export refinement and batch structure | PRT-002/#76; required export gates GATE-050/#86 and REL-018/#421 | Compare coarse/fine/custom tessellation and actual triangle counts; export selected bodies as one/per-body files and test collisions/cancellation without losing prior outputs. |

All row links in the reconciliation ledger point to this proposal. Open issues
remain open, mapped closed issues remain bounded evidence, and package/native
acceptance remains with the cumulative release gates.
