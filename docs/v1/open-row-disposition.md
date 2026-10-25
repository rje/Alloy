# First-v1 minimums for the 27 open-owner rows

2026-10-25 burndown clarification: the canonical issue graph now names separate
minimum owners for naming/reference repair, selection, constant Fillet and
STEP/IGES/DXF/SVG; broader family parents remain open outside v1. See
[the work-order contract](work-order-contract.md) and [the dated split map](burndown-contracts-2026-10-25.md).
The selected minima are unchanged; dated counts and owner annotations below are
historical, not the live board. Typed gate evidence/coverage/runner delivery is
FND-031/032/033; CI-013 separates promotion-tool readiness from publication.


REL-030 planning freeze, 2026-10-20. These are the selected **first-release boundaries**, not statements that native input, geometry, packages or platform runtime pass. The `O` labels in the dated execution ledger identify earlier ownership uncertainty. This table chooses one supported subset per row while retaining every wider noncloud mode with its family issue. Shared release gates still apply to the chosen subset. See the [63-row partial disposition](partial-row-disposition.md) for `S` rows and the [execution ledger](release-execution-ledger.md) for receipts.

| Inventory row | First-v1 minimum | Retained wider work / accountable issue |
| --- | --- | --- |
| `document.units` | Choose mm/in display units for sketch and feature values and selected-solid Properties without changing stored physical geometry. | Broader unit systems and locale/format polish stay DOC-002. |
| `parameter.model` | Bind named typed lengths to the accepted sketch/feature fields and a typed angle to the supported sketch Angle; edit, rename, unbind and reject mismatched types atomically. | Other expression grammar and arbitrary field families remain PAR-001. |
| `history.visibility` | Hide/show a committed body and sketch without altering authored geometry, and keep the representative active-preview path responsive. | Other hierarchy/large-document performance modes remain REL-048/REL-049; measured stable-v1 cost still runs under REL-049. |
| `solid.press-pull` | Press Pull one supported planar profile with preview, direct distance entry/drag, Join/Cut as supported, Cancel and retained edit. | Broad multi-selection and unsupported topology remain SOL-016; physical modifier toggling remains SOL-058 and the packaged input gates. |
| `surface.boundary-fill` | Select and retain supported closed cell(s) from a bounded solid/sheet mixed-tool fixture, with explicit choice, edit and refusal. | General cell networks, all tool combinations and broader recovery remain SRF-023/024 and the family owner. |
| `command.search` | Find a registered named command and invoke it by keyboard with predictable focus return to the active workspace. | Global aliases, full accessibility polish and every modal focus combination remain FND-007/015. |
| `exchange.step` | Import a supported solid STEP file and export a selected authored solid with correct units, geometry, names/placements where represented, and independent readback. | Wider color/assembly/repeated-instance fidelity remains XCH-003. |
| `print.stl` | Export selected closed printable solids in declared units with independent closedness/bounds checks and an honest open-sheet warning. | Advanced refinement and batch structure follow the separate `print.refinement` minimum; print-fit/repair breadth remains PRT-003. |
| `sketch.delete-geometry` | Delete one supported authored line/circle/arc/spline or a qualified dependent/derived group with explicit relation disposition, one undo step and reopen. | General mixed generated groups, every construction type and hard topology repair remain SKT-011/021 and their retained children. |
| `sketch.solver-status` | Show solved/underconstrained/overconstrained state and a recoverable direct relation conflict plus rejected driving-dimension edit with named supported culprits. | General minimal conflict sets and arbitrary coupled systems remain SKT-100/168. Their bounded native gates are first-v1 prerequisites. |
| `exchange.obj-output` | Export one selected closed solid as material-free OBJ in declared mm/in, with independent mesh readback and native chooser/Cancel. | Multi-file structure, other body types, materials and expanded units remain XCH-013; native gate XCH-012 is first-v1. |
| `sketch.chamfer` | Retained Equal Distance, Two Distance and Distance And Angle Chamfer on supported connected finite line corners, including repeat and a visible distance edit. | Crossing-line kept-side selection remains SKT-114 and other sketch options ADV-005. |
| `sketch.intersection` | Author one retained sketch intersection from the accepted supported source pairing and follow valid source edits. | Broader analytic projection, silhouettes and relink repair remain SKT-026. |
| `solid.coil` | Retain one supported circular-section helical solid with editable pitch/revolutions and refusal of invalid self-intersection. | Other section, end, direction and operation modes remain ADV-005/SOL-060. |
| `solid.cone-torus` | Create/edit New Body Cone and Torus with independent geometry/export checks and readable ordinary controls. | Other operations, plane/manipulator options and physical/platform input remain ADV-005/release gates. |
| `solid.emboss` | Emboss or deboss one supported retained closed sketch region on one supported face with editable depth and safe refusal. | Multi-region/curved-face mapping, wrap and other options remain ADV-005/SOL-063. |
| `solid.face-edit` | Move, Replace and Delete one supported retained face each, with explicit identity, preview, valid-result check and authored undo/reopen. | General direct modeling and unsupported source/topology cases remain ADV-001; bounded children SOL-068–070 own the first results. |
| `solid.advanced-fillet` | One retained single-edge Start/End Variable-radius mode, with supported downstream references and invalid-radius refusal. | Chord Length, intermediate points, Asymmetric, setback and multi-edge modes remain ADV-005/SOL-003. |
| `solid.path-pattern` | Pattern a supported body or feature seed on one retained path with count/spacing, edit and stable instance identity. | Other distribution, orientation, path and seed combinations remain SOL-009/ADV-005. |
| `surface.stitch` | Stitch one supported two-sheet matching-edge pair within a declared tolerance, report measured gap, retain a valid open shell when that is the result, and refuse excessive-gap or invalid geometry. | General networks, healing strategy and tolerance controls remain SRF-006. |
| `exchange.iges` | Import and export one supported surface fixture with units, explicit healing/loss report and independent geometry readback. | Broad entity coverage, full color/layer mapping and difficult healing remain XCH-002. |
| `exchange.mesh-input` | Import STL and OBJ as honest nonparametric mesh bodies with declared units, bounds and object identity, without implying editable B-rep conversion. | Mesh repair and wider material/group semantics remain XCH-004. |
| `exchange.3mf` | Export Core 3MF objects/build items in mm, retaining supported selected-object placement and independent readback. | Import and optional 3MF extensions remain XCH-005; Core export is XCH-011 and first-v1 mandatory. |
| `exchange.dxf` | Import/export one declared 2D sketch subset with explicit units, layer/curve support and loss report. | Other DXF versions/entities/layer semantics remain XCH-006. |
| `exchange.svg` | Import one declared planar SVG path subset with explicit scale/transform into editable sketch geometry. | Text, styling, broad path effects and output remain XCH-007. |
| `print.mesh-audit` | Report selected printable solids' watertightness, components, bounds, units and representative defects without changing the model. | Repair, printer-specific clearance and broad manufacturability remain PRT-003. |
| `exchange.loss-report` | Show import progress/cancel and a per-entity accepted/skipped/failed summary for supported format fixtures, preserving the previous document on failure. | Broad format/entity diagnostics and long-running import performance remain XCH-008. |

The [scope freeze](scope-reconciliation-2026-10-20.md) records the independent graph review and outside-closure classification. If a minimum later proves too large for its owner, split a finishable child before implementation and retain the parent as the wider roadmap owner. No paid CI, release tag or publication follows from this table.
