"""Compare saved actual-processor renders with identical fixture/parameter hashes.
Not a hardware roundtrip or perceptual listening test.
"""
import json
from pathlib import Path
import sys
import numpy as np
from ValidateReferenceWindows import read_pcm

studio_dir, live_dir = map(Path, sys.argv[1:3])
old_file = next(studio_dir.rglob('metrics.json'))
new_file = next(live_dir.rglob('metrics.json'))
old = {r['id']: r for r in json.loads(old_file.read_text())['results']}
new = {r['id']: r for r in json.loads(new_file.read_text())['results']}
assert old.keys() == new.keys() and len(old) == 48
manifest = json.loads(next(live_dir.rglob('manifest.json')).read_text())
entries = {r['id']: r for r in manifest['entries']}
rows=[]
for name, before in old.items():
    after = new[name]
    assert before['input_sha256'] == after['input_sha256']
    old_wav, new_wav = [p.parent / entries[name]['file'] for p in (old_file,new_file)]
    a,b = [json.loads(Path(str(p)+'.json').read_text()) for p in (old_wav,new_wav)]
    assert a['effectiveParameters'] == b['effectiveParameters']
    assert before['detection_median_abs_cents'] == after['detection_median_abs_cents']
    if not name.startswith('detuned_452_'):
        continue
    onsets=[]
    for wav in (old_wav,new_wav):
        sr,signal=read_pcm(wav)
        indexes=np.flatnonzero(np.abs(signal)>1e-5)
        onsets.append(int(indexes[0]) if len(indexes) else None)
    rows.append(dict(id=name, input_sha256=after['input_sha256'],
        studio_nominal_samples=a['hostReportedLatencySamples'], live_nominal_samples=b['hostReportedLatencySamples'],
        studio_output_first_over_1e_minus_5_sample=onsets[0],live_output_first_over_1e_minus_5_sample=onsets[1],
        studio_output_hz=before['output_dominant_hz'],live_output_hz=after['output_dominant_hz']))
report=dict(identical_inputs_and_parameters=48,hardware_measured=False,
    note='Output threshold indices are absolute file positions, not hardware latency. Wet grain read positions vary around nominal delay.',rows=rows,
    studio_deadline_exceedances=sum(r['nominal_deadline_exceedances'] for r in old.values()),
    live_deadline_exceedances=sum(r['nominal_deadline_exceedances'] for r in new.values()),
    live_clipped_samples=sum(r['clipped_samples'] for r in new.values()))
Path('live-comparison.json').write_text(json.dumps(report,indent=2))
print('::notice title=LiveSameInputComparison::'+json.dumps(report,separators=(',',':')))
