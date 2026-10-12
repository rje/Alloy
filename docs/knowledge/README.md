# Workflow knowledge

The portable workflow records, source registry, visual references, and derived
SQLite index live outside this checkout in `../Alloy-reference-records/knowledge/`.
This keeps reference-specific names, URLs, and images out of the public source
tree. The JSON records are the editable source of truth; `knowledge.sqlite` is
reproducible and disposable.

Use the stdlib-only client from the repository root:

```sh
python3 tools/knowledge.py validate
python3 tools/knowledge.py build
python3 tools/knowledge.py search 'lost support'
python3 tools/knowledge.py search 'circle arc radius diameter'
python3 tools/knowledge.py show sketch.projected-geometry
```

The default corpus path is the sibling reference archive. Use `--root PATH` for
another corpus and `--db PATH` to place or query the generated index elsewhere.
Search accepts SQLite FTS5 syntax. Add or revise JSON under the external
`records/` directory, attach evidence to a stable source ID, record unknowns
and implementation proposals separately, then validate and rebuild. Store
visual captures and their provenance in the external `visuals/` directory;
these references are for comparison and are not application assets.

Each record includes an evidence list, a structured workflow, edge cases,
dependencies, uncertainty, verification scenarios, and related Alloy issue
keys. Evidence types distinguish documented facts, observed captures, inference,
and proposals. A record is a bounded research aid, not a claim of complete
behavior coverage.

Initial library acceptance (2026-10-12): six workflow records and nine archived
public images. Luna researched and authored the corpus; Astra independently
checked high-risk claims and four representative images. Record validation,
source/image linkage, hashes, index payload consistency and focused search passed.
This qualifies the initial library, not exhaustive compatibility or app behavior.
The external visual manifest records expected state, source, retrieval date,
image dimensions, and comparison limits; unknown build/theme/scale remain unknown.

For visual comparison, open
`../Alloy-reference-records/knowledge/visuals/index.html`. The external contact
sheet links full-size references and lists each expected state, source, date and
comparison limits. It is a convenience view of the manifest, not a separate
source of evidence. New batches remain provisional until independently reviewed.

The expanded batch is independently accepted on 2026-10-12: eight records and
twelve images, adding solid fillet/chamfer and a beginner face-sketch/Cut/Join
recipe. Astra checked source attribution, operation labels, image provenance and
index consistency. Tutorial execution and Alloy visual parity remain unverified.

The exact illustrated bracket tutorial has a separate source-step audit at
`../Alloy-reference-records/knowledge/tutorials/illustrated-bracket/audit.json`
and a scene/command-icon provenance manifest beside it. This audit is not yet a
workflow-record entry in the SQLite index. The public implementation contract is
[the illustrated bracket gate](../research/illustrated-bracket-gate.md); TUT-003
tracks exact execution and constraint-icon/visibility acceptance.
