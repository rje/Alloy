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

## 2. The first parametric modeling workflow

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
