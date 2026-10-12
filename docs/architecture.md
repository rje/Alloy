# Architecture and stack

The application language is C++20, selected by the user. Authored storage and
transaction authority are selected in [decision 001](decisions/001-authored-document.md);
other unimplemented layers below remain proposals. Build and runtime instructions describe what
actually exists in [development.md](development.md); do not treat this document
as proof that modeling features have been implemented.

## Recommended foundation

The language choice reflects the CAD kernel and desktop ecosystem. Rust is
outside the planned stack.

| Layer | Recommendation | Reason and limit |
| --- | --- | --- |
| Desktop application | C++20 and Qt 6 Widgets | Mature menus, docks, trees, dialogs, actions, input, and model/view support; a useful fit for a dense CAD desktop interface |
| CAD geometry | OCCT | Established BRep solids/surfaces, modeling algorithms, CAD exchange, and visualization; robustness still needs workflow validation |
| CAD viewport | Alloy CAD presentation on an established Metal/Vulkan RHI | Accepted destination in decision 003; QRhiWidget/bgfx shortlist is researched, native spike and migration remain open. Current prototype uses GPU OCCT AIS/V3d/OpenGL |
| Sketch constraints | PlaneGCS via a replaceable shared-library boundary | Integrated for line/point relations and driving/driven dimensions in decision 004; portable packaging and broader constraint classes remain pending |
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

Origin-plane sketch intent is now implemented within `alloy-document/1`. A sketch
is a typed feature with a stable UUID, root owner, XY/XZ/YZ plane, canonical-mm
points and line entities referencing stable endpoint IDs. Rectangles contain four
authored lines and four shared points. A sketch has no solid `bodyId` and does not
contribute to body count or volume. `SketchGeometry` supplies one plane/frame and
OCCT edge evaluation for both application previews and committed geometry.
New rectangles also author four horizontal/vertical constraints atomically.
Optional constraint/dimension arrays preserve compatibility with earlier files;
opening an unconstrained rectangle does not invent constraints for it. Supported
relations are coincidence, horizontal, vertical, parallel, perpendicular and
fixed points; linear/horizontal/vertical dimensions may drive geometry or measure
it. All records and their point/line references use stable IDs. Fixed points retain
their authored target coordinates; driven records persist no driving value.

`SketchSolver` maps a copied candidate to transient PlaneGCS storage and returns
normalized point coordinates plus status, whole-sketch DOF, conflict/redundancy
IDs, residuals and measured dimensions. Horizontal/vertical readouts are positive;
their equation branch follows the signed endpoint difference in the seed.
`SketchGeometry` accepts the solve before replacing a draft or evaluating its
OCCT edges. A rejected solve retains the last usable draft. `CadDocument` validates
and normalizes every sketch before installing a commit or opened document, so a
failed candidate cannot replace live authored state, geometry or journal entries.
`SketchProfiles` derives one connected closed line loop into a validated planar
OCCT face. Stable profile identity combines the sketch ID and source entity IDs;
summary provenance and area are rebuilt caches. Authored intent remains points/
lines/relations, with optional dimension annotation coordinates. Moving a label
validates the candidate while preserving exact accepted point records. Multiple
regions, holes and curved boundaries remain later sketch work.

`CadDocument` keeps authored intent paired with an immutable `EvaluatedDocument`
containing separate body and sketch caches. The active sketch session owns a
disposable authored draft. Finish validates/evaluates the complete candidate and
commits once through the existing journal; Cancel discards the draft. Existing
sketches re-enter through the same commands. Native save persists authored points,
entities, constraints, dimensions, solved coordinates and IDs; reopen rebuilds
solver state and edges. See the [sketch receipt](validation/sketches.md) and
[constraints receipt](validation/sketch-constraints.md).

OCAF is worth evaluating for document persistence, transactions, and naming.
Its existence does not solve all persistent-reference problems automatically.
The permitted round-trip prototype selected readable authored JSON and one Qt
undo authority, rather than an OCAF file container. See decision 001 for schema,
migration and failure boundaries. OCAF naming remains a separate evaluation.

## Operation history and undo/redo

The user requires these architectural questions to be settled before modeling
operations accumulate. FND-003 is the initial decision/prototype task; DOC-002
depends on it. Decision 001 selects QUndoStack as the sole editing journal;
DOC-002/003/004 and SKT-001/002/003/004 now integrate it for primitives and sketches.
HIS-001/SOL-001 extend this authority to ordered sketch/profile extrusion.

The authored feature history describes how the model is constructed: stable
operation IDs, typed parameters and units, input references, dependencies and
evaluation order. The undo history records editing transactions applied to that
document. Editing an earlier feature changes the authored graph; it does not
mean replaying a list of mouse actions. Evaluated OCCT shapes and display meshes
are revision-associated results, and cannot be the sole durable source of intent.

One document transaction authority must serve both UI and automation. A committed
create, edit, delete or reorder produces one intentional undo entry. Interactive
previews remain disposable until commit; cancel and failed validation leave no
partial document change or undo entry. Undo restores authored values, IDs and
references; redo restores the corresponding committed edit. A new edit after
undo discards the abandoned redo branch. Multi-step gestures need deliberate
grouping rather than one entry per preview update.

Recompute must validate dependencies and preserve explicit per-feature errors.
Retained last-valid geometry must be marked stale when current intent fails,
and downstream features cannot silently consume it as a successful new result.
Long-running evaluation must reject results for an obsolete document revision.
Saved-state tracking belongs to authored state: undoing to the saved checkpoint
can clear the dirty flag. Whether session undo history survives reopening is a
separate decision from persistence of the editable feature history.

The implemented document transaction holds paired authored JSON and immutable
evaluated body/sketch snapshots. It evaluates and validates a candidate before pushing
one QUndoStack command. A rejected candidate leaves live state untouched;
undo/redo installs the corresponding authored/geometry pair and advances the
document revision. This keeps the first synchronous workflow small. Native
QSaveFile writes authored JSON atomically; open validates/evaluates before
replacing state and starts a clean, empty session journal. AIS presentations
are regenerated from evaluated bodies and sketches and are never persisted as intent.

FND-003 compared a thin authored schema with OCAF using the same small
primitive/edit fixture, investigated OCAF's transaction facilities and Qt's
QUndoStack grouping/clean-state support and selected one authority without two
independently advancing stacks. The first real-app primitive workflow verifies
create/edit/undo/redo, branching after undo, cancel/invalid edits, stable IDs,
save/reopen, unknown schema versions and atomic-write failure. Broader gesture
grouping and dependent feature graphs follow the workflows that need them.
FND-004 covers persistent geometric references alongside this decision. Larger
recompute scheduling follows broader feature dependencies in HIS-004.

The first dependent chain now uses one synchronous full ordered evaluation pass.
An authored extrusion stores sketch ID, derived profile ID and signed distance;
its feature/body IDs are allocated once and retained across edits. Each extrusion
resolves its exact profile from a freshly evaluated preceding sketch. Missing,
forward, self and invalid-profile dependencies produce contextual feature errors;
there is no arbitrary profile fallback or successful stale cache. Candidate
normalization/evaluation is shared by preview, commit and open. Only a successful
complete candidate can replace the live paired state. The profile-to-solid
boundary uses a real OCCT prism along the sketch frame normal and validates the
resulting closed solid and measurements. Incremental propagation, named
expressions, general graph editing and face-reference resolution remain separate
work. See [the extrusion receipt](validation/sketch-extrusion.md).

Public library references reviewed 2026-10-11:
[OCAF](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__ocaf.html)
and [Qt QUndoStack](https://doc.qt.io/qt-6/qundostack.html). These are living
guides; prototype conclusions must identify the actual pinned library versions.

## Persistent references are an early risk

Fillets, projections, and downstream sketches depend on faces and edges that can
change when an upstream feature changes. Raw face/edge indices are insufficient.
Prototype supported reference tracking before building a large command catalog.
The permitted primitive/fillet/split/deletion/merge fixture passed; see
[decision 002](decisions/002-geometric-references.md). The application resolver
remains DOC-012, with no raw-index or nearest-edge fallback.

The first integrated edit fixture changes a sketch dimension and verifies the
extrusion rebuilds with stable profile/feature/body IDs. Fillet references and
split/merge integration remain later coverage.
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

[Decision 004](decisions/004-sketch-solver.md) selects PlaneGCS through a replaceable
shared-library boundary, with pinned dependencies and passing native fixtures.
Its LGPL-2.1-or-later source retains its terms; Alloy's original adapter remains MIT.
The verified extraction builds as `alloy_planegcs`, separate from the application,
with Eigen 5.0.1 (`EIGEN_MPL2_ONLY`) and Boost 1.90.0 headers. Source hashes, notices
and standalone shims are recorded in the dependency inventory. Other-platform
solver builds and complete runtime/source-access/replacement distribution remain
unqualified.

The active sketch palette and API share point/line selection. Selected geometry
is orange, fixed points green, and the sketch is black at zero DOF or blue while
underconstrained. The black/blue indication is whole-sketch state, not a
per-entity freedom analysis. Dimension labels are derived display data; canvas
placement, double-click editing, native point dragging and per-entity badges are
still [specific compatibility follow-ups](research/commands/sketch-controls.md).

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

[Decision 003](decisions/003-renderer-direction.md) records the accepted native
graphics destination and its implementation/evidence boundaries. The [RHI shortlist](research/rhi-candidates.md)
recommends a bounded QRhiWidget spike before selecting a production backend;
no native Metal/Vulkan migration is implemented. Qt QDockWidget is used for
modeless command panels and the Sketch Palette; authored command sessions remain
their authority.
