# Onset note-target fix

## Scope

Only production file changed: `Source/DSP/Pitch.h`, specifically the correction
controller's onset hysteresis policy. Detector, shifter, sample buffering, routing,
parameter IDs, presets, UI, and effects are unchanged. The historical Reference
remains at `26e8bb3`; its published artifacts are not replaced. Its workflow is
now manually dispatched and pins the previously validated Reference/tool commit
`5eba0d8`, rather than accidentally validating new candidate DSP as Reference.
The candidate has a separate Windows validation workflow and artifact name.

## Reproduction and root cause

452 Hz is below the A4/Bb4 nearest-note midpoint. With 100 ms silence and a 5 ms
amplitude ramp, the initial partially voiced YIN window can estimate above that
midpoint. The controller immediately used that target as its hysteresis anchor.
Subsequent accurate estimates around MIDI 69.466 could not release the Bb4 target:
the advantage of A4 is smaller than the 15-cent hysteresis allowance.

`Tests/OnsetTests.cpp` was first built against the unchanged production header.
Its baseline mode failed (exit 1) at 44.1/48/96 kHz and measured approximately
466.1/466.055/466.040 Hz, matching the previously observed product bug.
`before.txt` preserves those results. This was a real synthetic regression failure,
not a hypothetical target or a successful test label.

## Narrow change

During the first THREE consecutive voiced detector analysis frames after reset or
an unvoiced result, nearest-note selection is provisional and not pinned by the
previous target. Correction still runs immediately with the existing strength and
retune smoothing. On later frames the original 15-cent hysteresis is used.
The counter advances on detector results, NOT audio samples or host blocks.
An unvoiced detector result and `prepare` reset it; manual exclusions still take
precedence. No allocation, waiting, new audio buffer, or host latency change.

This is not a general new onset detector. Very short rests that never yield an
unvoiced detector result do not re-arm the window; continuous legato keeps the
established policy. The first few provisional targets can still move while
analysis settles. Audible behaviour on real breathy attacks requires listening
and real recordings; the three-frame policy is not a claim of perfect acquisition.

## Tests

- Exact 452 Hz / 100 ms silence / 5 ms ramp, PCM16 quantisation, three sample rates.
- Actual shifter audio checked with independent positive zero-crossing frequency
  estimation in the settled region; Windows product output is also FFT-analysed.
- Envelope matrix: 1/5/20/50 ms ramps, zero/100 ms leading silence, pure tone and
  harmonic proxy, 452 Hz below and 454 Hz above the boundary at all three rates.
- Reacquisition after silence, established boundary vibrato (hysteresis preserved),
  manual target removal and empty allowed-note mask.
- Existing Pitch/Realtime tests: latency, other shifts, zero strength and allocation
  audit retained. The uncached correction oracle now explicitly includes the new
  onset specification; it still checks sample-wise automation/cache equivalence.
  Requiring equality with the OLD onset latch would incorrectly preserve the bug.
- Five portable CTests pass; Onset also passes AddressSanitizer/UBSan locally.

Local before/after audio frequency measurements (synthetic component chain, not
DAW/hardware) are in `before.txt` and `after.txt`. Largest adjacent-sample jumps in
that fixture did not increase, but this is not a perceptual click/pop certification.

## Reproduce locally

```sh
cmake -S . -B build-onset-tests -DXIFFECTS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-onset-tests --parallel 2
ctest --test-dir build-onset-tests -V
g++ -O1 -g -std=c++17 -fsanitize=address,undefined -I Source Tests/OnsetTests.cpp -o /tmp/xiffects-onset
/tmp/xiffects-onset
```

To reproduce the old failure without restoring/changing working production files,
export `Source/DSP/Pitch.h` and `Effects.h` from `26e8bb3` into a separate include
folder under `DSP/`, compile OnsetTests against that folder, and run with
`--baseline`. Expected exit code is 1. Do not run a destructive reset.

## Windows product acceptance gate

The new workflow uses the existing real-product renderer, synthetic corpus and
pluginval. `--require-onset-a4` adds a hard gate: all three detuned_452 WAV outputs
must have dominant-frequency error less than 10 cents from 440 Hz. Merely running
the renderer successfully is no longer enough to pass this onset scenario.
Full WAVs, settings, states, traces and timings are uploaded under the separate
`Onset-Fix-Windows-Evidence` artifact. No live mode or hardware latency improvement
is claimed. See the subsequent Windows result record for actual execution status.

## Observed Windows results

Run https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39687622124
validated candidate commit `9c4271c`. Windows x64 Release built successfully, all
seven CTests passed (including Onset, Host and ReferenceProcessor), all 48
synthetic inputs were rendered through the real product processor, the new
onset-A4 acceptance gate passed, and pluginval strictness 5 returned exit 0.
The production scope/hash checks also passed.

| Sample rate | Reference output (Hz) | Fixed product WAV dominant frequency (Hz) | Error from A4 (cents) |
|---|---:|---:|---:|
| 44.1 kHz | 466.0998 | 439.93492 | -0.25608 |
| 48 kHz | 466.0530 | 439.89758 | -0.40305 |
| 96 kHz | 466.0401 | 439.84625 | -0.60504 |

This is the same 452 Hz synthetic onset scenario/configuration, independently
FFT-measured from the product's saved WAVs, not inferred from the pitch display.
Detector error/acquisition timing did not change; the fix is target-selection
policy. In these three renders no clipped samples or nominal block-deadline
exceedances were recorded. This is not a claim that all CPU spikes are resolved:
the previous offline-overrun investigation remains separate and open.

The focused OnsetFixResults annotation contains all three complete result records
and is saved as `windows-onset-results.json`. The generic 48-case annotation is
truncated by GitHub's annotation limit; do not use it as a complete 48-case table.
The FULL corpus report, WAVs, traces, states and timings are in the downloadable
artifact, separate from the historical Reference artifact:
https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39687622124/artifacts/12386259737

No physical loopback, interactive REAPER session, real voice/sax recording or
perceptual listening session was performed. Existing algorithmic latency is not
reduced. This is a tested fix for the demonstrated onset latch, not certification
of a commercial/live-ready product. Live Experimental has not been started.
