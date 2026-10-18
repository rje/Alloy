# Dependency admission — FND-002

Reviewed 2026-10-11. Alloy's original code/docs remain MIT. Astra independently
audited pinned public headers/license files and the installed Qt SPDX inventory;
the primary agent checked the upstream texts and reproduced local dynamic
linkage. This is a dependency selection decision, not package qualification.

[components.json](components.json) records exact research/admission pins and
SHA-256/source/date for retained [license texts](../../LICENSES).
[local-link-audit.json](local-link-audit.json) records actual `otool -L` dependency
listing, local versions, candidate bytes and installed Qt SPDX checksum. It follows
absolute Homebrew library paths; relative `@rpath` entries are retained as unresolved,
so this is not a complete deployed closure or a relocation test.

| Component | License selected | Decision |
| --- | --- | --- |
| Qt 6.11.2 Core/Gui/Widgets/Network | LGPL-3.0-only | Admit shared modules; check each additional module separately |
| OCCT 7.9.3, a016080… | LGPL-2.1-only WITH OCCT-exception-1.0 | Admit shared modeling/visualization libraries; attribution and LGPL obligations remain |
| CI FreeType 2.13.3, 42608f7… | FTL | Admit with bundled-code notices |
| PlaneGCS, ec6852e… | LGPL-2.1-or-later | Separate shared library from thirteen pinned source files; application adapter integration added after FND-005. Runtime packaging remains pending |
| Eigen 5.0.1 | MPL-2.0 with EIGEN_MPL2_ONLY | Solver headers; exact archive fallback and covered-source notices |
| Boost 1.90.0 | BSL-1.0 | Solver Graph/math headers; exact archive fallback and notices |
| OndselSolver1.0.1, 4be80eef… | LGPL-2.1 | Bounded shared assembly backend; original POD/C ABI; macOS core evidence only, packaging/platform replacement pending |
| libslvs, ef96633… | GPL-3.0-or-later | Not admitted under current integration decisions |
| lib3mf 2.5.0, 64bb454… | BSD-2-Clause | Admitted for bounded Core 3MF export; pinned static build and bundled notices, with package/runtime gates still open |

Permissive Alloy source licensing is compatible with an LGPL library boundary
when the applicable distribution conditions are met. Do not replace this choice
with a GPL-only solver or GPL-only Qt module implicitly. Use versioned license
files as the authority, rather than relying on a repository's top-level label.

## Development and native SDKs differ

The local Homebrew link closure includes FreeType 2.14.3, TBB 2023.1.0, QtDBus,
ICU, GLib, OpenSSL and further font/image/compression support libraries. It is
not a three-library package. System frameworks are supplied by macOS. The native
CI source build instead uses pinned FreeType 2.13.3 and disables OCCT TBB plus
external optional FreeType libraries. Qt SDK dependencies/plugins still require
inventory. `FT_DISABLE_ZLIB` does not erase FreeType's bundled-code notices.

Reproduce the host audit with `otool -L build/alloy.app/Contents/MacOS/alloy`, then
follow the actual non-system shared-library paths recursively. Inspect
`/opt/homebrew/opt/qtbase/share/qt/sbom/qtbase-6.11.2.spdx` for that installation;
each native SDK/package needs its own corresponding inventory. This record
does not call every installed Homebrew library an admitted bundled dependency.

## Distribution requirements assigned to the queue

CI-002 must inventory **actual shipped** libraries/plugins per platform and retain
their applicable licenses/attributions, corresponding source/build/patch records
and a selected compliant source-distribution mechanism. Add source archives and
replacement/relinking/installation instructions; MIT application source alone
does not supply all library obligations. Compatible LGPL replacement must be
usable, including any necessary platform re-signing steps. CI-003 verifies that
this material travels with each published package. FND-009 records reproducible
build inputs and support boundaries. Runtime/GPU/relocation acceptance remains
GATE-005, separate from this admission record.

The [PlaneGCS extraction](../../third_party/planegcs/README.md) records the audited
thirteen-file set, SHA-256 checked downloads, Eigen/Boost header pins, standalone
export/logging shims and explicit dated C++20 patch. No unrelated FreeCAD
application code is admitted by this choice. The lib3mf export build retains
libzip (BSD-3-Clause), zlib/cpp-base64 (Zlib), and fast_float MIT notices in
`build-preview-release/third-party-notices/lib3mf`. The bundled compiled libzip
sources are copied there because their copyright years extend beyond the
submodule's top-level LICENSE. The pinned credits list zlib 1.3.0 while the
bundled header identifies 1.3.1; the staged notice comes from that actual
header. CI artifacts and the macOS packaging recipe carry the staged files;
qualified cross-platform bundles remain separate work.

## Primary sources

Retrieved 2026-10-11. Exact pins are in components.json.

- [Qt 6.11 licensing](https://doc.qt.io/qt-6.11/licensing.html),
  [LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations),
  [installation-specific SBOMs](https://doc.qt.io/qt-6.11/sbom.html).
- [OCCT exception](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/OCCT_LGPL_EXCEPTION.txt).
- [FreeType license inventory](https://github.com/freetype/freetype/blob/42608f77f20749dd6ddc9e0536788eaad70ea4b5/LICENSE.TXT).
- [PlaneGCS header](https://github.com/FreeCAD/FreeCAD/blob/ec6852e6caadf55045881a6003cc7ad038c8d2a7/src/Mod/Sketcher/App/planegcs/GCS.h),
  [libslvs grant](https://github.com/solvespace/solvespace/blob/ef966335e7fbc617ba8e759149a567158b37aa7b/README.md).
- [lib3mf license](https://github.com/3MFConsortium/lib3mf/blob/64bb454d1fcb53effa57d3cef752a10d740d41a2/LICENSE),
  [credits](https://github.com/3MFConsortium/lib3mf/blob/64bb454d1fcb53effa57d3cef752a10d740d41a2/SDK/Credits.txt),
  [included zlib version](https://github.com/3MFConsortium/lib3mf/blob/64bb454d1fcb53effa57d3cef752a10d740d41a2/Libraries/zlib/Include/zlib.h).

[Decision 004](../decisions/004-sketch-solver.md) records the original PlaneGCS
selection fixture. The application adapter and focused solver test are now built
by CMake; application workflow evidence must qualify each claimed constraint and
dimension feature. The historical local link audit above predates this library.
Deployed runtime closure remains separate work.
