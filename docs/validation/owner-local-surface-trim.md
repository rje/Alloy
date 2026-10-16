# Owner-local sketch Trim

Accepted 2026-10-15, SRF-021/#463, commit `3c7dda8`.

Creating the handle lens inside the active iron component previously previewed
correctly but failed Trim commit because admission allowed only root sketches.
The shared evaluator now also accepts a same-owner sketch in one direct,
identity-placed component. Existing root-sketch and same-owner sheet behavior
remains supported; foreign, linked and placed sketch sources remain outside this
bounded addition. No geometry algorithm or native control changed.

The focused core verifies identical geometry and retained region IDs for the
same-owner cutter, preserves the root route and refuses a foreign owner.
The actual from-blank run activates the component, authors its lens, commits the
two-sided Trim and builds the retained handle bridge. Both resulting bodies are
valid two-face surfaces in that component; the preview capture is readable.
Astra independently accepted the source and this bounded runtime evidence.

External evidence: `surface-iron-continuation-2026-10-15/owner-local-trim-01/`
and `from-blank-02/transcript.json`, with captures `06-opening-preview.png`,
`08-inner-preview.png` and `09-bridge.png`. That cumulative run subsequently
failed at separate extrusion-chain selection, tracked by SRF-022; it is not a
completed tutorial receipt. This correction does not establish general
cross-component modeling or new physical-input coverage.
