# Public reference: eventual feature breadth

Retrieved **2026-10-11**. Status: **research and proposed decomposition**, not
implemented Alloy capability or an approved implementation sequence. All
evidence below is public Autodesk documentation. No proprietary reference application, web
client, installation, private API, or account-gated material was accessed.
No reference images or Autodesk assets were copied into application assets.

These notes complement the initial shape-creation scope. An extension label is
a commercial packaging fact, **not evidence that a feature requires cloud
computation** and not a reason to drop it from Alloy's eventual scope.

## Evidence and boundaries

**D** means documented behavior, **P** means an Alloy proposal/inference, and
**U** means unresolved. This pass reads documentation; it makes no runtime or
pixel-parity claims. Source links are listed with stable local IDs below; all
share the retrieval date above. Living help can change without a release tag.

| Family | D: public behavior relevant to compatibility | Cloud/offline evidence and U |
|---|---|---|
| Assemblies | Components hold geometry, sketches, origins, relationships and child components; external components reference separate designs. Joint positions previously unpositioned components; As-Built Joint relates already positioned components. Seven named joint types, motion limits, rigid groups and contact sets are documented. [B01–B03] | Cached components can be inserted offline [B04]. Complete external-reference updating, collaborative editing and every constraint command's offline behavior remain U. Assembly structure must not be conflated with a collection of bodies. |
| Drawings | Drawings are derived documents. A base view projects the design; projected, section and detail views supplement it. Documented tutorial adds dimensions, leaders, parts lists and balloons, then PDF output; DWG is also named. [B05–B07] | Local versus cloud execution of the entire drawing lifecycle is U in these sources. Do not defer drawings merely because their reference workflow stores them in a project. |
| Sheet metal | Rules specify thickness, K-factor, gap, bend radius and reliefs. Flange covers base, edge and contour cases. Unfold supports editing across bends then Refold; Flat Pattern is a separate derived representation for manufacturing/export, with updates from folded geometry and potentially flat-only edits. [B08–B10] | Per-command offline coverage U. Flat Pattern and Unfold are distinct representations; treating them as the same operation would break downstream manufacturing. |
| Direct modeling | Direct mode does not record parametric features; a Base Feature permits direct editing inside a parametric timeline. Geometry created directly does not later turn into its original individual parametric feature sequence. [B11] | No cloud dependency established by this source. P: preserve explicit base-feature authoring rather than pretending imported geometry has recoverable construction history. |
| Form/freeform | Edit Form operates on selected faces, edges and vertices with move/rotate/scale manipulators. Neighboring geometry moves for continuity. Internal/duplicate mirror and circular symmetry, weld, clear and isolate symmetry are documented. Self-intersection can prevent solid conversion. [B12–B13] | No cloud dependency established. Exact representational compatibility, conversion tolerances and a suitable open implementation remain U. A generic subdivision preview alone is not full Form parity. |
| Configurations | Rows identify variants; columns can configure dimensions, suppression, visibility, materials, appearance and sheet-metal/plastic rules. Variants feed assemblies, drawings, render, animation, simulation and manufacture; the default table row cannot be deleted. [B14–B16] | Command-level offline and external-variant resolution U. P: separate variant identity from row position and preserve references from downstream documents. |
| Mesh | Parametric mode selects face groups; direct mode can select faces. Repair, remesh, reduce, plane cut and conversion are separate operations. Watertight conversion yields a solid; otherwise it yields surfaces. Faceted, prismatic and organic conversion have different semantics. [B17–B19] | Organic conversion is extension-labeled, not established cloud-only [B19]. Repair modes can destroy inner structure [B18], so a visually plausible repair is not sufficient geometry evidence. |
| Manufacture | Setups define process, models, stock/WCS and post settings; additive setups add machine print settings and build-volume placement. Workflow runs through operations/toolpaths, simulation and machine output. Scope includes milling, turning, drilling, cutting, additive and inspection. [B20–B23] | These sources do not establish offline support for every process. Manufacturing toolpath simulation [B22] is distinct from cloud FEA in the Simulation workspace [B27]. Mesh export for a slicer is not built-in additive manufacturing parity. |
| Nesting and advanced manufacture | Sheet-metal sources need flat patterns for nesting. Source settings include quantity, grain, rotation and priority. Advanced machining, nesting/fabrication and metal additive are grouped in the Manufacturing Extension. [B24–B25] | Extension access does not prove cloud computation. Operation-specific dependencies, machine-specific output coverage and native library licenses remain U. |
| Rendering | Separate local and cloud render choices exist. In-canvas rendering runs on the computer and explicitly does not need internet to start. Scene/camera/material setup precedes render. [B26] | Local compute must remain in eventual scope. Cloud render can be deferred. Asset-library downloads and all file/gallery operations offline remain U; local computation alone does not prove the entire product workflow is offline. |
| Simulation | Current help explicitly says reference CAD application Simulation studies run in cloud. It enumerates static/nonlinear stress, modal, buckling, thermal/thermal stress, shape optimization, event simulation, injection molding and electronics cooling. [B27] | Cloud solve is a justified deferral candidate; keep setup/result representation as explicit later work. Older official tutorial still offers local solve [B28], so it must not be used as current availability evidence. Numerical validation requires more than an attractive stress plot. |
| Electronics | An umbrella document synchronizes schematic, 2D PCB and 3D PCB. Symbols/connectivity, footprints, packages and board shape are distinct data. Manufacturing uses CAM processing, Gerber RS274X/X2 and Excellon with DRC. [B29–B30] | Available-library discovery can fail in offline mode [B31]; this establishes a dependency for that operation, not all electronics editing. Offline schematic/layout/routing/package generation behavior remains U. |
| Animation | Storyboards contain timed camera/component transforms, visibility and callouts. At time zero or the scratch zone, transforms are applied without recording actions. Local video export is documented separately from web sharing. [B32–B33] | Local video destination is explicit; complete offline authoring/export U. Web viewing/sharing is separately deferrable. Animation is not a mechanically solved joint motion system. |
| Generative | Setup specifies design space and requirements; generation produces alternatives, followed by exploration. Outcome generation explicitly uses cloud computation. Preserve geometry remains in the result; obstacles prevent unwanted geometry. [B34–B36] | Generation is a justified cloud deferral. Setup, result inspection and editable outcome conversion remain separate roadmap items. Do not equate a single topology-optimization result with full generative workflow coverage. |
| Specialized plastic design | Boss can connect bodies and reinforce thin regions with optional ribs; plastic rules supply dimensions. This is extension-labeled. [B37] | No cloud requirement established. Broader lip/snap-fit/design-advice parity needs command-specific research; this source supports Boss only. |

### Cloud classification policy proposed for the backlog

1. **Established cloud operation:** generative outcome generation, current
   Simulation workspace solves and cloud rendering. Allow explicit deferral;
   record setup/data/result work separately.
2. **Established local capability:** in-canvas render can start without internet;
   local render computes on the machine; cached design editing and cached
   component/derive insertion are documented offline [B04, B26]. These claims
   have different scopes and must not be inflated into whole-workspace parity.
3. **Network-specific workflow:** library discovery, web sharing, upload,
   cloud project versions/history. Defer the remote operation, retaining local
   authoring/persistence requirements [B04, B31, B33].
4. **Unknown:** absence of cloud wording is not proof of offline capability.
   Keep the family in eventual scope, with a research task for each uncertain
   operation. Do not reproduce reference CAD application's license-validation interval as an Alloy
   product requirement.

## Proposed later epics / decomposition seeds

All are P, **not implementation claims**. They depend on the initial document,
stable references, shared UI/API command path, undo and persistence foundation.
Each epic must add inspectable domain state, command discovery, structured
failure/completion, save/reopen and real-app workflow coverage. These are
bounded epic seeds; split numerical algorithms and user commands further when
scheduled. Source IDs indicate the reference basis, not ready specifications.

| ID | Coherent deliverable | Dependencies and completion evidence |
|---|---|---|
| BR-01 | Component/occurrence structure and local external references [B01] | Core document; reused part has independent occurrences, stable identifiers and saved placements. Changing its source updates an assembly with explicit missing-reference errors. |
| BR-02 | Joint origins, Joint and As-Built Joint [B02–B03] | BR-01; fixture with rigid/revolute/slider joints reaches expected placements and degrees of freedom through UI and API. |
| BR-03 | Remaining joint types, limits, drive and relationship diagnostics [B03] | BR-02; cylindrical, pin-slot, planar and ball fixtures; numerical limits and conflicting/redundant relationship errors. Resolve documentation contradiction before exact Ball specification. |
| BR-04 | Rigid groups, motion links/contact-set research and assembly motion [B02–B03] | BR-03; small mechanism preserves selected relationships during a driven cycle; distinguish kinematic validation from dynamics. Research detailed contact/link semantics first. |
| BR-05 | Drawing document, sheets, base/projected/section/detail views [B05–B07] | BR-01 and stable references; a model edit updates correct views and flags broken references. |
| BR-06 | Dimensions, leaders, parts lists, balloons and drawing export [B05] | BR-05; useful assembly drawing with correct quantities, units, scaling and PDF; DWG/DXF support requires library/license decision. |
| BR-07 | Sheet-metal rules and base/edge/contour flanges [B08–B09] | Solid features; folded bracket changes thickness/radius through authored rule and feature overrides. |
| BR-08 | Unfold/edit/refold and derived Flat Pattern [B10] | BR-07; cross-bend cut survives refolding; flat cut geometry and bend allowance are numerically verified. |
| BR-09 | Flat-pattern drawing and cutting exchange [B05, B10] | BR-06/08; drawing and DXF agree on dimensions and bends; model changes invalidate/update derived outputs. |
| BR-10 | Direct edit/base-feature boundary [B11] | Solid history; imported geometry can be edited without fictitious feature reconstruction; loss of history is explicit and undoable. |
| BR-11 | Editable freeform control representation and symmetry [B12–B13] | Solver/library selection; handle-like shape supports vertex/edge/face edits and mirrored changes. Validate authored topology separately from tessellation. |
| BR-12 | Freeform repair and solid/surface conversion [B12] | BR-11; conversion produces valid geometry or identifies self-intersection; subsequent CAD edits preserve the authored Form. |
| BR-13 | Mesh inspection and minimally destructive repair [B17–B18] | Mesh import; damaged scan fixture reports open/nonmanifold/orientation issues and retained/lost structures after repair. |
| BR-14 | Mesh editing and faceted/prismatic/organic conversion [B19] | BR-13/10/12; reduction and conversion report tolerance and topology limits, with open versus closed input tests. Separate staged conversion methods. |
| BR-15 | Configuration table, dimensions, suppression and variant identity [B14–B16] | Parametric recompute; two sizes share history, undo and persistence; suppression does not silently redirect references. |
| BR-16 | Variant materials/rules and downstream configured assemblies/documents [B14–B16] | BR-01/06/08/15; drawings and flat patterns point to intended variant after reorder/rename. |
| BR-17 | Plastic rules and boss/rib authoring [B37] | Solid history; editable enclosure fastening geometry inherits rule dimensions with feature overrides. Research remaining specialized plastic commands separately. |
| BR-18 | Local materials, scene/camera setup and rendering [B26] | Geometry/material data; produce local image with independently licensed assets, explicit dimensions and cancellable render status. |
| BR-19 | Storyboards, exploded assembly and local animation output [B32–B33] | BR-01; saved actions reproduce timed visibility/transform sequence without changing modeled placements; output plays as a local video. |
| BR-20 | Manufacturing setup, tools, stock, WCS and process libraries [B20–B21] | Solids/components; tool and stock units explicit, invalid setups actionable, authored CAM separate from toolpath cache. |
| BR-21 | First 2.5D milling/drilling path, simulation and postprocessing [B20–B22] | BR-20; bounded machined bracket, verified stock/collision results and inspected machine-specific output; never send to hardware automatically. |
| BR-22 | Turning, 3D/multi-axis machining and inspection increments [B20, B25] | BR-21; decompose by process, with distinct machine/kinematic verification fixtures and output validators. |
| BR-23 | FFF setup, placement/support, toolpath and local print output [B23] | BR-20 and mesh pipeline; integrate suitable mature slicing foundation; inspect generated layers/output independently of STL export. |
| BR-24 | Sheet nesting/fabrication and additional additive processes [B24–B25] | BR-09/20/23; start with flat parts, grain/quantity/material constraints; separate later powder/resin/metal support and output contracts. |
| BR-25 | Electronics schema, schematic, board layout, checks and fabrication exchange [B29–B30] | Mature ECAD integration decision; local small-board project produces matched schematic/board connectivity plus checked Gerber/drill output. Split routing, rule checking and file formats. |
| BR-26 | ECAD/MCAD 3D packages and enclosure synchronization [B29] | BR-25/01; board-outline and mounting changes propagate predictably; missing package shows a labeled placeholder, not fabricated geometry. |
| BR-27 | Simulation studies, setup, solve boundary and qualified results [B27] | Materials/meshing/library research; cloud solve may defer. Any local solve proposal requires verified benchmark problems, units, convergence and limits before engineering claims. |
| BR-28 | Generative study and outcome lifecycle [B34–B36] | BR-27; explicit cloud-generation deferral allowed; later outcomes preserve interfaces, avoid obstacles and become editable geometry with provenance. |

## Three cumulative useful-object validation gates

These are proposed Alloy acceptance workflows, independently authored fixtures,
not instructions to operate reference CAD application. Later gates retain earlier capabilities and
saved files. Numeric tolerances belong in fixture specifications before coding.

1. **Adjustable printed desk clamp.** Build a dimensioned clamp body and jaw,
   edit opening and wall dimensions upstream, inspect clearances, add a threaded
   or explicitly simplified screw, constrain the assembly, save/reopen and
   export each printable part. First-release prerequisites are real constrained
   sketch/history and valid solids; later BR-01–03 add assembly behavior.
   Evidence: API replay, dimensions/volume/topology, mesh validity and unit
   check, actual-app captures and a bounded human editing walkthrough.
   Screenshot beauty is not printability or physical-load certification.
2. **Configurable benchtop instrument enclosure.** Retain that modeling and
   assembly capability, then create two enclosure sizes with folded sheet-metal
   chassis, local PCB/connector geometry, printed feet and mounting bosses.
   Produce folded and flat representations, drawing with quantities and
   dimensioned views, DXF cut data and per-variant printable parts. Change board
   width once and demonstrate intended downstream updates or explicit broken
   references. Exercises BR-05–09, 15–17 and eventually 25–26.
3. **Serviceable instrument product package.** Retain both enclosure variants;
   add an ergonomic freeform handle and a machined bracket, render locally,
   author exploded service animation, and create bounded milling/FFF outputs
   with simulation evidence. Damaged-mesh repair is exercised on a replaceable
   scanned grip fixture. Add electronics fabrication files when BR-25 lands.
   Cloud simulation/generative results are an optional separately labeled
   extension, never a prerequisite disguised as finished offline functionality.
   Manufacturing review covers output correctness; no machine is operated by
   this acceptance test.

## Source register and unresolved pitfalls

All entries are primary Autodesk public documentation retrieved **2026-10-11**.
Text was available through public web retrieval/search indexing. This pass did
not verify documented behavior in reference CAD application. Links using `contextId` are Autodesk
help routes; titles identify the intended page if routing changes.

| ID | Source |
|---|---|
| B01 | Components (reference `B01`) |
| B02 | Assembly relationships (reference `B02`) |
| B03 | Joint types (reference `B03`) |
| B04 | Work offline in reference CAD application (reference `B04`) |
| B05 | Drawing tutorial: multiple views, dimensions, parts list and balloons (reference `B05`) |
| B06 | Base views (reference `B06`) |
| B07 | Section views (reference `B07`) |
| B08 | Sheet metal rule reference (reference `B08`) |
| B09 | Create base/edge/contour flange (reference `B09`) |
| B10 | Unfold, including distinction from Flat Pattern (reference `B10`) |
| B11 | Modeling modes in reference CAD application (reference `B11`) |
| B12 | Modify T-Spline bodies with Edit Form (reference `B12`) |
| B13 | T-Spline symmetry (reference `B13`) |
| B14 | Configurations overview (reference `B14`) |
| B15 | Configurable aspects (reference `B15`) |
| B16 | Configuration Table (reference `B16`) |
| B17 | Mesh overview (reference `B17`) |
| B18 | Repair a mesh body (reference `B18`) |
| B19 | Convert a mesh body to a solid body (reference `B19`) |
| B20 | Manufacture overview (reference `B20`) |
| B21 | Setups for manufacturing (reference `B21`) |
| B22 | Simulation for manufacturing (reference `B22`) |
| B23 | Workflow: preparing parts for additive manufacturing (reference `B23`) |
| B24 | Component sources for nesting (reference `B24`) |
| B25 | Manufacturing Extension (reference `B25`) |
| B26 | Render the design (reference `B26`); Create a rendered image, explicit in-canvas offline statement (reference `B26b`) |
| B27 | reference CAD application study types (reference `B27`) |
| B28 | Older Analyze Simulation Results lesson PDF (reference `B28`) |
| B29 | Electronics Design document (reference `B29`) |
| B30 | Prepare manufacturing data (reference `B30`) |
| B31 | Available tab does not list libraries; Autodesk Support, 2023-10-08 (reference `B31`) |
| B32 | Animation overview (reference `B32`) |
| B33 | Publish and share animations (reference `B33`) |
| B34 | Generative design workflow overview (reference `B34`) |
| B35 | Outcome generation (reference `B35`) |
| B36 | Preserve geometry (reference `B36`); Create other obstacle geometry (reference `B36b`) |
| B37 | Create a boss (reference `B37`) |

Specific evidence problems to preserve in future specifications:

- B03's Ball prose says three rotations and no translations, while its
  summary line says one translation and one rotation. Record the inconsistency;
  corroborate the intended degrees of freedom with another public primary
  source before treating it as exact command specification.
- B27 explicitly describes cloud studies; B28 offers local solve. Treat B28 as
  a conflicting older teaching artifact, not proof of today's local capability.
- Sources span evolving terminology, commercial extensions and collaboration
  regimes. Retrieval date does not establish one matched release/platform.
  Select a baseline before claiming exact UI/default compatibility.
- Form representation, advanced CAM, ECAD integration, drawing exchange and
  solver/library licensing need independent open-source foundation research.
  Public descriptions specify behavior, not implementation internals.
- Reference libraries, standard hardware, material datasets, environments,
  machine definitions and postprocessors have their own provenance/licenses.
  Public availability does not grant permission to redistribute those assets.
- This is a broad family inventory, not an exhaustive command/default list.
  Assemblies' newer constraint relationships, specialized plastic features,
  PMI, inspection, sophisticated machining strategies and each electronics
  editor still need detailed public specifications as implementation approaches.
