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
save/reopen pass. The focused DOC-031 Qt test now passes 3/3 on the unchanged
product dialog. It delivers a Browser `QContextMenuEvent`, clicks the actual
Rename menu item, and verifies active modal, line-edit focus and selected name
before typing `Bottom` and pressing Return. It separately opens the dialog and
presses Escape before typing, leaving authored data unchanged. The final Browser
label is readable and the solid geometry is unchanged. One initial application
activation occurs before the demonstrated inputs; no focus repair occurs between
steps. Evidence is in external `../Alloy-reference-records/deliverables/body-rename-2026-10-14/native-09/`.

Earlier focused attempts `native-01` through `native-07` saw focus settle on an
unnamed button. A bounded `native-08` event trace on the unchanged product instead
recorded dialog WindowActivate followed by QLineEdit FocusIn; a no-trace `native-09`
rerun passed. The earlier failure remains unexplained and was not reproduced in
the final run. This acceptance establishes the Qt-delivered Browser route, not
OS right-click parity or a product focus correction.

The public enclosure source requires naming the split halves Top and Bottom;
dated source evidence is kept outside Git in the enclosure split/snap contract.

Astra accepted bounded authoring/API and context open/Cancel scope on 2026-10-14
at `2c86584` (integrated `cc52857`). DOC-031 native Qt validation was accepted
on 2026-10-14 with the qualified focus evidence above. The original receipt refused named body consumption. The current
[SOL-071 accepted slice](combine-naming-recovery.md) retains dormant tool names for
supported Combine consumption; it does not clean up unknown names automatically.
