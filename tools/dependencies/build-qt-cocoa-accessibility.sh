#!/usr/bin/env bash
set -euo pipefail

if [[ $(uname -s) != Darwin ]]; then
    echo "This Cocoa plugin experiment requires macOS" >&2
    exit 1
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(cd "$script_dir/../.." && pwd -P)
dependency_root=$(cd "$repo_root/.." && pwd -P)/Alloy-dependencies
archive=${ALLOY_QTBASE_ARCHIVE:-$dependency_root/qtbase-v6.11.2.tar.gz}
qt_prefix=${ALLOY_QTBASE_PREFIX:-$(brew --prefix qtbase)}
work_root=${ALLOY_COCOA_PATCH_WORK:-$dependency_root/qt-cocoa-accessibility}
source_dir=$work_root/qtbase-6.11.2
build_dir=$work_root/build
stage_dir=$work_root/staged
patch_file=$script_dir/qt-cocoa-accessibility/765434.patch
expected_archive=06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414
expected_patched=0878955b0b6243eff703895be673528f8cb25300cea70aefdd0da548a7ddd81f

if [[ ! -f $archive ]]; then
    echo "Missing pinned Qt source archive: $archive" >&2
    exit 1
fi
actual_archive=$(shasum -a 256 "$archive" | awk '{print $1}')
if [[ $actual_archive != "$expected_archive" ]]; then
    echo "Qt archive SHA-256 mismatch" >&2
    exit 1
fi
if [[ $(basename "$(cd "$qt_prefix" && pwd -P)") != 6.11.2_1 ]]; then
    echo "Expected the exact Homebrew qtbase 6.11.2_1 private-header build" >&2
    exit 1
fi

mkdir -p "$work_root"
if [[ ! -d $source_dir ]]; then
    tar -xzf "$archive" -C "$work_root"
fi
if (cd "$source_dir" && patch --dry-run --silent -p1 < "$patch_file") >/dev/null 2>&1; then
    (cd "$source_dir" && patch --silent -p1 < "$patch_file")
fi
patched_file=$source_dir/src/plugins/platforms/cocoa/qcocoaaccessibilityelement.mm
actual_patched=$(shasum -a 256 "$patched_file" | awk '{print $1}')
if [[ $actual_patched != "$expected_patched" ]]; then
    echo "Qt Cocoa source differs from the pinned upstream revision" >&2
    exit 1
fi

cmake -S "$script_dir/qt-cocoa-accessibility" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_PREFIX_PATH="$qt_prefix" -DQTBASE_SOURCE_DIR="$source_dir"
cmake --build "$build_dir" --target qcocoa

mkdir -p "$stage_dir/platforms"
cp "$build_dir/platforms/libqcocoa.dylib" "$stage_dir/platforms/libqcocoa.dylib"
if [[ $(find "$stage_dir/platforms" -maxdepth 1 -type f | wc -l | tr -d ' ') != 1 ]]; then
    echo "The staged platforms directory must contain exactly one plugin" >&2
    exit 1
fi
printf 'export QT_PLUGIN_PATH=%q\nexport QT_QPA_PLATFORM_PLUGIN_PATH=%q\n' \
    "$stage_dir" "$stage_dir/platforms" > "$work_root/env.sh"
echo "Staged patched plugin: $stage_dir/platforms/libqcocoa.dylib"
echo "To opt into this experiment: source $work_root/env.sh"
