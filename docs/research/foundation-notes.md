# C++ CAD foundations: evidence, decisions, and bounded spikes

Research date: **2026-10-11**. All external sources linked below were retrieved
on that date. This is public documentation/source research and proposed work,
not an implementation report. No reference CAD application application, installation, resource,
private API, account, or runtime was accessed. C++20 is accepted; Rust is outside
the planned stack. **MIT is the accepted license for original Alloy source and
documentation.** Dependencies retain their own licenses and require an
integration/distribution audit. The root [AGENTS.md](../../AGENTS.md) remains
authoritative.

The current [continuation context](../context.md) records a Qt shell and OCCT
7.9.3 discovery, with no integrated CAD geometry. Research did not rebuild or
operate Alloy. The live OCCT documentation now identifies itself as **8.0.1**;
its API descriptions must be checked against the chosen dependency revision
before implementation. Upstream `main`/`master` links are observations, not pins.

## Recommended direction, pending implementation evidence

Keep Qt Widgets and OCCT as the first implementation path. First prove one
selectable solid, an authored operation, undo, save/reopen, and STEP exchange
through the existing application command path. Evaluate OCAF on that same
workflow before choosing a native format. Evaluate PlaneGCS first for sketches,
behind an audited LGPL library boundary. Retain SolveSpace as comparison
research; adopting its GPL-only library in distributed Alloy would require an
explicit later licensing decision. Add lib3mf when the print workflow reaches
export.

These are dependency recommendations; the original-code MIT decision is
accepted, while exact dependency versions and integration terms remain to be
verified. The important early uncertainties are document ownership, persistent selection
references, solver integration, native viewport behavior on macOS, and exchange
fidelity. Solve those with small fixtures before implementing a large command
catalog or a generic plugin/geometry abstraction.

## Capability and license evidence

### Qt and OCCT

| Foundation | Verified public evidence | Alloy implication / unresolved work |
| --- | --- | --- |
| Qt 6 | Qt documents LGPL obligations, including library replacement and notices; some modules have different license availability. [Qt licensing](https://www.qt.io/development/open-source-lgpl-obligations) | Inventory the modules actually shipped, linkage, plugins, fonts, and packaging. Existing Qt discovery/build does not establish distributable bundle completeness. Keep the current Widgets shell. |
| OCCT license | OCCT publishes LGPL 2.1 plus its additional exception for object code incorporating header material. [OCCT license](https://occt3d.com/dev/doc/overview/html/occt_public_license.html) | Preserve notices, exact source/version, changes, and bundled dependency obligations. The exception is not a blanket removal of LGPL duties. Alloy's MIT license does not relicense OCCT. |
| Solid/surface algorithms | Public guide covers primitives, sweeps, loft-related construction, Booleans, offsets, fillets/chamfers, and shape modification history. [Modeling algorithms](https://occt3d.com/dev/doc/overview/html/occt_user_guides__modeling_algos.html) | Good kernel coverage for the initial shape-creation scope. Alloy still owns command semantics, parameter editing, input validation, selection, preview, recompute, and errors. Documented algorithms do not imply reference CAD application option/default parity or success on arbitrary geometry. |
| Shape validity/healing | OCCT documents diagnostic and repair tools for problematic shapes. [Shape healing](https://occt3d.com/dev/doc/overview/html/occt_user_guides__shape_healing.html) | Treat imported/healed geometry and changed tolerance as observable results. Preserve the original import and report repairs; avoid silently widening tolerances until an operation happens to succeed. |
| Visualization | AIS supplies interactive presentations/selection; V3d and the camera supply projection/navigation; the documented graphic driver uses OpenGL. [Visualization](https://occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html) | Begin with OCCT presentation and picking inside Qt. Prove native window/context integration, Retina scaling, overlays, resize, and capture on the actual supported OS. Do not assume Qt's graphics backend choice changes OCCT's renderer. |

**Integration inference:** one OCCT viewer and one authoritative document is the
smallest promising path. A custom renderer would also require tessellation,
picking, edge styling, clipping, and interaction work. This research establishes
no reason to take that on before the native viewport spike.

### History, naming, and persistence are separate problems

OCAF documents labels/attributes, transactions, undo/redo, persistence, function
services, and topological naming. It expects modeling operations to report
topology evolution. Its own examples assign semantics such as prism base, top,
and side faces. [OCAF guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__ocaf.html)

`BRepTools_History` records and merges generated, modified, and removed
subshapes. This describes relationships across executed operations; it is not
the user's editable feature timeline.
[History API](https://occt3d.com/dev/doc/refman/html/class_b_rep_tools___history.html)

**Proposed Alloy ownership:**

- Authored state: stable sketch/entity/constraint/feature identifiers, parameter
  values and expressions, feature order, dependencies, suppression, and selected
  reference intent.
- Evaluated state: current kernel shapes, status and diagnostics, and topology
  evolution used to resolve references.
- Display state: tessellation/presentation caches, camera, hover/selection
  presentation. Rebuilding these must not erase authored intent.

**Hard unknown:** neither the presence of TNaming nor a successful box example
proves robust references after face splits, merged faces, repeated symmetric
geometry, changed sketch topology, deleted features, or reordered operations.
Do not persist face indices, traversal order, raw pointers, or display triangle
IDs as design intent. Do not silently rebind an ambiguous reference to the
nearest face. Resolve uniquely or expose a repairable broken reference.

**Bounded decision:** compare an OCAF document using standard attributes with
a small explicit Alloy schema on the same sketch/extrude/fillet fixture. Assess
transaction ownership, readable diagnostics, save/load versioning, and naming
records. Select one authored-state authority. Do not build two independently
mutable document models or design a universal serializer. An OCAF choice can
still expose a simple Alloy command/state API; labels need not become public
agent identifiers.

### Mature sketch solver candidates

| Candidate | Verified public capability | Integration / license consequence |
| --- | --- | --- |
| FreeCAD PlaneGCS | `GCS.h` exposes 2D geometric constraints, driving/driven parameters, solve status, degrees-of-freedom/diagnostic facilities, and line/circle/conic/B-spline-related constraints. Its SPDX identifier is `LGPL-2.1-or-later`. [GCS.h](https://raw.githubusercontent.com/FreeCAD/FreeCAD/main/src/Mod/Sketcher/App/planegcs/GCS.h) | Strong first candidate for the sketch feature scope. It is part of FreeCAD, not evidence of an independently packaged stable solver SDK. Preserve upstream license/header notices and audit the selected revision's whole extracted file set. |
| PlaneGCS dependencies | The implementation includes Eigen, Boost graph, FreeCAD Base/Console/Tools/config headers; the public header also uses Sketcher export definitions. [GCS.cpp](https://raw.githubusercontent.com/FreeCAD/FreeCAD/main/src/Mod/Sketcher/App/planegcs/GCS.cpp), [SubSystem.h](https://raw.githubusercontent.com/FreeCAD/FreeCAD/main/src/Mod/Sketcher/App/planegcs/SubSystem.h) | Measure extraction cost with a minimal build and a patch inventory. Do not claim it is dependency-free or bring in FreeCAD's entire application to avoid a small adaptation. Pin a release/commit and plan upstream updates. Audit Eigen/Boost and copied support files separately. |
| SolveSpace library | The project explicitly provides a solver library and states GPLv3 licensing. Its library page warns that the solver is coupled to the application. [Library page](https://solvespace.com/library.pl) | Comparison research only under the accepted MIT direction. Do not adopt GPL-only libslvs in distributed Alloy without an explicit deliberate later licensing decision and integration audit. A C ABI does not remove license obligations. The website's old example archive is not a suitable version pin. |
| SolveSpace interface | Current `slvs.h` exposes C structures/handles, workplanes, 2D/3D points, lines/circles/arcs/cubics, constraint types, DOF and failure results, and dragged parameters. Some internal entity kinds are explicitly unexposed. [slvs.h](https://raw.githubusercontent.com/solvespace/solvespace/master/include/slvs.h) | Test the actual exposed library's curve and diagnostic coverage. Do not infer that every application capability exists in the public solver API. Use Alloy IDs and serialized inputs rather than persisting solver memory/layout. |

**Selection experiment, not a solver framework:** use a short-lived adapter for
an anchored rectangle with two driving dimensions, tangent line/arc profile,
an underconstrained drag, duplicate constraint, conflicting dimension, and a
near-degenerate small profile. Record convergence, remaining DOF, conflicting
IDs, and geometry preservation on failure. Test rebuild after undo/load. Add
one spline case to identify the limitation without making spline authoring a
first-slice prerequisite. Favor PlaneGCS through a verified modular LGPL
library boundary or a qualified permissive alternative. A successful SolveSpace comparison does not authorize
its distribution. Do not implement permanent runtime solver switching.

Solver success alone does not establish valid closed profiles, trim/extend,
automatic constraints, stable drag behavior, constraint glyphs, or projection
dependencies. Those remain Alloy sketch workflow work. A solver enum named
success must be translated into the documented application contract, including
residual/tolerance and whether evaluated geometry is usable.

### Import, exchange, and printing

| Format / library | Public capability | Bounded adoption and boundary |
| --- | --- | --- |
| STEP / OCCT | OCCT supplies STEP translation; XDE supports richer exchange including assemblies, names, colors, and other document attributes. [STEP](https://occt3d.com/dev/doc/overview/html/occt_user_guides__step.html), [XDE](https://occt3d.com/dev/doc/overview/html/occt_user_guides__xde.html) | First prove two solids with names, placement, units, and color; then a repeated component assembly. Choose shape-only versus XDE intentionally. Import produces editable imported geometry, not recovery of the original application's feature history. Record which STEP schemas/attributes were tested. |
| IGES / OCCT | OCCT supplies IGES translation. [IGES](https://occt3d.com/dev/doc/overview/html/occt_user_guides__iges.html) | Add after STEP with trimmed surface and open-shell fixtures. Report unsupported entities and sewing/healing; do not assume a surface import is a watertight solid. |
| STL / OCCT | OCCT supplies an STL writer; its mesher controls triangulation of shapes. [StlAPI_Writer](https://occt3d.com/dev/doc/refman/html/class_stl_a_p_i___writer.html), [Mesh guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html) | First printing export: select body, explicit unit convention, tessellation settings, triangle count and bounds, then independent readback. Verify orientation and closedness. Keep export tessellation separate from camera-dependent display detail. |
| 3MF / lib3mf | The consortium's C++ library reads/writes 3MF and provides validation/conversion; upstream declares BSD-2-Clause and lists bundled dependencies. [lib3mf](https://github.com/3MFConsortium/lib3mf), [3MF specifications](https://3mf.io/spec/) | Prefer this established implementation when exporting units, meshes, objects/components, and build items. Start with Core; extension support must be explicitly qualified. Inventory libzip, zlib, base64, and fast-float in the pinned build rather than treating the top-level BSD license as the entire bundle. |
| 3DM / openNURBS | McNeel documents reading/writing 3DM and NURBS evaluation; it explicitly excludes Boolean/intersection/meshing and SubD-to-BRep capabilities from this toolkit. [Capabilities and limitations](https://developer.rhino3d.com/guides/opennurbs/what-is-opennurbs/) | Appropriate later interchange candidate, not a substitute CAD kernel or freeform conversion solution. Prove trimmed BRep/curve/unit mapping to OCCT, including unsupported object reports. |
| openNURBS licensing | McNeel's current developer license explicitly includes openNURBS under MIT; the `8.x` repository LICENSE also contains historical notices and zlib terms. [Current developer license](https://developer.rhino3d.com/en/license/), [repository license](https://raw.githubusercontent.com/mcneel/opennurbs/8.x/LICENSE) | Archive the selected release's own notices and applicable license text. Do not substitute an old “public domain” description for the delivered source's terms. |
| DXF / libdxfrw | LibreCAD's current README identifies ASCII/binary DXF reading/writing under GPL-2.0-or-later; it labels several DWG and opaque-data paths experimental. [libdxfrw](https://github.com/LibreCAD/libdxfrw) | Research its format coverage, but GPL-only integration needs an explicit later licensing decision before distribution under the accepted MIT direction. Start with line, arc, circle, polyline/bulge, spline, layers, blocks/transforms, and units. DXF support is not blanket DWG or embedded solid support. |
| DXF / dxflib | QCAD's dependency notices and source identify GPLv2-or-later with separate commercial licensing. [QCAD notices](https://github.com/qcad/qcad/blob/master/LICENSE.txt), [dxflib source header](https://github.com/qcad/qcad/blob/master/src/3rdparty/dxflib/src/dl_entities.h) | Comparison candidate on the same 2D fixtures; GPL-only adoption requires an explicit later licensing decision. Do not commit to both implementations. Preserve entity-loss diagnostics and assess unsupported versions explicitly. |

**Proposed format order:** STEP and STL first; 3MF next for useful print
metadata; IGES and bounded DXF next; 3DM after the conversion spike. Preserve
long-term proprietary-format requirements as separate research issues. A
commercial translator advertised alongside OCCT is not evidence it is part of
the open-source kernel. No F3D/F3Z, ACIS, Parasolid, Inventor, or DWG fidelity
claim follows from installing OCCT.

**Exchange acceptance:** self-round-trip is useful but can hide paired reader
and writer mistakes. Use independently generated, redistributable fixtures and
an independent reader where feasible. Compare semantic quantities (units,
body count, placements, bounds, volume, names/colors) with tolerances; a
screenshot cannot establish exact interchange fidelity. Retain source URLs and
licenses for external test assets.

## Later foundations: keep the capability gaps visible

| Family | Public candidate evidence | What the evidence does not solve |
| --- | --- | --- |
| Mesh processing | CGAL offers packages with GPL/LGPL/commercial licensing depending on component. [CGAL licensing](https://www.cgal.org/license.html) | Select exact repair/remesh/Boolean packages only when a mesh workflow needs them; review their individual licenses. No package selected or tested here. |
| Freeform/subdivision | OpenSubdiv evaluates subdivision surfaces on CPU/GPU; its source uses a modified Apache license with trademark provisions. [Project](https://github.com/PixarAnimationStudios/OpenSubdiv), [license](https://raw.githubusercontent.com/PixarAnimationStudios/OpenSubdiv/dev/LICENSE.txt) | An evaluator does not implement familiar cage editing, T-spline semantics, or reliable watertight BRep conversion. Those require separate representation and conversion decisions. |
| CAM | OpenCAMLib documents drop-cutter and push-cutter/waterline algorithms with several cutter shapes and LGPL licensing. [OpenCAMLib](https://github.com/aewallin/opencamlib) | A candidate algorithm library, not full manufacturing parity: setup, stock, fixtures, tool libraries, strategies, simulation, machine kinematics, postprocessing, and verification remain application work. |
| Analysis meshing | Gmsh documents 3D finite-element meshing, C++ API, and GPL licensing with a linking exception described by the project. [Gmsh](https://gmsh.info/) | A mesher does not provide material models, boundary conditions, solution validation, or a complete CAE workspace. Revisit exact license/build and solver choice when this family is scheduled. |

These are research leads, not dependencies to install for the first release.
Local drawing, assembly, sheet metal, mesh/freeform, manufacturing, electronics,
and analysis capabilities remain roadmap families; the cloud deferral does not
remove them. This foundation review does not establish coverage of those
families or the target's complete feature inventory.

## Proposed early issues, with exit criteria

IDs below are local research/backlog proposals, not created external issues.
“Depends” names preceding proposals; sequencing may be consolidated around the
same runnable fixture. Every implementation issue extends the shared UI/API
command path and exposes structured state/errors; kernel-only success is not
application acceptance.

| ID | Proposed issue / kind | Depends | Concrete exit evidence |
| --- | --- | --- | --- |
| F01 | Record MIT decision and audit dependency admission — decision/verification | — | Record accepted MIT for original source/docs, separate dependency licenses, required notices/source/replacement arrangements, and exact modules/files reviewed. GPL-only distribution dependencies require an explicit later licensing decision. |
| F02 | Pin C++20/Qt/OCCT build and redistribution baseline — implementation | F01 | Record dependency versions/revisions and configure a reproducible clean build. Launch a relocatable local app bundle without relying on development library paths; list remaining OS coverage. |
| F03 | Embed OCCT viewport and select one real solid — spike/implementation | F02 | Real app creates a box through UI/API, renders shaded faces/edges, picks body/face/edge, handles Retina resize, reports camera/selection, and captures the actual CAD viewport. |
| F04 | Decide document authority and native persistence — spike/decision | F02 | Same authored primitive and parameter edit saved/reopened through candidate storage; transactions/undo and versioned schema demonstrated. Choose OCAF or explicit Alloy format with rationale, migration boundary, and no duplicate authoritative state. |
| F05 | Establish units, tolerance, and geometry-result contract — implementation | F03, F04 | Millimeter/inch fixture agrees in physical bounds and volume; invalid dimensions/degenerate shapes produce structured errors without mutation. API reports validity and units. |
| F06 | Deliver transactional feature edit, undo/redo, and atomic save — implementation | F04, F05 | One create/edit/failure/cancel sequence produces intended undo steps; reopen preserves IDs/parameters. Failed save leaves previous file usable. Dirty state and completion are inspectable. |
| F07 | Prove persistent references across recompute — spike | F04, F06 | Extrude/fillet fixture survives dimension changes, save/reopen and undo; split/delete/symmetric ambiguity cases either resolve correctly or report broken references. Store operation history; never silently select another edge. |
| F08 | Qualify PlaneGCS extraction and solver license boundary — spike/decision | F01, F02 | Small fixture suite above builds under C++20 and reports DOF/conflicts/residuals. Record licenses, copied/adapted files, dependencies, curve gaps, and a selected solver compatible with the accepted licensing direction after audit. SolveSpace remains comparison research unless a later licensing decision authorizes adoption. Time-box extraction; report blockers rather than importing the full application. |
| F09 | Implement authored sketch entities and constrained profile workflow — implementation | F05, F06, F08 | Create anchored dimensioned rectangle and tangent arc profile, drag remaining DOF, diagnose conflicts, undo, save/reopen, identify closed regions, and inspect constraints through API. |
| F10 | Implement first recomputable solid workflow — implementation | F07, F09 | Sketch → extrude → edge fillet → upstream dimension edit works in the real app; volume/bounds/validity and reference outcome agree with expectations. Invalid fillet leaves recoverable authored state and useful diagnostics. |
| F11 | Qualify STEP/XDE import and export — implementation | F04–F06 | Independent fixture with two named, colored, placed solids retains tested semantics; malformed input reports failures without corrupting the document. Publish precise supported schema/attribute matrix. |
| F12 | Export STL and 3MF for printing — implementation | F05, F10 | Chosen body exports with explicit units and controlled mesh error; independent readback checks bounds, orientation/closedness, triangle count, and 3MF object/build references. Record lib3mf version/licenses. |
| F13 | Qualify initial surface operations and IGES — implementation/spike | F05, F06, F11 | Ruled/lofted or swept surface, trim/sew/thicken fixture, edit and save/reopen; distinguish open shells from solids and report healing. Use bounded operation coverage, not a generic “surfaces complete” claim. |
| F14 | Select DXF adapter and scope 3DM conversion — research/spike | F01, F09, F11 | Read independent 2D DXF fixtures into sketch geometry with units/layers/blocks; list entity loss. Separately convert one trimmed 3DM BRep into valid OCCT geometry or document the conversion blocker. |
| F15 | Assemble real-app qualification and local package evidence — implementation | F03, F06, F10–F12 | A repeatable agent script performs create/edit/undo/save/reopen/import/export and captures the real viewport. Retain separate build, API, geometry, image, and physical interaction evidence plus remaining limitations. |

The first implementation stop point should be F03–F06 with a selectable solid
and usable persistence, followed by the F07–F10 parametric workflow. Start
dependency-license audits and solver decisions early enough to avoid a later rewrite. Leave broad
rendering replacement, multi-kernel infrastructure, generalized background job
scheduling, and optional format dependencies out unless these fixtures expose
a concrete need.
