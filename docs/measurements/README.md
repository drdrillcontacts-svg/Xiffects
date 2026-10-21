# Offline pitch audit measurements — 2026-10-22

These are measured **software pitch-subsystem** results, not live-input,
full-plugin callback, physical round-trip, concert or mobile certification.

## Environment / method

Debian 12 sandbox, GCC 12.2, x86_64, 2 virtual CPUs,
Intel Xeon Processor @ 2.60 GHz as reported by the VM. Compile with
`g++ -O2 -DNDEBUG -std=c++17 -I Source Tools/PitchProfile.cpp`.
The baseline used the unmodified `Pitch.h`/`Effects.h` copied before this audit's
edits; the cached version uses the final edited source. CSVs are one run each,
not a statistically controlled hardware benchmark. VM scheduling, host load,
clock overhead and compiler settings affect CPU numbers, especially maxima.

- Unity impulse through two shifters measures actual sample delay at ratio 1,
  mix 0. This matches the pitch bank's nominal delay but is not a measurement of
  variable-grain transient latency under arbitrary pitch shifts.
- First accurate voiced detection (within 20 cents) after a sine starts from
  reset at 146.83, 440 and 880 Hz measures acquisition, not end-to-end audio delay.
- Two seconds per block size/rate, following 64 warm-up blocks; pre-generated
  452 Hz input, correction + octave + pitch shifters, fixed musical parameters.
- Timing records p50/p99/max of each offline block. Excludes JUCE wrapper,
  parameter smoothing, other six effects, GUI, device/driver and DAW overhead.
  A missed deadline here means block wall time > blockSize/sampleRate. Zero
  observed misses in this short synthetic workload does **not** mean no xruns
  under live conditions. Timing is deliberately outside the production callback.

## Results

| Sample rate | Measured two-stage unity delay | Samples | First accurate detection (tested tones) |
|---|---:|---:|---:|
| 44,100 | 60.0907 ms | 2650 | 34.8299 ms |
| 48,000 | 60.0833 ms | 2884 | 42.6667 ms |
| 96,000 | 60.0417 ms | 5764 | 42.6667 ms |

These delay values are unchanged by caching. Lower CPU cost is **not** lower
algorithmic/audio latency. Acquisition and steady-state transit are different
measurements and should not simply be summed into a “total latency” claim.

Example, 48 kHz / 64 samples, pitch subsystem only:

| Version | p50 | p99 | maximum |
|---|---:|---:|---:|
| Baseline | 17.538 µs | 38.963 µs | 52.872 µs |
| Cached | 10.182 µs | 29.689 µs | 46.135 µs |

The median in this run improved ~42%. Do not extrapolate this into a whole-plugin
CPU percentage. Both original and final full per-rate/block CSVs are included.

## Reproduce current measurements

```sh
cmake -S . -B build-profile -DXIFFECTS_DSP_ONLY=ON \
  -DXIFFECTS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-profile --parallel 2
ctest --test-dir build-profile --output-on-failure
./build-profile/XiffectsPitchProfile > pitch-current.csv
```

The tool intentionally fails only on correctness (nonfinite output, wrong
impulse latency, failure to acquire tested sine pitches), not CPU timing, which
is host-dependent. Do not run benchmark timing under sanitizers.

## Remaining measurements

Physical input/output loopback, DAC/ADC/safety buffers, live sax/voice, full engine
worst-case automation, 30-minute+ soak, real DAWs and each mobile platform.
Record device, driver, host, mode, OS, rate and actual buffer sizes with results.
No audio device or physical loopback is available in this sandbox.
