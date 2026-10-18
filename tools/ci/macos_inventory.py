#!/usr/bin/env python3
"""Index the existing accepted local macOS package and retained load receipts.

Read-only with respect to the archive/extraction. No new app launch, SDK build,
repackaging, or claim that newer CI build evidence belongs to this legacy app.
"""
import argparse
import json
import re
from pathlib import Path
import subprocess
from runtime_inventory import SCHEMA, file_record, package_files, reference, sha, digest

ROOT = Path(__file__).resolve().parents[2]
NOTICES = 'Contents/Resources/Notices/'
SOURCE = NOTICES + 'Source-and-replacement/'


def generate(candidate, relocated, output):
    output.mkdir(parents=True, exist_ok=True)
    app = (relocated / 'Alloy.app').resolve()
    archive = candidate / 'Alloy-macos-arm64-preview.zip'
    receipt = candidate / 'package-candidate.json'
    runtime = relocated / 'result.json'
    source_commit = json.loads(runtime.read_text())['sourceCommit']

    def git_file(name):
        return subprocess.check_output(['git', '-C', str(ROOT), 'show', f'{source_commit}:{name}'])

    # This snapshot is from the existing receipt's commit, never current HEAD.
    contract = {'source_commit': source_commit,
                'cmake': git_file('CMakeLists.txt').decode(),
                'fonts': {'version': re.search(r'DejaVu Fonts ([0-9.]+)', git_file('THIRD_PARTY_NOTICES.md').decode()).group(1).rstrip('.'),
                          'source': 'pinned THIRD_PARTY_NOTICES.md',
                          'notice': git_file('THIRD_PARTY_NOTICES.md').decode(),
                          'sha256': {name: digest(git_file('third_party/dejavu/' + name))
                                     for name in ('DejaVuSans.ttf', 'DejaVuSans-Bold.ttf')}},
                'built_input_status': 'Legacy package has no dependency-build.json or Qt success manifest; '
                                      'use only available package source-input hashes, never a newer CI build.'}
    contract_path = output / 'source-contract.json'
    contract_path.write_text(json.dumps(contract, indent=2) + '\n')
    evidence_paths = {
        'package': receipt, 'runtime': runtime, 'loads': relocated / 'loaded-paths.json',
        'dyld': relocated / 'evidence/app.log', 'source-contract': contract_path,
        'bundled': app / (NOTICES + 'BUNDLED_COMPONENTS.json'),
        'planegcs': app / (SOURCE + 'third_party/planegcs/sources.json'),
        'ondsel': app / (SOURCE + 'third_party/ondsel/sources.json'),
        'qt-recipe': app / (SOURCE + 'tools/dependencies/private_qt_recipe.py'),
    }
    evidence = {key: {'path': str(path.resolve()), 'sha256': sha(path)} for key, path in evidence_paths.items()}
    loads = json.loads(evidence_paths['loads'].read_text())['nonSystem']
    bundled = json.loads(evidence_paths['bundled'].read_text())
    groups = {key: [] for key in ('alloy', 'alloy-local-client', 'planegcs', 'assembly-adapter',
                                  'ondsel', 'qt', 'qt-plugins', 'occt', 'freetype', 'tbb',
                                  'libpng', 'fonts', 'notices', 'resources')}
    for name in sorted(package_files(app)):
        basename = Path(name).name
        if name == 'Contents/MacOS/alloy':
            cid = 'alloy'
        elif name.startswith('Contents/MacOS/'):
            cid = 'alloy-local-client'
        elif name == NOTICES + 'DejaVu-LICENSE':
            cid = 'fonts'
        elif name.startswith(NOTICES):
            cid = 'notices'
        elif 'Qt' in name and name.startswith('Contents/Frameworks/'):
            cid = 'qt'
        elif name.startswith('Contents/PlugIns/'):
            cid = 'qt-plugins'
        elif basename.startswith('liballoy_planegcs'):
            cid = 'planegcs'
        elif basename.startswith('liballoy_assembly_solver'):
            cid = 'assembly-adapter'
        elif basename.startswith('libOndselSolver'):
            cid = 'ondsel'
        elif basename.startswith('libTK') or name.startswith('Contents/Resources/occt/'):
            cid = 'occt'
        elif basename.startswith('libfreetype'):
            cid = 'freetype'
        elif basename.startswith('libtbb'):
            cid = 'tbb'
        elif basename.startswith('libpng'):
            cid = 'libpng'
        else:
            cid = 'resources'
        groups[cid].append(file_record(app, name))
    sources = {
        'alloy': reference('source-contract', '/source_commit'),
        'alloy-local-client': reference('source-contract', '/source_commit'),
        'assembly-adapter': reference('source-contract', '/source_commit'),
        'resources': reference('source-contract', '/source_commit'),
        'notices': reference('package'), 'fonts': reference('source-contract', '/fonts'),
        'planegcs': reference('planegcs'), 'ondsel': reference('ondsel'),
        **{key: reference('bundled', '/' + name) for key, name in
           (('qt', 'Qt'), ('qt-plugins', 'Qt'), ('occt', 'OCCT'), ('freetype', 'FreeType'),
            ('tbb', 'TBB'), ('libpng', 'libpng'))},
    }
    licenses = {
        'planegcs': ['LICENSES/PlaneGCS-LGPL-2.1-or-later.txt', 'LICENSES/BSL-1.0.txt', 'LICENSES/MPL-2.0.txt'],
        'ondsel': ['OndselSolver-LICENSE'], 'assembly-adapter': ['LICENSE', 'OndselSolver-LICENSE'],
        'qt': ['LICENSES/LGPL-3.0-only.txt'], 'qt-plugins': ['LICENSES/LGPL-3.0-only.txt'],
        'occt': ['OCCT-LGPL-2.1.txt', 'OCCT-exception.txt'],
        'freetype': ['FreeType-2.14.3.txt'], 'tbb': ['TBB.txt'], 'libpng': ['libpng.txt'],
        'fonts': ['DejaVu-LICENSE'],
    }
    components = []
    for cid, files in groups.items():
        owned = {str((app / item['path']).resolve()) for item in files if item['kind'] == 'file'}
        observed = sorted(set(loads) & owned)
        resource = cid in ('fonts', 'notices', 'resources')
        built = {'status': 'unavailable', 'reason': contract['built_input_status']}
        if cid == 'alloy':
            ref = reference('package', '/source_app_sha256')
            built = {'status': 'recorded', 'reference': ref,
                     'sha256': json.loads(receipt.read_text())['source_app_sha256']}
        elif cid in ('occt', 'freetype', 'tbb', 'libpng'):
            key = {'occt': 'OCCT', 'freetype': 'FreeType', 'tbb': 'TBB', 'libpng': 'libpng'}[cid]
            built = {'status': 'recorded', 'reference': reference('bundled', '/' + key + '/source_sha256'),
                     'sha256': bundled[key]['source_sha256'],
                     'scope': 'representative library only, named by source_library in the same evidence record'}
        elif cid == 'qt':
            built = {'status': 'recorded', 'reference': reference('bundled', '/Qt/linked_sdk_framework_sha256/Core'),
                     'sha256': bundled['Qt']['linked_sdk_framework_sha256']['Core'],
                     'scope': 'QtCore only; Core/Gui/Widgets/Network hashes in source evidence; DBus input hash unavailable'}
        elif cid == 'qt-plugins':
            built = {'status': 'recorded', 'reference': reference('bundled', '/Qt/cocoa_plugin_source_sha256'),
                     'sha256': bundled['Qt']['cocoa_plugin_source_sha256'],
                     'scope': 'Cocoa only; other plugin input hashes unavailable'}
        component = {'id': cid, 'role': 'resource' if resource else 'runtime-library-or-executable',
                     'source': sources[cid], 'notices': [NOTICES + p for p in licenses.get(cid, ['LICENSE', 'THIRD_PARTY_NOTICES.md'])],
                     'built_input': built, 'files': files,
                     'runtime': {'status': 'observed', 'paths': observed} if observed else
                                {'status': 'nonloaded-resource' if resource else 'not-observed',
                                 'reason': 'Not a dyld image; resource usage is not established by load logging.' if resource else
                                           'Packaged; not present in this retained process load observation.'}}
        if cid == 'fonts':
            component['embedded_resources'] = {
                'container': 'Contents/MacOS/alloy', 'resource_prefix': ':/alloy/fonts/',
                'source_sha256': contract['fonts']['sha256'],
                'status': 'Pinned CMake declares these embedded fonts; individual embedded bytes and font usage not extracted/observed.'}
        components.append(component)
    manifest = {
        'schema': SCHEMA, 'target': 'macos-arm64', 'archive_sha256': sha(archive),
        'archive_package_root': 'Alloy.app', 'observed_package_root': str(app),
        'evidence': evidence, 'package_receipt': reference('package'), 'bundled_sources': reference('bundled'),
        'source_contract': reference('source-contract'), 'source_commit': reference('runtime', '/sourceCommit'),
        'load_observation': {'paths': reference('loads', '/nonSystem'), 'outside': reference('loads', '/outsideBundle'),
                             'raw': reference('dyld')},
        'components': components,
        'lib3mf': 'not-applicable: absent from pinned source build contract',
        'limits': [contract['built_input_status'],
                   'Historical dyld observation proves paths for that process, not that every packaged plugin/resource was used.',
                   'Built-input hashes precede install-name edits and ad-hoc signing; packaged hashes are separately anchored in the unchanged archive.',
                   'Embedded font bytes/usage are not independently extracted; pinned source contract and containing executable are hashed.',
                   'No Windows/Linux, clean-host, physical-input, source-access, trusted-signing or release qualification.'],
    }
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--relocated', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = generate(args.candidate.resolve(), args.relocated.resolve(), args.output.resolve())
    print(json.dumps({'manifest': str(args.output / 'manifest.json'), 'archive_sha256': manifest['archive_sha256']}, indent=2))


if __name__ == '__main__':
    main()
