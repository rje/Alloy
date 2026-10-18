# Package and observed-runtime inventory (CI-012)

`tools/ci/runtime_inventory.py` validates an inspectable join of source records,
available pre-deployment binary hashes, every packaged file/symlink, notices and
retained process load evidence. It reads the package and archive without changing
or launching them. This is an inventory join, not an SBOM, a reproducible-build
claim, source-access review or release/platform qualification.

The first real input is the already accepted navigation archive from
[CI-005](macos-relocatable-preview.md). It remains byte-for-byte unchanged:

- Archive: `../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-candidate-01/Alloy-macos-arm64-preview.zip`
- SHA-256: `c5eedaabacf504c845cf42973f536afa0af21c6bf97e909b5cc2130122ca2fa9`
- Extraction and historical runtime evidence: sibling `navigation-relocated-01/`.
- Inventory, pinned source contract and validator result:
  `../Alloy-reference-records/deliverables/runtime-inventory-2026-10-18/`.

Reproduce the bounded adapter and validation from the repository root:

```sh
python3 tools/ci/macos_inventory.py \
  --candidate ../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-candidate-01 \
  --relocated ../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-relocated-01 \
  --output ../Alloy-reference-records/deliverables/runtime-inventory-2026-10-18
python3 tools/ci/runtime_inventory.py \
  --manifest ../Alloy-reference-records/deliverables/runtime-inventory-2026-10-18/manifest.json \
  --package ../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-relocated-01/Alloy.app \
  --archive ../Alloy-reference-records/deliverables/mac-package-2026-10-14/navigation-candidate-01/Alloy-macos-arm64-preview.zip \
  --result ../Alloy-reference-records/deliverables/runtime-inventory-2026-10-18/result.json
python3 -m unittest discover -s tools/ci -p 'test_runtime_inventory.py' -v
```

## Evidence and limits

The 2026-10-18 validation retained these exact SHA-256 receipts:

| Artifact | SHA-256 |
| --- | --- |
| `manifest.json` | `b6543efd80d06f8d1f4012a52288e94844f74b7b0868f1ae7be81bfe8dbf644e` |
| `source-contract.json` | `7d9c13323eab2f3f607103376c5806bbf397523c1ea0360e2ab92545d7bde67d` |
| `result.json` | `853aae8b261cf7ae961d03b620e844e401533f7b837fee056334471c12caf2fd` |
| Validator script | `151316f0eb4871b25df4a9eaeced4f7dfd14f4241497cf7434d5466e325062cb` |

The real validation passes for 14 component families, 265 package entries and
43 observed non-system images. Every file and symlink is compared with the
unchanged ZIP, and every Mach-O hash is also compared with the original package
receipt. The observed paths must match both `loaded-paths.json` and the raw dyld
trace. Package symlinks and observed runtime paths must resolve within the app.
Cocoa, Qt, the app, OCCT, FreeType, PlaneGCS, the assembly adapter and Ondsel must
have real observed paths. The trace is the historical accepted app run; this
work does not rerun its CAD/GPU workflow or establish use of every plugin.

The manifest derives version/source records from the archive's
`BUNDLED_COMPONENTS.json`, packaged Qt recipe/patch records, and packaged solver
source manifests. The legacy package's exact source revision is `a355c50` from
its runtime receipt. `source-contract.json` snapshots that commit's CMake and
font source hashes using `git show`, rather than consulting today's build tree.
The hash of each evidence file is checked before its links are resolved.

The package predates `dependency-build.json` and the coherent private-Qt success
manifest. Those records are **unavailable for this archive**; a newer CI receipt
is not substituted. Its available built-input hashes cover the source executable,
QtCore (with the other recorded framework hashes in its source evidence), Cocoa
and representative OCCT/FreeType/TBB/libpng libraries. Solver and other plugin
built-input hashes remain explicitly unavailable. Source-input and packaged
hashes are distinct because loader edits and ad-hoc signing change binaries.
These limitations do not fail the legacy inventory join; a missing required
present file, source link, notice, recorded hash or observed runtime does fail.
They do leave the current-build provenance and release gates open.

The two DejaVu 2.37 source fonts are declared as embedded resources by the pinned
CMake contract. Their source hashes, containing packaged executable and actual
packaged font license are joined. Individual embedded font bytes and font usage
were not extracted/observed; this is recorded explicitly in the font row.
The `lib3mf` row is **not applicable to this revision**, whose pinned build
contract does not include it. Today's application does include it, so a future
package of that source must provide its source/dependency provenance and exact
packaged `lib3mf` notices; the macOS validator refuses a contract containing
lib3mf without those entries. The accepted old archive is not modified to add
a library it never shipped.

The package/load coverage is finite evidence for one existing local arm64 app.
It does not close CI-004's current-build provenance, CI-006/007, CI-002/003,
clean-host, physical input/GPU or trusted-signing/release gates.

## Reusable schema and platform invocation

`alloy-runtime-inventory/1` is a JSON evidence-index contract:

| Field | Contract |
| --- | --- |
| `target` | Native target identifier, currently `macos-arm64`, `macos-x64`, `windows-x64` or `linux-x64`. |
| `archive_sha256`, `archive_package_root` | Exact ZIP checksum and its package-root member name. |
| `observed_package_root` | Absolute package root at the time the retained load observation was made. |
| `evidence` | Named records with file `path` and SHA-256. Paths locate evidence; they do not assert runtime loads. |
| Evidence reference | `{ "evidence": "id", "pointer": "/json/path" }`. Empty pointer means the whole JSON document for `.json`, otherwise whole UTF-8 text. JSON pointers support escaped `/` and `~`. |
| `components[]` | Stable `id`, `role`, `source` evidence reference, nonempty relative `notices`, `built_input`, packaged `files` and `runtime`. Each package entry belongs to exactly one family. |
| `built_input` | `recorded` with a SHA-256 and resolving evidence reference, or `unavailable` with a reason. A representative hash states its scope. |
| `files[]` | Package-relative `path`, `kind` (`file`/`symlink`), SHA-256 of bytes or symlink target text. Internal directory symlinks are included. |
| `runtime` | `observed` with absolute image paths owned by that component, `not-observed` with reason, or `nonloaded-resource` with reason. Observed family membership does not assert that all its files were loaded. |
| `load_observation` | Evidence links `paths` (non-system image list), `outside` (must be empty), and macOS `raw` (dyld trace). |
| `limits` | Explicit unavailable provenance and unqualified behavior. |

The common invocation for later platforms is the same:

```sh
python3 tools/ci/runtime_inventory.py --manifest <manifest.json> \
  --package <extracted-package-root> --archive <unchanged-package.zip> \
  --result <result.json>
```

The common file/hash/source/notice/load checks are portable; the only existing
platform adapter and required-component/raw-trace policy is macOS arm64.
Windows/Linux must add their native package adapters and required-component
policies, use their own actual loader observations (including OS-library
classification), and join their existing `dependency-build.json`, private Qt
success manifest, build receipt and package receipt. Do not reuse this archive's
legacy unavailable exemptions for a new native build. A generic inventory result
alone cannot qualify those platforms; CI-006/CI-007 retain that work. Native
build scripts continue to own source/dependency facts rather than introducing
another independently maintained dependency version catalog.
