# Cross-platform milestone builds and releases

Research date: **2026-10-11**. User requirement: support macOS, Windows and Linux,
and publish builds at milestones. This document is a proposed implementation
path, with public-source observations and one downloader discovery experiment.
It does **not** establish working Windows/Linux builds, packaged applications,
CI jobs, signed releases, or cross-platform GPU acceptance. Only Alloy and public
open-source foundations are in scope; no reference CAD application runtime was accessed.

## Minimal recommendation

Use native GitHub Actions jobs for **macOS arm64**, **Windows x64/MSVC 2022**, and
**Linux x64/Ubuntu 24.04**. Obtain exact Qt 6.11.2 public binaries, build OCCT
`V7_9_3` as shared libraries with the same native toolchain, build Alloy in Release,
and stage its complete runtime. Package a zipped `.app`, a Windows ZIP and a
Linux tar.gz first. Defer installer UX, universal Alloy binaries, AppImage,
Flatpak and package-manager submissions until these relocatable packages work.
These are implementation choices, not claims that other architectures are out
of the product scope.

Every milestone candidate should produce all three **Actions artifacts** plus
logs, dependency manifests, hashes and evidence. An explicitly selected tag then
promotes the same tested packages to a **GitHub prerelease** after applicable
license, package and cumulative workflow gates pass. A stable release additionally
requires the declared stable capability gate. Do not treat an Actions artifact,
a closed milestone, or a successful compilation as permission to claim supported
GPU/CAD workflows. A foundation milestone may truthfully ship a prerelease shell.

| Proposed native job | Initial package target | Main checks still required |
| --- | --- | --- |
| `macos-15` arm64, Apple Clang | Apple silicon, initially the tested macOS baseline | Bundle relocation, native Retina/GPU, signing/notarization policy |
| `windows-2022` x64, MSVC 2022 | Windows 11 x64 first | WNT/WGL integration, named-pipe API/client, DPI, runtime deployment |
| `ubuntu-24.04` x64, GCC 13 | Ubuntu 24.04 x64 desktop/X11 or XWayland | Xw/GLX integration, xcb dependencies, GPU driver, relocatable runtime |

GitHub currently documents these runner labels. Explicit OS labels avoid the
moving `-latest` alias, but runner image revisions still change: record the image
version and compiler versions in each receipt. Standard hosted machines are VMs;
their listing is not a guarantee of qualifying desktop graphics. A later Intel
macOS job can use `macos-15-intel`. These observations come from the [runner
reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners),
retrieved 2026-10-11.

## Exact Qt channel and a verified downloader pitfall

**Observed public metadata:** the following `Updates.xml` files were fetched and
parsed successfully without an account on 2026-10-11. The extra version/variant
subdirectory is significant; the older direct `qt6_6112/Updates.xml` paths fail.

| Host | Official metadata | Base package / repository version |
| --- | --- | --- |
| macOS | [mac Updates.xml](https://download.qt.io/online/qtsdkrepository/mac_x64/desktop/qt6_6112/qt6_6112/Updates.xml) | `qt.qt6.6112.clang_64`, `6.11.2-0-202608131016` |
| Linux x64 | [Linux Updates.xml](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6112/qt6_6112/Updates.xml) | `qt.qt6.6112.linux_gcc_64`, `6.11.2-0-202608131018` |
| Windows MSVC x64 | [Windows Updates.xml](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6112/qt6_6112_msvc2022_64/Updates.xml) | `qt.qt6.6112.win64_msvc2022_64`, `6.11.2-0-202608131017` |

The Qt base archives are respectively named
`qtbase-MacOS-MacOS_15-Clang-MacOS-MacOS_15-X86_64-ARM64.7z`,
`qtbase-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z`, and
`qtbase-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z`.
The macOS package contains both CPU architectures; that does **not** make an
arm64-only Alloy/OCCT build universal. Qt Core/Gui/Widgets/Network come from
qtbase. Keep deployment tools and the needed runtime plugins; do not install
all optional modules into Alloy's distribution.

**Experiment, not a full SDK installation:** in a disposable `/tmp` Python venv,
released `aqtinstall==3.3.0` successfully listed Linux/macOS 6.11.2 architectures
but failed Windows architecture/archive discovery with an `Updates.xml` checksum
retrieval error. Public upstream commit
[`076e1659807d0b362a3ed684d54c2e9c775eb9c7`](https://github.com/miurahr/aqtinstall/tree/076e1659807d0b362a3ed684d54c2e9c775eb9c7)
(version `3.3.1.dev166`) explicitly handles Windows Qt >=6.11 variant repositories.
Installing that exact commit into the venv made all three archive-list commands
succeed:

```sh
python -m aqt list-qt mac desktop --archives 6.11.2 clang_64
python -m aqt list-qt windows desktop --archives 6.11.2 win64_msvc2022_64
python -m aqt list-qt linux desktop --archives 6.11.2 linux_gcc_64
```

Observed results: mac lists qtbase/qtdeclarative/qtdoc/qtsvg/qttools/qttranslations;
Windows additionally lists d3dcompiler_47/opengl32sw; Linux additionally lists
icu/qtwayland. Discovery is proven; full archive download, extracted SDK relocation,
and native compilation remain CI implementation checks. Never count an included
software OpenGL library as hardware acceptance.

**Proposed downloader:** use a reviewed, checksum-pinned archive of that aqt
commit, with locked Python dependencies, then run the corresponding native
installation command. Replace it with a tested released version when available.
Do not let an install-Qt action silently install the older aqt default. A wrapper
action is optional; pin its full commit and explicitly configure the tested
downloader. Avoid writing another Qt installer.

```sh
python -m aqt install-qt mac desktop 6.11.2 clang_64 --outputdir .deps/qt
python -m aqt install-qt windows desktop 6.11.2 win64_msvc2022_64 --outputdir .deps/qt
python -m aqt install-qt linux desktop 6.11.2 linux_gcc_64 --outputdir .deps/qt
```

These are alternative native-job commands, not three commands for one machine.
The [aqt maintainer's documentation](https://aqtinstall.readthedocs.io/en/latest/getting_started.html)
describes discovery, architecture names, extraction and installation; retrieved
2026-10-11. aqt is an unofficial downloader of Qt's public packages. Preserve the
selected Qt metadata and archive hashes in the dependency receipt.

**Version pitfalls:** Qt 6.11 documents MSVC 2022, macOS 13+ and appropriate Linux
toolchains. Its same [supported-platform page](https://doc.qt.io/qt-6.11/supported-platforms.html)
contains both a Linux ARM-area Ubuntu-24.04/glibc-2.39 note and a general binary
glibc-2.34 note. The x64 metadata names RHEL 9.6. Do not resolve this by guessing:
inspect the downloaded ELF symbol requirements and test the declared minimum.
Ubuntu 24.04 is a conservative first Linux package target, not an assertion that
all Linux distributions work. Homebrew/apt/vcpkg rolling defaults do not reproduce
exact Qt/OCCT pins. Preserve macOS deployment target and Windows toolset/runtime
settings across every dependency; do not mix MinGW OCCT with MSVC Qt. Sources
retrieved 2026-10-11.

## OCCT dependency recipe

**Documented:** OCCT 7.9.3 supports CMake source builds on Windows, Linux and
macOS. FreeType supplies visualization text; Tcl/Tk is associated with DRAW.
Shared libraries, individual modules, C++ standard, OpenGL, resource installation
and third-party paths are configurable. Use the version-pinned [build guide](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/dox/build/build_occt/building_occt.md)
and [CMake source](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/CMakeLists.txt),
retrieved 2026-10-11, rather than current 8.x documentation.

**Proposed foundation configuration:** source archive plus SHA-256 and resolved
commit for `V7_9_3`; `BUILD_LIBRARY_TYPE=Shared`; `BUILD_CPP_STANDARD=C++20`;
FoundationClasses, ModelingData, ModelingAlgorithms and Visualization enabled;
ApplicationFramework, DataExchange, DETools and Draw disabled until used;
`USE_OPENGL=ON`, `USE_GLES2=OFF`, `USE_FREETYPE=ON`; unused TBB, FreeImage, FFmpeg,
VTK, OpenVR and Tk disabled. Retain installed resources; deployment must resolve
shader and future exchange resources from the package, never the developer's
`CASROOT`. Verify the actual cache because module dependencies can enable work.

Build/cache FreeType with the same architecture/toolchain and retain its license.
Its exact accepted revision and optional dependencies belong in FND-009/FND-002;
they are not yet chosen here. A pinned vcpkg baseline for FreeType is reasonable
on Windows; a common small source build reduces per-host variation. Do not
replace exact OCCT with a package-manager default just to save the source build.
Cache by OS, architecture, compiler, dependency hashes, build type and CMake
options; changing any of these invalidates the cache.

## Platform integration gaps before support claims

**Local source observation:** macOS is the only compiled/observed platform.
`ReferenceView.cpp` has WNT_Window and Xw_Window branches that still need native
compilation/runtime tests. The current Python client uses AF_UNIX, the test uses
`/tmp`, and startup enforces Unix directory permissions. Qt documents Windows
QLocalServer as named pipes; this needs a Windows transport and access-control
path, not a different mock protocol. Keep command IDs/state/results identical.
[QLocalServer](https://doc.qt.io/qt-6.11/qlocalserver.html), retrieved 2026-10-11.

**Linux proposal:** initially run Qt with `QT_QPA_PLATFORM=xcb` and a real X11
server or XWayland. The existing `Xw_Window` consumes an X window ID; a Wayland
native surface is not interchangeable. OCCT desktop OpenGL needs GLX/OpenGL and
a functioning GPU driver. Audit xcb/xkbcommon/X11, Fontconfig/FreeType and platform
plugin dependencies. Qt's [Linux requirements](https://doc.qt.io/qt-6.11/linux-requirements.html)
list the applicable runtime/development libraries. Use that list and distro
packages instead of assuming `libGL` alone is sufficient; retrieved 2026-10-11.

Hosted Xvfb/Mesa runs can exercise a real process, API and framebuffer in a
separately labeled software diagnostic job. They cannot close the hardware gate.
The existing strict GPU smoke must remain strict. Add a separate diagnostic mode
only if needed, preserving `hardwareAccelerated:false` and never converting it
into accepted GPU evidence. Similarly, Windows hosted desktop/OpenGL availability
must be measured rather than inferred from a successful WGL compile.

Minimum hardware receipt for **each OS/package**: launch the extracted package
without developer dependency paths; API discovery/commands/errors; live driver
identity; native drawable dimensions before offscreen capture; scale/resize;
physical Top/Home/search; matching displayed view and API framebuffer; clean
exit. As modeling lands, run all previous useful-object fixtures cumulatively.
Use a trusted desktop test machine/manual receipt tied to the package SHA first;
a permanent self-hosted runner fleet is not required for the initial pipeline.

## Runtime bundles and release publication

Use one CMake installation layout with
`install(TARGETS ...)`, `qt_generate_deploy_app_script(...)` and `cmake --install`
into an empty stage. Qt documents native Windows, macOS and Linux deployment
through this API; unsupported cross-compilation should fail, not be suppressed.
[Qt CMake deployment](https://doc.qt.io/qt-6.11/cmake-deployment.html),
[deployment command](https://doc.qt.io/qt-6.11/qt-generate-deploy-app-script.html),
retrieved 2026-10-11. Installation rules are not in Alloy yet.

Qt deployment does not establish that every OCCT/FreeType runtime and resource
was collected. Explicitly stage their runtime dependency closure, notices and
resources; inspect `otool -L`, `dumpbin /DEPENDENTS`, or ELF dependencies and
run from a relocated directory with build SDK paths removed. CMake's
[runtime dependency installation](https://cmake.org/cmake/help/latest/command/install.html#runtime-dependency-set)
and [CPack archive formats](https://cmake.org/cmake/help/latest/cpack_gen/archive.html)
provide existing machinery; retrieved 2026-10-11.

- **macOS:** `.app` with Qt plugins/frameworks and OCCT dylibs/resources using
  relative install names; ZIP for initial candidates. Qt supplies `macdeployqt`.
  Publish normal user-facing releases with the chosen Developer ID signing and
  notarization process; early unsigned candidates must say so. Certificate/account
  availability is unresolved, not a reason to stop native builds. Do not claim
  notarization from an ad-hoc development signature.
  [Qt macOS deployment](https://doc.qt.io/qt-6.11/macos-deployment.html),
  [Apple notarization](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution).
- **Windows:** ZIP with executable, Qt platform plugins, OCCT and FreeType DLLs.
  `windeployqt` handles Qt; explicitly account for additional libraries. Use the
  official Microsoft VC++ Redistributable path for the selected toolset rather
  than copying arbitrary DLLs from a developer machine. A later installer can
  manage this prerequisite; the first ZIP should include clear verified setup.
  [Qt Windows deployment](https://doc.qt.io/qt-6.11/windows-deployment.html),
  [Microsoft runtime distribution](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).
- **Linux:** tar.gz with executable, bundled non-system runtime dependencies,
  Qt xcb plugin, OCCT resources and relative library/plugin paths. Leave system
  glibc and vendor GPU driver integration to the declared distro; copying a GPU
  driver from CI is not portable. A launcher may select xcb for this implementation.
  [Qt Linux deployment](https://doc.qt.io/qt-6.11/linux-deployment.html).

All platform deployment sources above were retrieved 2026-10-11.

**Proposed workflow:** pull requests/main run native builds and checks. A
`workflow_dispatch` milestone-candidate input selects a commit/gate and uploads
three runtime archives, debug symbols separately, dependency/evidence JSON,
notices/source-access materials and SHA256SUMS. Retain failures/logs explicitly;
an incomplete platform matrix is a failed candidate, not an omitted asset.
Use a full-commit-pinned upload-artifact action and explicit retention; archive
files first so Unix modes/symlinks survive artifact transport. Actions artifacts
are retained workflow outputs, not the permanent release channel.
[GitHub artifacts](https://docs.github.com/en/actions/tutorials/store-and-share-data),
[upload-artifact implementation](https://github.com/actions/upload-artifact),
retrieved 2026-10-11.

For a selected existing tag, a separate publication job verifies package hashes,
license/package/GPU receipts and the milestone's cumulative gate. Publish the
same artifacts using `gh release create --verify-tag --prerelease --notes-file`
(for a stable tag omit `--prerelease` only after stable gates). Never automatically
mint a tag from an issue closure; never rebuild different untested bytes during
promotion. Grant release write permission only to that job; no GitHub browser
interaction is needed. [GitHub CLI release creation](https://cli.github.com/manual/gh_release_create),
retrieved 2026-10-11.

## License evidence to carry into packaging

Alloy's original source/docs remain MIT. Qt's module licenses vary; its official
LGPL guidance describes library source availability, notices/license texts,
replacement/relinking rights and installation information. Dynamic shared-library
distribution is the proposed practical route, not automatic proof of compliance.
Retain an exact module/transitive inventory and actual upstream license files.
[Qt licensing](https://doc.qt.io/qt-6.11/licensing.html),
[Qt's published LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations).
OCCT 7.9.3 distributes [LGPL 2.1](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/LICENSE_LGPL_21.txt)
with its [additional exception](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/OCCT_LGPL_EXCEPTION.txt).
FreeType and other transitive components need their own retained terms. These
are upstream evidence for FND-002's admission/distribution decision, not legal
advice or a finding that an unbuilt package already complies. Sources retrieved
2026-10-11. Public candidate binaries also require this packaging inventory;
labeling something a CI artifact does not remove distribution considerations.

## Backlog changes to make this executable

Strengthen existing issues, preserving their keys and dependencies:

- **FND-009:** all three OS families are accepted requirements now; replace the
  earlier open OS-target choice with explicit baseline/architecture matrix,
  exact SDK/OCCT/FreeType pins, checksums, compiler settings and reproducible builds.
- **FND-002:** evaluate the actual three-platform runtime inventories, source
  access, notices, relinking/replacement and signing implications.
- **FND-006 / FND-010:** keep Windows IPC and portable test paths in the real API
  contract; separate diagnostic hosted rendering from strict hardware acceptance.
- **DOC-005:** qualify WNT/Xw adapters and native DPI/capture on each target;
  no geometry/picking closure from an empty cross-platform shell.
- **REL-003:** complete relocatable supported-platform packages; future initial
  release must rerun cumulative modeling from the packaged executable.
- **REL-006 / GATE-050 and later gates:** release notes must expose platform,
  architecture, tested driver, package status, feature scope and cumulative receipts.

Five bounded additions are sufficient to start (parent owns final keys):

1. **Native CI matrix:** three jobs, pinned public downloader/dependencies, cache
   keys, clean configure/build, logs and candidate artifacts. Prove each toolchain
   and report graphics unavailable separately. Includes Linux xcb/GLX compile setup.
2. **Portable Windows IPC:** real QLocalServer named-pipe access control/client,
   discovery/liveness, tempfile paths and unchanged command/error semantics;
   execute the same shell suite in a native Windows session.
3. **Runtime bundles:** common CMake stage plus three small platform adapters,
   OCCT resources/transitive libraries/notices, package hashes and relocated
   launch without development paths. Split adapters into children if implementation
   stops being one reviewable task; do not call bundle success from build-tree launch.
4. **Milestone publisher:** explicit candidate input, complete asset set, retained
   evidence and checked tag promotion using gh/API; test draft/prerelease flow with
   authorized publication, never silently call an artifact a stable release.
5. **Cross-platform package/GPU gate (proposed GATE-005):** blocked on the relevant
   dependency-license, native CI, Windows IPC and runtime-bundle work. Require
   real hardware/package receipts for macOS/Windows/Linux plus cumulative shell
   baseline before declaring the foundation portable. Gate-010 and subsequent
   modeling gates should inherit it. It cannot be closed by Xvfb or compilation.

No source, CI, manifest or GitHub state was changed in this research task. The
only runtime experiment was public Qt metadata/downloader discovery; no full
cross-platform build or SDK installation was performed.
