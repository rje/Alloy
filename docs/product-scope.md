# Product scope

## Accepted requirements

Alloy is a standalone desktop application, not a Blender extension. The eventual
goal is functional feature-for-feature replacement of the reference CAD application, with an
experience intimately familiar to existing users. It must be open source and
modifiable. Features that currently require cloud access may be deferred.

The implementation is clean-room: public documentation, screenshots, and
instructions are permitted evidence; operating or inspecting reference CAD application and using
non-public information are forbidden. [AGENTS.md](../AGENTS.md) defines the
complete boundary. Public documentation is an input to an independently authored
implementation, not permission to copy product assets.

Agent control must exist from the first runnable app. Every implemented workflow
must gain corresponding commands and observable state so agents can operate,
self-test, and capture Alloy through the real application.

The 3D viewport must be GPU accelerated, with the actual renderer verified.
This is an explicit user requirement, including the initial viewport foundation.

macOS, Windows and Linux are required product targets. Validated milestones
should publish downloadable native builds with runtime dependencies, provenance,
and per-platform workflow evidence. Architecture and minimum OS versions must
be qualified rather than inferred from a successful build.

## Initial product focus

The first useful release should create and modify shapes, import common formats,
and export printable geometry. The table is a proposed coverage target, not an
implementation report. These features can arrive in several runnable increments.

| Area | Initial coverage target |
| --- | --- |
| Sketches | Lines, rectangles, circles, arcs; dimensions and constraints; construction geometry; projection, trim, offset; splines after the basic solver path works |
| Solids | Extrude, revolve, sweep, loft, primitives; join, cut, intersect |
| Modifications | Fillet, chamfer, shell, hole, mirror, patterns, move/copy |
| Surfaces | Extrude, revolve, sweep, loft, patch, offset, trim, stitch, thicken |
| History | Named parameters, edit feature/sketch, recompute, suppression, undo/redo; references that survive supported upstream edits |
| Inspection | Selection, visibility, measurement, sections, useful geometry/error diagnostics |
| Local documents | Save/open authored history and evaluated geometry; recovery; explicit units |
| Interchange | STEP and IGES CAD data; STL, OBJ, and 3MF mesh data; DXF/SVG sketch paths when suitable readers are integrated |
| Printing | Watertight mesh export with explicit units, refinement, and body selection; STL and 3MF |

Start with one complete modeling workflow: create a dimensioned sketch, extrude,
fillet, change the original dimension, recompute, save/reopen, and export a mesh.
An enclosure or bracket makes a useful first fixture. Surface creation and
imported-model editing follow as additional complete workflows.

## Import and export expectations

| Format | Proposed role | Important limit |
| --- | --- | --- |
| STEP | Import/export precise CAD geometry; assemblies where supported | Imported shapes do not ordinarily carry the original editable feature history |
| IGES | Import/export surfaces and supported geometry | Healing and tolerances require validation |
| STL | Mesh import and printing export | No authored feature history; units require an explicit policy |
| OBJ | Mesh import/export | Mesh bodies, not automatic parametric reconstruction |
| 3MF | Printing-oriented mesh import/export | Evaluate lib3mf for units, components, validation, and metadata |
| DXF | Primarily sketch interchange initially | Requires a suitable reader/writer; do not assume it is included in open-source OCCT |
| SVG | Sketch path import initially | Curves, transforms, scale, and supported subsets need a declared policy |

Native reference CAD application `.f3d`/`.f3z` interoperability is a separate research item. Do not
promise it for the first release or infer support from a file extension. Any
later work must respect the reference restrictions in AGENTS.md. Native Alloy
files must preserve Alloy's own editable document, not only a triangulation or
flattened STEP shape.

## Long-term coverage

These families remain in scope. They need their own public-source inventories and
workflow acceptance criteria before implementation.

- Assemblies, joints, motion, components, configurations, bills of materials,
  and engineering drawings.
- Sheet metal, unfolding, direct modeling, advanced surfaces, and freeform
  modeling.
- Manufacturing: tools, stock, setups, posts, simulation/verification, drilling,
  facing, pockets, contours, then broader milling and turning workflows.
- Simulation and engineering analysis, with appropriate solvers and clear
  distinctions between supported analyses.
- Electronics design and mechanical integration. KiCad exchange may be a useful
  early integration; it does not provide integrated feature parity by itself.
- Materials, rendering, animation, inspection, and other documented offline
  workflows.

Cloud-dependent features should be tagged individually as deferred. That
exception does not automatically defer every feature in a workspace that also
offers cloud services. The precise current inventory still needs research.

## Status and open decisions

The product direction, clean-room restriction, initial shape-creation focus,
familiarity goal, and built-in agent control are accepted user requirements.
Coverage ordering and libraries are recommendations. The user selected C++20
and the MIT license for Alloy's original source and documentation. macOS,
Windows and Linux are required. The exact dated reference release, minimum OS
versions, solver and native document format remain open. The local development
host is macOS; runtime qualification is required on every platform.

## Public references

Reference review date: 2026-10-11. Use these as public feature descriptions, not
runtime observations or proof of Alloy implementation.

- reference CAD application overview (reference `extra-080`) and
  feature families (reference `extra-081`).
- reference CAD application workspaces (reference `extra-078`).
- Supported file formats (reference `extra-082`).
- Mesh export tools (reference `meshexport`).
- Create surfaces from sketches (reference `surface`).
