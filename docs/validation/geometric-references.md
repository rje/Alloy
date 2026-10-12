# FND-004 bounded naming fixture

Observed 2026-10-11, Apple silicon macOS, AppleClang 17, Qt 6.11.2 / OCCT 7.9.3.
[geometric-references.json](geometric-references.json) records executed source,
fixture executable and output hashes. This is library geometry evidence only;
API, native save/reopen, exports, physical input and visuals are non-applicable.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/homebrew -DALLOY_BUILD_SPIKES=ON
cmake --build build --target topology_reference_spike
build/topology_reference_spike build/evidence/FND-004
```

The fixture passed unique primitive face/edge role rebinding after width edit,
generated/modified selected-edge fillet history, a two-descendant edge split,
explicit edge deletion and a real two-to-one edge merge. Every shape was checked
valid. Its output records resolved/ambiguous/unresolved/merged repair states and
refuses automatic reattachment for destructive cases. No actual authored CAD
operation or user-facing reference repair is implemented by this prototype.

[Decision 002](../decisions/002-geometric-references.md) defines IDs, tolerance,
history lineage, failure policy and existing follow-up issues. The spike remains
optional and is excluded from application/native CI dependencies.
