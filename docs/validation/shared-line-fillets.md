# SKT-039 paired shared-line fillets

2026-10-13 candidate for #221, source base de46402. Public source ID
sweep-paper-clip-warwick-2017-20261013; detailed source contract and annotation
clarification remain outside the repository in the independent paper-clip record.
This is a bounded fillet prerequisite, not a full paper-clip tutorial gate.

## Implementation contract

Plan corner setbacks against original lines and resolve both endpoint edits
before mutation. Negative residuals refuse atomically. Exact equal-radius returns
retire the exhausted segment and share actual center/contact point IDs between
both authored quarter arcs. Existing arc rules imply equal radii through the
shared center/contact, so no redundant Equal equation is added for that pair.
Tangencies to surviving outer lines remain explicit. No handcrafted semicircle,
zero-length line, virtual corner, hidden fixed equation or new solver type is used.
Noncoincident return centers and unsupported dependency repair refuse.

Surviving source line IDs and all six new arc IDs persist. A consumed horizontal
width dimension becomes an existing driven point-pair horizontal span between
the surviving outer tangent endpoints, preserving its dimension ID and any
annotation placement. Removing its driving value is an explicit Alloy policy:
the radii now determine that span. The source shows the visible 8 span in rounded
preview; post-Enter drivenness/storage are unknown. Reference parentheses honestly
show Alloy's adaptation. Analogous 14/11 spans follow the same policy.

Affected original line lengths retire; consumed line H/V and obsolete corner
coincidence constraints retire with explicit IDs. Preview reports converted
reference dimension IDs, retired dimension/constraint/entity IDs and visibly
explains the changes before Enter. Cancel changes neither authored draft nor
history. Unrelated references are preserved or refuse atomically. A supported
radius edit changes the reference width without resurrecting a retired line.

## Focused evidence

Core source fixture contains initial H/V and all seven length dimensions plus
one fixed origin. Real paired R7, R5.5 and R4 operations consume the 14/11/8 lines.
Four surviving source line IDs and six quarter arcs yield path length
135 + 16.5*pi = 186.8362787842316 mm. Core verifies negative residual refusal,
unrelated dependent geometry refusal and 7→6.5 radius edit with shared identities
and oriented G1 derivative continuity. No fully constrained claim is made.
Existing nonexhausted fillets pass the sketch_curves regression.

API runner authors the constrained source seed, selects actual corners through
viewport-space commands, previews/Cancel/accepts each pair, verifies unchanged
preview/history, negative residual refusal, converted dimension IDs, analytic
length, supported edit, exact undo/redo and authored save/reopen. This setup is
explicit API evidence, not typed chained-line evidence (tracked separately).

Native Qt case injects actual viewport mouse picks for all six corners, types
7/5.5/4 into the real radius editor and presses Enter. It captures the final R4
preview with (8), (14), (11), all returns and the conversion/retirement notice.
Qt events are not physical OS input evidence. Two early attempts exposed an
out-of-viewport top corner in test framing; reducing zoom resolved that issue.
Initial successful capture exposed a clipped notice; its form sizing/text were
corrected and the affected native case repeated. No unrelated full replay.

Evidence paths under `build-shared-fillets/evidence/shared-fillets/`:
`core.log`, `native.log`, `native-r4-preview.png`, and
`api/{report.json,paired-returns.alloy,pair-1-preview.png,pair-2-preview.png,
pair-3-preview.png,all-returns.png}`. API process exited cleanly. Final native capture was inspected: all three reference spans and the complete
conversion/retirement notice are readable. Final native Qt result: 3/3 in
5.202 seconds; final focused core including the explicit contact-agreement guard: 1/1 in
0.63 seconds. Independent acceptance is recorded below. GUI slot released; no further app replay planned.

## Independent acceptance

Astra independently reviewed candidate `3b32a276f8f22100af0ba253f9537c7adc3d9b4b`
on 2026-10-13. Source, focused core and Qt logs, API runner/report, saved authored
model and final native preview support bounded #221 acceptance. No new app or
test run was needed. The saved document exactly matches the report's authored
JSON; its measured path length is 186.83627878423158 mm.

Original-line trim planning precedes mutation. Exhaustion requires two trims,
agreeing contacts and equal-radius return centers; surviving line IDs persist.
Shared actual center/contact identities avoid a redundant Equal equation, while
outer arc-line tangencies remain explicit. The tested radius edit retains
oriented G1 continuity. Unsupported references and negative residuals refuse
without changing the accepted draft. The implementation does not promise
general arc/arc fillets or arbitrary dependency repair.

The final actual Qt capture shows complete returns, readable (14), (11), (8),
and the conversion/retirement notice before acceptance. The open-profile warning
is truthful for this path, though not specific to Sweep. The Qt case uses API
seed/tool activation followed by actual viewport picks, typed radii and Return;
it does not establish chained-line authoring or physical OS input. API evidence
separately covers preview/Cancel, refusal, history, radius edit and exact reopen.
The source's visible 8-span is preserved as an explicitly driven-by-geometry
reference adaptation; source post-acceptance drivenness remains unknown. Full
source authoring and the cumulative Sweep tutorial remain separate gates.
