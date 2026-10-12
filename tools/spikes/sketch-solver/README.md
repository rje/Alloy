# PlaneGCS native spike

Original harness and runner: MIT. Downloaded third-party sources retain their own
licenses and live only under ignored build evidence. See
[research](../../../docs/research/sketch-solver.md) and
[decision](../../../docs/decisions/004-sketch-solver.md).

```sh
python3 tools/spikes/sketch-solver/run.py
```

Requires C++20 and the observed Eigen 5.0.1 / Boost 1.90.0 headers under
`/opt/homebrew` (`--prefix` changes that location). The runner checks those
versions, downloads an exact FreeCAD revision, records original hashes, applies
one explicit nonnumerical C++20 compatibility patch, and builds a separate
shared solver library plus a real fixture executable. It does not link into or
launch Alloy. `--output` selects the local evidence directory.

The public `observed.json` retains the executed result and build identity.
Re-execution creates local evidence and does not silently rewrite this receipt.
No libslvs implementation or GPL solver is downloaded or linked by this script.
