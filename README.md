# Xiffects — Unique Sounds by Xivan

Native C++17 / JUCE 8.0.6 soprano saxophone multi-effect, VST3 and Standalone.
This is a working implementation in development, **not an acoustically validated
commercial release**. No web view or full-interface screenshot is used as UI.

## Low-latency Live candidate (Windows)

A separate **Xiffects Live** VST3/Standalone now removes the serial pitch-buffer
stage. Actual processor impulse tests measure approximately **12.04 ms nominal
software transit** at 48 kHz versus Studio's ~60.08 ms. This is **not hardware
input-to-output latency**, nor a claim of identical wet-grain transient timing.
Windows build, seven CTests, 48 synthetic processor renders and pluginval level 5
passed for `557416f`. Studio remains separate and its Windows build/tests also passed.

[Download Live experimental Windows artifact](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39690697623/artifacts/12388239303)
(GitHub sign-in may be required). See [measurements, tradeoffs and installation](docs/live/README.md).
Short grains may affect tone; physical ASIO/DAW/live-input and voice/sax listening
validation remain outstanding. Build explicitly with `-DXIFFECTS_BUILD_LIVE=ON`.
The original chain and latency description below refer to **Studio**, not Live.

## Verified build

Windows VST3 and Standalone builds, DSP/Pitch/Realtime tests and the JUCE host/editor
smoke test passed for source commit `5f679ab` in
[Actions run 39633323816](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39633323816).
The [Windows artifact](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39633323816/artifacts/12359853536)
contains the binaries and native UI snapshot (GitHub sign-in may be required).
This confirms compilation/tests, not listening quality or DAW certification.

## Audio modules

All nine module enable switches and controls are connected to processing:

| Module | Implementation |
| --- | --- |
| Octaver | Pitch-synchronous dual-grain shifting, −2 to +2 octaves |
| Compressor | Linked peak detector; 10 ms attack, 120 ms release; threshold, ratio, makeup |
| Reverb | Four parallel damped combs per channel; interpolated room-size taps and approximate RT60 |
| Delay | Fractional stereo delay, 10 ms–2 s, feedback capped at 95% |
| Distortion | Tanh saturation with first-order antiderivative anti-aliasing and low-pass tone |
| EQ | Four complementary broad one-pole bands; unity reconstruction at zero gains |
| Pitch Shifter | Pitch-synchronous dual-grain shifting, ±12 semitones |
| Phaser | Six all-pass stages, rate, depth and feedback |
| Auto Tune | Decimated YIN analysis, scale/manual targeting, hysteresis, smoothed correction and audio resynthesis |

Chain: correction → parallel octaver/pitch voices → compressor → distortion →
EQ → phaser → delay → reverb → output trim / safety ceiling. Octaver and pitch
mixes are normalized if their sum exceeds 100%, avoiding an inverted dry voice.
Compressor sidechain follows original input peaks. Analysis uses the left/mono
input (rather than cancelling anti-phase stereo by summing it).

Parameter IDs from the starter are retained; POWER was added. All parameters
are host-automatable and serialized through APVTS. Enabled effects and continuous
parameters are smoothed. Allocation occurs in prepare, not in the audio loop.
DSP is separate from the native GUI, with a lock-free SPSC telemetry queue.

## Pitch correction / manual notes

- Keys C–B, eight scales: Chromatic, Major, Minor, Harmonic Minor, ascending
  Melodic Minor, Dorian, Mixolydian, major Pentatonic.
- Effective targets = selected scale transposed by key **AND** manual switches.
  To use any arbitrary manual set, select Chromatic first. Red key indicators
  mean effective permission; grey keys may be manually off or out of scale.
- Nearest allowed note, with 15-cent hysteresis to reduce boundary chatter.
  Removing a target immediately invalidates hysteresis. An empty mask releases
  to unity rather than selecting an unintended note.
- Strength scales the semitone correction; Retune Speed is its smoothing time
  in milliseconds. Mix blends the delayed dry signal and corrected signal.
- Detector: approximately 12–16 kHz analysis rate, 512-sample window, 128-sample
  hop, YIN normalized difference with parabolic interpolation. Intended range
  **100–1600 Hz**, with confidence/energy gating for silence and unpitched input.
- Shifter: two complementary Hann grains; grain length follows an even number
  of detected periods. Left/right channels share phase for stereo coherence.
- Pitch graph: grey is detected input MIDI pitch, red is the **applied correction
  estimate**, not an independent output analysis. With partial wet/dry mix there
  are two fundamentals; the red curve is only an indicative interpolation.

### Latency and quality limits

**About 60 ms fixed audio latency**, reported to the host, including POWER-off
bypass and all-effect-bypass. Each of two pitch stages has a nominal ~30 ms delay.
Analysis needs another ~32–43 ms of input before a confident decision. This is
not a low-latency stage-monitoring solution. Host bypass behavior depends on the
host; the plugin's POWER control uses a latency-aligned dry delay.

Wet pitch grains have variable read positions about the nominal delay; dry/wet
alignment is nominal, not phase-perfect after shifting. Transients and breath
noise can smear, especially at ±2 octaves or weak fundamentals. There is **no
formant preservation**, polyphonic correction, or commercial Auto-Tune engine.
Strong overtones can still cause octave errors. Use isolated monophonic input.
Synthetic tests are not a substitute for saxophone recordings/listening tests.

The reverb is a basic comb bank rather than a finished hall algorithm; size/time
changes can bend pitch. Distortion anti-aliasing reduces but does not eliminate
aliasing. The EQ is not a parametric biquad EQ. Processed output is hard-clamped
to ±1 as a safety ceiling, **not** transparently limited; leave headroom. Bypassed
audio is not clamped. The conservatively reported effect tail is 300 seconds.

## Native interface

- Real JUCE controls with custom charcoal/crimson rotary drawing and value units.
- Nine separated module panels; EQ faders; dedicated tune panel with an editable
  chromatic piano, key/scale, strength and speed. Tune Mix is in its module.
- Live input/output peak meters and actual output min/max waveform envelope.
- Embedded hero cropped from the supplied reference, preserving the musician.
  See `Assets/README.md`: this is not an independently supplied original photo.
- Resizable, fixed-aspect 1440×900 design; 1152×720 minimum. Settings provides
  80/100/120% sizes and processing/latency information.
- Factory recipes; Default resets every parameter to its default. Other recipes
  preserve note selections and change a subset of effect parameters.
- SAVE / LOAD `.xiffects` XML files on the message thread; invalid/unknown or
  nonfinite parameter values are rejected, numeric values are range-clamped.
  Host projects save all parameter values. Preset dropdown text and UI zoom
  are not part of audio state; the label is not an unsaved-change indicator.

The reference is a visual direction, not a claim of pixel-exact reproduction.
Original photograph, logo/font assets and native-window visual review remain
important before a finished branded release.

## Build on Windows

Install Visual Studio 2022 Desktop C++, CMake 3.22+, and Git. First configure
fetches JUCE from GitHub.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Products: `build/Xiffects_artefacts/Release/VST3/` and `Standalone/`.
Install the complete `.vst3` bundle under the host's VST3 search directory (usually
`C:\Program Files\Common Files\VST3`). Rescan in REAPER/Cubase or another host.
Avoid enabling speaker-to-microphone feedback when testing Standalone.

GitHub Actions builds Windows binaries, runs portable and JUCE host tests, then
uploads `Xiffects-Windows-x64` **only after success**. The artifact also includes a
software-rendered native editor PNG. Download it from a successful workflow run:
https://github.com/drdrillcontacts-svg/Xiffects/actions

## Linux dependencies

On an unrestricted Debian/Ubuntu system:

```sh
sudo apt-get install build-essential cmake ninja-build pkg-config \
  libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

JUCE host/editor smoke tests require a graphical session (or `xvfb-run` on CI).
Browser and curl support are disabled. The Arena sandbox lacks Linux GUI/audio
development packages and cannot reach normal system package repositories;
full plugin verification is therefore performed by the Windows workflow.

## Tests without JUCE / system GUI libraries

```sh
cmake -S . -B build-tests -DXIFFECTS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
# Optional portable sanitizers (GCC/Clang):
g++ -O1 -g -std=c++17 -fsanitize=address,undefined -I Source \
  Tests/PitchTests.cpp -o /tmp/xiffects-pitch-tests
/tmp/xiffects-pitch-tests
```

Portable tests run locally at 44.1, 48 and 96 kHz and cover:
- Compressor ratio, delay timing/isolation, EQ unity, finite effect responses.
- Harmonic-rich pitch detection; silence/noise rejection; manual exclusions;
  empty masks and zero strength.
- Independent spectral checks of octave/pitch output, end-to-end corrected audio,
  unity impulse/declared latency and coherent stereo channels.
- No ordinary heap allocations during the tested combined DSP loop; concurrent
  SPSC queue ordering/wraparound. This does not constitute complete host-side
  real-time certification or an exhaustive CPU benchmark.

Full JUCE host tests additionally exercise mono/stereo, empty and variable blocks
(including blocks larger than prepare's hint), latency-compensated global bypass,
all-parameter state round-trip, native editor construction/resizing and screenshot.
See `docs/VALIDATION.md` for release acceptance gates and observed verification.

## Commercial-readiness workstream (2026-10-22)

The new [technical audit and staged plan](docs/COMMERCIAL_AUDIT.md) inventories
current risks, all 44 requested scales, microtonal/preset compatibility and each
platform's release gates. The main UI, existing parameter IDs and routing are
preserved. This stage adds detector-boundary regression protection and caches
unchanged correction targets/retune coefficients without changing automation
semantics. Four portable CTest suites pass locally; a fresh native configure is
blocked by missing Linux `PkgConfig`/development dependencies.

[Offline before/after measurements](docs/measurements/README.md) distinguish
software audio transit, detector acquisition and CPU timing. Measured nominal
pitch-path delay is still about 60 ms; CPU optimization is **not** a latency fix.
No live-device test, new commercial release, 44-scale library, 50-preset bank or
mobile binary is claimed by this stage. The historical Windows artifact above
predates these audit changes.
