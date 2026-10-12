# Experimental macOS Qt accessibility patch

Status on 2026-10-12: **local experiment, not an accepted or shipped dependency**. Alloy uses shared Qt 6.11.2. During native accessibility inspection, Qt's Cocoa platform plugin crashed while resolving selected children of the sketch tree. A publicly proposed Qt fix guards deletion of parent-managed accessibility elements. The proposal is [Qt review 765434](https://codereview.qt-project.org/c/qt/qtbase/+/765434), revision `c7fd3f34b997bb363be15650647665b3b6b8a5f4`, status **NEW** on the retrieval date, and is associated with [QTBUG-149612](https://bugreports.qt.io/browse/QTBUG-149612). This repository retains the two-guard [patch](../../tools/dependencies/qt-cocoa-accessibility/765434.patch) against the public Qt 6.11.2 source. It does not modify the Homebrew installation.

The [build recipe](../../tools/dependencies/build-qt-cocoa-accessibility.sh) uses a locally supplied `qtbase-v6.11.2.tar.gz`, SHA-256 `06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414`. The archive is the [Qt `v6.11.2` tag archive](https://github.com/qt/qtbase/archive/refs/tags/v6.11.2.tar.gz). It must already be at `../Alloy-dependencies/qtbase-v6.11.2.tar.gz`, or be named by `ALLOY_QTBASE_ARCHIVE`; the script never downloads it. The script checks the archive hash, applies the pinned review patch to an isolated extracted copy, and checks the patched `qcocoaaccessibilityelement.mm` SHA-256 `0878955b0b6243eff703895be673528f8cb25300cea70aefdd0da548a7ddd81f` before compilation.

The recipe requires macOS, CMake, Ninja, AppleClang, and the *same installed Qt private build* used by Alloy: Homebrew `qtbase/6.11.2_1`, including `Qt6::CorePrivate` and `Qt6::GuiPrivate`. Qt warns that private headers tie the plugin to this specific module build and that a different build can crash. The CMake target compiles the public Cocoa platform-plugin sources and resources from the pinned archive, links the installed matching Qt frameworks, and produces one `libqcocoa.dylib` under `../Alloy-dependencies/qt-cocoa-accessibility/staged/platforms/`. Source, build, and staged output stay outside the Alloy checkout. The checked-in [CMake definition](../../tools/dependencies/qt-cocoa-accessibility/CMakeLists.txt) is a small standalone equivalent of the public [Qt Cocoa plugin target](https://github.com/qt/qtbase/blob/v6.11.2/src/plugins/platforms/cocoa/CMakeLists.txt); it is not part of Alloy's normal CMake build.

To reproduce the isolated build on that exact host dependency version:

```sh
tools/dependencies/build-qt-cocoa-accessibility.sh
source ../Alloy-dependencies/qt-cocoa-accessibility/env.sh
QT_DEBUG_PLUGINS=1 build-tutorial/alloy.app/Contents/MacOS/alloy
```

`env.sh` sets `QT_PLUGIN_PATH` to the staged parent directory and `QT_QPA_PLATFORM_PLUGIN_PATH` to its `platforms` child. Verify the Qt plugin loader reports the staged `libqcocoa.dylib` as **loaded**, not merely discovered. Verify `otool -L` points to the intended Qt 6.11.2 frameworks and compare the staged dylib hash to any packaged copy. Keep one Cocoa plugin in the app-private `platforms` directory; deployment tools such as `macdeployqt` can replace it, so check the loaded path and hash again *after* packaging. Do not install the patch into Homebrew's shared plugin directory. [Qt's deployment guide](https://doc.qt.io/qt-6/deployment-plugins.html) describes plugin search and version compatibility.

The first local patched-plugin run loaded the isolated module and passed the formerly crashing accessibility query through a two-corner Fillet preview and native acceptance. A later Undo crashed through `QComboBox::clear()` and `QAccessibleTable::modelChange`, a distinct path addressed by retaining dimension-combo rows in Alloy. Native preview/editor100, Return, Undo/Redo and accessibility reads now pass with retained combo rows; exact accepted/redo draft equality and the loaded patched dylib were verified and independently reviewed. Evidence is in `build-tutorial/evidence/fillet-combo-*` and `qt-cocoa-combo-qualified-runtime.txt`. Accessibility warnings remain. Default launch/packaged-app checks still need evidence; this experiment does not qualify a release.

Qt's original `qcocoaaccessibilityelement.mm` carries `Copyright (C) 2016 The Qt Company Ltd.` and `SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`. The patched plugin remains Qt-derived code under its applicable license, separate from Alloy's MIT originals. Distribution requires the matching Qt and patch notices, corresponding source/build instructions, and a usable library replacement path under the selected LGPL terms. Record those in the package inventory and verify the deployed library closure before shipping; see [dependency admission](README.md) and [native releases](../releases.md).

## Private bundle launch qualification

On 2026-10-12 the already-qualified plugin was copied into
`build-tutorial/alloy.app/Contents/PlugIns/platforms/libqcocoa.dylib` without a
`qt.conf`. Launching with both plugin-path environment variables unset loaded
that exact bundled plugin and retained the Homebrew native style plugin fallback.
Qt 6.11.2's [application search order](https://github.com/qt/qtbase/blob/v6.11.2/src/corelib/kernel/qcoreapplication.cpp#L2782-L2807)
puts the bundle plugin directory before its installation directory. Do not add a
`qt.conf` that unintentionally removes that fallback for this private build.

The same bulk fillet preview followed by native Return, Undo and Redo with
accessibility reads passed. Accepted and redone active sessions were exactly
equal. Evidence: `build-tutorial/evidence/qt-bundled-launch.log` and
`qt-bundled-{accepted,undo,redo}.json`. This qualifies the local private bundle
layout, not portable distribution, signing or other-platform deployment. The
human testing app and global Qt installation were unchanged.


## Item-view cache recurrence and coherent private runtime

A later native second-circle dimension insertion crashed in
`QAccessibleTable::modelChange` (PID97793, report
`alloy-2026-10-12-151125.ips`). The table retained IDs whose interfaces had been
evicted from Qt's global accessibility cache. Both row-change loops asserted
then dereferenced the missing interface; release builds therefore crashed.
Table, Tree and List child lookup also returned null for expired cached IDs.
Earlier Alloy row-retention workarounds do not qualify this general path.

The [local patch proposal](../../tools/dependencies/qt-itemview-cache/itemviews-accessible-cache.patch)
prunes expired IDs in the two cache rebuild loops and recreates expired children
in the three view lookups. It is our proposal, not an accepted upstream change.
Source is public Qt6.11.2 `src/widgets/accessible/itemviews.cpp`, retrieved from
the pinned archive on2026-10-12. Its copyright and license expression are the
same Qt Company2016 and Commercial/LGPL3/GPL2/GPL3 alternatives recorded above.
The standalone regression source is Alloy-authored MIT; the patch modifies Qt
and retains Qt's applicable licensing.

The [coherent private build recipe](../../tools/dependencies/build-private-qt.sh)
builds Qtbase libraries and plugins together outside the checkout, with both
patches, then installs under `../Alloy-dependencies/qt-private-runtime/install`.
It does not change Homebrew or disable accessibility. Configure Alloy with
that prefix first, and verify all loaded Qt frameworks/plugins belong to the
same private build; do not reuse the earlier system-linked Cocoa plugin.
The recipe retains Widgets, Network, Test and native OpenGL support. SQL and
PrintSupport are omitted from this local experiment.

The focused regression creates real Table/Tree/List widgets, explicitly evicts
ordinary child interfaces, re-queries them, then inserts/removes actual model
rows. It checks recreated names, validity and row counts. Explicit model events
make this independent of an attached OS accessibility client. It does not
qualify row/column-header behavior. The unpatched installed Qt6.11.2 fails all
three recreated-child checks, confirming that the fixture detects the defect.
The same regression passes all three view cases with the coherent patched
private runtime (5QtTest results including setup/cleanup, 0failures). Logs are
`/tmp/alloy-qt-cache-baseline.log` and `/tmp/alloy-qt-cache-private.log`. Native Alloy qualification also passes the formerly crashing second-dimension
insertion, selected sketch-row Undo/Redo, Finish and selected Browser-row
Undo/Redo. Accepted/redone draft JSON is exactly equal. Astra independently
verified `profile-cut/build-cut/evidence/qt-native-*` and the launch log: all
Qt frameworks and Cocoa/style plugins load from the private installation, and
Apple M4 hardware acceleration remains active. Some AX row labels still become
inconsistent after history changes; FND-015 remains open. This is a bounded
local runtime qualification, not complete accessibility or release acceptance.
