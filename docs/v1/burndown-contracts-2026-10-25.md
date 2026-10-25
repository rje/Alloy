# V1 burndown work orders — 2026-10-25

The task-contract cleanup reviewed all99 previously open v1 issues. It adds21
smaller owners and moves ten broad/publication parents outside the first-v1
closure, producing109 open v1 work orders before any implementation is closed.
These are decomposition counts, not additional features or product completion.
The canonical issue graph and live board determine current state.

[Work-order contract](work-order-contract.md) defines specific inputs, behavior,
UI/controls, expected results and proportional closure checks. Recent native
contracts already pin their actual-hit selections, keys/focus, handles, numeric
inputs, previews, commit/cancel and GPU results; those requirements are retained.

| Finishable deliverable | Owner(s) | Retained wider requirement |
| --- | --- | --- |
| Authored primitive/extrusion face and edge naming | DOC-046 | DOC-012 |
| Early singleton body/face/edge picking | DOC-047 | DOC-006 |
| Face/edge selection sets, then type filters | DOC-048 → DOC-049 | DOC-006 |
| Constant-radius selected-edge/tangent-chain minimum | SOL-098 | SOL-003 |
| Common downstream references and explicit support repair | HIS-014 | HIS-002 |
| Single-solid STEP minimum from existing accepted routes | XCH-025 | XCH-003 |
| Trimmed planar IGES round-trip core, then native controls | XCH-026 → XCH-027 | XCH-002 |
| Declared 2D DXF core, then Insert/export controls | XCH-028 → XCH-029 | XCH-006 |
| Editable cubic SVG core, then native placement controls | XCH-030 → XCH-031 | XCH-007 |
| Variable Fillet endpoint-control research decision | SOL-099 → SOL-064 | SOL-003 / ADV-005 |
| Prepared-triangle mesh auditor, then native report/consumer check | PRT-017 → PRT-018 → PRT-003 | PRT-019; repair/edit/conversion remains ADV-002 |
| Typed evidence ingestion, finite coverage plan, gate orchestration | FND-031 → FND-032 → FND-033 | Individual cumulative gates retain their substantive workflows |
| Local promotion-tool readiness and dry-run | CI-013 | CI-003 retains actual authorized foundation publication |

## Existing feature work orders made concrete

ASM-011 now requires a successful changed-source update with explicit affected
reference resolution and a downstream cutout; refusing all updates cannot pass.
SKT-101 names its actual model/plane intersection entities. SOL-063/064/065 and
SOL-068/069/070 pin bounded source models, controls/options, exact geometry,
meaningful edits and refusal behavior. SRF-023/024 separately own mixed Boundary
Fill tools and retained multi-cell selection. The shared mesh audit has nine
original fixed defect cases rather than unbounded proof of every mesh.

Detailed public-source records, permitted visual references and original large
fixtures remain in `../Alloy-reference-records/knowledge/specs/v1-contract-burndown-2026-10-25/`.
Repository issues use neutral source IDs. Numeric fixture values, implementation
policies and unresolved parity are explicitly distinguished from documented
controls. SOL-099 is the one narrow unresolved control decision: implementation
of SOL-064 remains dependent on a decisive answer, not an invented UI default.
XCH-001 remains the existing finite format/library admission decision; accepted
STEP/mesh/3MF records are reused, not declared unknown again.

## Dependency and release changes

First-v1 consumers now depend on the named minimum rather than broad parents.
The early picking/naming foundation precedes face-supported sketches; later
selection sets can reuse those sketches without introducing a cycle. Closed
SKT-010/SKT-013/HIS-006 receipts are not reaccepted or erased: only their declared
foundation dependency is aligned with the selected minimum, with cumulative
qualification still open.

GATE-030 creates a closed duct by the supported surface/Thicken route and uses a
companion two-planar-sheet Stitch/gap fixture. It does not silently require the
later general closed sewn network. GATE-110 explicitly retains the moved wider
parents; every issue formerly reachable from the full gate remains reachable.
Metal/Vulkan migration remains post-v1.

The cumulative gates use a finite coverage plan, explicit evidence roles and
candidate applicability. FND-031/032/033 are implementation dependencies: the new
proposed receipt/candidate flags and later gate adapters do not work yet. Research,
license, package and human native receipts do not require fake CAD runtime cases.
Unchanged focused geometry evidence may be reused; exact-archive three-platform
runtime, GPU, native input, save/reopen and required independent output remain
mandatory. REL-001 pins a finite negative corpus, REL-004 defines finite budget
workloads and REL-049 keeps the every-command cost audit with a stopping rule.

## Review and publication limits

An independent Astra planning pass checked scope retention, dependencies and
sampled geometry/control contracts. It found and resolved a path-pattern station
overflow and an unspecified SVG cubic fixture. The corrected original SVG has
seven Lines/one cubic, two loops, analytic bounds/area and a transform oracle.
This is planning acceptance only: no modeling feature, native/platform gate or
release is closed by these edits. No build, GUI run or paid CI is needed for
this documentation/backlog change. Synchronize the published issues, project
membership/Release scope and local review-site data before treating the board as
updated. New work starts Todo; In Progress is reserved for active implementation.
