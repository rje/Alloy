# Alloy

Alloy is a standalone, open source, modifiable parametric CAD application.
The long-term goal is complete functional coverage of professional CAD workflows,
with familiar commands, keyboard controls, and layouts. Features that currently
require cloud access may be deferred.

This is a clean-room implementation based on public documentation, instructions,
and screenshots. We do not run or inspect proprietary reference applications. Read [AGENTS.md](AGENTS.md)
before researching, designing, or implementing anything in this project.

The first product focus is shape creation: sketches, solids, surfaces, editable
history, common-format import, and export for 3D printing. The current application
creates and edits real OCCT boxes and cylinders, and supports origin-plane
sketches with connected lines, rectangles, geometric constraints and driving/driven
dimensions. Placed dimensions have witness lines/arrows, an adjacent numeric editor
and draggable labels; solved closed line loops produce selectable real profiles.
Finished profiles remain selectable and support editable one-side New Body,
Cut and successive Join extrusions with retained connected result bodies. Upstream sketch edits rebuild their solids through the same history.
GPU rendering, shared undo/redo and native save/reopen cover this chain.
Analytic circles/arcs, curve dimensions, editable Combine Join/Cut/Intersect,
independent design tabs, draft undo, construction lines, midpoint/tangent circle
inference and paired sketch fillets are implemented. The illustrated first-activity
outline has a complete native modeling receipt. Rounded sketches now produce
three real selectable regions with stable IDs through dimension edits and storage.
Connected multi-region Press Pull now produces one editable solid; sketches can
attach to positive extrusion end caps and follow parent edits. General face support,
curved model-edge references and broader imports/exports remain ahead. Straight
cap-edge references support retained Parallel, native endpoint snaps and dimensions.
Rounded cap centers now anchor fully constrained native construction guides.
Circle centers can now use retained point-to-straight-edge driving dimensions;
[the bounded candidate receipt](docs/validation/point-edge-dimensions.md) records
its direct D/center/edge gesture and upstream hole-placement evidence.
Inline diameter entry and Equal create five attached circles; native arrow-driven
Cut produces five holes in one retained solid. Earlier sketch edits, history and
reopen preserve that chain. See [the five-circle Cut receipt](docs/validation/five-circle-cut.md)
and [the complete bracket workflow receipt](docs/validation/illustrated-bracket.md).
The blank-document API chain passes; a separate native walkthrough reaches the
same five-hole solid with explicitly recorded selection and visual limitations.
Bounded line Trim now removes intervals at line/circle intersections with
retained constraints, undo and storage. Overlapping lines and circles form real
selectable regions, including coincident strip boundaries; the next illustrated
support sketch passes [independent native checks](docs/validation/support-independent-review.md).
Successive Join and constant-radius solid-edge Fillet have
[independent native acceptance](docs/validation/support-solid-independent-review.md),
including stable history, readable edge selection and retained disk-cap support.
The [support tutorial](docs/validation/support-tutorial-independent.md) now passes
a cumulative functional video-variant gate through an angled cutter, Combine Cut
and a closed millimeter STL. Its native movie is a labeled continuation; a separate 78-second movie shows
the complete blank-model workflow through the shared API with command overlays.
This is not complete CAD parity.
Annotated real-app tutorial movies can now be recorded with local commands; see
[the video recipe and verified results](docs/validation/tutorial-video.md).
The overnight tutorials now include independently accepted
[nested cubes](docs/validation/nested-cubes-independent.md) and a
[lofted egg cup](docs/validation/egg-cup-independent.md), with full creation
recordings, editable models and independently checked STL files. These exercise
inward Shell, body Mirror, offset planes, centered rectangles, retained planar
face sketches and ordered circular Loft sections. The full
[toy block](docs/validation/toy-block-independent.md) also passes feature patterns,
Shell roof references, retained circle Offset and nineteen-edge rounding. The
[die tutorial](docs/validation/die-independent.md) adds named local component
ownership, Chamfer trials, face-positioned spherical cuts and Repeat, with a
complete creation movie and checked component STL.
[Point-to-edge dimensions](docs/validation/point-edge-dimensions-independent.md)
now retain circle offsets when model edges move, with a verified Dimension
selection/placement workflow. The [Timeline](docs/validation/timeline-overflow.md)
keeps fixed-size feature targets and scrolls when history grows.
Acceptance is bounded to the
documented workflows; exact visual and physical-input parity remains qualified.
The next illustrated object now passes an integrated API chain: command-first
Circle, a joined boss, a sketch on its new cap, an editable body Move and Combine
Cut. The [straw-bung checkpoint](docs/validation/straw-bung.md) includes a closed
26 x 26 x 11 mm STL and separate native/visual evidence limits.
The completed bracket can be exported as one valid STEP solid or a closed STL
mesh. A full-size design and millimeter STEP/STL plus a labeled 0.2-scale STL
print sample have [independent readback evidence](docs/validation/bracket-exchange.md).

The public repository is [rje/Alloy](https://github.com/rje/Alloy), with the
[issue backlog](https://github.com/rje/Alloy/issues) and
[Alloy project board](https://github.com/users/rje/projects/1).

The active delivery order is [tutorial-first checkpoints](docs/backlog/tutorial-sequence.json).
Current evidence includes [constraint tools](docs/validation/constraint-tools.md),
[angular sketches](docs/validation/angular-sketch.md), the
[first illustrated activity](docs/validation/bracket-activity1.md),
[document tabs](docs/validation/document-tabs.md),
[construction lines](docs/validation/sketch-construction.md),
[tangent circles](docs/validation/sketch-tangent.md),
[paired fillets](docs/validation/sketch-fillet.md) and
[retained regions](docs/validation/sketch-regions.md) and
[multi-profile extrusion](docs/validation/multi-profile-extrusion.md).
Native fillet history qualification uses an isolated
[Qt accessibility patch experiment](docs/dependencies/qt-cocoa-accessibility.md);
default deployment qualification remains open. Exact physical-input tutorial
parity remains separate from the accepted functional gates above.

## Project documents

- [Continuation context](docs/context.md): decisions from this conversation,
  current implementation, evidence, and where to resume.
- [Work backlog](docs/backlog/README.md): researched tasks, dependencies,
  cumulative validation gates, and the live pull queue.
- [Deep-dive research](docs/research/cad-workflows.md): public workflow
  findings behind the backlog, with a dated source registry.
- [External workflow knowledge](docs/knowledge/README.md): search instructions for
  source-linked behavior records and visual references stored outside the checkout.
- [Product scope](docs/product-scope.md): accepted goals, initial coverage, and
  the longer-term feature families.
- [Architecture](docs/architecture.md): recommended stack, document model,
  geometry boundaries, and unresolved choices.
- [Roadmap](docs/roadmap.md): workflow milestones and acceptance criteria.
- [Compatibility](docs/compatibility.md): how we track familiar behavior and
  separate research evidence from implementation claims.
- [Target comparison](docs/target-comparison.md): animation, procedural content,
  and parametric CAD paths considered before selecting CAD.
- [Public interface research](docs/research/interface-reference.md): Astra's
  source-linked interface and interaction findings.
- [Development](docs/development.md): build, run, and verification instructions.
- [Agent control](docs/agent-control.md): local command protocol and client.
- [Native builds and releases](docs/releases.md): required OS targets, CI,
  runtime packaging, and milestone promotion gates.

## Starting decisions

C++20 is the selected application language. Qt 6 and Open CASCADE Technology
(OCCT) are the recommended foundation.
Python is the initial automation client. Authored JSON and one Qt undo authority
are selected in [decision 001](docs/decisions/001-authored-document.md).
[Dependency admission](docs/dependencies/README.md) covers the shared Qt/OCCT
foundation; [PlaneGCS solver selection](docs/decisions/004-sketch-solver.md) is
integrated through a separate shared library for the supported sketch relations.
Complete runtime distribution and other-platform solver qualification remain open.
Rust is outside the planned stack.

Alloy's original source and documentation use the [MIT license](LICENSE).
Third-party libraries and public reference material retain their own terms.

The 3D viewport uses GPU-accelerated OCCT OpenGL. Live Apple M4 diagnostics,
Retina resize, native window display, and framebuffer capture were verified in
the [GPU foundation receipt](docs/validation/gpu-foundation.md). The current
[post-cleanup shell receipt](docs/validation/neutral-shell.md) records the rebuilt
style and real-app test. The [primitive workflow receipt](docs/validation/primitives.md)
records the first editable solids and their real-app regression. macOS, Windows, and Linux are required
targets; only the macOS runtime is verified today.
The [viewport input receipt](docs/validation/viewport-input.md) records the later
camera/redraw and native-coordinate fixes. The later [Mac editing receipt](docs/validation/mac-editing.md)
records fractional Retina picking, blue selection and the Qt docked editor.
The [sketch receipt](docs/validation/sketches.md) records origin-plane drawing,
one-transaction Finish/Cancel, mixed-document undo and native reopen, alongside
the previous workflows on the same candidate.
The [constraints receipt](docs/validation/sketch-constraints.md) records the later
line/point selection, rectangle constraints, driving width/height, driven
measurement, failed-solve preservation and storage/history checks. This supports
the declared subset, rather than full sketch interaction parity. The later
[annotation/profile receipt](docs/validation/sketch-annotations.md) records complete
line dimensions, native inline editing, exact placement-only dragging and actual
closed-face derivation/picking. The [extrusion receipt](docs/validation/sketch-extrusion.md)
records the first finished-profile → solid → upstream width edit workflow, including
native selection, E, distance typing, label editing and undo/redo. That candidate
uses capability-selected 4× MSAA on Apple M4, dark edges and fixed world-space
lighting for distinct shaded face tones; other-platform runtime remains unqualified.

For a first hands-on test, build using [development.md](docs/development.md),
open [examples/gauge-block.alloy](examples/gauge-block.alloy) through File → Open,
and click a Timeline feature to edit its dimensions. Box and Cylinder open
a dockable side panel with field-driven disposable Preview and Cancel. Try undo/redo and Save As;
the [sample instructions](examples/README.md) describe the supported controls.
Open [examples/rectangle-sketch.alloy](examples/rectangle-sketch.alloy) to inspect
and re-enter an independently authored 40 × 25 mm sketch. Create Sketch opens an
origin-plane chooser; Line and Rectangle accept canvas clicks or explicit numeric
coordinates. New rectangles automatically receive four horizontal/vertical
constraints. Open [examples/dimensioned-rectangle.alloy](examples/dimensioned-rectangle.alloy)
for an anchored 50 × 25 mm rectangle with editable driving width/height and a
driven diagonal. Select points or lines to apply coincidence, horizontal,
vertical, parallel, perpendicular or fixed relations. The palette adds linear,
horizontal and vertical driving dimensions or driven measurements; Finish commits
the entire sketch once. Select an edge, press D, click to place its dimension,
then type a value and press Enter. Double-click driving labels to edit, drag
labels to reposition, and press Escape to abandon pending edits. Closed-profile
interiors are filled and selectable both during editing and after Finish.
Open [examples/sketch-extrusion.alloy](examples/sketch-extrusion.alloy) to edit a
rectangle and its dependent 5 mm solid. After Finish, select a visible region and
press E (or Extrude). The adjacent dock supports Profile Plane / One Side /
Distance / New Body, with an opaque shaded preview; Home shows its depth.
OK creates one feature and hides the consumed sketch. Browser checkboxes show it
again; Cancel preserves the document. Signed distance reverses the plane normal.
The API supports mm/in conversion; extrusion UI unit suffix entry, a Flip button,
interactive handles and other extent/operation modes remain ahead.
Inline rectangle entry, snapping and per-entity constraint diagnostics remain
[tracked interaction gaps](docs/research/commands/sketch-controls.md).

To list tasks whose dependencies are complete:

```sh
python3 tools/backlog.py ready
```

GitHub operations use `gh` and documented APIs.

Alloy is the working project name. Public naming and trademark clearance have
not been performed. There is no affiliation with Autodesk.
