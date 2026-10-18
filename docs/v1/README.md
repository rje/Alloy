# Printable-part v1 review

Private review site: https://alloy-v1-review.rjevans1.chatgpt.site

The user reviewed this as a reasonable working direction on 2026-10-17. REL-030 still tracks reconciliation of every inventory row with a finishable implementation owner and the final release gate; this is not a claim that the listed gaps are complete. The long-term full noncloud product contract remains intact.

The first ownership pass linked 20 previously broad-only proposed tool areas to
bounded Todo issues [#496–#515](../backlog/github.md). This establishes pullable
owners, not feature completion or a final v1 scope decision. Existing partial
rows still require real-workflow evidence and explicit release-gate disposition.
The [ownership audit](ownership-audit.md) added six missing narrow owners
[#518–#523](../backlog/github.md) and identified broad parent issues that must be
split before implementation. Thirteen partial rows have accepted subsets but
still need an explicit v1-versus-later option decision under REL-030.
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

- [Command inventory](command-inventory.json): 150 capability rows and 397 IDs in its dated original runtime snapshot, with newer bounded capability amendments and explicit gaps. The added `document.export3mf` command was validated separately in the real app.
- [Inventory notes](inventory-notes.md): definitions and verification limits.
- [Backlog map](backlog-map.json) and [workstreams](backlog-notes.md): dated 484-issue reconciled snapshot, not a live board. It includes corrected TUT-018/TUT-034 acceptance, bounded TUT-035 clearance acceptance and the earlier REL-033–041 responsiveness state. REL-037 has a newer [final Open receipt](../validation/open-final-presentation.md).
- [Iron reassessment](iron-reassessment.md): final shape rejected; public-view comparison and corrected fixture acceptance.

Sequence: finish the remaining enclosure gate → reconcile each v1 gap into a bounded owner → creation/revision reliability → native interaction polish → native renderer and packaged platform qualification. Close each active issue with its stated evidence before pulling another; split only explicit remaining requirements into linked Todo work.

Site source is external at ../Alloy-reference-records/planning/v1-review-site; hosting identity is retained in its .openai/hosting.json. Site is owner-private. JavaScript syntax, static serving and data mappings checked; no browser surface was available for interaction QA. The hosting-generated screenshot was inspected: initial page, counts, typography and layout render correctly. Deployment succeeded 2026-10-17.
