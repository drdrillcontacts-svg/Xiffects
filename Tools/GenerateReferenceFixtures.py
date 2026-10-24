#!/usr/bin/env python3
"""Deterministic synthetic fixtures only; not recordings of a singer/saxophonist."""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import random
import sys
import wave


def generate(destination, rates):
    destination.mkdir(parents=True, exist_ok=True)
    cases = [
        ('silence', 'unvoiced', 0), ('noise', 'unvoiced', 0),
        ('sine_100', 'sine', 100), ('sine_146', 'sine', 146.83),
        ('sine_440', 'sine', 440), ('detuned_452', 'sine', 452),
        ('sine_880', 'sine', 880), ('sine_1397', 'sine', 1396.91),
        ('sine_1600', 'sine', 1600),
        ('below_range_82', 'out_of_range', 82.41),
        ('above_range_1700', 'out_of_range', 1700),
        ('harmonic_sax_proxy', 'harmonic_proxy', 220),
        ('harmonic_voice_proxy', 'harmonic_proxy', 196),
        ('note_step', 'step', 440), ('glide', 'glide', 220),
        ('vibrato', 'vibrato', 440),
    ]
    entries = []
    for rate in rates:
        for name, kind, base in cases:
            rng = random.Random(20261022)
            count = int(rate * 1.3)
            pcm = array.array('h')
            truth = []
            phase = 0.0
            for i in range(count):
                t = i / rate
                active = 0.1 <= t < 1.1 and name != 'silence'
                elapsed = t - 0.1
                hz = base
                if kind == 'step':
                    hz = 440 if elapsed < 0.5 else 493.883301
                elif kind == 'glide':
                    hz = 220 * 2 ** max(0, min(1, elapsed))
                elif kind == 'vibrato':
                    hz = 440 * 2 ** (35 * math.sin(2 * math.pi * 5 * elapsed) / 1200)
                phase += 2 * math.pi * hz / rate
                if not active:
                    sample = 0
                elif name == 'noise':
                    sample = rng.uniform(-0.15, 0.15)
                elif name == 'harmonic_sax_proxy':
                    sample = sum(a * math.sin(k * phase) for k, a in [(1, .12), (2, .10), (3, .07), (4, .03)])
                elif name == 'harmonic_voice_proxy':
                    sample = sum(a * math.sin(k * phase) for k, a in [(1, .14), (2, .04), (3, .09), (5, .02)])
                else:
                    sample = .2 * math.sin(phase)
                # Controlled 5 ms onset/release envelope. A note step is intentional.
                if active:
                    sample *= min(1, elapsed / .005, (1 - elapsed) / .005)
                pcm.append(round(max(-1, min(1, sample)) * 32767))
                if i % max(1, round(rate * .005)) == 0:
                    truth.append({'sample': i, 'voiced': active and base > 0,
                                  'hz': hz if active and base > 0 else None,
                                  'in_detector_range': active and base > 0 and 100 <= hz <= 1600})
            if sys.byteorder != 'little':
                pcm.byteswap()
            path = destination / f'{name}_{rate}.wav'
            with wave.open(str(path), 'wb') as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(rate)
                wav.writeframes(pcm.tobytes())
            entries.append({'id': path.stem, 'file': path.name, 'sample_rate': rate,
                            'channels': 1, 'samples': count, 'kind': kind,
                            'provenance': 'synthetic generated mathematics; no performance recording',
                            'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                            'truth': truth})
    manifest = {'schema': 1, 'seed': 20261022,
                'truth_definition': 'Analytic input fundamental at each labelled sample; not detected pitch or corrected-output ground truth.',
                'entries': entries}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Generated {len(entries)} synthetic WAV fixtures plus analytic input labels in {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    parser.add_argument('--rates', nargs='+', type=int, default=[44100, 48000, 96000])
    args = parser.parse_args()
    if not args.rates or any(r < 8000 or r > 192000 for r in args.rates):
        parser.error('rates must be between 8000 and 192000')
    generate(args.output, args.rates)
