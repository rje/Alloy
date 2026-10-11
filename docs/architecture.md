# Architecture and stack

The application language is C++20, selected by the user. The remaining architecture
is a proposal. Build and runtime instructions describe what
actually exists in [development.md](development.md); do not treat this document
as proof that modeling features have been implemented.

## Recommended foundation

The language choice reflects the CAD kernel and desktop ecosystem. Rust is
outside the planned stack.

| Layer | Recommendation | Reason and limit |
| --- | --- | --- |
| Desktop application | C++20 and Qt 6 Widgets | Mature menus, docks, trees, dialogs, actions, input, and model/view support; a useful fit for a dense CAD desktop interface |
| CAD geometry | OCCT | Established BRep solids/surfaces, modeling algorithms, CAD exchange, and visualization; robustness still needs workflow validation |
| CAD viewport | Evaluate OCCT AIS/V3d first | CAD selection and presentation already exist; avoid writing a game renderer before proving modeling workflows |
| Sketch constraints | Evaluate FreeCAD PlaneGCS; compare SolveSpace libslvs | Existing solvers reduce reinvention; functionality, embedding boundaries, maintenance, and license must be checked |
| Assembly constraints | Evaluate later, including FreeCAD's OndselSolver fork | A later family; do not put assembly infrastructure in the first sketch slice |
| 3MF | Evaluate lib3mf | Existing read/write and validation support |
| Automation | Local structured protocol; Python standard-library client initially | Easy agent access without embedding a scripting runtime before it is needed |
| Build | CMake; explicitly discovered dependencies | Conventional tooling for Qt/OCCT; record actual tested versions |

C++ lets Alloy integrate Qt and OCCT directly and use their established native
interfaces. This keeps the first modeling workflows focused on document,
geometry, and interaction work without maintaining an additional binding layer.

## Document and geometry boundaries

The durable document must retain authored intent: sketches, dimensions,
constraints, feature parameters, components, dependencies, units, and stable
identifiers. Evaluated OCCT shapes are results of that intent. Tessellated meshes
and viewport presentations are display/export data, not the editable document.

A small initial chain should suffice:

```text
UI / agent client
        |
application commands + transactions
        |
authored document -> feature evaluation -> OCCT shapes
        |                                    |
native persistence                    viewport / mesh export
```

Begin with the feature dependencies needed by the first workflow. Introduce
incremental recomputation when real edits justify it. Keep save/open and undo
consistent with the same document state rather than serializing UI widgets.

OCAF is worth evaluating for document persistence, transactions, and naming.
Its existence does not solve all persistent-reference problems automatically.
Choose the native storage format after a small round-trip prototype, with a
versioned, documented schema and a plan for migrations and recovery.

## Persistent references are an early risk

Fillets, projections, and downstream sketches depend on faces and edges that can
change when an upstream feature changes. Raw face/edge indices are insufficient.
Prototype supported reference tracking before building a large command catalog.

The first edit fixture should change a sketch dimension and demonstrate whether
the extrude and fillet update correctly. Add split/merge cases as coverage grows.
When a reference cannot be resolved, surface a meaningful failure; never silently
attach a feature to an arbitrary different face. Record which edits are supported.

## One command path for people and agents

UI actions and automation dispatch the same commands against the same state.
Each command needs an identifier, input schema, structured errors, completion
semantics, and enough observable state to validate its effect. Undoable modeling
actions should use the same transaction boundary regardless of their caller.

The first shell must support discovery, state inspection, real UI command
execution, and screenshot capture. Each later feature adds control and inspection
at the same time. Geometry summaries should eventually expose body count,
bounding boxes, volume/area where meaningful, topology validity, units, parameter
values, and diagnostics. [Agent control](agent-control.md) specifies the actual
initial transport and commands.

Keep the initial interface private and local. A plugin SDK, embedded Python,
network service, or generalized RPC framework is not needed to prove the first
workflow. Add those only for a concrete product requirement.

## Licensing and dependency decisions

The user selected the MIT license for Alloy's original source and documentation.
Record exact component versions and module licenses before integrating or distributing them.
In particular, Qt modules have differing terms, OCCT has its own LGPL exception,
and a GPL solver choice can affect the application's licensing requirements.
Public availability alone is not a reuse license.

Evaluate solver choices against the permissive project-license goal. Reusable
third-party code and independently authored application code
must retain appropriate notices. Reference screenshots belong in research
evidence, not bundled product assets.

## Public technical references

Reference review date: 2026-10-11. These support library evaluation; adoption is
not proof of functional coverage.

- [Qt dock widgets](https://doc.qt.io/qt-6/qdockwidget.html),
  [model/view programming](https://doc.qt.io/qt-6/model-view-programming.html),
  and [LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations).
- [OCCT technology](https://www.opencascade.com/occt3d-technology/),
  [OCAF guide](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__ocaf.html),
  [visualization guide](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html),
  and [exchange format availability](https://www.occt3d.com/data-exchange-formats/).
- [FreeCAD PlaneGCS](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Sketcher/App/planegcs/GCS.h),
  [SolveSpace library](https://solvespace.github.io/solvespace-web/library.html),
  and [FreeCAD OndselSolver](https://github.com/FreeCAD/OndselSolver).
- [lib3mf](https://github.com/3MFConsortium/lib3mf).
