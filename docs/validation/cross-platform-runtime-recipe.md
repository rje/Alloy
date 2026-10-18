# Windows/Linux source Qt recipe candidate

CI-009/#427 candidate, 2026-10-18. This receipt records source-policy and
cache-integrity checks on macOS. Windows/Linux native compilation and the
offscreen regression still require a deliberate native matrix. No application
was launched, and no packaging, native input or GPU acceptance is claimed.
An independent Astra source review accepted the bounded patch policy and
identified missing failed-build diagnostics and stale CI-008 wording; both
were corrected before the matrix. The patch file is forced to LF on checkout
for the Windows `git apply` path. Native acceptance remains open.

## Admitted input policy

The pinned public Qtbase 6.11.2 archive has SHA-256
`06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414`.
Its `src/widgets/CMakeLists.txt` includes `accessible/itemviews.cpp` under
`QT_FEATURE_accessibility AND QT_FEATURE_itemviews`, without a platform
restriction. The item-view cache patch therefore applies to the shared Widgets
implementation on Windows and Linux. Its patched source hash is
`1c1b20c1c4d4c0a73c22036ca656466558b015ec8eb226fd59534e82d0eba7c5`,
the same admitted source as [CI-008](mac-runtime-recipe.md).

Qt's `src/plugins/platforms/CMakeLists.txt` adds Cocoa only under `MACOS`.
Windows/Linux builds omit that patch and retain the pristine Cocoa source
hash `1e1c3699f9de098a0c4979128ce8f43779976416ef3d41ca875073a2a4b26f27`.
This is source-level applicability evidence, not observation of a Windows or
Linux accessibility service. Qt source is public library evidence:
[Qtbase 6.11.2](https://github.com/qt/qtbase/tree/v6.11.2), retrieved 2026-10-18.

## Implementation and retained provenance

[`build_native_qt.py`](../../tools/dependencies/build_native_qt.py) builds
coherent native Qt libraries and plugins from that archive using Ninja, with
the shared item-view patch. It requires accessibility, item views, Widgets,
Network, Test, FreeType and OpenGL; Linux additionally requires XCB/GLX,
Fontconfig, D-Bus and the ATSPI bridge. Windows requires the x64 MSVC developer
environment. The existing macOS recipe remains separate.

The input manifest pins archive/patch/recipe/regression hashes, requested
configuration, compiler path/binary/version, build-tool versions and Windows
SDK environment or Linux installed package versions. The completion manifest
records effective CMake options and compiler configuration, complete patched
source-tree and installed-file identities, plugin and license inventories,
`qtpaths --query`, and the offscreen regression evidence. A success stamp is
written only after compilation, install and regression pass. A reused cache
must match the recipe, archive, source tree, installed outputs, CMake cache and
regression evidence. Incomplete caches require a new empty work root.
Failed native Qt builds retain the available input, CMake/toolchain and
regression evidence in the CI artifact while preserving the original error.

[`tools/ci/build.py`](../../tools/ci/build.py) now selects this source recipe
on Windows/Linux instead of stock archives, retains the regression's text/JUnit
results and binary, and hashes the resulting manifest into `build.json`.
`dependency-build.json` records actual pinned OCCT/FreeType checkout revisions,
their installed files, application/solver library hashes, effective dependency
and application CMake caches, embedded DejaVu fonts/notices, build contracts,
and read-only native link inspection. The Qt manifest independently inventories
all installed Qt plugins. Linux system dependency versions are recorded rather
than claimed to be fully pinned or bundled. Binary reproducibility is unproven.

Build prerequisites were checked against Qt's public
[Linux requirements](https://doc.qt.io/qt-6/linux-requirements.html) and
[Windows source-build instructions](https://doc.qt.io/qt-6/windows-building.html),
retrieved 2026-10-18. These rolling pages currently describe Qt 6.12; the pinned
6.11.2 archive controls source applicability and actual feature names. The
native workflow retains tag/manual dispatch and explicitly initializes MSVC
and the Linux XCB/ATSPI development dependencies.

## Checks and remaining gate

The pinned archive was freshly extracted to a temporary external source tree;
`git apply --check` and actual patch application succeeded, and the resulting
item-view hash matched. The Cocoa source remained byte-identical to the
archive. Relevant source build conditions and hashes are retained externally
at `../Alloy-reference-records/deliverables/runtime-reproducibility-2026-10-18-ci009/source-policy-check.json`.
The temporary source tree was removed afterward.

`python3 tools/dependencies/test_native_qt_recipe.py -v` passed 4 tests,
including refusal of changed source/plugin/configuration/regression evidence,
a changed recipe and an incomplete stamp. Python compilation and
`git diff --check` passed. These checks do not execute the native build recipe.

Before closing CI-009, run the selected exact commit through the manual native
matrix and inspect Windows/Linux Qt success manifests, effective mandatory
features/compiler baselines, retained item-view regression results, dependency
inventory and Alloy compilation/link evidence. Preserve any failures before
bounded correction. Independent review remains required. Packaging and actual
installed-runtime/GPU/input qualification remain CI-006/007 and GATE-005; they
are not part of source-policy admission.
