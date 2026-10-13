#!/usr/bin/env python3
"""Targeted real-app export follow-up using preserved ornament documents."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import traceback
from app_harness import application
from stl_export_api import inspect_stl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True, type=Path)
    parser.add_argument('--original', required=True, type=Path)
    parser.add_argument('--edited', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    report = {'status': 'incomplete', 'fullTutorialAccepted': False,
              'appSha256': hashlib.sha256(args.app.read_bytes()).hexdigest(), 'exports': []}
    transcript = []
    try:
        with application(args.app, out, command_timeout=180) as (call, transcript):
            for label, source in [('original', args.original), ('edited', args.edited)]:
                source = source.resolve()
                digest = hashlib.sha256(source.read_bytes()).hexdigest()
                (out/'progress.json').write_text(json.dumps({'model': label, 'command': 'document.open'}))
                call('document.open', {'path': str(source), 'discard': True})
                before = call('document.inspect')
                bodies = before['evaluation']['bodies']
                assert len(bodies) == 1
                body = bodies[0]
                assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
                path = out/(label+'-mm.stl')
                result = call('document.exportStl', {'path': str(path), 'bodyId': body['bodyId'],
                    'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
                mesh = inspect_stl(path, True)
                assert len(mesh['components']) == 1
                assert mesh['triangles'] == result['triangles']
                assert path.stat().st_size == result['bytes']
                assert result['bodyIds'] == [body['bodyId']] and result['authoredChanged'] is False
                assert math.isclose(mesh['signedVolume'], body['volumeMm3'], rel_tol=.01)
                assert call('document.inspect')['authored'] == before['authored']
                assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
                report['exports'].append({'model': label, 'source': str(source), 'sourceSha256': digest,
                    'path': str(path), 'body': body, 'export': result, 'mesh': mesh})
            assert report['exports'][0]['body']['bodyId'] == report['exports'][1]['body']['bodyId']
            report['status'] = 'reexport-api-pass'
    except BaseException as error:
        report['status'] = 'failed'
        report['failure'] = {'type': type(error).__name__, 'message': str(error),
                             'traceback': traceback.format_exc()}
        raise
    finally:
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
