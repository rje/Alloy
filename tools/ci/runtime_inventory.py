#!/usr/bin/env python3
"""Validate a package/source/load evidence join without launching or changing it.

The inventory is an evidence index, not an SBOM or a source-access/release gate.
See docs/validation/runtime-inventory.md for schema and platform adapter contract.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
import zipfile

SCHEMA = 'alloy-runtime-inventory/1'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def sha(path):
    return digest(path.read_bytes())


def safe(root, relative):
    name = PurePosixPath(relative)
    if not relative or name.is_absolute() or '..' in name.parts or '\\' in relative:
        raise ValueError(f'Unsafe package path: {relative}')
    path = root / relative
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError(f'External package symlink: {relative}')
    return path


def package_files(root):
    return {p.relative_to(root).as_posix() for p in root.rglob('*')
            if p.is_file() or p.is_symlink()}


def file_record(root, relative):
    path = safe(root, relative)
    if path.is_symlink():
        return {'path': relative, 'kind': 'symlink', 'sha256': digest(path.readlink().as_posix().encode())}
    return {'path': relative, 'kind': 'file', 'sha256': sha(path)}


def reference(evidence, pointer=''):
    return {'evidence': evidence, 'pointer': pointer}


def resolve_ref(ref, evidence):
    key = ref['evidence']
    if key not in evidence:
        raise ValueError(f'Unresolved evidence link: {key}')
    path = Path(evidence[key]['path'])
    pointer = ref.get('pointer', '')
    if pointer == '':
        text = path.read_text(errors='strict')
        return json.loads(text) if path.suffix == '.json' else text
    if not pointer.startswith('/'):
        raise ValueError(f'Invalid JSON pointer in {key}: {pointer}')
    value = json.loads(path.read_text())
    for part in pointer[1:].split('/'):
        part = part.replace('~1', '/').replace('~0', '~')
        try:
            value = value[int(part)] if isinstance(value, list) else value[part]
        except (KeyError, IndexError, TypeError, ValueError) as error:
            raise ValueError(f'Unresolved provenance link: {key}: {pointer}') from error
    if value is None or value == '' or value == {}:
        raise ValueError(f'Empty provenance in {key}: {pointer}')
    return value


def validate(manifest, root, archive):
    """Raise actionable ValueError on any missing or contradictory evidence."""
    if manifest['target'] not in ('macos-arm64', 'macos-x64', 'windows-x64', 'linux-x64'):
        raise ValueError('Unsupported target identifier')
    if manifest['schema'] != SCHEMA:
        raise ValueError('Unsupported inventory schema')
    if sha(archive) != manifest['archive_sha256']:
        raise ValueError('Archive SHA-256 mismatch')
    evidence = manifest['evidence']
    for key, item in evidence.items():
        if sha(Path(item['path'])) != item['sha256']:
            raise ValueError(f'Evidence SHA-256 mismatch: {key}')
    recorded = {}
    ids = set()
    loaded = set()
    for component in manifest['components']:
        cid = component['id']
        if cid in ids:
            raise ValueError(f'Duplicate component: {cid}')
        ids.add(cid)
        if not component['role']:
            raise ValueError(f'Missing role: {cid}')
        resolve_ref(component['source'], evidence)
        if not component['notices']:
            raise ValueError(f'Missing notice link: {cid}')
        for notice in component['notices']:
            if not safe(root, notice).is_file():
                raise ValueError(f'Unresolved notice: {cid}: {notice}')
        built = component['built_input']
        if built['status'] == 'recorded':
            value = resolve_ref(built['reference'], evidence)
            if value != built['sha256'] or not re.fullmatch('[0-9a-f]{64}', value):
                raise ValueError(f'Built input hash/reference mismatch: {cid}')
        elif built['status'] != 'unavailable' or not built.get('reason'):
            raise ValueError(f'Unresolved built-input provenance: {cid}')
        files = {item['path'] for item in component['files']}
        if not files:
            raise ValueError(f'Missing packaged file: {cid}')
        for item in component['files']:
            name = item['path']
            if name in recorded:
                raise ValueError(f'Duplicate packaged entry: {name}')
            if file_record(root, name) != item:
                raise ValueError(f'Packaged hash/kind mismatch: {name}')
            recorded[name] = item
        runtime = component['runtime']
        if runtime['status'] == 'observed':
            if not runtime['paths']:
                raise ValueError(f'Missing observed loaded paths: {cid}')
            for path in runtime['paths']:
                relative = Path(path).relative_to(Path(manifest['observed_package_root'])).as_posix()
                target = safe(root, relative).resolve()
                if not any(safe(root, name).resolve() == target for name in files):
                    raise ValueError(f'Loaded path not owned by component {cid}: {path}')
                loaded.add(path)
        elif runtime['status'] not in ('not-observed', 'nonloaded-resource') or not runtime.get('reason'):
            raise ValueError(f'Unresolved runtime role: {cid}')
    actual = package_files(root)
    if actual != recorded.keys():
        raise ValueError(f'Package coverage mismatch; missing={sorted(actual-recorded.keys())}, '
                         f'stale={sorted(recorded.keys()-actual)}')
    for component in manifest['components']:
        if not set(component['notices']) <= recorded.keys():
            raise ValueError(f'Unhashed notice: {component["id"]}')
    # The ZIP itself, not a mutable extraction alone, anchors every entry.
    with zipfile.ZipFile(archive) as zipped:
        prefix = manifest['archive_package_root'] + '/'
        members = {info.filename[len(prefix):]: info for info in zipped.infolist()
                   if info.filename.startswith(prefix) and not info.is_dir()}
        if members.keys() != recorded.keys():
            raise ValueError('Archive/extracted file coverage mismatch')
        for name, info in members.items():
            kind = 'symlink' if stat.S_ISLNK(info.external_attr >> 16) else 'file'
            if kind != recorded[name]['kind'] or digest(zipped.read(info)) != recorded[name]['sha256']:
                raise ValueError(f'Archive/extracted hash mismatch: {name}')
    load = manifest['load_observation']
    observed = resolve_ref(load['paths'], evidence)
    if not isinstance(observed, list) or not observed or set(observed) != loaded:
        raise ValueError('Loaded runtime coverage mismatch')
    if resolve_ref(load['outside'], evidence):
        raise ValueError('External runtime paths recorded')
    if manifest['target'] == 'macos-arm64':
        # Adapter requirements are independent of the manifest's component list.
        required = {'alloy', 'alloy-local-client', 'planegcs', 'assembly-adapter', 'ondsel',
                    'occt', 'freetype', 'tbb', 'libpng', 'fonts', 'qt', 'qt-plugins',
                    'notices', 'resources'}
        if not required <= ids:
            raise ValueError(f'Missing required components: {sorted(required-ids)}')
        by_id = {item['id']: item for item in manifest['components']}
        for cid in ('alloy', 'planegcs', 'assembly-adapter', 'ondsel', 'occt', 'freetype', 'qt', 'qt-plugins'):
            if by_id[cid]['runtime']['status'] != 'observed':
                raise ValueError(f'Required runtime not observed: {cid}')
        if not any('/PlugIns/platforms/libqcocoa.dylib' in p for p in loaded):
            raise ValueError('Required Cocoa plugin not observed')
        # Check the raw retained dyld trace rather than trusting its summary alone.
        trace = resolve_ref(load['raw'], evidence)
        raw = set(re.findall(r'^dyld\[\d+\]: <[^>]+> (.+)$', trace, re.M))
        nonsystem = {p for p in raw if not p.startswith(('/System/Library/', '/usr/lib/'))}
        if nonsystem != loaded:
            raise ValueError('Raw dyld trace/loaded summary mismatch')
        bundled = resolve_ref(manifest['bundled_sources'], evidence)
        for patch, expected in bundled['Qt']['patch_sha256'].items():
            matches = [name for name in recorded if name.endswith('/' + patch)]
            if len(matches) != 1 or sha(safe(root, matches[0])) != expected:
                raise ValueError(f'Qt patch source/package hash mismatch: {patch}')
        record = resolve_ref(manifest['package_receipt'], evidence)
        if record['archive_sha256'] != manifest['archive_sha256']:
            raise ValueError('Package receipt archive mismatch')
        for name, item in record['mach_o'].items():
            if sha(safe(root, name)) != item['sha256']:
                raise ValueError(f'Package receipt Mach-O hash mismatch: {name}')
        contract = resolve_ref(manifest['source_contract'], evidence)
        fonts = by_id['fonts'].get('embedded_resources', {})
        if fonts.get('container') not in recorded or fonts.get('source_sha256') != contract['fonts']['sha256']:
            raise ValueError('Unresolved embedded font source/container link')
        for name in contract['fonts']['sha256']:
            if name not in contract['cmake']:
                raise ValueError(f'Font absent from pinned resource contract: {name}')
        if contract['source_commit'] != resolve_ref(manifest['source_commit'], evidence):
            raise ValueError('Source contract/runtime receipt commit mismatch')
        if 'lib3mf' in contract['cmake'].lower():
            if 'lib3mf' not in ids or not any('/lib3mf/' in p for p in actual):
                raise ValueError('Required lib3mf source/notices missing')
        elif manifest.get('lib3mf') != 'not-applicable: absent from pinned source build contract':
            raise ValueError('Missing lib3mf applicability disposition')
    return {'schema': 'alloy-runtime-inventory-result/1', 'status': 'passed',
            'target': manifest['target'], 'scope': 'inventory join only; platform/release qualification remains separate',
            'archive_sha256': manifest['archive_sha256'],
            'components': len(ids), 'packaged_entries': len(recorded), 'observed_images': len(loaded),
            'limits': manifest['limits']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = validate(json.loads(args.manifest.read_text()), args.package.resolve(), args.archive)
        result['manifest_sha256'] = sha(args.manifest)
        result['validator_sha256'] = sha(Path(__file__))
    except (ValueError, KeyError, OSError, TypeError, zipfile.BadZipFile) as error:
        result = {'status': 'failed', 'error': str(error)}
    args.result.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(result['status'] != 'passed')


if __name__ == '__main__':
    main()
