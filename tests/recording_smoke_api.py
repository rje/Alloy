#!/usr/bin/env python3
"""Record a short, real-app sketch-to-extrusion API clip."""
import argparse
from contextlib import ExitStack
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--video', type=Path, required=True)
    args = parser.parse_args()
    video = args.video.resolve()
    assert video.is_absolute() and video.suffix.lower() == '.mp4'
    assert not video.is_relative_to(Path(__file__).resolve().parents[1])
    output = Path(args.output).resolve()
    video.parent.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    receipt = {'result': 'failed', 'video': str(video), 'source': 'Alloy local API',
               'physicalInput': 'not qualified by this API clip'}
    transcript = []
    try:
        with application(args.app, output) as (call, transcript), ExitStack() as recording_guard:
            call('window.resize', {'width': 1280, 'height': 800})
            initial = call('document.inspect')['authored']
            assert not initial['features']
            started = call('recording.start', {'path': str(video), 'fps': 15})
            active = True

            def stop_active_recording():
                if active:
                    call('recording.stop')

            recording_guard.callback(stop_active_recording)
            assert call('recording.status')['active']
            assert call('document.inspect')['authored'] == initial
            call('recording.step', {'text': 'Alloy local API  |  Blank design'})
            time.sleep(1.2)
            call('recording.step', {'text': 'Sketch  |  Draw a 40 × 25 mm rectangle'})
            session = call('sketch.begin', {'plane': 'XY', 'name': 'Recording smoke sketch'})
            time.sleep(0.8)
            call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 40, 'y2': 25, 'units': 'mm'})
            preview = call('command.preview', {'session': session['id']})
            assert len(preview['sketchGeometry']['profiles']) == 1
            profile_id = preview['sketchGeometry']['profiles'][0]['profileId']
            sketch_id = preview['authoredDraft']['id']
            time.sleep(1.2)
            call('recording.step', {'text': 'Profile  |  Finish and select the closed region'})
            call('sketch.finish')
            call('solid.pressPullDialog', {'sketchId': sketch_id})
            call('selection.profileSet', {'sketchId': sketch_id, 'profileId': profile_id})
            time.sleep(1.2)
            call('recording.step', {'text': 'Solid  |  Extrude 5 mm'})
            active = call('app.state')['activeSession']
            call('command.update', {'session': active['id'], 'inputs': {'distance': 5, 'units': 'mm'}})
            call('command.preview', {'session': active['id']})
            time.sleep(1.2)
            call('command.commit', {'session': active['id']})
            call('view.home')
            solid = call('document.inspect')
            assert solid['bodies'] == 1 and solid['evaluation']['bodies'][0]['valid']
            assert abs(solid['evaluation']['bodies'][0]['volumeMm3'] - 5000) < 1e-4
            call('recording.step', {'text': 'Complete  |  One valid 40 × 25 × 5 mm solid'})
            time.sleep(2.2)
            assert call('recording.status')['active']
            active = False
            stopped = call('recording.stop')
            assert not call('recording.status')['active']
            assert call('document.inspect')['authored'] == solid['authored']
            assert video.is_file() and video.stat().st_size > 1024
            receipt.update(result='passed', start=started, stop=stopped,
                           videoSha256=hashlib.sha256(video.read_bytes()).hexdigest(),
                           authoredFeatures=len(solid['authored']['features']))
            if shutil.which('ffprobe'):
                probe = subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v:0',
                                        '-show_entries', 'stream=codec_name,width,height,nb_frames',
                                        '-show_entries', 'format=duration', '-of', 'json', str(video)],
                                       capture_output=True, text=True, check=True)
                receipt['probe'] = json.loads(probe.stdout)
                assert float(receipt['probe']['format']['duration']) >= 7
    finally:
        receipt['transcript'] = transcript
        (output / 'recording-smoke.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'result': receipt['result'], 'receipt': str(output / 'recording-smoke.json')}))


if __name__ == '__main__':
    main()
