# Publication cleanup verification

Verified 2026-10-11 at rewritten main `35cf641dbf4ea6a5ecb6b40b2aef9431f93dd316`.
The user authorized neutral public terminology and a complete history rewrite.
All six commits were transformed; file paths, URLs, prose and the coincidentally
named Qt widget style were included. The rewritten branch was force-pushed with
an exact old-head lease. No other branches or tags existed.

The primary agent and Astra independently checked:

- Every local Git object and a fresh remote mirror: 146 objects, six commits,
  with no case-insensitive removed-name matches in blobs, trees or messages.
- Current tracked paths/content, relative Markdown links and the 135-issue graph.
- All 135 GitHub issue titles and bodies, including exact authored-body matching.
- All three current evidence comments, project metadata/fields and 135 board items.
- Neutral shell source hashes against the current files; the rebuilt real-app
  smoke passed 1/1 and still identified Apple M4 hardware rendering.

The earlier Actions run/artifacts and old evidence comments were deleted.
Astra found stale release status, which was corrected after the audit. Reference
URLs and images remain in separate local records; public IDs, dates and hashes
preserve the research chain without adding reference imagery to application assets.

This verifies the reachable repository history and current planning records.
GitHub can retain old cached commit views and issue-body edit revisions outside
advertised Git history. The documented API has no revision-deletion mutation;
no complete provider-cache purge is claimed. Removing a name does not establish
copyright compliance. The clean-room boundary and independently authored assets
remain required. See [reference provenance](../research/reference-records.md).
