# Workflow roadmap

This is a proposed sequence, not a schedule or an implementation report. The
full replacement goal remains in [product-scope.md](product-scope.md). Favor
complete, runnable workflows and expand coverage from them.

## 0. Clean-room foundation and controllable shell

- Establish the root agent rules and source-linked research.
- Build a standalone desktop shell with independently authored assets and the
  researched interface structure.
- Expose command discovery, command execution, state, and screenshot capture.
- Launch the real app, exercise real shell commands, and inspect a captured
  image. Mark modeling commands unavailable until implemented.

Acceptance: a documented build, a running Alloy window, successful agent smoke
test, and an actual Alloy screenshot. This proves the foundation, not CAD or
visual parity.

## 1. A real geometry document

Integrate the kernel and viewport. Create a primitive, select it, inspect its
dimensions, save/reopen it, and import/export a small STEP fixture. Add undo/redo
and automation through the same document commands.

Acceptance: valid kernel geometry, expected dimensions/body count, round-trip
document state, accurate selection, and a visible result in the real app. Keep
native document and imported evaluated shapes distinct.

## Next: the sketch-first human-test milestone

After the input/editor fix, the execution priority is FND-005 solver admission,
SKT-001 origin-plane sketch lifecycle, SKT-002 lines/rectangles, SKT-003 constraints,
SKT-004 linear driving/driven dimensions, SKT-014 simple closed profile, HIS-001
ordered recompute and SOL-001 extrusion. SKT-015 checks this complete workflow:
40 by 25 mm rectangle, 5 mm extrusion, upstream width changed to 50, undo/redo,
cancel and save/reopen. This is the next interesting human-test target.

The first workflow does not wait for STEP, every platform package, planar-face
attachment, holes or named expressions. Those remain required tasks and full
product gates. SKT-013 preserves planar-face attachment; SKT-009 retains complex
regions and holes; PAR-001/HIS-004 add named expressions and broader dependencies.
The shared operation/undo authority is extended before extrusion lands.

## 2. The broader parametric modeling workflow

Create a plane sketch, draw a profile, constrain and dimension it, finish the
sketch, extrude, and fillet. Change the original dimension, recompute downstream
features, undo/redo, save/reopen, and export STL. Add 3MF after its units and
validation path is integrated.

Acceptance: a bracket/enclosure fixture created through agent commands, with
the same UI command implementation. Validate dimensions and geometry, authored
history round-trip, supported reference updates, and printable mesh output.
Compare the implemented interaction layout to permitted public references.

## 3. Shape creation breadth

Add arcs/circles, broader constraints, revolve, booleans, chamfer, shell, holes,
patterns, and mirror. Add sweep/loft and a coherent surface workflow: create,
trim or stitch, then thicken. Expand imported CAD and mesh inspection and
supported sketch interchange.

Acceptance: several ordinary objects that use the new operations together;
clear failure diagnostics; documented tolerances and supported input limits;
agent-accessible editing and inspection for every new feature.

## 4. A dependable first release

Broaden the initial command catalog, input/selection fidelity, keyboard behavior,
recovery, reference stability, geometry robustness, and exchange validation.
Qualify chosen operating systems and representative larger documents. Reconcile
the compatibility catalog against a declared public reference baseline.

Acceptance: the agreed initial scope is covered by real workflows and a
published capability/limitations list. Separate functional coverage from exact
visual fidelity and unsupported import subsets.

## 5. Expand toward full functional replacement

Plan assemblies/drawings, sheet metal, advanced/direct/freeform modeling,
manufacturing, simulation, electronics, rendering, and remaining offline
workflows as separate evidence-backed increments. Start each family with a
complete useful workflow, then grow the command inventory. Cloud-dependent
deferrals remain explicit at the feature level.

Full parity is a substantial, multi-year engineering effort with specialist
domains. No delivery dates are committed by this roadmap. Public documentation
reduces specification ambiguity; it does not eliminate geometry, solver,
manufacturing, or compatibility engineering.

## Evidence for each increment

Record the build configuration, commands exercised, fixture inputs, measured
results, screenshots, and remaining gaps. A successful build is build evidence.
A smoke test is control/workflow evidence. Geometry checks and matched visual
comparisons establish different claims. Run checks appropriate to the increment
and stop once the requested behavior is convincingly demonstrated.

Native renderer migration is tracked in RHI-001/RHI-002 and GATE-045 under [decision 003](decisions/003-renderer-direction.md), as post-v1 Todo work by user decision 2026-10-24 unless material immediate advantage warrants reconsideration. Actual packaged GPU/visual/platform requirements remain v1 gates. Published builds follow [version cadence](releases.md).
