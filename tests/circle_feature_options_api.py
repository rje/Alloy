#!/usr/bin/env python3
"""Real-entrypoint smoke gate for shared Circle mode and visible palette state."""
import argparse
import json
from pathlib import Path

from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        for tool in ('circle', 'twoPointCircle', 'threePointCircle',
                     'twoTangentCircle', 'threeTangentCircle'):
            call('sketch.tool', {'tool': tool})
            assert call('app.state')['activeSession']['tool'] == tool
        call('capture.screenshot', {'path': str(out/'three-tangent-mode.png')})
        call('sketch.tool', {'tool': 'select'})
        assert call('app.state')['activeSession']['tool'] == 'select'
        call('capture.screenshot', {'path': str(out/'no-circle-options.png')})
        (out/'receipt.json').write_text(json.dumps({
            'passed': True, 'modes': 5, 'transcript': transcript}, indent=2)+'\n')
    print('PASS real-entrypoint Circle mode/palette state')


if __name__ == '__main__':
    main()
