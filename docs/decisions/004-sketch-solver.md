# 004 — Select PlaneGCS for sketch constraints

Date: 2026-10-11. Status: selected and integrated for the bounded line/point
constraint and driving/driven dimension subset.

Use FreeCAD PlaneGCS at `ec6852e6caadf55045881a6003cc7ad038c8d2a7` behind a
separate replaceable shared library. Preserve its LGPL-2.1-or-later notices and
source terms. Alloy's original source remains MIT. Use Eigen 5.0.1 with
`EIGEN_MPL2_ONLY` and Boost 1.90.0 for the tested baseline. Do not admit GPL-only
libslvs under the present dependency decision.

The [research and actual evidence](../research/sketch-solver.md) compared public
interfaces/licenses and executed PlaneGCS on a dimensioned rectangle, tangent
circle, inconsistent dimensions and temporary drag target at representative
scales. It did not run a second candidate or benchmark complete CAD applications.
That selection fixture supports the library choice, not general solver robustness.
The later [application constraints receipt](../validation/sketch-constraints.md)
records the integrated authored-document/UI/API workflow separately.

## Ownership and call contract

1. `CadDocument` retains stable sketch/entity/constraint/dimension UUIDs and
   canonical-mm authored values. Solver data is transient evaluated state.
2. The adapter receives a copied candidate and the last valid solution as a seed.
   Allocate all coordinate/dimension doubles before giving pointers to PlaneGCS;
   keep their storage stable through diagnosis, solve and result extraction.
   Constants are omitted from the declared unknowns. Map each authored constraint
   to a positive integer tag and return diagnostics mapped back to its UUID.
3. Start with supported line/rectangle constraints and DogLeg. Call
   `declareUnknowns`, `initSolution`, `solve`, and `applySolution` on the private
   candidate. Expose DOF, conflicting/redundant IDs, status and residual summaries.
   `Converged` or nonempty conflicting tags cannot masquerade as successful
   authored geometry. Check finite results and independent geometric residuals
   with tolerances appropriate to length/angle constraints.
4. Commit only the accepted candidate and matching evaluated geometry through
   Alloy's existing one-QUndoStack boundary. Cancel, inconsistent dimensions and
   failed solves leave live authored state and the journal unchanged. Opening a
   saved sketch rebuilds solver state from authored IDs/constraints.
5. The implemented `sketch.movePoint` seeds a copied point coordinate and solves
   existing relations; it creates no journal entry until Finish. Native point
   dragging remains future work. Its temporary-target policy should follow the
   researched negative-tag fixture, discard preview constraints on completion
   and commit a successful gesture once. Reject stale session/revision results
   before publishing either kind of candidate.

Do not invent a generic solver framework or duplicate constraint equations that
PlaneGCS already provides. Keep a small Alloy adapter and add constraints as
their real sketch workflows enter scope. Do not expose PlaneGCS pointers/types
as public file-format or plugin ABI commitments.

## Portability and distribution boundary

The tested extraction comprises eleven `planegcs` files and two public support
headers. One recorded C++20 portability patch replaces the C++23 impossible-case
hint with `std::abort`; explicit standalone shims provide logging/config/export
services. No numerical algorithm was rewritten.

`cmake/PlaneGCS.cmake` now builds that pinned extraction as `alloy_planegcs` SHARED,
checks every original against `third_party/planegcs/sources.json`, retains the
patch/notices and supplies standalone Windows import/export definitions. Matching
Eigen/Boost headers are discovered locally or obtained from pinned hash-checked
archives; Eigen compiles with `EIGEN_MPL2_ONLY`. Alloy's MIT `SketchSolver` adapter
links to this replaceable library; solver pointers/types are not authored data.
Components and terms are recorded in the dependency inventory. Portable export
definitions are implemented, but actual Windows/Linux solver builds, runtime
closure, source-access and replacement materials still require qualification.
The original selection spike used absolute local library paths; neither that
fixture nor current local build evidence is a deployable package.
Platform compilation belongs at an explicit version cut or manual dispatch;
runtime/package qualification remains separately required on each target.

## Implemented subset and follow-ups

SKT-003/004 integrate coincidence, horizontal, vertical, parallel, perpendicular
and fixed-point relations, plus linear/H/V driving dimensions and driven
measurements. Rectangles author four H/V relations atomically. Optional authored
constraint/dimension arrays preserve older unconstrained files; stable IDs and
strict references survive undo and native save/reopen. Fixed relations store the
captured target; driven dimensions store no driving value. Positive H/V readouts
use the signed endpoint branch from seeded coordinates, including reversed
endpoint selection.

Accepted mutations normalize coordinates before OCCT evaluation; failed solves
preserve the usable draft. Commit/open normalize the complete copied document
before replacing live authored/evaluated state. Diagnostics expose whole-sketch
DOF, conflict/redundancy UUIDs, measured dimensions and independent length/angle
residuals (1e-6 mm for lengths, 1e-8 for normalized angular residuals). The
palette/API share point/line selection;
orange selection, green fixed points and whole-sketch black/blue status are
implemented, not per-entity freedom analysis.

[SKT-005 #32](https://github.com/rje/Alloy/issues/32) owns canvas dimension
placement, inline entry and double-click editing;
[SKT-008 #35](https://github.com/rje/Alloy/issues/35) owns per-entity badges/colors
and visibility controls. Native dragging is unimplemented. The next modeling
checkpoint adds closed profiles, dependent recompute and extrusion before
claiming dimension edits update a solid. Tangent-circle library evidence does
not by itself implement SKT-006.

Unqualified: arbitrary drag branch behavior, complex/spline sketches, numerical
conditioning over the entire document range, redundant-constraint UX,
multithreaded use and other-platform solver runtime. Address these when their
bounded workflows are implemented; they do not justify a replacement solver or
a broader framework now.
