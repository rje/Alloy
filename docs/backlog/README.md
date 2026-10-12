# Implementation backlog

Prepared 2026-10-11 from public references and the accepted Alloy requirements.
[issues.json](issues.json) is the machine-readable source for GitHub publication;
[sources.json](sources.json) records the 134 dated reference entries cited by its
issues. Stable keys survive GitHub issue numbering. Published numbers and links
are in [github.md](github.md) and [github.json](github.json); do not replace
dependency keys with issue numbers.

There are **147 issues**: **94 implementation, 14 research, 22 epics, and 17
validation gates**. This is a substantial starting decomposition, not a claim
that 143 tickets exhaust reference CAD application parity. Later epics require linked bounded child
issues before implementation and remain open until that declared coverage passes.

| Phase | Count | Result |
| --- | ---: | --- |
| P0 Foundation | 19 | Public baseline, MIT dependency audit, document/naming/solver decisions, portable controls, native CI/packages/releases and hardware evidence |
| P1 Geometry/document | 17 | Real hardware-backed OCCT viewport, authored primitive document, selection, undo/save and STEP |
| P2 Parametric modeling | 32 | Constrained sketches, parameters, editable extrusion/fillet chain and broader basic solid tools |
| P3 Shape/import/print breadth | 27 | Solids/surfaces, common interchange and qualified printing export |
| P4 Initial release | 8 | Robustness, recovery, packaging, performance, physical/visual workflow evidence |
| P5 Assemblies/drawings | 10 | Components/joints, configurations and associative drawings |
| P6 Advanced design | 11 | Sheet metal, direct/mesh/Form, advanced modeling, plastic rules and local presentation |
| P7 Manufacturing | 9 | Setups, toolpaths, verification/posts, additive, nesting and metrology |
| P8 Analysis/electronics | 10 | Traceable study/result data, local ECAD/MCAD and fabrication |
| P9 Full replacement | 4 | Remaining interchange, customization, generative lifecycle and full coverage reconciliation |

## Pullable work and current evidence

The shell gate and runner, authored-history decision, dependency admission,
geometric-reference prototype, command sessions, native-toolchain audit and
thin modeling kernel are complete with independently reviewed evidence. The
first real primitive workflow now verifies rendering/picking, editable box and
cylinder intent, one undo authority, trustworthy measurements and native
save/reopen. See [development](../development.md) and the
[primitive receipt](../validation/primitives.md).

Run `python3 tools/backlog.py ready` before pulling work; live issue states and
dependencies determine readiness. The next main modeling work is the sketch-first sequence recorded in [roadmap](../roadmap.md), with a local useful-part checkpoint in SKT-015. Its dependencies no longer wait for the broad GATE-010/platform gates. Remaining geometry/document work includes
face/edge selection, Browser breadth, STEP interchange, persistent-reference
integration and observable document decisions. Foundation work still includes
a dated compatibility baseline, portable Windows controls and
qualified runtime packages. The issue count measures the starting backlog,
not product coverage or a replacement claim.

MIT is accepted for Alloy's original source and documentation. Each dependency
keeps its own terms. A GPL-only solver or format library must not slip into the
distributed application through this research; require an explicit deliberate
later licensing decision or an audited compatible alternative. C++20 is selected;
Rust is outside this plan.

## Cumulative product gates

| Gate | Useful result | Additional evidence |
| --- | --- | --- |
| `GATE-000` | Existing native shell and shared commands | Physical S/search/Home/browser path, API inspection, actual window capture; zero CAD claims |
| `GATE-005` | Installed packages on macOS, Windows and Linux | Native runtime relocation, portable control API, actual hardware renderer, pre-capture drawable/scale/resize and physical inputs |
| `GATE-010` | Gauge block and cylinder | Actual hardware GL diagnostics, rendered/picked BRep, analytic measurements, undo/save and independent STEP readback |
| `GATE-020` | Editable mounting bracket | Constrained sketch → extrusion/join/cut → fillet → upstream edit → reopen/export |
| `GATE-030` | Surface-authored duct plus enclosure/knob/slot fixtures | Surface continuity, gaps/stitch, thickening, new solid/sketch operations and editable history |
| `GATE-040` | Imported adapter and printable parts | Independent source files/readback across declared formats, units, losses, mesh validity |
| `GATE-045` | Native Metal/Vulkan CAD viewport | Packaged hardware and cumulative workflow evidence on every declared target |
| `GATE-050` | Dependable initial shape release | Packaged app, recovery/negative corpus, performance, physical workflow and qualified visuals |
| `GATE-060` | Adjustable clamp assembly and drawings | Occurrences, DOF/limits, variants, drawing association/BOM and exports |
| `GATE-070` | Configured instrument enclosure | Folded/flat metal, Form/direct/mesh edits, advanced modeling, local rendering/animation |
| `GATE-080` | Manufacturing package | Toolpath/stock/collision/post, layers, nesting, process-specific outputs and units |
| `GATE-090` | Traceable analysis study/results | Real study/mesh mapping, numerical probe checks and invalidation; imported results are not a solve |
| `GATE-100` | Electronics-to-enclosure product | ERC/DRC, connectivity, board/package alignment and independent fabrication-file checks |
| `GATE-110` | Full noncloud scope reconciliation | Every accepted command/option/family traced to implemented children and cumulative workflows; only evidenced cloud suboperations may defer |

Each gate depends on actual feature issues plus earlier gates. Every newly
implemented capability must map into the growing fixture ledger; the main
showcase object need not exercise every option, so retain small focused fixtures
alongside it. Run **all previously accepted representative workflows** on the
same candidate build. A previous green issue is not a fresh regression result.

The runner is implemented by `FND-010`; the [ledger](../../tests/workflows/ledger.json)
lists current shell, authored, session, kernel, native-audit and primitive cases.
`python3 tests/workflows/run.py --gate GATE-020 --cumulative
--evidence build/evidence/GATE-020` refuses acceptance until that gate's evidence
specification, actual feature cases and live prerequisites are complete. Read
[workflow validation](../workflow-validation.md). Existing `ctest` remains valid.
Use Alloy automation for modeling/inspection/capture; direct computer input is
reserved for uncovered physical interaction. Gate reports separate build, API,
geometry, save/reopen, export, physical input and visual results. Unit tests,
mocked state, static drawings, issue counts or kernel-only demos cannot close
product gates. No reference CAD application runtime comparison is permitted.

If later research finds a missing noncloud family or option, add a real task and
update the appropriate gate prerequisites. Do not use the cloud exception to
discard an entire family: `FULL-001` owns dated operation-specific deferrals and
unknowns. For analysis, current reference solves are cloud-dependent; local
study/result capability is planned separately. A future local solver must add
blocking numerical/convergence benchmark tasks; result import cannot count as
solver validation.

## Validation and publication

```sh
python3 docs/backlog/validate.py
python3 tools/backlog.py ready
```

The stdlib validator checks root/issue schema, unique keys, dependency existence,
acyclicity, dependency-first publication order, required issue-body sections,
dated source links, epic decomposition requirements, the cumulative gate chain,
and reachability of all tasks from the final gate. Initial tasks reach the
initial-release gate, apart from the explicitly separate cloud inventory which
reaches the full gate. The recorded output is
[validation-summary.json](validation-summary.json).

Each issue includes problem/result, bounded scope, acceptance, exact proposed
verification, evidence dimensions, public source/date and limitations. Most
source pages are living documentation, not release-pinned specifications; see
[the deep dive](../research/cad-workflows.md) for contradictions and missing
defaults. All 135 issues were published and checked through `gh`/GitHub APIs:
unique keys, complete bodies, linked dependencies, kind/phase/priority labels,
ten milestones, and readiness labels. See [publication.json](publication.json)
for the dated receipt. The repository is public and MIT-licensed.

The public [project board](https://github.com/users/rje/projects/1) has a
repository auto-add workflow. Projects access is now available through `gh`;
all 135 board items were verified by API. Keep Status synchronized as tasks start
and finish. Issue publication and the pull queue are verified independently.

To synchronize dependency labels after closing work:

```sh
python3 tools/backlog.py ready --apply-labels
```

`python3 tools/backlog.py publish` explicitly creates or reconciles the authored
issues, labels, and milestones. It is resumable using stable issue-body markers.
Review changes to `issues.json` before republishing; remote states are preserved.
All ongoing GitHub operations use `gh` and documented APIs.

The cross-platform requirement added five bounded tasks to the original 130:
`FND-012` Windows local control, `CI-001` native build matrix, `CI-002` runtime
bundles, `GATE-005` hardware package qualification and `CI-003` milestone release
promotion. Existing dependency/packaging/product gates now require all three
OSes. The [release plan](../releases.md) distinguishes the current compile-only
CI seed from the future qualified runtime packages.

The later input/editor and renderer decisions added UX-001, RHI-001/RHI-002 and
GATE-045. The sketch reprioritization split SKT-013 face attachment, SKT-014 simple
profiles and HIS-004 broader recompute from the first origin-plane path, and added
SKT-015 as its focused human-test checkpoint. Original publication.json is the
historical 135-issue receipt; github.json and validation-summary.json reflect the
current 147-task backlog. SKT-015 is a bounded local workflow check, distinct from
the cumulative all-platform product gate chain.

The public beginner-project [tutorial recipes](../research/tutorial-gates.md) add
TUT-001 #146 (complete toy block) and TUT-002 #147 (die/component/STL). They are
blocked future workflow gates and have not been executed. SKT-016 #144 supplies
construction/midpoint/point-to-edge coverage; SOL-014 #145 supplies genuine
face-positioned Sphere/Cut/Repeat. TUT-001 feeds GATE-030; the full component die
feeds GATE-060. SKT-015 remains the earlier rectangle/extrusion human checkpoint.

The source registry now also includes the annotation workflow and tutorial
evidence. One supplied crop has unknown public provenance, version and scale;
its observation is qualified separately from retrieved public documentation.
