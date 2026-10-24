# Xiffects Live — separate experimental Windows candidate

This is a **software-path latency change**, not a hardware roundtrip measurement
or a claim of concert readiness. Studio and the immutable historical Reference
remain available. Live uses a different VST3 identity (`Xifl`,
`com.xivan.xiffects.live`), so installing it does not replace an old session's plugin.

## Cause and change

Studio routes correction into a second parallel harmony bank: two nominal
~30 ms stages, even when effects are off. Live feeds three short-grain voices
from the **same raw input**, combining the correction and harmony ratios.
The bank uses 16 ms nominal grains instead of 40 ms, and one latency-aligned
bypass delay. This changes the actual sample path, not only host compensation.
No runtime mode switch or changing host latency is introduced.

All nine modules and existing controls/state remain. No new scales, presets,
formant algorithm, detector redesign or unrelated UI redesign is included.
The editor identifies the new product as Xiffects LIVE. Existing Studio remains
the default CMake build; enable Live explicitly with `-DXIFFECTS_BUILD_LIVE=ON`.
The historical Reference workflow remains pinned; the completed onset-only
workflow is now also pinned to its verified source rather than applying its
old scope/hash gate to a different experiment.

## Software-delay measurements

| Sample rate | Studio nominal samples | Live measured impulse samples | Live software delay |
|---|---:|---:|---:|
| 44,100 Hz | 2,650 | 531 | 12.0408 ms |
| 48,000 Hz | 2,884 | 578 | 12.0417 ms |
| 96,000 Hz | 5,764 | 1,154 | 12.0208 ms |

About 80% less nominal plugin transit delay. `LiveHostTests` feeds actual
`XiffectsAudioProcessor::processBlock` impulses through both POWER-off and
POWER-on/all-effects-off, with mono/stereo, three sample rates and variable
blocks (including empty blocks), and checks every output sample against the
reported delay. Studio's earlier Host test likewise checks real bypass output;
its 40 ms default shifter is unchanged. Pitch-shifted wet grains have variable
read positions around this nominal delay: **not every wet transient has exactly
12 ms delay**. Reverb/delay tails are effects, not this compensation measurement.

Detector acquisition still needs input history (roughly 32–43 ms window); this
is not an additional fixed audio buffer. Driver, input/output buffers, converter,
OS and monitoring-device delays remain outside these measurements.

## Evidence and tests

Production source candidate: `557416f`. `production-hashes.json` restricts the
production changes to five files relative to `e2cc9e9`. Source hashes are checked
before Windows compilation. Raw environment, compiler/build logs, JUnit,
pluginval output, 48 rendered WAVs/telemetry/timing reports and binaries are
retained in the Windows artifact.

- Local portable suite: all six CTests passed; raw `portable-tests.txt`.
- Live DSP tests under ASan/UBSan passed; raw `sanitized.txt`.
- Allocation-hook test now includes the prepared Live bank: no DSP-loop allocations.
  This is not an assertion that a whole JUCE host never allocates.
- Standalone DSP tests cover impulse/channel isolation, corrected static tones
  100–1396.91 Hz (within 15 cents), and octave ratios 0.25/0.5/2/4 at all three rates.
- Default Studio shifter output compared byte-for-byte with `e2cc9e9` for identical
  three-rate, unity/non-unity stereo inputs: equal SHA-256 in
  `studio-shifter-equivalence.txt`. This checks that component, not bit-identical
  compiled products on every platform.

### Verified Windows outcome

[Run 39690697623](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39690697623)
passed: real x64 VST3/Standalone build, all seven selected CTests, all 48 actual
processor renders and the <10-cent onset gate, pluginval 1.0.4 strictness 5 exit 0.
The twelve actual-product impulse cases produced the sample counts in the table
above (`windows-impulse-results.txt`). The three 452 Hz fixtures settled to
439.936799 / 439.889925 / 439.889660 Hz at 44.1 / 48 / 96 kHz, respectively.

**[Download Xiffects Live Windows Experimental](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39690697623/artifacts/12388239303)**
— VST3, standalone and raw evidence; GitHub sign-in may be required.

Studio's unchanged-route Windows build and CTests also passed at the same source
commit: [run 39690697378](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39690697378).
Original onset-fixed Studio is retained at `e2cc9e9`; immutable pre-fix Reference
and its pinned replay are not replaced.

The generic 48-row annotation is truncated by GitHub. Complete raw metrics live
in the artifact; focused onset/impulse annotations are complete. A separate review
workflow recomputes both saved Live and onset-fixed Studio WAV measurements and
checks that all 48 input SHA-256 hashes and parameter sets match.

## Tradeoffs and outstanding validation

Short grains may colour low notes, attacks, vibrato and breath noise. There is
still no formant preservation. With partial Tune Mix, Live harmonies interpolate
correction in semitones rather than receiving Studio's mixed dry/wet input;
timbre is intentionally not identical. Extreme shifts require listening tests.

The 48 inputs are synthetic fixtures, including harmonic proxies, **not licensed
real voice or saxophone recordings**. Voice and sax must each receive separate
listening/live-input validation. No ASIO physical loopback, interactive REAPER
session, long-duration stage stability, or macOS/iOS/iPadOS/Android Live build/run
has been performed here. Offline timing is descriptive; no universal no-dropout
claim is made.

## Windows use

1. Keep the old Xiffects installed. Extract the artifact and copy the **entire**
   `Xiffects Live.vst3` bundle to your usual VST3 folder, e.g.
   `C:\Program Files\Common Files\VST3`. Rescan and select **Xiffects Live**, not Xiffects.
   The artifact also includes `Xiffects Live.exe` for standalone use.
2. Select your audio interface's native ASIO driver when available. Start with
   48 kHz / 64 samples; this is a test starting point, not certified hardware advice.
   If it crackles, try 128 samples and measure the resulting delay. Avoid Bluetooth
   and high-latency Windows shared/DirectSound monitoring paths for this test.
3. Test in an empty DAW project without lookahead/linear-phase processors on the
   monitored track or master. Do not monitor both the dry hardware input and the
   software return at once when evaluating the corrected output.
4. Compare the same input/preset with Studio. Keep Live as a separate candidate
   until acceptable tone, dropout behaviour and physical roundtrip are established
   on your actual interface. Turning POWER off deliberately retains alignment;
   it does not bypass the driver's buffers.

## Local reproduction

```sh
cmake -S . -B build-live-tests -DXIFFECTS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-live-tests --parallel 2
ctest --test-dir build-live-tests -V
g++ -O1 -g -std=c++17 -fsanitize=address,undefined -I Source Tests/LivePitchTests.cpp -o build-live/sanitized
build-live/sanitized
```

For the default-shifter equivalence test, export `Source/DSP/Pitch.h` and
`Source/DSP/Effects.h` from `e2cc9e9` into a separate `DSP/` include root. Compile
`studio-equivalence.cpp` with `g++ -O2 -std=c++17` once against that root and once
against current `Source`, redirect both stdout streams to PCM files and `cmp`.
Native JUCE Linux tests are not run locally because the required audio/GUI system
dependencies are unavailable; Windows CI tests the actual processor instead.

## Completed same-input audio comparison

[Review run 39692007566](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39692007566)
recomputed all 96 saved WAV analyses (48 Studio + 48 Live), passed analyser tests,
and verified all 48 input hashes and parameter sets are identical. Complete compact
rows are in `reviewed-metrics.json`; full-precision focused comparison is in
`same-input-comparison.json`. Detector median errors are unchanged for every fixture.

With correction active on the same 452 Hz input, first output crossing of 1e-5
occurs at these absolute positions in the saved files:

| Rate | Studio sample | Live sample | Live output arrives earlier by |
|---|---:|---:|---:|
| 44,100 | 7,061 | 4,942 | 48.0499 ms |
| 48,000 | 7,684 | 5,379 | 48.0208 ms |
| 96,000 | 15,364 | 10,755 | 48.0104 ms |

These actual rendered onset comparisons corroborate the shorter audio path;
they are threshold-defined measurements on synthetic inputs, not a promise about
every wet transient or hardware roundtrip.

**CPU caveat:** Live had one offline block over its nominal deadline in the full
48-case run: `sine_1600_96000`, max 697.7 us versus a 666.7 us / 64-sample budget
(p99 86.484 us). The earlier Studio run had 28 such events. The runners/times differ;
this is not a controlled CPU speedup or proof of dropout-free performance. No
clipped samples were detected across these Live fixtures. Physical stability
validation remains open.

Evidence collection note: the first review run 39691872444 succeeded, but putting
all notices in one step hit GitHub's annotation count limit. The second review
separated the steps and retained all comparison data. Direct archived-log download
from this sandbox failed (external storage EOF); the successful runner recomputed
from the downloaded artifacts. Both run statuses remain independently inspectable.
