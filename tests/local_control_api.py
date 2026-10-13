#!/usr/bin/env python3
"""Bounded real-app local discovery and Qt byte bridge regression."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from app_harness import application
from alloyctl import request, exchange, verify_instance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--helper', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    helper = str(Path(args.helper).resolve())
    with application(args.app, output) as (call, transcript):
        identity = call('app.identity')
        assert verify_instance(call.discovery, helper=helper)['instanceId'] == identity['instanceId']
        bridge = request(call.endpoint, 'app.state', transport=call.transport, helper=helper)
        assert bridge['ok'] and bridge['result']['document']['features'] == 0
        stale = dict(call.discovery, instanceId='stale-identity')
        try:
            verify_instance(stale, helper=helper)
        except RuntimeError as error:
            assert str(error) == 'Stale discovery record: live instance identity does not match'
        else:
            raise AssertionError('Stale identity accepted')
        cli = subprocess.run([sys.executable, str(Path(__file__).parents[1]/'tools/alloyctl.py'),
            '--ready-file', str(call.ready_file), '--transport-helper', helper, 'exec', 'app.identity'],
            capture_output=True, text=True, timeout=5)
        assert cli.returncode == 0, cli.stderr
        assert json.loads(cli.stdout)['result']['instanceId'] == identity['instanceId']
        malformed = json.loads(exchange(call.endpoint, b'not JSON\n', transport=call.transport, helper=helper))
        assert not malformed['ok'] and malformed['error']['code'] == 'invalid_request'
        call('view.orientation', {'name':'top'})
        assert call('app.state')['viewport']['orientation'] == 'top'
        call('commands.search')
        assert call('app.state')['ui']['commandSearchVisible']
        call('commands.close')
        capture = call('capture.screenshot', {'path':str(output/'local-control.png')})
        assert (output/'local-control.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
        report = {'result':'passed', 'scope':'One real app; platform discovery, trusted Qt bridge, identity/stale refusal, commands, capture, clean quit. No Windows desktop or second-user claim.',
            'discovery':call.discovery, 'identity':identity, 'capture':capture, 'bridgeState':bridge['result'], 'transcript':transcript}
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: real-app discovery, Qt bridge, identity, commands, capture and clean quit')


if __name__ == '__main__':
    main()
