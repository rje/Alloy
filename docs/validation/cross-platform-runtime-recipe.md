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

The first manual matrix at commit `b663b4d` is
[`39125613075`](https://github.com/rje/Alloy/actions/runs/39125613075).
Its Windows job stopped before Qt source download: the VS 2022 environment
script was looked up under 32-bit Program Files, so `cl` was absent. Commit
`dd9de1f` corrected that path and added a Windows-only manual matrix option;
[`39125746935`](https://github.com/rje/Alloy/actions/runs/39125746935) is the
targeted retry. Neither initial failure nor the pending retry qualifies the
Windows input. Retain both run IDs and inspect all remaining jobs before closure.

The targeted `dd9de1f` run was canceled while queued behind the all-target
matrix. A Windows-only run at `ea012ec`,
[`39126604062`](https://github.com/rje/Alloy/actions/runs/39126604062),
confirmed the VS environment but stopped after patch application because the
item-view source bytes differed from the admitted LF hash. Commit `d6dbc21`
forces LF patch application and admits a CRLF-normalized result only when its
SHA-256 equals the previously reviewed LF source; any other mismatch still
fails. Targeted run
[`39126716732`](https://github.com/rje/Alloy/actions/runs/39126716732)
checked that correction; its result and the subsequent adapter failure are
recorded below.

The original matrix's macOS arm64 job completed successfully at `b663b4d`.
Artifact `12136049704` has GitHub digest
`sha256:90424267be2336f760a3ba3a6e208811984f1a577ba26d05594c33b9d01d9245`.
The downloaded `private-qt-build.json` records a complete patched Qt build,
`FEATURE_accessibility`, `FEATURE_itemviews`, Widgets, Network, OpenGL and
FreeType enabled, the admitted item-view source hash, and
`passed-offscreen-itemview-cache`. `build.json` records Alloy compilation and
linking and `dependency-build.json` inventories OCCT/FreeType outputs. This is
additional Mac build evidence; the artifact explicitly marks runtime/GPU and
bundling untested.

The same matrix's Linux x64 job completed successfully at `b663b4d`.
Artifact `12138153247` has GitHub digest
`sha256:8174481f64e1064e45fe2fd317f04d48d12bfbd34df3cb8b27c63c43ace434bf`.
The downloaded manifest records the admitted patched item-view source hash,
Qt 6.11.2 built with accessibility, item views, Widgets, Network, FreeType,
Fontconfig, OpenGL, XCB/GLX, D-Bus and the ATSPI bridge enabled. The offscreen
item-view regression passed 5/5 under GCC 13.3.0 on Ubuntu 24.04. The Alloy
binary compiled and linked; `dependency-build.json` inventories the pinned
OCCT/FreeType outputs and application/solver libraries. The artifact explicitly
marks bundling, loaded runtime and GPU untested.

The same matrix's macOS x64 job completed successfully at `b663b4d`.
Artifact `12139010720` has GitHub digest
`sha256:0f7908ab0c5d3ecff21d865aa1beaa32edbb014e02dd140d2e26e4f271e79c03`.
Its downloaded manifests record the admitted patched item-view source hash,
mandatory Qt features, `passed-offscreen-itemview-cache`, AppleClang
17.0.0.17000013, pinned OCCT/FreeType output inventory, and an Alloy binary
compiled and linked for `macos-x64`. The manifest explicitly marks runtime,
GPU and bundling untested. This establishes an Intel compile/link input only,
not a relocatable or physical Intel application check.

The corrected Windows x64 run `39126716732` reached a complete pinned Qt
source build and passed the offscreen item-view regression (5/5) on MSVC
19.44.35229. Artifact `12137843820` has GitHub digest
`sha256:68c59c1a730b4a1c4039e7147c65bc6af7ae04b5058e1840c75e02d20b30e345`.
Its `private-qt-build.json` confirms the admitted patched item-view hash and
mandatory accessibility, item views, Widgets, Network, OpenGL and FreeType
features. OCCT and the original Ondsel library compiled, but Alloy's assembly
adapter failed to compile because the public Ondsel `FullMatrix.h` uses `M_PI`
and MSVC requires `_USE_MATH_DEFINES` before including math headers. The
artifact retains `17-cmake.log` and the Qt manifest; it contains no Alloy
`build.json`, so Windows compilation/link acceptance is still open. A
Windows-only adapter compile definition is now the bounded correction. A
manual targeted Windows rerun
[`39132726346`](https://github.com/rje/Alloy/actions/runs/39132726346)
ran at `be858a6` and was canceled at the 120-minute job limit during OCCT
compilation. Its Qt source build and offscreen regression passed (5/5), and
FreeType installed successfully; `14-cmake.log` ends in an interrupted OCCT
MSBuild invocation. No Alloy configure/build or success manifest was reached.
The retained artifact was inspected locally at
`/tmp/alloy-ci009-windows-timeout`. This timeout supplies no evidence about
the adapter correction or Alloy linking. CI-009 remains open.


## Bounded Windows timeout correction (2026-10-18)

The next candidate uses single-configuration Ninja for FreeType, OCCT and
Alloy on Windows, matching the existing native Qt recipe. The workflow still
initializes x64 MSVC/SDK through `vcvarsall`; no compiler or architecture change
is intended. Release is selected by `CMAKE_BUILD_TYPE=Release`, and the existing
three-job limit applies to individual Ninja compilation jobs. The pinned OCCT
source already enables MSVC `/MP`, so prior MSBuild compilation must not be
characterized as serial. Avoiding its project-level scheduling plus `/MP`
oversubscription is a performance hypothesis requiring the native retry.

Windows application/client artifact paths now use the build root rather than
the Visual Studio `Release/` subdirectory. Solver library hashes and `dumpbin`
inspection already search recursively, including the nested Ondsel output.
OCCT's Windows install directories depend on MSVC compiler identity and bitness;
its installed CMake package remains the discovery authority. The success
`dependency-build.json` retains the effective generator/build type and complete
library inventory for FreeType, OCCT and Alloy. Inspect these on the retry;
Ninja does not itself prove successful compiler selection or linking.

Only explicitly dispatched Windows jobs receive a bounded 180-minute timeout;
tag jobs and other targets keep 120 minutes. This allows a cold source build
more headroom while the scheduling change is assessed. Source/library pins,
mandatory modules and native compiler baseline checks remain unchanged.

The single-config and generator behavior follows the public CMake 3.31
[Ninja documentation](https://cmake.org/cmake/help/v3.31/generator/Ninja.html)
and [build-type documentation](https://cmake.org/cmake/help/v3.31/variable/CMAKE_BUILD_TYPE.html).
OCCT's compiler/install and `/MP` behavior was checked in its pinned public
[source](https://github.com/Open-Cascade-SAS/OCCT/tree/a016080bf6738d6aeae020badee4e888ad1540a5)
(`CMakeLists.txt`, `adm/cmake/occt_macros.cmake`,
`adm/cmake/occt_defs_flags.cmake`), retrieved 2026-10-18.

Focused Python compilation, static generator/artifact/timeout contract checks,
the four native Qt recipe tests and `git diff --check` pass locally on macOS.
No native Windows build was run locally; elapsed-time improvement, Windows
Alloy compile/link, packaging, loaded runtime and GPU support remain unverified.
