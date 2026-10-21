#!/usr/bin/env python3
"""Collect evidence; capture success does not certify a native/plugin/hardware build."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)  # Never overwrite an evidence run.
    cmake = str(Path(args.cmake).resolve()) if Path(args.cmake).is_file() else args.cmake
    env = dict(os.environ)
    if Path(cmake).is_absolute():
        env['PATH'] = str(Path(cmake).parent) + os.pathsep + env.get('PATH', '')
    records = []

    def run(name, command):
        try:
            result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=180)
            code, stdout, stderr = result.returncode, result.stdout, result.stderr
        except (OSError, subprocess.TimeoutExpired) as error:
            code, stdout, stderr = -1, '', str(error)
        (output / (name + '.stdout.txt')).write_text(stdout)
        (output / (name + '.stderr.txt')).write_text(stderr)
        records.append({'name': name, 'command': command, 'cwd': str(ROOT),
                        'exit_code': code, 'status': 'PASS' if code == 0 else 'FAILED',
                        'stdout': name + '.stdout.txt', 'stderr': name + '.stderr.txt'})
        print(name, code, flush=True)
        return code

    baseline = json.loads((ROOT/'docs/reference/baseline-files.json').read_text())
    unchanged = {}
    for name, digest in baseline['sha256'].items():
        if name.startswith(('Source/', 'Assets/')):
            unchanged[name] = hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest
    if not all(unchanged.values()):
        raise RuntimeError('Production source/assets differ from Reference; refusing this capture')
    (output/'source-integrity.json').write_text(json.dumps(unchanged, indent=2)+'\n')
    (output/'environment.json').write_text(json.dumps({
        'platform': platform.platform(), 'machine': platform.machine(),
        'python': sys.version, 'purpose': 'offline Reference evidence, no hardware access',
        'build_type': 'Release', 'dsp_only': True,
        'benchmarks': True, 'source_baseline_commit': (ROOT/'docs/reference/baseline-commit.txt').read_text().strip()
    }, indent=2)+'\n')
    run('cmake-version', [cmake, '--version'])
    run('compiler-version', ['c++', '--version'])
    run('git-head', ['git', 'rev-parse', 'HEAD'])
    run('git-status', ['git', 'status', '--short'])
    portable = 'build-reference-portable'
    configure = run('portable-configure', [cmake, '-S', '.', '-B', portable,
        '-DXIFFECTS_DSP_ONLY=ON', '-DXIFFECTS_BUILD_BENCHMARKS=ON', '-DCMAKE_BUILD_TYPE=Release'])
    build = run('portable-build', [cmake, '--build', portable, '--parallel', '2']) if configure == 0 else -1
    outcomes = {}
    if build == 0:
        for test in ['DSP', 'Pitch', 'CorrectionRegression', 'Realtime']:
            outcomes[test] = run('test-'+test, ['ctest', '--test-dir', portable, '-R', '^'+test+'$', '-V'])
        run('pitch-profile-independent', [str(ROOT/portable/'XiffectsPitchProfile')])
    run('fixture-generation', [sys.executable, 'Tools/GenerateReferenceFixtures.py', 'build-reference-fixtures'])
    outcomes['FixtureTests'] = run('fixture-tests', [sys.executable, 'Tests/FixtureTests.py'])
    manifest = ROOT/'build-reference-fixtures/manifest.json'
    if manifest.exists():
        corpus = json.loads(manifest.read_text())
        summary = [{k: v for k, v in entry.items() if k != 'truth'} for entry in corpus['entries']]
        (output/'synthetic-fixtures.json').write_text(json.dumps({
            'generated_manifest_sha256': hashlib.sha256(manifest.read_bytes()).hexdigest(),
            'entries': summary, 'full_truth': 'Regenerate build-reference-fixtures/manifest.json'
        }, indent=2)+'\n')
    native = run('native-configure', [cmake, '-S', '.', '-B', 'build-reference-native',
        '-DXIFFECTS_BUILD_REFERENCE_TOOLS=ON', '-DCMAKE_BUILD_TYPE=Release'])
    # This collector is intentionally the Linux/offline evidence job, not a Windows driver.
    (output/'execution-status.json').write_text(json.dumps({
        'portable_tests': {k: 'PASS' if v == 0 else 'FAILED' for k, v in outcomes.items()},
        'native_configure': 'PASS' if native == 0 else 'FAILED_SEE_RAW_LOG',
        'ReferenceRender': 'NOT_RUN', 'ReferenceProcessor': 'NOT_RUN', 'Host': 'NOT_RUN',
        'Windows_VST3_build': 'NOT_RUN_IN_THIS_CAPTURE',
        'VST3_in_DAW': 'NOT_RUN', 'hardware_round_trip': 'NOT_MEASURED',
        'real_voice': 'NOT_RUN_MISSING_LICENSED_RECORDINGS',
        'real_saxophone': 'NOT_RUN_MISSING_LICENSED_RECORDINGS',
        'note': 'Capture completion does not mean native renderer, Windows or hardware validation.'
    }, indent=2)+'\n')
    (output/'commands.json').write_text(json.dumps(records, indent=2)+'\n')
    auxiliary_ok = all(record['exit_code'] == 0 for record in records
                       if record['name'] in ['fixture-generation', 'pitch-profile-independent'])
    return 0 if build == 0 and outcomes and all(code == 0 for code in outcomes.values()) and auxiliary_ok else 1


if __name__ == '__main__':
    sys.exit(main())
