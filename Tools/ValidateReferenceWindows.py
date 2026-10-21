"""Run the actual product renderer on synthetic fixtures; analyse offline, not hardware."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import wave
import numpy as np


def read_pcm(path):
    with wave.open(str(path), 'rb') as f:
        rate, channels, width = f.getframerate(), f.getnchannels(), f.getsampwidth()
        raw = f.readframes(f.getnframes())
    if width == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        x = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        x = (x ^ 0x800000) - 0x800000
        x = x.astype(float) / 8388608
    elif width == 2:
        x = np.frombuffer(raw, dtype='<i2').astype(float) / 32768
    else:
        raise ValueError('Unsupported PCM width')
    return rate, x.reshape(-1, channels)[:, 0]


def dominant(x, sr):
    if len(x) < 4 or np.max(np.abs(x)) < 1e-7:
        return None
    nfft = 1 << (len(x) * 8 - 1).bit_length()
    spectrum = np.abs(np.fft.rfft((x - x.mean()) * np.hanning(len(x)), nfft))
    freqs = np.fft.rfftfreq(nfft, 1 / sr)
    selected = np.where((freqs >= 50) & (freqs <= 4000))[0]
    i = selected[np.argmax(spectrum[selected])]
    logs = np.log(np.maximum(spectrum[i-1:i+2], 1e-30))
    denominator = logs[0] - 2 * logs[1] + logs[2]
    offset = 0 if abs(denominator) < 1e-15 else .5 * (logs[0] - logs[2]) / denominator
    return float((i + np.clip(offset, -.5, .5)) * sr / nfft)


def analyse(entry, wav):
    metadata = json.loads(Path(str(wav)+'.json').read_text())
    rows = list(csv.DictReader(Path(str(wav)+'.pitch.csv').open()))
    sr, signal = read_pcm(wav)
    truth = entry['truth']
    errors, octaves, acquisitions = [], 0, []
    false_voiced, tested_unvoiced = 0, 0
    for row in rows:
        sample = int(row['available_by_block_end_sample'])
        t = sample / sr
        if not .1 <= t < 1.1:
            continue
        known = min(truth, key=lambda point: abs(point['sample']-sample))
        midi = float(row['detected_midi'])
        if not known['voiced']:
            tested_unvoiced += 1
            false_voiced += midi > 0
        elif known['in_detector_range'] and midi > 0:
            cents = 100 * (midi - (69 + 12 * math.log2(known['hz']/440)))
            if abs(cents) < 25:
                acquisitions.append(max(0, sample-int(.1*sr)))
            if .25 <= t < 1.0:
                errors.append(abs(cents))
                octaves += abs(cents) >= 600
    latency = metadata['hostReportedLatencySamples']
    steady = signal[int(.45*sr)+latency:int(.95*sr)+latency]
    is_static = entry['kind'] in ['sine', 'harmonic_proxy', 'out_of_range']
    expected = next((p['hz'] for p in truth if p['voiced']), None) if is_static else None
    target = None
    if expected:
        target = expected if entry['kind'] == 'out_of_range' else 440*2**((round(69+12*math.log2(expected/440))-69)/12)
    measured = dominant(steady, sr) if expected else None
    timing = [float(row['processBlock_wall_us']) for row in csv.DictReader(Path(str(wav)+'.timing.csv').open())]
    jumps = np.abs(np.diff(signal))
    result = dict(id=entry['id'], sample_rate=sr,
        detection_median_abs_cents=float(np.median(errors)) if errors else None,
        detection_p95_abs_cents=float(np.percentile(errors,95)) if errors else None,
        detection_octave_error_frames=octaves, detection_compared_frames=len(errors),
        first_accurate_detection_upper_bound_samples=min(acquisitions) if acquisitions and is_static else None,
        unvoiced_false_positive_frames=false_voiced, unvoiced_compared_frames=tested_unvoiced,
        expected_output_hz=target, output_dominant_hz=measured,
        output_dominant_error_cents=1200*math.log2(measured/target) if measured and target else None,
        processBlock_p50_us=float(np.median(timing)), processBlock_p99_us=float(np.percentile(timing,99)),
        processBlock_max_us=max(timing), timing_blocks=len(timing),
        nominal_deadline_exceedances=sum(v > 1e6*metadata['blockSize']/sr for v in timing[:-1]),
        peak=float(np.max(np.abs(signal))), clipped_samples=int(np.sum(np.abs(signal)>=.999)),
        max_adjacent_sample_jump=float(jumps.max()),
        input_sha256=entry['sha256'], output_sha256=hashlib.sha256(wav.read_bytes()).hexdigest(),
        hardware_latency_measured=False)
    return result


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--renderer', required=True, type=Path)
    p.add_argument('--fixtures', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    manifest = json.loads((args.fixtures/'manifest.json').read_text())
    results = []
    for entry in manifest['entries']:
        source = args.fixtures/entry['file']
        assert hashlib.sha256(source.read_bytes()).hexdigest() == entry['sha256']
        wav = args.output/entry['file']
        command = [str(args.renderer.resolve()), str(source.resolve()), str(wav.resolve()),
                   str(Path('docs/reference/configs/correction.json').resolve()), '64']
        process = subprocess.run(command, capture_output=True, text=True, timeout=60)
        (args.output/(entry['id']+'.run.json')).write_text(json.dumps(dict(command=command, exit_code=process.returncode,
            stdout=process.stdout, stderr=process.stderr), indent=2))
        if process.returncode:
            raise RuntimeError(process.stderr)
        results.append(analyse(entry, wav))
    report = dict(measurement='Actual product processor, offline synthetic WAV; not VST3 host or hardware',
        analysis='Telemetry compared at block end; no delay compensation for detection. Octave error threshold >=600 cents. Output FFT dominant peak 50-4000 Hz, not proof of perceptual quality. Static fixtures only for output cents; variable pitch excluded.',
        results=results)
    (args.output/'metrics.json').write_text(json.dumps(report, indent=2))
    # Small machine-readable result in the workflow annotation, retrievable via GitHub API.
    concise = [{k:r[k] for k in ['id','detection_median_abs_cents','detection_octave_error_frames',
        'output_dominant_error_cents','first_accurate_detection_upper_bound_samples',
        'processBlock_p99_us','processBlock_max_us','nominal_deadline_exceedances','clipped_samples']} for r in results]
    print('::notice title=ReferenceMetrics::'+json.dumps(concise,separators=(',',':')))
    return 0

if __name__ == '__main__':
    sys.exit(main())
