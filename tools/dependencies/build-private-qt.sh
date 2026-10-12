#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
[[ $(uname -s) == Darwin ]] || { echo 'This private runtime recipe currently requires macOS' >&2; exit 1; }
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(cd "$script_dir/../.." && pwd -P)
dependency_root=$(cd "$repo_root/.." && pwd -P)/Alloy-dependencies
archive=${ALLOY_QTBASE_ARCHIVE:-$dependency_root/qtbase-v6.11.2.tar.gz}
work_root=${ALLOY_PRIVATE_QT_WORK:-$dependency_root/qt-private-runtime}
source_dir=$work_root/source
build_dir=$work_root/build
prefix=$work_root/install
check_hash() {
    [[ $(shasum -a 256 "$1" | awk '{print $1}') == "$2" ]] || {
        echo "Pinned source hash mismatch: $1" >&2; exit 1;
    }
}
check_hash "$archive" 06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414
if [[ ! -d $source_dir ]]; then
    mkdir -p "$source_dir"
    tar -xzf "$archive" -C "$source_dir" --strip-components=1
fi
for patch_file in "$script_dir/qt-cocoa-accessibility/765434.patch" "$script_dir/qt-itemview-cache/itemviews-accessible-cache.patch"; do
    if patch -d "$source_dir" --dry-run --silent -p1 < "$patch_file" >/dev/null 2>&1; then
        patch -d "$source_dir" --silent -p1 < "$patch_file"
    fi
done
check_hash "$source_dir/src/plugins/platforms/cocoa/qcocoaaccessibilityelement.mm" 0878955b0b6243eff703895be673528f8cb25300cea70aefdd0da548a7ddd81f
check_hash "$source_dir/src/widgets/accessible/itemviews.cpp" 1c1b20c1c4d4c0a73c22036ca656466558b015ec8eb226fd59534e82d0eba7c5
cmake -S "$source_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DQT_BUILD_TESTS=OFF \
    -DQT_BUILD_EXAMPLES=OFF -DQT_BUILD_BENCHMARKS=OFF \
    -DFEATURE_sql=OFF -DFEATURE_printsupport=OFF
cmake --build "$build_dir" --parallel "${ALLOY_BUILD_JOBS:-6}"
cmake --install "$build_dir"
cmake -S "$script_dir/qt-itemview-cache" -B "$work_root/cache-test" -G Ninja \
    -DCMAKE_PREFIX_PATH="$prefix" -DQt6_DIR="$prefix/lib/cmake/Qt6"
cmake --build "$work_root/cache-test" --parallel 2
QT_QPA_PLATFORM=offscreen "$work_root/cache-test/itemview_cache_test"
echo "Private Qt runtime: $prefix"
echo 'Native Alloy interaction and loaded-library verification are separate qualification steps.'
