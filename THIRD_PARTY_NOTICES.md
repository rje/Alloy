# Third-party foundations

Alloy's original source and documentation are MIT licensed. Third-party licenses
are separate; this file does not relicense libraries or their bundled code.

Alloy uses Qt 6.11.2 Core, Gui, Widgets and Network, dynamically linked under the
GNU Lesser General Public License version 3. Copyright belongs to The Qt Company
and other Qt contributors. Local Qt may also link QtDBus. See
[Qt source](https://github.com/qt/qtbase/tree/v6.11.2),
[LGPL 3](LICENSES/LGPL-3.0-only.txt) and its
[GPL 3 terms](LICENSES/GPL-3.0-only.txt).

Alloy uses Open CASCADE Technology 7.9.3, copyright Open CASCADE SAS and its
contributors, under LGPL version 2.1 with the OCCT exception.
See [exact source](https://github.com/Open-Cascade-SAS/OCCT/tree/a016080bf6738d6aeae020badee4e888ad1540a5),
[LGPL 2.1](LICENSES/LGPL-2.1-only.txt) and the
[OCCT exception](LICENSES/OCCT-exception-1.0.txt).

The native CI foundation uses FreeType 2.13.3 under the FreeType License option.
Portions of this software are copyright © 2024 The FreeType Project
(www.freetype.org). All rights reserved.
See [exact source](https://github.com/freetype/freetype/tree/42608f77f20749dd6ddc9e0536788eaad70ea4b5),
[FTL](LICENSES/FTL.txt) and the
[bundled-license inventory](LICENSES/FreeType-LICENSE.txt).
Local Homebrew development currently uses FreeType 2.14.3; this is a different
build, with its own source and bundled notices.

The sketch solver uses FreeCAD PlaneGCS at revision
`ec6852e6caadf55045881a6003cc7ad038c8d2a7` as a separate shared library under
LGPL-2.1-or-later. Copyright belongs to Konstantinos Poulios and the respective
FreeCAD contributors identified in each retained source header. The exact
[source manifest](third_party/planegcs/sources.json) preserves their original
notices in downloaded sources. See the [upstream license](LICENSES/PlaneGCS-LGPL-2.1-or-later.txt)
and [extraction, modification and replacement instructions](third_party/planegcs/README.md).
Alloy's dated C++20 portability patch replaces the C++23 impossible-case hint;
its independent shims provide configuration, exports and logging.

PlaneGCS uses Eigen 5.0.1 headers with `EIGEN_MPL2_ONLY`, under
[MPL-2.0](LICENSES/MPL-2.0.txt), copyright the Eigen contributors as identified
in their source headers. Boost 1.90.0 headers retain the respective contributors'
notices under [BSL-1.0](LICENSES/BSL-1.0.txt). Exact archives, hashes and license
sources are recorded in [components.json](docs/dependencies/components.json).

Sketch Text bundles the unmodified DejaVu Sans regular TTF from DejaVu Fonts
2.37. Font SHA-256: `7da195a74c55bef988d0d48f9508bd5d849425c1770dba5d7bfc6ce9ed848954`.
The DejaVu changes and the underlying Bitstream and Arev contributions carry
their own terms and required notices in [the release license](third_party/dejavu/LICENSE).
The [official release archive](https://sourceforge.net/projects/dejavu/files/dejavu/2.37/)
was retrieved 2026-10-13; its SHA-256 is
`fa9ca4d13871dd122f61258a80d01751d603b4d3ee14095d65453b4e846e17d7`.
The font is not MIT licensed. Authored text records pin its asset ID and digest;
geometry evaluation refuses missing or changed bytes.

The [dependency admission record](docs/dependencies/README.md) distinguishes these
approved foundations from candidate libraries. A future downloadable package
must add its complete deployed libraries/plugins, source availability, all
third-party notices and compatible-library replacement/installation instructions.
The current repository and unbundled CI artifacts are not a completed runtime
distribution bundle.

## Disposable Metal spike shader assets

The opt-in `research/prototypes/metal-rhi/shaders` source and precompiled bundles
come from Qtbase6.11.2 commit `ef55f427f2c8b410d34f8a7681020a3000cf6866`.
Copyright (C) The Qt Company Ltd.; used under BSD-3-Clause, separately from
Alloy's MIT code. Corresponding source, full license, upstream REUSE provenance
and SHA-256 hashes are retained in that directory. No Shader Tools/qsb compiler
is included. This does not change Qt runtime licensing or admit a production RHI.
