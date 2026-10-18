# Alloy

Current direction: [printable-part v1 plan](https://alloy-v1-review.rjevans1.chatgpt.site) and [versioned inventory](docs/v1/README.md). The user reviewed the design as a reasonable working direction; individual gaps still require bounded completion and verification. The corrected iron tutorial is accepted; the full enclosure remains open.

[Core 3MF export](docs/validation/3mf-export.md) now saves separate placed
millimeter solids through Save As Mesh, with independent readback and a separate
slicer check. [Modeled external threads](docs/validation/modeled-external-thread.md)
now have a retained native/control workflow and independently checked printable
STL; broader thread modes and print fit remain open.
[Regular sketch polygons](docs/validation/sketch-regular-polygons.md) now have
three retained creation modes, editable size/count, and a verified extrusion
and STL path. [Canvas size annotations](docs/validation/sketch-polygon-annotations.md)
are editable for all three modes. [First-point placement relations](docs/validation/sketch-polygon-first-anchor.md)
now follow an upstream sketch point or midpoint. [Second-point relations](docs/validation/sketch-polygon-second-driver.md)
also drive Polygon size and rotation through source edits, with conflicting
typed values refused.
[Three-Point Rectangle](docs/validation/sketch-three-point-rectangle.md) now
creates an oblique editable profile through three canvas picks, with optional
typed lengths, source following and a verified extrusion/STL path.

Current priority: [initial-build readiness](docs/initial-build-readiness.md)—
familiar navigation, reliable everyday controls, recovery and installable packages,
alongside completion of the advanced modeling tutorials.

The [Surface Extrude responsiveness gate](docs/validation/surface-extrude-responsive-preview.md)
now keeps the corrected iron's long edit previews off the UI thread, with
independently checked navigation, status, geometry, history and reopen evidence.
The [Boundary Fill commit gate](docs/validation/boundary-fill-responsive-commit.md)
also keeps the corrected iron's final solid evaluation responsive. Initial
[cell enumeration](docs/validation/boundary-fill-responsive-cells.md) now runs
on the same worker. [Interactive native Open](docs/validation/responsive-native-open.md)
also evaluates large models without freezing the window; its final presentation
has a [bounded optimization](docs/validation/open-final-presentation.md) and
still has a measured roughly 0.4 s pause.
[Historical Sketch Edit](docs/validation/responsive-historical-sketch.md) now
evaluates the saved iron prefix off-thread while status and navigation remain
responsive. Its changed-sketch
[Finish checkpoint](docs/validation/responsive-historical-sketch-finish.md)
now passes exact geometry, focus, history and reopen checks.
[Direct sketch source edits](docs/validation/responsive-sketch-source-edit.md)
also evaluate off-thread: the accepted iron edit preserved its exact solid,
references and history while navigation and status stayed responsive.
[Surface Trim panel partitions and commits](docs/validation/responsive-surface-trim.md)
now keep the measured corrected-iron paths responsive, with exact committed
geometry and history. [Mesh-independent preview](docs/validation/mesh-independent-surface-trim.md)
now shows the same regions as commit. The
[interactive target click](docs/validation/responsive-interactive-trim.md)
also partitions off-thread while preserving the original region through navigation.
[Inner handle Loft preview](docs/validation/responsive-inner-loft.md) now
evaluates off-thread, and OK commits the prepared result without repeating the
kernel evaluation. Guided and component Loft paths remain separate work.
[Single-solid millimeter STL export](docs/validation/responsive-stl-export.md)
now runs on a worker and keeps the saved corrected iron responsive; the complete
[responsive corrected-iron creation replay](docs/validation/iron-responsive-cumulative.md)
also passes its cumulative frame-cadence, geometry, export and history gate.

The [complete surface shoehorn](docs/validation/shoehorn-complete.md) now has
an independently accepted full creation movie, editable model and checked STL:
guided surface, exact corner/perimeter rounds and retained hanging hole. Source
length edits, history and reopening pass. Physical Browser gestures remain a
separate gate. The [corrected surface iron](docs/validation/iron-corrected-cumulative.md)
now has an independently accepted 44-stage full creation movie, editable model
and checked STL/STEP. The earlier adapted recording remains marked withdrawn
after the user rejected its shape; see [reassessment](docs/v1/iron-reassessment.md).

The [bilateral surface exterior](docs/validation/surface-bilateral-rail-loft.md)
now supports the iron tutorial's retained mirrored outline, guide rail and two
trimmed handle openings. The [retained handle bridge](docs/validation/surface-handle-bridge.md)
now passes source edits, native selection and reopening. The
[curved sheet Trim trials](docs/validation/surface-curved-trim.md) also pass
sequential cuts, tool Remove and exact undo. The new
[corrected iron shape](docs/validation/iron-corrected-shape.md) passes independent
visual, source-edit, solid, history and export review. Its cumulative creation
recording and replacement gallery entry are now accepted under
[the corrected cumulative receipt](docs/validation/iron-corrected-cumulative.md).

[Open fit-spline Mirror](docs/validation/fit-spline-mirror.md) now retains
reflected points and tangent handles through source/axis edits and reopening.
Native controls and the shared API are independently verified.

The [development preview guide](docs/development-preview.md) lists supported
workflows, local package/start instructions, example models and current limits.

See the [navigation guide](docs/navigation.md) for mouse controls, no-middle-button
tools, projection, saved Home and custom views.

The [local macOS arm64 package](docs/validation/macos-relocatable-preview.md)
passes distinct-path extraction, bundled runtime and GPU/API checks on the
development Mac. Clean-host and other-platform qualification remain open.

Navigation now includes [free orbit](docs/validation/free-orbit-navigation.md),
[constrained orbit and pivots](docs/validation/constrained-orbit-pivot.md),
[cursor-anchored zoom and gesture routing](docs/validation/trackpad-navigation.md),
and [orientation-cube clicks](docs/validation/orientation-cube-regions.md) and
[dragging](docs/validation/orientation-cube-drag.md),
[cube accessories](docs/validation/orientation-cube-accessories.md), plus distinct
[Fit and Home](docs/validation/navigation-fit.md) controls. These have bounded Qt/GPU
validation; physical trackpad and broader platform input remain open gates.
Visible [Pan, Zoom and Zoom Window](docs/validation/navigation-pan-zoom-window.md)
and Orbit tools support users without a middle mouse button.
[Fit Selection and planar Look At](docs/validation/navigation-selection-look-at.md)
frame bodies, supported faces and visible sketch geometry.
The [Camera menu](docs/validation/navigation-projection.md) provides orthographic,
perspective and hybrid projection, with per-tab restoration.
[Custom named views](docs/validation/navigation-named-views.md) can be saved,
renamed and restored from the Browser and survive document save/reopen.

The [retained enclosure inset](docs/validation/trimmed-seam-inset.md) preserves
its profile and downstream Join/Fillet references through source edits and
reopening. Exact paired R0.5 [rim rounding](docs/validation/paired-ridge-fillet.md)
now survives source edits, history and reopen. The
[final-stage enclosure recording](docs/validation/enclosure-rim-fit-stage.md)
and both halves’ separate STL/STEP exports have passed independent review.
[Reference-clearance inspection](docs/validation/enclosure-reference-clearance.md)
found a small connector collision in both halves. The user-approved
[clearance adaptation](docs/validation/enclosure-connector-clearance.md)
now measures 0.5 mm to that unchanged nominal connector from each half, with
zero positive overlap across all 24 reference/half pairs and checked separate
exports. Source dimension parity, physical fit and the full enclosure tutorial
remain open.

[Straight-line Extend](docs/validation/sketch-line-extend.md) now previews and
closes sketch gaps while preserving endpoint identity and constraints. Its
focused workflow closes a rectangle and creates a verified solid.

[Extrude Distance formulas](docs/validation/extrude-distance-expression.md) now
retain arithmetic such as `Width*2` through parameter edits, undo and reopening.
The native field supports formula preview and editing with explicit length units.

The [tangent-arc construction walkthrough](docs/validation/base-sketch-construction.md)
now passes from blank through circle contacts, one-junction R10 Fillet and a
hold-drag tangent Arc. Its 64-second Qt-input movie includes a circle-size edit
and exact save/reopen. The [dimension continuation](docs/validation/base-sketch-dimensions.md)
now passes the eight-Tangent audit and all source dimensions with native input.
The [first solid continuation](docs/validation/base-sketch-first-solid.md) passes
three-region selection, 48.25 mm extrusion, Cancel, editing and save/reopen.
The [face-hole continuation](docs/validation/base-sketch-face-holes.md) now passes
referenced Ø10/8/8 circles and native Cut/All/Flip with three verified through-holes.

The [Center to Center Slot](docs/validation/center-to-center-slot.md) now follows
the two-cap-center creation route, with retained snaps and separately placed
length/radius dimensions. [Direct dimension formulas](docs/validation/sketch-dimension-expression.md) now
retain expressions such as `d3 + 50` through source edits and reopening.
The [complete sketch walkthrough](docs/validation/sketch-expression-tutorial.md)
now passes from blank through direct formula editing, undo/redo and reopen.
Its 1m50s Qt-input recording shows automatic line alignment and readable formula
entry, with the menu-opening adaptation documented.

The [piston component fixture](docs/validation/piston-components.md) now creates
four editable native parts, converts them to components and nests the piston
parts beneath a parent. Source edits, history, reopen and subtree STL export
pass. A [retained Revolute and root Pin](docs/validation/as-built-revolute.md) now
support circular-edge selection, signed angle preview and history/reopen. The
[Cylindrical joint](docs/validation/as-built-cylindrical.md) adds a
retained nested piston origin with exact rotation and slide preview. The
[coupled piston linkage](docs/validation/piston-coupled.md) now follows the arm
through three Revolute joints, one Cylindrical joint and Pin, with bounded native
drag/Revert evidence and a full creation recording. [Linked local copies](docs/validation/linked-local-occurrences.md)
now share source geometry with distinct placement and selection, surviving source
edits and reopen. [Copied piston motion](docs/validation/copied-piston.md) now
retains the internal joint and solves both pistons through one hinge driver.
[Placed STEP/STL export](docs/validation/linked-instance-export.md) preserves
all six instances or one selected piston subtree. The independently accepted
5m52s cumulative API movie includes creation, linked motion and placed exports.
[Native Browser organization](docs/validation/piston-components.md) and
[joint-origin hover locking](docs/validation/joint-origin-inference.md) are also
accepted, along with [Paste drag and contextual Revert](docs/validation/linked-local-occurrences.md).
[Shared As-Built Type and component picks](docs/validation/as-built-native-selection.md)
are accepted, including real component-label clicks. The [cumulative native
piston walkthrough](docs/validation/piston-native-workflow.md) now passes the
source-order assembly, linked copy, coupled motion and exact reopen, with an
independently accepted62-second film. These are bounded
adapted-fixture capabilities, not complete native tutorial parity.

The [Surface workspace](docs/validation/surface-workspace-navigation.md) now exposes
Extrude, guided Loft, [paired-sketch Trim](docs/validation/surface-trim.md)
and retained Remove through shared native/API controls.
[Local image canvases](docs/validation/local-reference-canvas.md) support placement,
two-point calibration and portable embedded saving for sketch reference. The
[three-section no-rail Loft](docs/validation/surface-closed-loft.md) now retains
closed spline/ellipse sections through source edits and reopening. These are
accepted ingredients used by the corrected complete surface iron workflow.

[Guided surface Loft](docs/validation/surface-loft.md) and
[retained Thicken](docs/validation/surface-thicken.md) now create an editable
solid from a dimensioned spline and endpoint sections. The original sheet stays
available, and upstream edits, history, reopen and solid STL checks pass.
[Component printing](docs/validation/component-mesh-visibility.md) exports the
visible solid while retaining its hidden source sheet. The complete shoehorn's finishing, retained hole and checked export are accepted.

The [enclosure ports and card-opening workflow](docs/validation/enclosure-port-card-workflow.md)
now has an independently accepted full creation movie: linked board placement,
rounded shell, mounting pegs, projected connector clearances, long slot and
rounded card opening. Save/reopen and Case-only STEP/STL checks pass. The
adapted nominal board is not a physical-fit certification. The [split-halves and snap checkpoint](docs/validation/enclosure-snaps-workflow.md)
now adds mirrored catches, reordered rounding, separate closed printing meshes
and a full creation recording. [Retained section support](docs/validation/section-split-support.md) now follows
the Split halves through edits. The alignment rim and native naming remain
separate follow-ups.

The [patterned ornament tutorial](docs/validation/ornament-tutorial-acceptance.md)
now has independently accepted creation footage, editable source and width-edit
models, and checked printing exports. It combines Revolve, Shell, section analysis,
exact spline Offset, Intersect, circular body Pattern, Join and a transverse bore.
The receipt distinguishes API workflow evidence from native-input and visual limits.

The [sliding-lid tutorial](docs/validation/sliding-lid-cumulative.md) is now
independently accepted: constrained dovetail sketches, retained projections,
clearance Offset, To Object extrusion, named parameter resizing, thumb relief,
and a limited Slider opening through the front. It has a full creation movie,
a labelled close-up replay, editable assembly and separate checked printing
files. The enclosure, surface shoehorn, surface iron and piston assembly remain
in the [dependency-ordered delivery queue](docs/backlog/README.md).

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
Cut, successive Join and retained-target Intersect extrusions with connected result bodies. Upstream sketch edits rebuild their solids through the same history.
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
[the bounded receipt](docs/validation/point-edge-dimensions.md) records
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
[Grouped Fillet](docs/validation/grouped-fillet.md) adds independent R5/R2.5
edge sets in one feature and is independently accepted and integrated.
The [support tutorial](docs/validation/support-tutorial-independent.md) now passes
a cumulative functional video-variant gate through an angled cutter, Combine Cut
and a closed millimeter STL. Its native movie is a labeled continuation; a separate 78-second movie shows
the complete blank-model workflow through the shared API with command overlays.
This is not complete CAD parity.
Annotated real-app tutorial movies can now be recorded with local commands; see
[the video recipe and verified results](docs/validation/tutorial-video.md).
The illustrated [Centerline and Revolve lesson](docs/validation/revolve-tutorial.md)
also has a complete creation movie, editable source and checked STL. Its partial
angular control and full revolution retain sketch/axis references through edits
and storage. [Original vector ribbon icons](docs/validation/ribbon-icons.md) now
fit the existing modeling commands at the minimum window width.
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
The complete [paper-clip lesson](docs/validation/paper-clip-tutorial.md) now adds
seven typed lines, six sketch fillets and a retained circular Sweep, with a full
creation movie, editable source and checked STL. A focused corrected capture
records recognizable tangent symbols; the movie predates that visual fix.
Typed chained Line now supports positive length entry, Tab lock, pointer H/V
direction and click continuation with retained dimensions; the bounded
[Line input receipt](docs/validation/typed-line-lock.md) records exact seven-segment
native Qt and real-entrypoint API evidence.
The bounded [Sketch Text foundation](docs/validation/sketch-text.md) retains
pinned-font initials on a planar face as selectable OCCT profiles. DSB and DS8
raise through the existing Join path. Independently accepted
[native placement controls](docs/validation/sketch-text-controls.md) add pinned
regular/bold fonts, preview/Cancel and on-canvas move/rotation handles.
The complete [initials keyring lesson](docs/validation/keyring-tutorial.md) now
combines a rounded outline, retained inward Offset, raised rim, filleted bore and
placed raised text, with a full creation movie and independently checked STL.
The completed bracket can be exported as one valid STEP solid or a closed STL
mesh. A full-size design and millimeter STEP/STL plus a labeled 0.2-scale STL
print sample have [independent readback evidence](docs/validation/bracket-exchange.md).

The [fully constrained outlet sketch](docs/validation/wall-outlet-tutorial.md)
also passes rough sketching, retained sketch Mirror, Concentric/Equal relations
and six driving dimensions, with a complete API creation movie and editable
sketch. Exact edit/history/reopen checks passed in a separate saved-model run.
This source ends at Finish Sketch and does not produce a print mesh.

[Two-line Collinear](docs/validation/sketch-line-collinear.md) now aligns
connected or separated authored straight lines with one retained constraint and
selectable badge. A split-edge plate preserves its profile and solid through
dimension edits, history and reopen; focused native input passes.

[Existing-circle Tangent](docs/validation/sketch-circle-line-tangent.md) now
adds either-order circle/line picks and retained finite contact through radius
and datum edits, history and reopen.

[Three-Point Circle](docs/validation/sketch-three-point-circle.md) now creates
an analytic circle through three ordered circumference picks. Snapped source
points retain solver relations; source edits update the circular profile and
downstream extrusion. Native input, history/reopen and STL readback pass.
[Two-Tangent Circle](docs/validation/sketch-two-tangent-circle.md) now creates
an editable analytic circle against two straight sketch lines with a typed or
pointer radius; moving the source lines retains both tangencies.
[Three-Tangent Circle](docs/validation/sketch-three-tangent-circle.md) now
creates a source-driven circle on the third straight-line pick. The rectangle
three-edge case, retained tangencies, source edit and downstream solid pass.

[Explicit arc-line Tangent](docs/validation/sketch-arc-line-tangent.md) now
constrains an arc and line sharing an endpoint, with either native pick order
and retained badges. A rounded plate stays tangent through radius/width edits,
solid recompute, history and reopen. Broader tangent combinations remain ahead.

[Sketch line Symmetry](docs/validation/sketch-line-symmetry.md) now constrains
two existing lines about a construction axis, with retained endpoint pairing
and a selectable constraint badge. A plate width edit updates its symmetric
profile and extrusion; focused native selection, conflict refusal, history and
reopen checks pass. Broader curve symmetry remains ahead.

[Named length parameters](docs/validation/named-length-parameters.md) now bind
sketch dimensions and Extrude distance by stable identity, with native name
entry, atomic recompute and undo/save/reopen. This bounded slice supports direct
names and numeric lengths. [User length expressions](docs/validation/length-expressions.md)
now add dependency-aware arithmetic such as `Length=Width*2`, with stable rename
references, atomic geometry updates and focused table/API/history/reopen evidence.
Functions, broader units and anonymous formulas in model fields remain ahead.
[Box and Shell parameters](docs/validation/box-shell-parameters.md)
extend those stable bindings to the three Box dimensions and inward Shell
thickness, with native field entry and focused real-app validation. Independent
review and integration are complete.
The [Extrude Offset start candidate](docs/validation/extrude-offset-start.md)
adds a signed start displacement before the independent Distance, with native
and API controls, direct named lengths, shifted support geometry and a focused
front-sketch Cut validation. Bounded independent review and integration are complete.

[Retained Countersink Hole](docs/validation/countersink-hole-controls.md) now
cuts from a face-supported sketch Point with editable bore/sink diameters and
angle, current-target All termination and native preview/Cancel/timeline edits.
Its scoped geometry, input and GPU evidence is independently accepted. The
[nine-hole Spacing/Adjust pattern](docs/validation/hole-spacing-pattern-controls.md)
is also independently accepted, including a thicker-target control. The complete
[parameterized plate gate](docs/validation/countersunk-plate-tutorial.md) is independently accepted, with a full creation movie, editable model and checked STL.

The private development repository is [rje/Alloy](https://github.com/rje/Alloy), with the
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

The independently accepted [Section Analysis](docs/validation/section-analysis.md)
adds retained origin-plane GPU clipping with inherited material and hatching.
Core, shared-API lifecycle and native Qt input pass with inspected GPU captures;
physical OS input and Windows/Linux runtime remain separately qualified.
The [selected-face section](docs/validation/selected-face-section.md)
adds retained planar support, signed Distance, Flip, native face selection and
unavailable-support recovery. Its focused real-app and native checks pass;
independent review and integration are complete.
The [origin-section Distance controls](docs/validation/origin-section-distance.md)
extends those controls to XY/XZ/YZ while preserving older origin analyses.

[Circular Body Pattern](docs/validation/circular-body-pattern.md) now retains
an origin-Z full pattern with editable total count, stable occurrence identities,
and native selection/preview/Cancel controls. Exact rigid rotations preserve
offset-curve geometry, with source edits, history and reopen verified. The full
[ornament workflow](docs/validation/ornament-tutorial-acceptance.md) is independently accepted.

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

The [multi-face validation candidate](docs/validation/multi-face-gate.md) records
passing core/API and native side/inclined evidence, plus the remaining native
support-refusal recovery gate; it is not full SKT-017 acceptance.

Retained same-sketch New Body Full/Partial Revolve is available through the
[shared API foundation](docs/research/solid-revolve.md). Native Revolve controls
and tutorial workflow acceptance remain pending; see [candidate evidence](docs/validation/solid-revolve.md).

The [native Revolve candidate](docs/validation/revolve-controls.md) adds Profile/
Axis selectors, unambiguous Centerline inference and a real angular Partial
preview before Full/OK through the shared authority. The full illustrated tutorial
and broader Revolve modes remain separate gates.

Sketch Text has [native control candidate evidence](docs/validation/sketch-text-controls.md):
the shared command now opens an editable Qt dialog with two pinned regular/bold
font choices, height, preview, move/rotation handles, Cancel/OK and edit reopening.

The [initials-keyring full runner](docs/validation/keyring-tutorial-prep.md) is
prepared but has not been executed. It retains the public lesson's step order;
its upper Cut-hole edge Fillet needs bounded provenance work before acceptance.

The [countersunk plate tutorial](docs/validation/countersunk-plate-tutorial.md)
now passes its full source sequence with named parameters, a retained Point,
Countersink Hole and nine-instance Spacing/Adjust, a complete creation movie,
editable file and independently checked STL.

[Center Point Slot](docs/validation/center-point-slot.md) is independently accepted:
center-first pointer/typed creation, retained analytic geometry, construction guide,
H/V and D150/R25 constraints, edits, cancellation and history/reopen.

[Fit Point Spline](docs/validation/fit-point-spline.md) now supports retained fit
points, insertion/removal and tangent direction/weight handles, real closed profiles
and editable storage/history, with independent bounded native/API acceptance.

[Center Point Arc Slot](docs/validation/center-point-arc-slot.md) now retains
analytic radial intent alongside straight slots, with native creation and separate
source dimensions, editable geometry and persistence evidence.

The complete [slots and splines lesson](docs/validation/slots-splines-tutorial.md)
now passes three source-order sketches, editable spline/conic operations, exact
slot/ellipse dimensions, a full creation movie and verified history/reopen.
[Per-entity solved colors](docs/validation/entity-solved-colors.md) distinguish
defined geometry from unrelated free curves; corrected images postdate the movie.
