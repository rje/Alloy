# Printable-part v1 review

Private review site: https://alloy-v1-review.rjevans1.chatgpt.site

This is a proposed tool/release contract, pending user review under REL-030. The long-term full noncloud product contract remains intact.

- [Command inventory](command-inventory.json): 150 capability rows and 397 runtime-cross-checked IDs, with explicit gaps and evidence.
- [Inventory notes](inventory-notes.md): definitions and verification limits.
- [Backlog map](backlog-map.json) and [workstreams](backlog-notes.md): dated 470-issue reconciled snapshot, not a live board. It includes the TUT-018 reopening and REL-030/TUT-034.
- [Iron reassessment](iron-reassessment.md): final shape rejected; public-view comparison and corrected fixture acceptance.

Sequence: finish iron and enclosure → review v1 scope → split accepted gaps into bounded owners → creation/revision reliability → native interaction polish → native renderer and packaged platform qualification. Do not pull later tutorial breadth merely to accumulate closed issues.

Site source is external at ../Alloy-reference-records/planning/v1-review-site; hosting identity is retained in its .openai/hosting.json. Site is owner-private. JavaScript syntax, static serving and data mappings checked; no browser surface was available for visual QA. Deployment succeeded 2026-10-17.
