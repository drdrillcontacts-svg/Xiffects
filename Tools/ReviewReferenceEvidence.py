"""Review saved Windows WAV/log artifacts without re-running or modifying DSP."""
import argparse
import csv
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import numpy as np
from ValidateReferenceWindows import analyse, read_pcm

p=argparse.ArgumentParser()
p.add_argument('artifact',type=Path)
p.add_argument('output',type=Path)
args=p.parse_args()
args.output.mkdir(parents=True,exist_ok=False)
base=args.artifact
metrics_file=next(base.rglob('metrics.json'))
old=json.loads(metrics_file.read_text())
manifest=json.loads(next(base.rglob('manifest.json')).read_text())
entries={r['id']:r for r in manifest['entries']}
rows=[]
diagnostics=[]
for result in old['results']:
    entry=entries[result['id']]
    wav=metrics_file.parent/entry['file']
    actual=analyse(entry,wav)
    assert actual==result,entry['id'] # Recompute from real saved WAVs and raw telemetry/timings.
    row=[result['id']]
    for key in ['detection_median_abs_cents','detection_p95_abs_cents','detection_octave_error_frames',
                'detection_compared_frames','output_dominant_error_cents','processBlock_p99_us',
                'processBlock_max_us','nominal_deadline_exceedances','clipped_samples',
                'unvoiced_false_positive_frames','unvoiced_compared_frames']:
        v=result[key]
        row.append(round(v,3) if isinstance(v,float) else v)
    rows.append(row)
    if entry['id'].startswith('detuned_452'):
        trace=list(csv.DictReader(Path(str(wav)+'.pitch.csv').open()))
        first=[r for r in trace if float(r['detected_midi'])>0][:10]
        stable=[r for r in trace if .5<float(r['available_by_block_end_sample'])/entry['sample_rate']<.8]
        params=json.loads(Path(str(wav)+'.json').read_text())['effectiveParameters']
        diagnostics.append(dict(id=entry['id'],expected_output_hz=result['expected_output_hz'],
            measured_output_hz=result['output_dominant_hz'],first_voiced=first,
            stable_applied_median=float(np.median([float(r['applied_estimate_midi']) for r in stable])),
            params={k:v for k,v in params.items() if k.startswith('TUNE')},
            onset_upper_bound_samples=result['first_accurate_detection_upper_bound_samples']))
columns=['id','det_median_cents','det_p95_cents','octave_error_frames','compared_frames',
         'output_dominant_error_cents','block_p99_us','block_max_us','deadline_exceedances',
         'clipped_samples','unvoiced_false_positive_frames','unvoiced_frames']
(args.output/'metrics-table.json').write_text(json.dumps(dict(columns=columns,rows=rows),indent=2))
(args.output/'detuned-onset-diagnostics.json').write_text(json.dumps(diagnostics,indent=2))
print('::notice title=MetricColumns::'+json.dumps(columns))
for i in range(0,len(rows),8):
    print('::notice title=MetricRows'+str(i//8)+'::'+json.dumps(rows[i:i+8],separators=(',',':')))
# Short onset evidence, all full traces remain in the artifacts.
summary=[]
for d in diagnostics:
    summary.append(dict(id=d['id'],expected=d['expected_output_hz'],measured=d['measured_output_hz'],
        applied_midi=d['stable_applied_median'],first_voiced=d['first_voiced'][:2],
        onset_upper_bound_samples=d['onset_upper_bound_samples']))
print('::notice title=OnsetDiagnostics::'+json.dumps(summary,separators=(',',':')))
ctest=next(base.rglob('ctest.xml'))
tests=[]
for node in ET.parse(ctest).getroot().iter('testcase'):
    tests.append(dict(name=node.attrib.get('name'),status=node.attrib.get('status'),
        failure=node.find('failure') is not None,time=node.attrib.get('time')))
(args.output/'ctest-summary.json').write_text(json.dumps(tests,indent=2))
print('::notice title=CTestResults::'+json.dumps(tests,separators=(',',':')))
pluginval=next(base.rglob('pluginval.txt')).read_text(encoding='utf-8',errors='replace')
(args.output/'pluginval.txt').write_text(pluginval)
# Match useful coverage lines without claiming a DAW session.
lines=[l for l in pluginval.splitlines() if any(w in l.lower() for w in ['pass','fail','sample rate','state','parameter','buffer','bypass','audio processing'])]
text='\n'.join(lines)[-3300:]
print('::notice title=PluginvalCoverage::'+text.replace('%','%25').replace('\r','%0D').replace('\n','%0A'))
