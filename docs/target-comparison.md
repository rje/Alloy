# Target comparison and earlier research

This preserves the standalone Maya, Houdini, and reference CAD application paths considered before
the project chose reference CAD application. The comparisons are engineering recommendations, not
completed prototypes or measured estimates. Reference review date: 2026-10-11.
Rust comparisons below preserve the earlier alternate-target research. Alloy's
selected application language is C++20; Rust is outside its planned stack.

## Maya path

A useful initial target would be a character rig and a short animated performance:
scene hierarchy, transforms, polygon objects, joints, skinning, FK/IK,
constraints, animation curves, timeline, and curve editing. That produces a
coherent workflow while exercising the scene, evaluation, editing, and viewport.

Keep scene ownership and dependency evaluation distinct. Later coverage includes
modeling, UVs, deformers, blend shapes, NURBS, references, grooming, dynamics,
materials, lighting, rendering, and the wider authoring toolset. A scene graph
or renderer alone is not a Maya replacement.

C++ and Qt would be a strong foundation, with OpenUSD for interchange where
appropriate, OpenSubdiv for subdivision, and established shading/color libraries.
Rust can participate through coarse native boundaries, but animation and DCC
integration libraries still make a hybrid approach practical.

Public starting points:
[Maya features](https://www.autodesk.com/ca-en/products/maya/features),
[Maya API overview](https://aps.autodesk.com/developer/overview/maya-api), and
[public C++ API documentation](https://help.autodesk.com/cloudhelp/2025/ENU/MAYA-API-REF/cpp_ref/index.html).
These references do not authorize runtime inspection under Alloy's rules.

## Houdini path

A useful first target would be a procedural geometry network that makes a
reusable building or terrain asset. It needs typed attributes on points,
vertices/face corners, primitives, and detail; generators and transforms;
merge/group operations; extrusion, curves, resampling, scattering, instancing;
parameter controls; a geometry spreadsheet; caching; and export.

That architecture can suit a Rust-led application with mature C++ geometry
libraries behind bulk-operation boundaries. Separate stateless geometry
evaluation from time-dependent simulation. A native expression/attribute
language comparable in role to VEX is a substantial later subsystem, not merely
embedding Python.

Grow into robust topology tools, volumes, terrain, larger instance sets, reusable
assets, profiling, then particles/rigids and wider simulation workflows. Cloth,
fluids, pyro, crowds, hair, procedural animation, USD workflows, compositing, and
batch processing each require substantial integration. A node editor with sample
operators is not broad functional parity.

Public starting points:
[Houdini documentation](https://www.sidefx.com/docs/houdini/index.html),
[SOP nodes](https://www.sidefx.com/docs/houdini/nodes/sop/),
[geometry architecture](https://www.sidefx.com/docs/hdk/_h_d_k__geometry__intro.html),
and [VEX documentation](https://www.sidefx.com/docs/houdini/vex/index.html).

## Why reference CAD application is the selected path

The user selected shape creation, common imports, and printing as the initial
focus, while retaining the long-term full replacement goal. reference CAD application provides a
clear sequence from sketch to feature history to editable geometry and export.
OCCT, Qt, existing sketch solvers, and printing libraries provide a mature
foundation, and C++20 is now the selected main application language.

FreeCAD is useful evidence that open-source parametric CAD can cover many related
families. Reusing suitable publicly licensed libraries is permitted; copying a
different application's interface is not a substitute for researching reference CAD application's
public specification. [FreeCAD features](https://www.freecad.org/features.php)
and [source repository](https://github.com/FreeCAD/FreeCAD) are starting points
for library evaluation.

## Shared technology cautions

- [OpenUSD](https://openusd.org/release/intro.html) is valuable interchange and
  composition infrastructure. It also includes
  [OpenExec](https://openusd.org/release/intro_to_openexec.html), an evaluation
  framework; do not describe it as lacking computation entirely or assume it
  supplies a finished rigging system. Alloy's native CAD history needs its own
  authored model; STEP is a more immediate exchange priority.
- Triangle geometry libraries such as
  [Manifold](https://github.com/elalish/manifold) can complement a mesh workflow;
  they do not replace a BRep kernel for parametric solids and surfaces.
- [libigl](https://libigl.github.io/license/) and
  [CGAL](https://www.cgal.org/license.html) have module-specific licensing
  considerations. Evaluate the actual packages selected.
- [OpenVDB](https://github.com/AcademySoftwareFoundation/openvdb),
  [Rapier](https://rapier.rs/docs/), and
  [PositionBasedDynamics](https://github.com/InteractiveComputerGraphics/PositionBasedDynamics)
  provide useful foundations for relevant VFX paths, not entire finished artist
  workflows.
- [Gaffer](https://github.com/gafferHQ/gaffer) and
  [Zeno](https://github.com/zenustech/zeno) are public projects worth studying for
  procedural DCC architecture within their licenses.
- [OpenCAMLib](https://github.com/aewallin/opencamlib) is a manufacturing
  algorithm candidate; a complete CAM system also needs setups, tool libraries,
  stock, posts, operation editing, and verification.

No alternate-target implementation was started as part of this research. The
Alloy project and its rules now govern further work.
