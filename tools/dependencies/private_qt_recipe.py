#!/usr/bin/env python3
"""Pin and record the coherent macOS Qt source build used by Alloy."""
# SPDX-License-Identifier: MIT
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import urllib.request

VERSION = '6.11.2'
ARCHIVE_URL = f'https://github.com/qt/qtbase/archive/refs/tags/v{VERSION}.tar.gz'
ARCHIVE_SHA256 = '06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414'
PATCHED_SOURCE = {
    'src/plugins/platforms/cocoa/qcocoaaccessibilityelement.mm':
        '0878955b0b6243eff703895be673528f8cb25300cea70aefdd0da548a7ddd81f',
    'src/widgets/accessible/itemviews.cpp':
        '1c1b20c1c4d4c0a73c22036ca656466558b015ec8eb226fd59534e82d0eba7c5',
}
HERE = Path(__file__).resolve().parent
PATCHES = (
    HERE / 'qt-cocoa-accessibility/765434.patch',
    HERE / 'qt-itemview-cache/itemviews-accessible-cache.patch',
)
REGRESSION_INPUTS = (
    HERE / 'qt-itemview-cache/CMakeLists.txt',
    HERE / 'qt-itemview-cache/tst_itemviewcache.cpp',
)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def tree_sha(root):
    """Hash every source path and content, including symlink targets."""
    digest = hashlib.sha256()
    files = 0
    for path in sorted(root.rglob('*')):
        relative = str(path.relative_to(root)).encode()
        if path.is_symlink():
            value = b'L' + os.readlink(path).encode()
        elif path.is_file():
            value = b'F' + sha(path).encode()
            files += 1
        else:
            continue
        digest.update(len(relative).to_bytes(4, 'big') + relative)
        digest.update(len(value).to_bytes(4, 'big') + value)
    return {'sha256': digest.hexdigest(), 'regular_file_count': files}


def command(*args):
    return subprocess.check_output(args, text=True).strip()


def write_json(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    os.replace(temporary, path)


def cache(path):
    values = {}
    for line in path.read_text().splitlines():
        if not line or line.startswith(('#', '//')) or '=' not in line:
            continue
        key, value = line.split('=', 1)
        values[key.split(':', 1)[0]] = value
    return values


def source_hashes(source):
    return {relative: sha(source / relative) for relative in PATCHED_SOURCE}


def output_paths(work):
    prefix = work / 'install'
    paths = {'qt_config': prefix / 'lib/cmake/Qt6/Qt6Config.cmake',
             'cocoa_plugin': prefix / 'plugins/platforms/libqcocoa.dylib',
             'itemview_cache_test': work / 'cache-test/itemview_cache_test'}
    for module in ('Core', 'Gui', 'Widgets', 'Network'):
        paths['qt_' + module.lower()] = prefix / f'lib/Qt{module}.framework/Versions/A/Qt{module}'
    return paths


def plugin_hashes(prefix):
    return {str(path.relative_to(prefix)): sha(path) for path in
            sorted((prefix / 'plugins').rglob('*.dylib')) if path.is_file()}


def verify_outputs(work, manifest):
    expected = manifest.get('output_sha256', {})
    for name, path in output_paths(work).items():
        if not path.is_file() or sha(path) != expected.get(name):
            raise RuntimeError(f'Cached private Qt output missing or changed: {name}: {path}')
    if not (work / 'build/CMakeCache.txt').is_file() or sha(work / 'build/CMakeCache.txt') != expected.get('cmake_cache'):
        raise RuntimeError('Cached private Qt CMake configuration changed')
    source = work / 'source'
    if source_hashes(source) != PATCHED_SOURCE or tree_sha(source) != manifest.get('source_tree'):
        raise RuntimeError('Cached private Qt patched source changed')
    if plugin_hashes(work / 'install') != manifest.get('installed_plugins_sha256'):
        raise RuntimeError('Cached private Qt plugin inventory changed')


def prepare(args):
    if sys.platform != 'darwin':
        raise RuntimeError('Coherent patched private Qt recipe is macOS-only')
    archive = args.archive.resolve()
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        temporary = archive.with_suffix(archive.suffix + '.download')
        try:
            with urllib.request.urlopen(ARCHIVE_URL, timeout=120) as response, temporary.open('wb') as output:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
            if sha(temporary) != ARCHIVE_SHA256:
                raise RuntimeError('Downloaded Qt source archive SHA-256 mismatch')
            os.replace(temporary, archive)
        finally:
            temporary.unlink(missing_ok=True)
    if sha(archive) != ARCHIVE_SHA256:
        raise RuntimeError('Pinned Qt source archive SHA-256 mismatch')
    work = args.work.resolve()
    recipe = {
        'schema': 'alloy-private-qt-recipe/1',
        'qt_version': VERSION,
        'archive_url': ARCHIVE_URL,
        'archive_sha256': ARCHIVE_SHA256,
        'patch_sha256': {str(path.relative_to(HERE)): sha(path) for path in PATCHES},
        'regression_input_sha256': {str(path.relative_to(HERE)): sha(path) for path in REGRESSION_INPUTS},
        'recipe_sha256': {str(path.relative_to(HERE)): sha(path) for path in
                          (HERE / 'build-private-qt.sh', Path(__file__).resolve())},
        'cmake_args': args.cmake_arg,
        'host_arch': platform.machine(),
        'compiler': {'c': command('xcrun', '-f', 'clang'),
                     'cxx': command('xcrun', '-f', 'clang++'),
                     'version': command('xcrun', 'clang', '--version')},
        'sdk': {'path': command('xcrun', '--sdk', 'macosx', '--show-sdk-path'),
                'version': command('xcrun', '--sdk', 'macosx', '--show-sdk-version')},
    }
    key = hashlib.sha256(json.dumps(recipe, sort_keys=True).encode()).hexdigest()
    existing = any((work / name).exists() for name in ('source', 'build', 'install', 'cache-test'))
    manifest_path = work / 'private-qt-build.json'
    if existing:
        if not manifest_path.is_file():
            raise RuntimeError(f'Unmanifested/incomplete Qt cache at {work}; choose an empty work root')
        manifest = json.loads(manifest_path.read_text())
        if manifest.get('recipe_key') != key or manifest.get('status') != 'complete':
            raise RuntimeError(f'Qt cache recipe mismatch at {work}; choose an empty work root')
        verify_outputs(work, manifest)
    else:
        work.mkdir(parents=True, exist_ok=True)
    recipe.update(recipe_key=key, archive_path=str(archive), work_root=str(work))
    write_json(work / 'private-qt-input.json', recipe)
    print(f'Pinned private Qt recipe {key}; cache={"verified" if existing else "fresh"}')


def complete(args):
    work = args.work.resolve()
    recipe = json.loads((work / 'private-qt-input.json').read_text())
    if sha(recipe['archive_path']) != ARCHIVE_SHA256 or source_hashes(work / 'source') != PATCHED_SOURCE:
        raise RuntimeError('Qt source/archive changed during build')
    cmake_cache = work / 'build/CMakeCache.txt'
    values = cache(cmake_cache)
    configured = {argument.split('=', 1)[0][2:]: argument.split('=', 1)[1]
                  for argument in recipe['cmake_args'] if argument.startswith('-D') and '=' in argument}
    for key in ('CMAKE_BUILD_TYPE', 'CMAKE_OSX_ARCHITECTURES', 'CMAKE_OSX_DEPLOYMENT_TARGET',
                'CMAKE_OSX_SYSROOT', 'CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER'):
        if values.get(key) != configured.get(key):
            raise RuntimeError(f'Effective Qt CMake option differs from recipe: {key}: {values.get(key)}')
    paths = output_paths(work)
    for name, path in paths.items():
        if not path.is_file():
            raise RuntimeError(f'Missing installed Qt output {name}: {path}')
    prefix = work / 'install'
    plugins = plugin_hashes(prefix)
    if 'plugins/platforms/libqcocoa.dylib' not in plugins:
        raise RuntimeError('Installed Qt has no coherent Cocoa platform plugin')
    licenses = {str(path.relative_to(work / 'source')): sha(path) for path in
                sorted((work / 'source/LICENSES').glob('*')) if path.is_file()}
    features = {key: value for key, value in values.items() if key.startswith(('FEATURE_', 'INPUT_', 'TEST_'))}
    manifest = {
        'schema': 'alloy-private-qt-build/1', 'status': 'complete',
        'recipe_key': recipe['recipe_key'], 'input': recipe,
        'source_patched_sha256': source_hashes(work / 'source'),
        'source_tree': tree_sha(work / 'source'),
        'output_sha256': {name: sha(path) for name, path in paths.items()}
                         | {'cmake_cache': sha(cmake_cache)},
        'effective_cmake': {key: values[key] for key in sorted(values) if key.startswith('CMAKE_')},
        'effective_features': features,
        'qtpaths_query': command(str(prefix / 'bin/qtpaths'), '--query'),
        'installed_plugins_sha256': plugins,
        'source_licenses_sha256': licenses,
        'cache_regression': 'passed-offscreen-itemview-cache',
        'runtime_loaded_tested': False,
        'dependencies_bundled': False,
    }
    write_json(work / 'private-qt-build.json', manifest)
    print(f'Completed private Qt recipe {recipe["recipe_key"]}: {work / "private-qt-build.json"}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('prepare', 'complete'))
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--cmake-arg', action='append', default=[])
    args = parser.parse_args()
    if args.phase == 'prepare':
        if args.archive is None or not args.cmake_arg:
            parser.error('prepare requires --archive and all --cmake-arg values')
        prepare(args)
    else:
        complete(args)


if __name__ == '__main__':
    main()
