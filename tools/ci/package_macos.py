#!/usr/bin/env python3
"""Stage one local arm64 Alloy preview archive from a validated native build.

The matching private Qt macdeployqt owns dependency copying/install-name edits.
This script supplies local tools/notices, removes development fallback paths and
refuses a bundle whose Mach-O closure still refers outside the app or macOS.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.dependencies.private_qt_recipe import ARCHIVE_SHA256, PATCHED_SOURCE, PATCHES


def run(*args, log=None):
    result = subprocess.run(list(map(str, args)), text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if log is not None:
        log.write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(f'{args[0]} failed ({result.returncode}): {result.stdout[-4000:]}')
    return result.stdout


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def macho(path):
    if not path.is_file():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in {
            b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe', b'\xfe\xed\xfa\xcf', b'\xbe\xba\xfe\xca'}


def dependencies(path):
    lines = run('otool', '-L', path).splitlines()[1:]
    return [line.strip().split(' (compatibility version', 1)[0] for line in lines]


def rpaths(path):
    lines = run('otool', '-l', path).splitlines()
    return [lines[i + 2].strip().split(' (offset', 1)[0].removeprefix('path ')
            for i, line in enumerate(lines[:-2]) if line.strip() == 'cmd LC_RPATH']


def audit(app):
    contents = app / 'Contents'
    executable = contents / 'MacOS/alloy'
    executable_rpaths = rpaths(executable)
    binaries = sorted(path for path in contents.rglob('*') if path.is_file() and macho(path))
    if not binaries:
        raise RuntimeError('No Mach-O files in staged app')
    entries = {}
    for path in binaries:
        relative = str(path.relative_to(app))
        deps = dependencies(path)
        paths = rpaths(path)
        for item in paths:
            if item.startswith('/') and not item.startswith(('/System/Library/', '/usr/lib/')):
                raise RuntimeError(f'Developer LC_RPATH in {relative}: {item}')
        for dep in deps:
            if dep.startswith(('/System/Library/', '/usr/lib/')):
                continue
            if dep.startswith('@executable_path/'):
                resolved = (executable.parent / dep.removeprefix('@executable_path/')).resolve()
            elif dep.startswith('@loader_path/'):
                resolved = (path.parent / dep.removeprefix('@loader_path/')).resolve()
            elif dep.startswith('@rpath/'):
                suffix = dep.removeprefix('@rpath/')
                candidates = []
                for item in paths:
                    if item.startswith('@executable_path/'):
                        candidates.append(executable.parent / item.removeprefix('@executable_path/') / suffix)
                    elif item.startswith('@loader_path/'):
                        candidates.append(path.parent / item.removeprefix('@loader_path/') / suffix)
                    elif item.startswith(('/System/Library/', '/usr/lib/')):
                        candidates.append(Path(item) / suffix)
                for item in executable_rpaths:
                    if item.startswith(('@executable_path/', '@loader_path/')):
                        candidates.append(executable.parent / item.split('/', 1)[1] / suffix)
                    elif item.startswith(('/System/Library/', '/usr/lib/')):
                        candidates.append(Path(item) / suffix)
                resolved = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            else:
                raise RuntimeError(f'External dependency in {relative}: {dep}')
            if resolved is None or not resolved.is_file() or not resolved.is_relative_to(app.resolve()):
                raise RuntimeError(f'Unresolved bundle dependency in {relative}: {dep}')
        entries[relative] = {'sha256': sha(path), 'dependencies': deps, 'rpaths': paths}
    cocoa = contents / 'PlugIns/platforms/libqcocoa.dylib'
    if not cocoa.is_file():
        raise RuntimeError('Matching Cocoa plugin was not bundled')
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--qt-prefix', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != 'darwin' or platform.machine() != 'arm64':
        raise RuntimeError('This bounded package recipe is macOS arm64 only')
    source = args.app.resolve()
    qt = args.qt_prefix.resolve()
    work = args.work.resolve()
    if source.name != 'alloy.app' or not (source / 'Contents/MacOS/alloy').is_file():
        raise RuntimeError('Expected an existing alloy.app native build')
    source_rpaths = rpaths(source / 'Contents/MacOS/alloy')
    if str(qt / 'lib') not in source_rpaths:
        raise RuntimeError('Chosen Qt SDK is not the source app linked runtime')
    if any('qt' in item.lower() and item != str(qt / 'lib') for item in source_rpaths):
        raise RuntimeError(f'Conflicting Qt runtime paths in source app: {source_rpaths}')
    patched_source = qt.parent / 'source'
    patch_hashes = {}
    for relative, expected in PATCHED_SOURCE.items():
        observed = sha(patched_source / relative)
        if observed != expected:
            raise RuntimeError(f'Qt source patch mismatch: {relative}')
        patch_hashes[relative] = observed
    if work.exists() and any(work.iterdir()):
        raise RuntimeError(f'Refusing to overwrite nonempty output: {work}')
    work.mkdir(parents=True, exist_ok=True)
    staged = work / 'Alloy.app'
    shutil.copytree(source, staged, symlinks=True, copy_function=shutil.copy2)
    contents = staged / 'Contents'
    build = source.parent
    client = build / 'alloy-local-client'
    if not client.is_file():
        raise RuntimeError(f'Missing matching local client: {client}')
    shutil.copy2(client, contents / 'MacOS/alloy-local-client')
    shutil.copy2(ROOT / 'tools/alloyctl.py', contents / 'MacOS/alloyctl.py')
    deploy = qt / 'bin/macdeployqt'
    run(deploy, staged, '-no-strip', '-no-codesign',
        f'-executable={contents / "MacOS/alloy-local-client"}', '-verbose=2',
        log=work / 'macdeployqt.log')
    copied = {}
    for match in re.finditer(r'copied: "([^"]+)"', (work / 'macdeployqt.log').read_text()):
        path = Path(match.group(1)).resolve(strict=True)
        if not macho(path):
            continue
        other = copied.setdefault(path.name, path)
        if other != path:
            raise RuntimeError(f'Colliding deployed Mach-O basename {path.name}: {other}, {path}')
    if copied.get('libqcocoa.dylib') != (qt / 'plugins/platforms/libqcocoa.dylib').resolve():
        raise RuntimeError('macdeployqt did not copy Cocoa from the selected private Qt SDK')
    # qt.conf shipped by the development build records its absolute SDK prefix.
    (contents / 'Resources/qt.conf').write_text('[Paths]\nPrefix=..\nPlugins=PlugIns\n')
    resources = Path('/opt/homebrew/opt/opencascade/share/opencascade/resources')
    if not resources.is_dir():
        raise RuntimeError(f'Missing local OCCT resources: {resources}')
    shutil.copytree(resources, contents / 'Resources/occt', symlinks=True)
    notices = contents / 'Resources/Notices'
    notices.mkdir(parents=True)
    for item in ('LICENSE', 'THIRD_PARTY_NOTICES.md'):
        shutil.copy2(ROOT / item, notices / item)
    shutil.copytree(ROOT / 'LICENSES', notices / 'LICENSES')
    shutil.copy2(ROOT / 'third_party/dejavu/LICENSE', notices / 'DejaVu-LICENSE')
    shutil.copy2(ROOT / 'third_party/ondsel/LICENSE', notices / 'OndselSolver-LICENSE')
    for component, path in {
        'OCCT-LGPL-2.1': Path('/opt/homebrew/opt/opencascade/share/doc/opencascade/LICENSE_LGPL_21.txt'),
        'OCCT-exception': Path('/opt/homebrew/opt/opencascade/share/doc/opencascade/OCCT_LGPL_EXCEPTION.txt'),
        'FreeType-2.14.3': Path('/opt/homebrew/opt/freetype/LICENSE.TXT'),
        'TBB': Path('/opt/homebrew/opt/tbb/LICENSE.txt'),
        'libpng': Path('/opt/homebrew/opt/libpng/LICENSE'),
    }.items():
        if not path.is_file():
            raise RuntimeError(f'Missing exact deployed license: {path}')
        shutil.copy2(path, notices / (component + '.txt'))
    source_records = notices / 'Source-and-replacement'
    source_records.mkdir()
    for input_path in ('tools/dependencies/build-private-qt.sh',
                       'tools/dependencies/private_qt_recipe.py',
                       'tools/dependencies/qt-cocoa-accessibility/765434.patch',
                       'tools/dependencies/qt-itemview-cache/itemviews-accessible-cache.patch',
                       'third_party/planegcs/README.md', 'third_party/planegcs/sources.json',
                       'third_party/ondsel/sources.json', 'docs/validation/mac-runtime-recipe.md'):
        destination = source_records / input_path
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / input_path, destination)
    (source_records / 'README.md').write_text(
        '# Local dynamic-library replacement and source record\n\n'
        'Alloy original code is MIT; bundled libraries retain their own licenses. '
        'Qt 6.11.2 source is the public v6.11.2 qtbase tag archive with SHA-256 '
        '`06cd7aa6b3ab19cb5a1664a2891ef0df960f78c97fa31b80b1d6d7ee70688414`; '
        'the two local patches and coherent build recipe are in this folder. '
        'The exact PlaneGCS/Ondsel source manifests and replacement notes are included. '
        'The Homebrew OCCT, FreeType, TBB and libpng versions actually copied are '
        'recorded in `../BUNDLED_COMPONENTS.json`; use their corresponding public source.\n\n'
        'Compatible modified shared libraries can replace files under '
        '`Alloy.app/Contents/Frameworks`, and compatible Qt plugins under '
        '`Alloy.app/Contents/PlugIns`. Re-sign modified app contents locally before '
        'launch (`codesign --force --deep --sign - Alloy.app`). This local ad-hoc '
        'signature is not Developer ID signing or notarization. Public release '
        'source-access and license review remain separate.\n')
    brew_components = {
        'OCCT': ('7.9.3_1', Path('/opt/homebrew/opt/opencascade/lib/libTKernel.7.9.dylib')),
        'FreeType': ('2.14.3', Path('/opt/homebrew/opt/freetype/lib/libfreetype.6.dylib')),
        'TBB': ('2023.1.0', Path('/opt/homebrew/opt/tbb/lib/libtbb.12.dylib')),
        'libpng': ('1.6.59', Path('/opt/homebrew/opt/libpng/lib/libpng16.16.dylib')),
    }
    components = {}
    for name, (version, library) in brew_components.items():
        resolved = library.resolve(strict=True)
        if f'/{version}/' not in str(resolved):
            raise RuntimeError(f'{name} version drift: {resolved}')
        components[name] = {'version': version, 'source_library': str(resolved),
                            'source_sha256': sha(resolved),
                            'bundled_library': f'Contents/Frameworks/{library.name}'}
    qt_framework_hashes = {}
    for name in ('Core', 'Gui', 'Widgets', 'Network'):
        library = qt / f'lib/Qt{name}.framework/Versions/A/Qt{name}'
        qt_framework_hashes[name] = sha(library)
    components['Qt'] = {
        'version': '6.11.2', 'source_archive_sha256': ARCHIVE_SHA256,
        'patched_source_sha256': patch_hashes,
        'patch_sha256': {path.name: sha(path) for path in PATCHES},
        'linked_sdk_framework_sha256': qt_framework_hashes,
        'cocoa_plugin_source_sha256': sha(qt / 'plugins/platforms/libqcocoa.dylib'),
        'source_app_lc_rpath': source_rpaths,
        'sdk_success_manifest': 'unavailable for this earlier locally qualified SDK; source/patch and binary hashes recorded',
    }
    (notices / 'BUNDLED_COMPONENTS.json').write_text(json.dumps(components, indent=2) + '\n')
    # Remove a development fallback that macdeployqt otherwise leaves in the app.
    for path in contents.rglob('*'):
        if path.is_file() and macho(path):
            for item in rpaths(path):
                if item.startswith('/') and not item.startswith(('/System/Library/', '/usr/lib/')):
                    run('install_name_tool', '-delete_rpath', item, path)
    closure = audit(staged)
    # The loader edits invalidate old signatures. This is local ad-hoc signing,
    # not Developer ID signing or notarization.
    run('codesign', '--force', '--deep', '--sign', '-', staged, log=work / 'codesign.log')
    run('codesign', '--verify', '--deep', '--strict', staged, log=work / 'codesign-verify.log')
    closure = audit(staged)
    archive = work / 'Alloy-macos-arm64-preview.zip'
    run('ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', staged, archive)
    result = {'status': 'candidate', 'source_app': str(source),
              'source_app_sha256': sha(source / 'Contents/MacOS/alloy'),
              'qt_prefix': str(qt), 'qt_cocoa_source_sha256': sha(qt / 'plugins/platforms/libqcocoa.dylib'),
              'bundled_cocoa_sha256': sha(contents / 'PlugIns/platforms/libqcocoa.dylib'),
              'archive': str(archive), 'archive_sha256': sha(archive),
              'mach_o': closure, 'runtime_tested': False, 'gpu_tested': False}
    (work / 'package-candidate.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'archive': str(archive), 'archive_sha256': result['archive_sha256'],
                      'mach_o_count': len(closure)}, indent=2))


if __name__ == '__main__':
    main()
