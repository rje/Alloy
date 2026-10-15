# Local macOS arm64 relocatable preview archive (CI-005)

The bounded [`package_macos.py`](../../tools/ci/package_macos.py) recipe stages a
copy of a native `alloy.app`, invokes `macdeployqt` from the *same* coherent
private Qt SDK named by the source app's runpath, bundles the local control
client, OCCT resources and exact deployed notices, and writes a relative
`qt.conf`. It removes remaining development runpaths, checks every staged
Mach-O dependency against actual owner/executable runpaths, ad-hoc signs the
staged bundle, and writes a ZIP with `ditto`. The source app and SDK remain
untouched. The archive includes replacement/source records and the two Qt
patches. Its Qt source tarball is identified by URL/hash in those records;
the tarball itself is not in this local archive.

To make a local candidate from an already validated arm64 app:

```sh
python3 tools/ci/package_macos.py \
  --app build-tutorial-polish/alloy.app \
  --qt-prefix ../Alloy-dependencies/qt-private-runtime/install \
  --work ../Alloy-reference-records/deliverables/mac-package-2026-10-14/new-candidate
```

The accepted local candidate was staged from pinned input app SHA-256
`ffa37a782784962ecf150a4d67617fdc81ca1cd67689ff063156aca685d85020`
and local-client SHA-256
`567b01abc73fe01a81251fa0c2d87993f0dd7a5462fecead050d6241e095c173`.
The final archive is
`../Alloy-reference-records/deliverables/mac-package-2026-10-14/candidate-03/Alloy-macos-arm64-preview.zip`,
SHA-256 `b46fde7612e49f5ca861f7629a6dd6e7217cea26b67220505f393cc60efdee41`.
Its `package-candidate.json` records 55 audited Mach-O entries. Extraction
to the separate `relocated-03` directory preserved 15 framework symlinks and
the executable mode; strict deep code-sign verification passed. The extracted
app's `qt.conf` has relative `Prefix=..` and `Plugins=PlugIns`.

The final extracted copy launched with SDK and plugin environment variables
removed. `relocated-03/run.log` shows the **bundled** Cocoa plugin loaded;
`loaded-paths.json` records 43 non-system loaded libraries/plugins, all from
the extracted app and zero from Homebrew or the build SDK. `final-state.json`
reports hardware acceleration on Apple M4 with OCCT OpenGL 4.1 Metal. The
bundle's Python client and bundled Qt transport helper reached the same live
local control API. Candidate-02, built from the same pinned source binary and
matching SDK, exercised one 40×30×10 mm box through save and binary STL export:
12,000 mm³, 12 STL triangles, valid 684-byte STL, and readable GPU capture in
`relocated-02/relocated-box.png`. The final candidate differs in notices and
ad-hoc app signature; 54 of 55 Mach-O hashes match candidate-02, while its
signed main executable has the same pinned source binary. Candidate-03's own
extracted launch, bundled path check and GPU state passed; the save/export
operation was not repeated after the notices-only archive change.

The bundle records actual deployed Homebrew versions OCCT 7.9.3_1, FreeType
2.14.3, TBB 2023.1.0 and libpng 1.6.59, distinct from native CI's FreeType
2.13.3. Its legacy local Qt SDK predates CI-008's clean success manifest; the
package recipe verified the source app's SDK runpath and both accepted patched
source hashes, and included framework/plugin and patch/archive hashes. It does
not present the different CI-008 SDK manifest as provenance for this app.

This proves local arm64 relocation on this host, loaded runtime closure for the
observed workflow, GPU rendering and local API/save/export. It does not prove
clean-host operation while those libraries remain installed, Intel, Windows or
Linux, physical inputs, accessibility history, signing/notarization, full
source-access compliance, or a publishable release. CI-002/GATE-005 and the
platform issues retain those gates. Ad-hoc signing is local, not a trusted
Developer ID signature.
