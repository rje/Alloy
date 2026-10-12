# 004 — Select PlaneGCS for sketch constraints

Date: 2026-10-11. Status: selected integration direction with a passing bounded
native-library fixture; authored application constraint integration is pending.

Use FreeCAD PlaneGCS at `ec6852e6caadf55045881a6003cc7ad038c8d2a7` behind a
separate replaceable shared library. Preserve its LGPL-2.1-or-later notices and
source terms. Alloy's original source remains MIT. Use Eigen 5.0.1 with
`EIGEN_MPL2_ONLY` and Boost 1.90.0 for the tested baseline. Do not admit GPL-only
libslvs under the present dependency decision.

The [research and actual evidence](../research/sketch-solver.md) compared public
interfaces/licenses and executed PlaneGCS on a dimensioned rectangle, tangent
circle, inconsistent dimensions and temporary drag target at representative
scales. It did not run a second candidate or benchmark complete CAD applications.
This is sufficient to select the next integration, not to claim general solver
robustness or completed application constraints.

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
5. Dragging uses seeded candidate geometry and temporary negative-tag constraints;
   preview has no document transaction. Remove the temporary constraints on
   completion. A committed gesture produces one intentional undo entry. Reject
   stale session/revision results before publishing.

Do not invent a generic solver framework or duplicate constraint equations that
PlaneGCS already provides. Keep a small Alloy adapter and add constraints as
their real sketch workflows enter scope. Do not expose PlaneGCS pointers/types
as public file-format or plugin ABI commitments.

## Portability and distribution boundary

The tested extraction comprises eleven `planegcs` files and two public support
headers. One recorded C++20 portability patch replaces the C++23 impossible-case
hint with `std::abort`; explicit standalone shims provide logging/config/export
services. No numerical algorithm was rewritten.

Production integration must pin and verify the extraction, retain notices and
the patch, supply real export definitions on Windows, and register the new
components/source-access/replacement materials in the dependency inventory.
The spike uses absolute local library paths; it is not a deployable package.
Platform compilation belongs at an explicit version cut or manual dispatch;
runtime/package qualification remains separately required on each target.

## Next implementation

SKT-001/002 may establish origin-plane authored geometry without a solver. SKT-003
integrates this adapter for real constraints and diagnostics; SKT-004 adds driving
and driven dimensions. Their real-app evidence must cover shared UI/API state,
undo, cancellation, invalid input and save/reopen. The sketch-led checkpoint
then verifies dimension edits recomputing a real extrusion. Tangent-circle
library evidence does not by itself implement SKT-006.

Unqualified: arbitrary drag branch behavior, complex/spline sketches, numerical
conditioning over the entire document range, redundant-constraint UX,
multithreaded use and other-platform solver runtime. Address these when their
bounded workflows are implemented; they do not justify a replacement solver or
a broader framework now.
