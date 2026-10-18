# Large-model Open final presentation

REL-037/#480, 2026-10-17. On the accepted saved corrected-iron fixture, the
background Open result is still adopted atomically into the active design tab.
The GUI now frames visible bodies using the exact `AddOptimal` bounds already
computed during worker evaluation; malformed or absent bounds fall back to
the kernel traversal. Sketches and canvases retain their existing Fit route.
`command.status` retains its complete document result. An opt-in
`ALLOY_TRACE_OPEN_PRESENTATION=1` log separates adoption, rebind, canvas,
Fit, result state, refresh and recovery time without changing the normal UI.

The current-build diagnostic baseline completed its terminal poll in 520–578 ms
across two runs; its Fit phase was 70–71 ms. The final candidate took 469.9 ms
in the acceptance run (469.4 ms in a preceding run), with Fit at 8.7–11.5 ms.
The prior REL-036 receipt measured 1,279 ms on an earlier build; this change
does **not** account for that entire difference. Final adoption still has a
visible roughly 0.4 s pause on this Mac. A concurrent local `app.state`
probe had 265 successful heartbeat replies through background work and final
adoption; the maximum final-window round trip was 415.5 ms, with one sample
over 200 ms. This is a measured bounded improvement, not an all-model latency
or physical-input claim.

The same real-app replay checked exact authored JSON against the
saved 12-feature fixture, unchanged active tab ID/count, a new document ID,
clean loaded state and cleared history. A post-Open units edit undid and redid
exactly, then returned to the source state. The final body retained its
856172.8343589634 mm³ volume, 20 edges, eight faces and one solid, with the
recorded exact bounds. The GPU capture is readable and framed. The existing
Open-control test passed, including failure, Cancel, stale result, loaded
authored output and recovery contract. The targeted Fit-control test passed.

External evidence is
`../Alloy-reference-records/validation/rel037-open-presentation-2026-10-17/`:
`baseline2/` and `phase2/` hold current-build diagnostic baselines;
`final/receipt.json` and `opened.png` hold final geometry and visual evidence;
`acceptance/receipt.json`, `app.log` and `opened.png` hold the concurrent
heartbeat, tab/history and phase timings. The tested app SHA-256 is
`e8df19d40009a8f292c858092beb351a265a305fe32e1479a96e5c01cea6b19e`;
the fixture SHA-256 is
`9a9720f0a167d4e6cb9523b83d02228e8536b096d01c5c0950ff9bfd13fbe108`.
The private editor closed. Startup, STEP import, Windows/Linux GPU runtime
and broader responsiveness remain under their existing gates.
