# Xiffects — Unique Sounds by Xivan

C++17 / JUCE 8.0.6 soprano saxophone multi-effect, targeting VST3 and
Standalone. **Incremental development build, not a finished commercial plugin.**

## Verified status

The portable DSP test executable was built and run locally with GCC 12,
CTest, and separately AddressSanitizer/UndefinedBehaviorSanitizer. Tests pass
at 44.1, 48 and 96 kHz. Full JUCE configuration was attempted but is blocked
in this sandbox by missing pkg-config and Linux audio/GUI development packages.
**Neither VST3 nor Standalone has been built or auditioned in this environment.**
The updated Windows workflow has not yet been run on these changes; no compiled
artifact is claimed available. A missing `createPluginFilter` factory was added.

## Implemented in source

- Six independently testable effects in `Source/DSP/Effects.h`:
  linked-peak compressor (10 ms attack / 120 ms release), fractional delay,
  tanh distortion with tone filter, complementary four-band EQ, six-stage
  phaser, and four parallel damped-comb reverb channels.
- Processing chain: compressor → distortion → EQ → phaser → delay → reverb
  → output trim → safety ceiling. Mono and stereo use the same chain.
- Existing effect parameters retained. Values and enable transitions are
  smoothed; APVTS attachments reflect automation and restored state.
- Global POWER crossfades to untouched dry input; OUTPUT is host-automatable.
  Bypassed effects continue ticking, preserving their internal tails.
- Preallocated delay/reverb storage; no allocation, locks or file I/O in the
  sample loop. Parameter pointers are cached during construction.
- Hero crop from the supplied reference embedded with JUCE BinaryData, real
  JUCE controls, separated module panels, scalable 1440×900 layout (minimum
  1152×720), actual block peak input/output meters, factory preset recipes.
- State serialization retains all original IDs plus POWER. Factory preset
  recipes modify implemented effect parameters, not manual note selections;
  their dropdown label is not a serialized preset identity.
- Scale/target-note helper with all eight requested scales: effective allowed
  notes are the intersection of the transposed scale and manual switches.
  Empty selection returns unchanged pitch. This helper is tested but **not yet
  connected to pitch detection/correction**.

## Explicitly unfinished

- Octaver, Pitch Shifter, pitch detection and pitch correction remain pending.
  Their controls are visibly marked with `*` and disabled, not represented as
  working effects. Parameters remain present for state compatibility.
- Note switches remain editable/persistent but do not currently change audio.
  They are not yet a custom piano keyboard. There is no live pitch graph or
  waveform monitor; the starter's invented meter values/waveform were removed.
- No save/load preset browser or settings panel yet. Host project state works
  through the existing serialization, but requires host integration testing.
- Visual work is an approximation, not pixel-identical or visually verified
  in a running JUCE window. See `Assets/README.md` for photograph provenance.
  An original standalone photograph is still needed for the final hero.
- Reverb is an early comb-bank implementation, not a polished hall algorithm.
  Changing room size can cause discontinuities; changing delay time bends pitch.
  Distortion is not oversampled and may alias. EQ bands are broad one-pole
  complements, not parametric biquads. CPU profiling and audio-thread allocation
  instrumentation are still needed. No pitch-related latency is introduced yet.
- Processed output uses a hard ±1 safety ceiling, not a transparent limiter.
  Keep gain conservative; clipping can distort. Global bypass is not clipped.
- A conservative 300-second tail is reported to accommodate maximum delay
  feedback; dynamic tail reporting is not implemented.

## Windows build

Install Visual Studio 2022 with Desktop C++ tools, CMake 3.22+, and Git.
The first configure downloads JUCE from GitHub.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Products are under `build/Xiffects_artefacts/Release/VST3/` and `Standalone/`.
The GitHub Actions workflow builds these targets, runs DSP tests, then uploads
`Xiffects-Windows-x64` only if the preceding steps succeed.

## Linux build dependencies

On an unrestricted Debian/Ubuntu machine:

```sh
sudo apt-get install build-essential cmake ninja-build pkg-config \
  libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Browser and curl support are disabled. This sandbox cannot use the normal
system package repositories under its outbound-host restrictions.

## Portable tests (no JUCE, GUI libraries, or network needed)

```sh
cmake -S . -B build-tests -DXIFFECTS_DSP_ONLY=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
# Optional memory / undefined-behaviour checks:
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I Source Tests/DSPTests.cpp -o /tmp/xiffects-tests
/tmp/xiffects-tests
```

Tests cover compressor steady-state ratio, delay impulse timing/channel
isolation, unity EQ, finite modulation/reverb output, nonzero effect response,
key transposition, manual exclusions and empty note-map behavior. They do not
replace listening tests, plugin validation or host automation/state round trips.

## Next acceptance gates

1. Run Windows CI, fix any JUCE/compiler errors, validate VST3 in a host, test
   mono/stereo and automation/state at multiple block sizes.
2. Review the native UI against the reference, implement custom knobs/piano,
   preset file controls/settings and acquire the original photograph.
3. Implement and test pitch analysis on synthetic signals and sax recordings;
   add confidence gating, target hysteresis and manual-mask integration.
4. Implement a measured-latency pitch shifter with aligned dry/wet paths;
   reuse it for Octaver and correction. Evaluate transitions and artifacts
   before enabling these controls. Add genuine detected/corrected pitch traces.
5. Refine reverb, oversample distortion, add limiting, profile CPU, validate
   silence/denormals, stress automation, and test multiple DAWs before release.
