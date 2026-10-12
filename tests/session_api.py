#!/usr/bin/env python3
"""Real command sessions, shared dialogs and live multi-instance discovery."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import request, discover, verify_instance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve(); output.mkdir(parents=True, exist_ok=True)
    processes, logs, transcript = [], [], []
    with tempfile.TemporaryDirectory(prefix='alloy-sessions-', dir='/tmp') as directory:
        directory = Path(directory)
        try:
            for i in range(2):
                ready = directory / f'ready-{i}.json'
                log = (output / f'app-{i}.log').open('w'); logs.append(log)
                process = subprocess.Popen([str(Path(args.app).resolve()), '--ready-file', str(ready), '--instance-dir', str(directory / 'instances')], stdout=log, stderr=subprocess.STDOUT)
                processes.append(process)
                deadline = time.monotonic() + 12
                while not ready.exists():
                    assert process.poll() is None, 'App exited before ready'
                    assert time.monotonic() < deadline, 'Startup timed out'
                    time.sleep(0.05)
            records = [json.loads((directory / f'ready-{i}.json').read_text()) for i in range(2)]
            assert all(verify_instance(record)['pid'] == process.pid for record, process in zip(records, processes))
            assert records[0]['instanceId'] != records[1]['instanceId']
            instances = discover(directory / 'instances'); assert len(instances) == 2 and all(i['discoveryStatus'] == 'live' for i in instances)
            cli = [sys.executable, str(Path(__file__).resolve().parents[1] / 'tools/alloyctl.py'), '--instance-dir', str(directory / 'instances')]
            ambiguous = subprocess.run([*cli, 'state'], capture_output=True, text=True)
            assert ambiguous.returncode == 2, ambiguous
            selected = subprocess.run([*cli, '--instance', records[1]['instanceId'], 'state'], capture_output=True, text=True)
            assert selected.returncode == 0 and json.loads(selected.stdout)['result']['identity']['pid'] == processes[1].pid
            endpoint = records[0]['socket']
            def call(command, parameters=None, error=None):
                response = request(endpoint, command, parameters)
                transcript.append({'command': command, 'args': parameters or {}, 'response': response})
                assert response['ok'] == (error is None), response
                assert response['completion'] == ('failed' if error else 'completed'), response
                if error: assert response['error']['code'] == error, response
                return response.get('result', {}), response
            initial, _ = call('app.state')
            original = initial['document']; original_authored = original['authored']
            session, _ = call('command.begin', {'command': fixture['target'], 'inputs': {}})
            key = {'session': session['id']}
            call('command.update', {**key, 'inputs': fixture['inputs']})
            state, _ = call('app.state')
            assert state['document'] == original and state['ui']['activeDialog']['inputs'] == fixture['inputs'] and state['ui']['activeDialog']['visible']
            preview, _ = call('command.preview', key)
            assert not preview['geometryEvaluated'] and preview['authoredDraft']['displayUnits'] == 'in'
            _, failed = call('command.update', {**key, 'inputs': fixture['invalid_inputs']}, 'invalid_arguments')
            assert failed['error']['field'] == 'units'
            capture, _ = call('capture.dialog', {'path': str(output / 'document-settings.png')})
            assert capture['pixelWidth'] > 100 and (output / 'document-settings.png').stat().st_size > 100
            call('command.cancel', key)
            call('command.commit', key, 'invalid_session')
            state, _ = call('app.state'); assert state['document'] == original and not state['activeSession']
            session, _ = call('command.begin', {'command': fixture['target'], 'inputs': fixture['inputs']})
            committed, _ = call('command.commit', {'session': session['id']})
            state, _ = call('app.state')
            assert state['document']['units'] == 'in' and state['document']['undo']['count'] == 1
            assert state['document']['lastTransactionId'] and state['document']['authored']['documentId'] == original_authored['documentId']
            assert committed['commandResult']['documentRevision'] == state['document']['revision']
            session, _ = call('command.begin', {'command': fixture['target'], 'inputs': {'units': 'in'}})
            call('document.units', {'units': 'mm'})
            call('command.commit', {'session': session['id']}, 'stale_session')
            call('command.cancel', {'session': session['id']})
            call('commands.search')
            state, _ = call('app.state'); session = state['activeSession']
            call('command.update', {'session': session['id'], 'inputs': {'query': fixture['search_query']}})
            state, _ = call('app.state'); assert state['ui']['activeDialog']['inputs']['query'] == fixture['search_query']
            call('commands.close')
            # Dropping an incomplete request cannot commit a command.
            before, _ = call('document.inspect')
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                client.connect(endpoint); client.sendall(b'{"command":"document.units"')
            after, _ = call('document.inspect'); assert after == before
            wrong = dict(records[0]); wrong['instanceId'] = records[1]['instanceId']
            (directory / 'wrong.json').write_text(json.dumps(wrong))
            stale = subprocess.run([*cli, '--ready-file', str(directory / 'wrong.json'), 'exec', 'document.units', '--args', '{"units":"in"}'], capture_output=True, text=True)
            assert stale.returncode == 2 and call('document.inspect')[0] == before
            call('app.quit'); assert processes[0].wait(timeout=5) == 0
            # A dead registry record is classified stale; the other live app is selected.
            (directory / 'instances/dead.json').write_text(json.dumps(records[0]))
            remaining = discover(directory / 'instances')
            assert sorted(i['discoveryStatus'] for i in remaining) == ['live', 'stale']
            recovered = subprocess.run([*cli, 'state'], capture_output=True, text=True)
            assert recovered.returncode == 0 and json.loads(recovered.stdout)['result']['identity']['pid'] == processes[1].pid
            request(records[1]['socket'], 'app.quit'); assert processes[1].wait(timeout=5) == 0
            report = {'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
                      'verifiedCapabilities': ['commands.sessions', 'commands.shared_dialog', 'commands.completion', 'instances.discovery'],
                      'initialState': initial, 'transcript': transcript, 'discovery': {'instances': instances, 'staleRecovered': remaining},
                      'evidenceDimensions': {'api': {'status': 'passed'}, 'visual': {'status': 'captured', 'reason': 'Actual settings dialog captured; not visual parity acceptance.'},
                                             'geometry': {'status': 'not_applicable', 'reason': 'Session target edits display units only.'}}}
            (output / 'session-api.json').write_text(json.dumps(report, indent=2) + '\n')
            print('PASS: real dialogs/sessions, commit/cancel/error isolation and live instance selection/recovery')
        finally:
            for process in processes:
                if process.poll() is None: process.terminate(); process.wait(timeout=5)
            for log in logs: log.close()


if __name__ == '__main__':
    main()
