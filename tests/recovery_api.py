#!/usr/bin/env python3
"""One isolated real-app interruption and committed-document recovery gate."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import record_endpoint, request


def launch(app, root, evidence, name):
    ready = root / f'{name}-ready.json'
    instances = root / f'{name}-instances'
    log = (evidence / f'{name}-app.log').open('w')
    environment = os.environ.copy()
    environment['ALLOY_RECOVERY_DIR'] = str(root / 'recovery')
    process = subprocess.Popen([str(app), '--ready-file', str(ready),
        '--instance-dir', str(instances)], env=environment, stdout=log,
        stderr=subprocess.STDOUT)
    deadline = time.monotonic() + 20
    while not ready.exists():
        assert process.poll() is None, f'{name} exited before ready'
        assert time.monotonic() < deadline, f'{name} startup timed out'
        time.sleep(.05)
    endpoint, transport = record_endpoint(json.loads(ready.read_text()))
    log.close()
    return process, endpoint, transport


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--evidence', required=True)
    parser.add_argument('--mesh-fixture', help='Optional retained STL for source-independent crash recovery')
    args = parser.parse_args()
    evidence = Path(args.evidence).resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    app = Path(args.app).resolve()
    transcript = []
    with tempfile.TemporaryDirectory(prefix='alloy-recovery-') as directory:
        root = Path(directory)
        def call(endpoint, transport, command, arguments=None, expected_error=None):
            response = request(endpoint, command, arguments, timeout=120,
                transport=transport, helper=None)
            transcript.append({'command': command, 'args': arguments or {},
                'response': response})
            assert response['ok'] == (expected_error is None), response
            if expected_error:
                assert response['error']['code'] == expected_error, response
            return response

        first, endpoint, transport = launch(app, root, evidence, 'before-kill')
        try:
            assert call(endpoint, transport, 'document.recoveryList')['result']['count'] == 0
            add = call(endpoint, transport, 'document.parameterAdd',
                {'name': 'Width', 'value': 37, 'units': 'mm'})
            assert add['recovery']['durable'], add
            box = call(endpoint, transport, 'solid.box',
                {'width': 37, 'depth': 12, 'height': 5, 'units': 'mm'})
            assert box['recovery']['durable'], box
            mesh_page = None
            if args.mesh_fixture:
                mesh_source=root / 'capacity.stl';shutil.copyfile(args.mesh_fixture,mesh_source)
                response=call(endpoint,transport,'document.importStl',{'path':str(mesh_source),'units':'mm'})
                assert response['recovery']['durable']
                mesh_state=call(endpoint,transport,'document.inspect')['result']
                mesh_id=mesh_state['meshes'][0]['id']
                mesh_page=call(endpoint,transport,'mesh.inspect',{'meshId':mesh_id,'vertexOffset':599000,'triangleOffset':199000})['result']
                assert mesh_page['verticesMm'][-1]==[498,1599,0] and mesh_page['triangles'][-1]==[599997,599998,599999]
                mesh_source.unlink()
            unnamed = call(endpoint, transport, 'document.inspect')['result']
            assert unnamed['dirty'] and not unnamed['path'] and unnamed['bodies'] == 1
            # document.inspect intentionally omits bulk retained arrays. Keep the
            # full durable pre-crash authored record for native-save equality.
            full_unnamed=unnamed['authored']
            if mesh_page is not None:
                snapshots=list((root/'recovery').glob('*/[0-9a-f-]*.json'))
                assert len(snapshots)==1
                full_unnamed=json.loads(snapshots[0].read_text())['authored']

            assert call(endpoint, transport, 'document.recoveryList')['result']['count'] == 0, 'live owner must be hidden'

            call(endpoint, transport, 'document.tabNew')
            call(endpoint, transport, 'solid.cylinder',
                {'radius': 4, 'height': 8, 'units': 'mm'})
            original = root / 'original.alloy'
            saved = call(endpoint, transport, 'document.save', {'path': str(original)})
            assert saved['recovery']['durable'] and not saved['result']['dirty'], saved
            original_hash = hashlib.sha256(original.read_bytes()).hexdigest()
            named_add = call(endpoint, transport, 'document.parameterAdd',
                {'name': 'Depth', 'value': 19, 'units': 'mm'})
            assert named_add['recovery']['durable'], named_add
            named = call(endpoint, transport, 'document.inspect')['result']
            assert named['dirty'] and named['path'] == str(original)
            os.kill(first.pid, signal.SIGKILL)
            first.wait(timeout=5)
        finally:
            if first.poll() is None:
                first.kill()
                first.wait(timeout=5)

        sessions = list((root / 'recovery').glob('*/[0-9a-f-]*.json'))
        assert len(sessions) == 2, sessions
        future_session = root / 'recovery' / str(uuid.uuid4())
        corrupt_session = root / 'recovery' / str(uuid.uuid4())
        future_session.mkdir(mode=0o700)
        corrupt_session.mkdir(mode=0o700)
        future_tab = str(uuid.uuid4())
        corrupt_tab = str(uuid.uuid4())
        future = json.loads(sessions[0].read_text())
        future['schema'] = 'alloy-recovery/2'
        (future_session / f'{future_tab}.json').write_text(json.dumps(future))
        (corrupt_session / f'{corrupt_tab}.json').write_text('{bad json')

        second, endpoint, transport = launch(app, root, evidence, 'after-kill')
        try:
            listing = call(endpoint, transport, 'document.recoveryList')['result']
            statuses = {row['status'] for row in listing['candidates']}
            assert listing['count'] == 4 and statuses == {'ready', 'malformed', 'unsupported_version'}, listing
            assert sorted(row['originalPath'] for row in listing['candidates'] if row['status'] == 'ready') == ['', str(original)]
            for row in listing['candidates']:
                if row['status'] == 'ready':
                    continue
                before = call(endpoint, transport, 'document.inspect')['result']['authored']
                call(endpoint, transport, 'document.recover', {'id': row['id']}, 'recovery_failed')
                assert call(endpoint, transport, 'document.inspect')['result']['authored'] == before
                call(endpoint, transport, 'document.recoveryDiscard', {'id': row['id']})
            ready = [row for row in listing['candidates'] if row['status'] == 'ready']
            recovered = []
            for row in ready:
                response = call(endpoint, transport, 'document.recover', {'id': row['id']})
                assert response['recovery']['durable'], response
                document = response['result']
                source = unnamed if row['originalPath'] == '' else named
                assert document['authored'] == source['authored'], 'exact authored IDs and parameters'
                assert document['dirty'] and not document['path'] and document['recoveredIntoNewTab']
                assert document['id'] == source['id']
                if source is unnamed and mesh_page is not None:
                    assert document['meshes']==unnamed['meshes']
                    assert call(endpoint,transport,'mesh.inspect',{'meshId':mesh_id,'vertexOffset':599000,'triangleOffset':199000})['result']==mesh_page
                    assert not mesh_source.exists()

                recovered.append({'source': row['originalPath'], 'tab': call(endpoint, transport, 'document.tabs')['result']['activeTabId'],
                    'documentId': document['id'], 'features': document['features'],
                    'parameterIds': [parameter['id'] for parameter in document['authored'].get('userParameters', [])]})
                # A failed native Save must not make this recovered tab clean.
                call(endpoint, transport, 'document.save', {'path': str(root / 'not-a-file.alloy' / 'bad.alloy')}, 'save_failed')
                assert call(endpoint, transport, 'document.inspect')['result']['dirty']
                call(endpoint, transport, 'document.tabClose',
                    {'tabId': recovered[-1]['tab']}, 'unsaved_changes')
                destination = evidence / ('recovered-unnamed.alloy' if not row['originalPath'] else 'recovered-named.alloy')
                saved = call(endpoint, transport, 'document.save', {'path': str(destination)})
                assert saved['recovery']['durable'] and not saved['result']['dirty']
                assert json.loads(destination.read_text()) == (full_unnamed if source is unnamed else source['authored'])
            assert call(endpoint, transport, 'document.recoveryList')['result']['count'] == 0
            assert hashlib.sha256(original.read_bytes()).hexdigest() == original_hash, 'source file changed'
            call(endpoint, transport, 'app.quit')
            assert second.wait(timeout=5) == 0
        finally:
            if second.poll() is None:
                second.kill()
                second.wait(timeout=5)
        (evidence / 'result.json').write_text(json.dumps({
            'ok': True, 'originalSha256': original_hash,
            'retainedMeshSourceIndependent': mesh_page is not None,
            'unnamedDocumentId': unnamed['id'], 'namedDocumentId': named['id'],
            'recovered': recovered, 'recoveryRootAfterQuit': list((root / 'recovery').glob('*/[0-9a-f-]*.json')),
        }, indent=2, default=str))
        (evidence / 'transcript.json').write_text(json.dumps(transcript, indent=2))
    print('PASS isolated forced-kill recovery, live exclusion, exact IDs/parameters, malformed/newer refusal, safe Save As')


if __name__ == '__main__':
    main()
