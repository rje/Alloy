# Bounded assembly preview reuse

REL-010 reduces repeated coupled-joint solving during one preview command and refresh. One graph evaluation returns the same inspected joints/occurrences and placed bodies. The UI shares it only while that command is active, with exact authored, evaluated-definition, tab, document revision and staged-coordinate identity. Pending reference moves use the previous independent paths. Window and tab labels read the document label directly, avoiding full-state assembly evaluation.

The focused actual 3R+1C/Pin core test checks signed motion, original relationships, history/reopen and unchanged inspection/body summaries. It counts one graph solve for a combined read versus two for separate reads. The local seven-joint API run checks ±15° preview, Revert and invalid-angle refusal; each preview used one solve in the command and none in refresh. Raw timings and limitations are in the external `../Alloy-reference-records/deliverables/assembly-preview-reuse-2026-10-14/receipt.md`.

This does not establish continuous native drag frame rate. The desktop input gate could not attach to the Alloy window, so pointer responsiveness remains a separate validation requirement. Candidate validation outside the scoped UI presentation may still perform its own solve.

Independent Astra review accepted `bd71bc0`, integrated at `00b0990`. The main application build and focused coupled core pass. REL-010 closes only this implementation slice; REL-011/#374 retains native drag profiling and visual responsiveness validation.
