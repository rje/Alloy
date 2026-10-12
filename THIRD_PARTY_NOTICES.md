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

The [dependency admission record](docs/dependencies/README.md) distinguishes these
approved foundations from candidate libraries. A future downloadable package
must add its complete deployed libraries/plugins, source availability, all
third-party notices and compatible-library replacement/installation instructions.
The current repository and unbundled CI artifacts are not a completed runtime
distribution bundle.
