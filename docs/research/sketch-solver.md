# Sketch solver study

Recorded 2026-10-11. This is public open-source library research and a native
library fixture, not evidence that Alloy already solves authored sketches.
The selected integration path is [decision 004](../decisions/004-sketch-solver.md).

## Bounded comparison

| Candidate | Documented/source-observed strengths | Integration and license result |
| --- | --- | --- |
| FreeCAD PlaneGCS, `ec6852e6caadf55045881a6003cc7ad038c8d2a7` | Sketch-specific points, lines, circles, arcs, ellipses and splines; geometric/dimensional constraints; DogLeg, LM and BFGS; DOF and conflicting/redundant tags; negative-tag temporary drag constraints | Selected. LGPL-2.1-or-later file markers; compile a separate replaceable shared library with a small audited portability boundary. Eigen and Boost headers required. Actual fixture executed below. |
| SolveSpace libslvs, `ef966335e7fbc617ba8e759149a567158b37aa7b` | Public C interface with parameter/entity/constraint handles, workplanes, result codes, DOF, failed constraints and preferred dragged parameters; established geometric constraint implementation | Comparison only. GPL-3.0-or-later remains outside the accepted dependency policy. No libslvs code was integrated, compiled or benchmarked here; no performance comparison is claimed. |

PlaneGCS is the smaller licensing/integration step for this C++20 application.
This conclusion is an Alloy engineering decision, not a claim that one solver is
universally more robust. Both are local nonlinear solvers: neither can substitute
for authored identity, undo, unit conversion, robust profile construction or
persistent geometry references.

The PlaneGCS API reports `Success`, `Converged`, `Failed`, and
`SuccessfulSolutionInvalid` separately. A convergence result alone must not be
accepted as a valid solved sketch. Constants omitted from `declareUnknowns`
remain fixed; coordinate pointers must stay alive through the solve. Constraint
tags are how diagnostics are returned, not persistent document identifiers.
`addConstraintDifference(first, second, distance)` means `second - first`.
These details were checked in the pinned implementation, not guessed from names.

## Actual native fixture

Run from the repository root:

```sh
python3 tools/spikes/sketch-solver/run.py
```

The script downloads 13 pinned public PlaneGCS/support files into ignored
`build/evidence/FND-005/source`, builds `liballoy_planegcs_spike.dylib` and links
an independent C++20 harness against it. It requires the observed Eigen 5.0.1 and
Boost 1.90.0 headers; it refuses different reported versions. Homebrew Eigen
5.0.1 was installed for this study. The command is presently qualified on macOS;
the script's non-macOS shared-library suffix is not cross-platform build evidence.

Observed on Apple silicon macOS 26.5.1, Apple Clang 17.0.0:

| Fixture | Result |
| --- | --- |
| Anchored 40 × 25 rectangle and radius-5 circle tangent to its lower line, circle center x=20 | Success, zero DOF, max reported constraint error 0, center y=5 |
| Same problem with numeric coordinates divided by 25.4 | Success, zero DOF; canonical dimensions and tangency agree within 1e-7 mm |
| Change rectangle width to 50 in both scales | Success; width 50, height 25, max reported constraint error 0 |
| Add incompatible width 41 alongside width 40 | Failed status (2), conflicting tags `[7,12]` |
| Rectangle with free width, temporarily drag upper-right x to 60 | Success, positive-width branch retained; removing temporary tag restores one DOF, width remains 60 |

The last retained run measured approximately 204 and 57 microseconds for the two
initial solves, including initialization/diagnosis. These single tiny-fixture
measurements are observations, not a performance benchmark. Per-tag errors have
constraint-dependent units/scaling; they are not all millimeter distances.
Independent coordinate/tangency assertions use canonical-mm tolerance 1e-7.

The first harness draft used the difference arguments in reverse; the independent
coordinate assertion rejected its mathematically consistent negative rectangle.
Correcting the adapter arguments produced the retained result. No numerical
solver algorithm was changed to make the fixture pass.

Retained local evidence:

- `build/evidence/FND-005/build.log`: exact compiler commands and diagnostics.
- `build/evidence/FND-005/build.json`: upstream URLs/hashes, explicit shim/patch
  hashes, compiler/host, fixture/runner/library/executable/result hashes.
- `build/evidence/FND-005/result.json`: executed case results.
- [Public observation receipt](../../tools/spikes/sketch-solver/observed.json):
  retained build identity and result, independent of ignored build output.

`otool -L` confirmed the fixture links the separate solver dylib, and that the
solver dylib links only macOS system C++/system runtime libraries. Eigen and
Boost are header dependencies in this build. This is not a relocated package
or a complete application distribution audit.

## Extracted boundary and admission

All eleven files in the pinned `planegcs/` directory, plus `SketcherGlobal.h`
and `src/boost_graph_adjacency_list.hpp`, carry LGPL-2.1-or-later SPDX markers.
The source also contains older library-license boilerplate; retain both the
original notices and the declared license rather than stripping either.

The fixture has one explicit C++20 portability patch: the single C++23
`std::unreachable()` becomes `std::abort()`, with `<cstdlib>` added. This affects
only the impossible algorithm-enum branch; it does not change constraint
equations. The script records original and compiled source hashes.

Standalone, independently authored shims replace FreeCAD's export/config headers
and console/timing services. Logging is a no-op only in this harness; application
integration should provide useful diagnostics. The solver's existing algorithms,
geometry types, Eigen/Boost calls and constraint code are retained.

| Component | Observed version / license | Admission boundary |
| --- | --- | --- |
| Extracted PlaneGCS set | Exact FreeCAD revision above; LGPL-2.1-or-later | Selected for a replaceable shared-library integration; original notices and source/patch availability required |
| Eigen | 5.0.1; MPL-2.0 selected, `EIGEN_MPL2_ONLY` defined | Core/QR/Sparse/Dense headers used; preserve upstream notices and applicable covered-source access |
| Boost | 1.90.0; BSL-1.0 | Graph/math headers; retain applicable notices/license in dependency materials |
| Alloy harness/adapter | Original MIT code | Does not relicense any upstream source |

Before bundling application binaries, the dependency manifest and notices must
cover this exact extracted set, Eigen/Boost, the portability patch, source-access
materials, and replacement/relink instructions for the shared library. Windows
export/import definitions and library ABI boundaries must be configured rather
than using the fixture's empty export macros. macOS signing/relocation and Linux
runtime paths remain package work. This study does not qualify those packages.

## Integration contract and limits

Keep authored UUIDs, constraints, dimensions and units in Alloy. Map them into
temporary solver storage; never persist pointers or the library's integer tags.
Normalize inputs to canonical millimeters before solving, then map the completed
solution back to the same authored IDs. Solve a copied candidate, inspect
diagnosis/status/residuals/finite coordinates, and publish through the existing
single document transaction only after all supported checks pass.

Use the previous valid geometry as a seed for dragging. Temporary negative-tag
constraints must be removed at gesture completion or cancellation. This fixture
qualified one positive-width drag branch, not arbitrary branch selection,
tangency flipping, interactive stability or overconstrained sketch repair.
Those must be qualified with their real UI workflows as implemented.

Circles, tangency and conflict diagnostics have library evidence here. Curves,
splines, driven dimensions, redundant-constraint policy, large sketches, extreme
coordinate ranges, multithreading and Windows/Linux runtime have not been
qualified by this fixture. Shared-library success does not close SKT-003/004 or
the sketch-to-extrusion checkpoint.

## Primary sources

Retrieved 2026-10-11. Source observations above refer to these public records;
recommendations and admission boundaries are Alloy decisions.

- [Pinned PlaneGCS directory](https://github.com/FreeCAD/FreeCAD/tree/ec6852e6caadf55045881a6003cc7ad038c8d2a7/src/Mod/Sketcher/App/planegcs)
  and [GCS API](https://raw.githubusercontent.com/FreeCAD/FreeCAD/ec6852e6caadf55045881a6003cc7ad038c8d2a7/src/Mod/Sketcher/App/planegcs/GCS.h).
- [Pinned constraint equations](https://raw.githubusercontent.com/FreeCAD/FreeCAD/ec6852e6caadf55045881a6003cc7ad038c8d2a7/src/Mod/Sketcher/App/planegcs/Constraints.cpp)
  and [solver implementation](https://raw.githubusercontent.com/FreeCAD/FreeCAD/ec6852e6caadf55045881a6003cc7ad038c8d2a7/src/Mod/Sketcher/App/planegcs/GCS.cpp).
- [SolveSpace library description](https://solvespace.com/library.pl),
  [pinned C interface](https://raw.githubusercontent.com/solvespace/solvespace/ef966335e7fbc617ba8e759149a567158b37aa7b/include/slvs.h),
  [pinned license](https://raw.githubusercontent.com/solvespace/solvespace/ef966335e7fbc617ba8e759149a567158b37aa7b/COPYING.txt).
- [Eigen 5.0.1 MPL text](https://gitlab.com/libeigen/eigen/-/raw/5.0.1/COPYING.MPL2)
  and [versioned source](https://gitlab.com/libeigen/eigen/-/tree/5.0.1).
- [Boost license](https://www.boost.org/LICENSE_1_0.txt) and
  [Boost 1.90.0 source](https://github.com/boostorg/boost/tree/boost-1.90.0).
