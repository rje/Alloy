# Retained local body names — DOC-030 candidate

The shared `body.rename` command stores a display-name override by stable current
local body ID. Geometry, feature labels, owner placement and topology are evaluated
from the original feature graph. A missing/imported body or invalid name refuses
atomically; an upstream edit that removes a named body also refuses. Historical
prefix evaluations omit final display-name overrides until the body exists.

The accepted ported Case Split fixture passes the real-app API run in
`../Alloy-reference-records/deliverables/body-rename-2026-10-14/api-04/`:
Bottom/Top names appear on the exact two bodies; body IDs, bounds, volumes and
authored feature records are unchanged. Unknown ID refusal, undo/redo and native
save/reopen pass. The Qt Browser context action opens a Rename dialog and Escape
discards it, but the current macOS Qt test could not establish line-edit focus
after context-menu selection; typed native commit and a readable Browser name
capture remain open. Failed attempts are labelled `native-01` through `native-05`
in the same external deliverable. This candidate does not claim that input gate.

The public enclosure source requires naming the split halves Top and Bottom;
dated source evidence is kept outside Git in the enclosure split/snap contract.
