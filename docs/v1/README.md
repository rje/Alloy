# Printable-part v1 review

2026-10-24 user timing amendment: hardware OpenGL may qualify for stable v1. Native Metal/Vulkan selection, migration and GATE-045 are deferred beyond v1 unless a material immediate advantage warrants reconsideration. RHI-001/RHI-002/GATE-045 remain Todo through GATE-110; GATE-050 no longer depends on GATE-045. Packaged actual GPU/platform, capture/resize/DPI, selection, CAD edge/seam/overlay quality, transparency and performance acceptance remain required. This supersedes earlier backend timing statements, not visual defects or runtime evidence requirements.

2026-10-24: [Command Search](../validation/command-search-focus.md) and the
bounded [circular Coil](../validation/solid-coil.md) are accepted after independent
review. Coil has real native/API creation and edit, named Pitch, history/reopen,
responsive preparation and independently read printable STLs. REL-056 accepts
historical mesh reuse (Undo/Redo 123–140 ms) and scoped responsive source updates;
the broader REL-049 cost audit and post-async continuation (FND-030) remain
required release work. Stitch seam SRF-059 remains unaccepted
after reverted prototype-renderer trials; no successful fix is implied.

[Current execution order](priority-burndown-2026-10-21.md) identifies the next
finishable Mac workflow, mesh import, viewport quality and package gates without
changing this scope freeze.
[A printable miniature bench vise](capstone-candidate.md) is a researched
first-v1 capstone proposal; its whole-workflow and physical-print gates are not
yet accepted.

Private review site: https://alloy-v1-review.rjevans1.chatgpt.site

The user reviewed this as a reasonable working direction on 2026-10-17. The [2026-10-20 planning freeze](scope-reconciliation-2026-10-20.md) selects the first-v1 minimums and release owners across all 134 proposed rows, including the [27 formerly open-owner rows](open-row-disposition.md). This fixes planning scope, not feature or package readiness; the execution gates remain open. The long-term full noncloud product contract remains intact. Older audit narrative below records how the plan was assembled and should be read as dated history.

Surface Revolve row117 now has an independently accepted retained joined-Line
sheet minimum under [SRF-037](../validation/surface-revolve-chain.md). Its canvas
selection, angle drag and native visual gate remain SRF-036, while curved
Line/Arc chains are tracked as later SRF-055.

The 2026-10-19 all-row audit found and corrected a release dependency contradiction: required Core 3MF **export** now enters GATE-040 through XCH-011 without making later 3MF import under XCH-005 a transitive prerequisite. Exact accepted import subsets, native exchange, all-platform packages and STL/STEP/3MF readback still need execution. REL-054/#615 is the stable-v1 exit after REL-049's post-packaging intrinsic-cost audit; earlier preview packages remain separate. The [private review site](https://alloy-v1-review.rjevans1.chatgpt.site) now serves the synchronized planning data; source/data checks and private deployment succeeded, while browser visual inspection remains open.

The first ownership pass linked 20 previously broad-only proposed tool areas to
bounded Todo issues [#496–#515](../backlog/github.md). This establishes pullable
owners, not feature completion or a final v1 scope decision. Existing partial
rows still require real-workflow evidence and explicit release-gate disposition.
The [ownership audit](ownership-audit.md) added six missing narrow owners
[#518–#523](../backlog/github.md) and identified broad parent issues that must be
split before implementation. Sixty-three broad partial rows now have a proposed
v1-versus-later option boundary under REL-030, though finishable owners and
release evidence remain open. Their [working disposition](partial-row-disposition.md) names
the proposed variants; ordinary Combine name/error recovery is accepted as
SOL-071/#566. REL-030 still owns final scope and release-gate reconciliation.
The 2026-10-19 initial decomposition added 32 bounded children for previously
bundled document/history, advanced solid, surface, datum, inspection and mesh
export minima. [The execution ledger](release-execution-ledger.md) links these
to the affected rows. Existing accepted spline, expression and circular-pattern
slices were reused instead of reissued. Subsequent Measure work accepted the DOC-039 analytic core and split native/source-integrity and later result modes into DOC-040–042. This allocation does not imply release qualification.
The user explicitly included 3MF export and modeled printable threads in the
first stable v1 on 2026-10-17. [XCH-011/#524](../backlog/github.md) and
[SOL-059/#504](../backlog/github.md) own those bounded deliverables. The
[Core 3MF export slice](../validation/3mf-export.md) is accepted on the local
Mac with native and independent output checks. The bounded [modeled external
Thread](../validation/modeled-external-thread.md) also passes on the local Mac;
other thread modes and print fit remain open.
The [DOC-016 native exchange-sheet gate](../validation/native-exchange-sheet.md)
now has independently checked STEP/STL output. The
[File-menu export path](../validation/native-exchange-menu.md) passes; DOC-036/#517
also passes the [body Browser export path](../validation/native-browser-mesh.md).

- [Inventory reconciliation](inventory-reconciliation.md): all 150 rows, remaining broad owners, stale evidence and cumulative release acceptance.
- [Release execution ledger](release-execution-ledger.md): row/subset owners and open native, geometry, save/reopen, export, GPU, package and three-platform evidence slots.
- [Planning scope freeze](scope-reconciliation-2026-10-20.md): required dependency repairs, bounded option choices, outside-closure dispositions and external platform-test handoff.
- [Open-owner-row minimums](open-row-disposition.md): selected first-v1 slice and retained remainder for the 27 rows the dated audit labeled `O`.
- [Command inventory](command-inventory.json): 150 capability rows and 397 IDs in its dated original runtime snapshot, with newer bounded capability amendments and explicit gaps. The added `document.export3mf` command was validated separately in the real app.
- [Inventory notes](inventory-notes.md): definitions and verification limits.
- [Backlog map](backlog-map.json) and [workstreams](backlog-notes.md): dated 484-issue reconciled snapshot, not a live board. It includes corrected TUT-018/TUT-034 acceptance, bounded TUT-035 clearance acceptance and the earlier REL-033–041 responsiveness state. REL-037 has a newer [final Open receipt](../validation/open-final-presentation.md).
- [Iron reassessment](iron-reassessment.md): final shape rejected; public-view comparison and corrected fixture acceptance.

Sequence: the [cumulative adapted enclosure gate](../validation/enclosure-cumulative-gate.md) is accepted → reconcile each v1 gap into a bounded owner → creation/revision reliability → native interaction polish → viewport quality and packaged platform qualification. Close each active issue with its stated evidence before pulling another; split only explicit remaining requirements into linked Todo work.

Site source is external at ../Alloy-reference-records/planning/v1-review-site; hosting identity is retained in its .openai/hosting.json. The site is owner-private. Source/data and local render/filter checks passed, and private deployment succeeded 2026-10-19. Browser visual QA for this revision was unavailable and remains open; an older hosting screenshot does not verify it.
