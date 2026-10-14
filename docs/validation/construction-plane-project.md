# Linked Project on retained construction planes

SKT-072/#325 is independently accepted and integrated (2026-10-13). Main app
build and focused linked Project integration test pass. The 26-call real-app
run and capture are API evidence; existing Project controls are reused, and no
new physical native-input coverage is claimed.

A sketch on an earlier construction plane resolves that plane's current frame and can Project exact current model edges from an earlier active body. The shared Project command accepts a planar source face or individual supported edges, retains source body/edge and imported occurrence identities, and refuses missing or dimension-changing sources. Face-sketch legacy boundary references remain face-only. Identity-root sibling ownership and earlier-body ordering are checked by the document validator.

The focused `linked_planar_project` core check covers a zero-offset face-derived plane, four projected lines, a retained plane edit, stable IDs, existing face and sibling point projection, save/reopen and stale-source refusal. The real-entrypoint `tests/construction_plane_project_api.py` reopens the accepted enclosure prefix, derives an Offset Plane from its Case front wall, projects the four actual imported connector-face edges, cancels and recommits, edits the plane by 0.25 mm, undoes/redoes, saves and reopens. It asserts a selectable profile and unchanged source/derived identities. The local result is in `build-tut021/evidence/skt072/final/` and its screenshot shows purple locked projected edges.

This validates the construction-plane Project ingredient. Closed-loop offsets of the three projected connector rectangles are tracked separately before the port-cut workflow can complete.
