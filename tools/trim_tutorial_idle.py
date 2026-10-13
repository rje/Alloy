#!/usr/bin/env python3
"""Remove idle gaps from a completed Alloy recording, retaining an explicit edit map.

Requires ffmpeg, ffprobe and Pillow. Raw media and events are never modified.
This creates an edited demonstration, not an uninterrupted verification receipt.
"""
import argparse
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw, ImageFont


def probe(path):
    result = subprocess.check_output(['ffprobe', '-v', 'error', '-select_streams', 'v:0',
        '-show_entries', 'stream=width,height,r_frame_rate,nb_frames,duration',
        '-of', 'json', str(path)], text=True)
    return json.loads(result)['streams'][0]


def digest(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def relevant(event):
    if event['type'] in {'mouse_move', 'mouse_press', 'mouse_release', 'key_press',
                         'key_release', 'wheel', 'step'}:
        return True
    if event['type'] != 'command':
        return False
    command = event.get('id', '')
    # Keep all UI commands and any API operation that could change visible state.
    # Read-only inspection is not a reason to retain a long desktop-control pause.
    return event.get('origin') != 'api' or not (
        command.startswith(('capture.', 'view.project')) or
        command in {'app.state', 'document.inspect', 'command.inspect', 'command.list',
                    'recording.status', 'recording.start', 'recording.stop'})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raw', type=Path, required=True)
    parser.add_argument('--events', type=Path, help='Defaults to RAW.mp4.jsonl')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--before', type=float, default=1.0)
    parser.add_argument('--after', type=float, default=3.0)
    args = parser.parse_args()
    raw, output = args.raw.resolve(), args.output.resolve()
    events_path = (args.events or Path(str(raw) + '.jsonl')).resolve()
    assert args.before >= 0 and args.after >= 0
    assert raw != output and not output.exists(), 'Choose a new output path'
    assert not output.is_relative_to(Path(__file__).resolve().parents[1]), 'Store media outside Git'
    events = [json.loads(line) for line in events_path.read_text().splitlines() if line.strip()]
    assert any(e['type'] == 'stop' for e in events), 'Recording must have stopped cleanly'
    info = probe(raw)
    rate = Fraction(info['r_frame_rate'])
    fps = float(rate)
    count = int(info['nb_frames'])
    observed = [(index, e) for index, e in enumerate(events) if relevant(e)]
    intervals = [(0, min(count, math.ceil(2 * fps))),
                 (max(0, count - math.ceil(3 * fps)), count)]
    for _, event in observed:
        time = event['elapsedMs'] / 1000
        intervals.append((max(0, math.floor((time - args.before) * fps)),
                          min(count, math.ceil((time + args.after) * fps) + 1)))
    windows = []
    for start, end in sorted(intervals):
        if end <= start:
            continue
        if windows and start <= windows[-1][1]:
            windows[-1][1] = max(windows[-1][1], end)
        else:
            windows.append([start, end])
    for _, event in observed:
        frame = min(count - 1, math.floor(event['elapsedMs'] * fps / 1000))
        assert any(start <= frame < end for start, end in windows), 'An action would be omitted'

    output.parent.mkdir(parents=True, exist_ok=True)
    banner_path = output.with_suffix('.banner.png')
    width = int(info['width'])
    height = max(28, round(width * 0.022))
    banner = Image.new('RGB', (width, height), '#17252e')
    ImageDraw.Draw(banner).text((10, 4), 'EDITED: idle waits removed | Original recording retained',
        font=ImageFont.load_default(size=max(16, round(height * 0.65))), fill='white')
    banner.save(banner_path)
    selection = '+'.join(f'between(n,{start},{end-1})' for start, end in windows)
    filters = (f"[0:v]select='{selection}',setpts=N/({rate}*TB)[cut];"
               '[cut][1:v]overlay=0:0:shortest=1[out]')
    with output.with_suffix('.ffmpeg.log').open('w') as log:
        subprocess.run(['ffmpeg', '-hide_banner', '-n', '-i', str(raw), '-loop', '1',
            '-i', str(banner_path), '-filter_complex', filters, '-map', '[out]', '-an',
            '-r', str(rate), '-c:v', 'libx264', '-preset', 'fast', '-crf', '20',
            '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(output)],
            stdout=log, stderr=subprocess.STDOUT, check=True)
    edited = probe(output)
    expected_frames = sum(end-start for start, end in windows)
    assert int(edited['nb_frames']) == expected_frames, 'Output timing disagrees with edit map'
    cursor, mapped = 0, []
    for start, end in windows:
        mapped.append({'rawStartFrame': start, 'rawEndFrameExclusive': end,
                       'editedStartFrame': cursor, 'editedEndFrameExclusive': cursor + end-start,
                       'rawStartSeconds': start/fps, 'rawEndSeconds': end/fps,
                       'editedStartSeconds': cursor/fps, 'playbackSpeed': 1.0,
                       'observedEventIndexes': [i for i, e in observed
                           if start <= min(count-1, math.floor(e['elapsedMs']*fps/1000)) < end]})
        cursor += end-start
    receipt = {'edited': True, 'policy': 'Normal-speed hard cuts remove idle gaps only; persistent edit banner.',
               'rawVideo': str(raw), 'rawSha256': digest(raw), 'rawEventLog': str(events_path),
               'rawEventLogSha256': digest(events_path), 'editedVideo': str(output),
               'editedSha256': digest(output), 'fps': str(rate),
               'rawDurationSeconds': count/fps, 'editedDurationSeconds': cursor/fps,
               'retainedActionCount': len(observed), 'allRelevantRecordedEventsRetained': True,
               'beforeSeconds': args.before, 'afterSeconds': args.after, 'windows': mapped}
    output.with_suffix('.edit-map.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'output': str(output), 'durationSeconds': cursor/fps,
                      'retainedActions': len(observed), 'segments': len(windows)}))


if __name__ == '__main__':
    main()
