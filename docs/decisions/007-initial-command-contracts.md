# 007 — Initial command contract research

Date: 2026-10-24. Status: bounded public-source research ready for independent review.

The initial sketch-to-solid command contract baseline and unresolved-default ledger
are maintained in the external evidence record referenced by
[the lightweight command index](../research/commands/initial-command-index.json).
The research covers sketch entry and finish, line/rectangle/circle, constraints and
dimensions, Extrude, Fillet, parameters, feature timeline, and mesh export.

The source registry records current public help/API sources and retrieval date.
Stable source IDs and external visual IDs are the only references retained here;
source names, URLs, images, and detailed workflow notes remain outside the repository.
Documented behavior, screenshot observations, API facts, Alloy policy, inference,
and unknowns are kept separate. The command-ID crosswalk is pinned to Alloy commit
`1160d0407a02cfaf39af59727599fa1409352456`. No proprietary application runtime,
Alloy execution, geometry, GUI, or export result is claimed by this research.

The reviewed pages leave initial picker/tool variants and numeric defaults,
several cancel/error details, and timeline state unspecified. Those are explicit
stable unknowns in the external ledger. Alloy has a registered `document.exportMeshDialog` action shared by the File menu
and Browser context route. The open remaining native chooser and format-breadth
obligations are owned by REL-053/#607, XCH-012/#604 and XCH-013/#605; the accepted
DOC-036/#517 receipt covers keyboard traversal to the chooser, not its OS save
completion. Existing open owners and exact remaining work are recorded in the ledger.
