# UI, tempo delay, reverb characters and pitch refinement candidate

Production candidate: `0eeeb36`. Historical Reference and previous release binaries
remain recoverable; these changes do not claim zero latency or commercial approval.

## Requested interface changes

- Previous/next factory preset buttons, with wraparound. From a user preset or no
  factory selection, next selects preset 1; previous selects preset 50. Factory
  selection still restores the entire recipe, not a partial parameter patch.
- Categories are popup headings; selected text shows the short preset name to avoid
  truncating larger text. The bank contains 50 complete snapshots, now 55 parameters.
- Removed the processing/About item and dialog from the editor. The settings button
  retains interface-size controls; latency/quality limitations remain documented here.
- Live brand now reads exactly **Xiffects Live**; the separate Studio identity stays
  Xiffects so old sessions/product identities are not silently renamed.
- Module labels/values/captions and popup/button text enlarged. Typical parameter
  text is 15 logical pixels (12 at the supported 80% desktop size), rather than 10
  logical pixels (8 at 80%). EQ units are indicated in its title to avoid squeezing
  four tiny value fields. Mobile controls stay within their existing pages.

## Delay sync

New automatable, serialised `DLY_SYNC` and `DLY_DIVISION`. Existing manual time is
retained and disabled visually while sync is active. Read host BPM once per block,
with explicit 120 BPM fallback when unavailable, and clamp to the documented
30–300 BPM range. UI shows BPM and fallback status. Eight rhythmic divisions:
1/16, 1/8 triplet, 1/8, dotted 1/8, 1/4 triplet, 1/4, dotted 1/4, 1/2.

Duration = 60/BPM × quarter-note beats. Allocate up to 4.1 seconds in prepare so
half notes at 30 BPM actually fit (4 seconds). Manual range remains 10 ms–2 s.
Existing time smoothing is retained: large tempo changes may bend pitch during
settling. This is tempo-following delay, not sample-accurate transport phase locking.
Standalone without a host tempo uses the displayed 120 BPM fallback.

## Four actual reverb characters

`REV_MODE`: Room / Hall / Plate / Chamber. Algorithmic characters, **not** convolution
IRs or a physical plate simulation. Room preserves the old sound; the other three
use differing delay lengths/damping plus four allpass diffusion stages per channel.
Four prepared engines run continuously, with a 50 ms coefficient-time-constant
crossfade when switching; buffers are not cleared. No processing-time allocation.
Cache unchanged comb feedback calculations rather than evaluating pow on every
sample. Mode switching retains state but is not a promise of inaudible transitions.
Long decay/high wet level can still invoke the existing output ceiling.

Old preset/host states migrate missing new values to manual delay, quarter-note
subdivision and Room, without changing legacy parameter ranges/IDs. Factory snapshots
explicitly include all three new values. Default bank recipes retain their existing
Room sound; other modes can be selected and saved as user presets.

## Actual pitch-quality defect found and corrected

The audit exposed a wrong-target case near a semitone boundary: 904 Hz at 48/96 kHz
could settle near 930 Hz instead of the intended nearest 880 Hz note. Increasing
Strength made this wrong correction stronger; simply turning the knob up was not a fix.

Cause: parabolic interpolation of the YIN normalised difference curve has a
short-period bias. CMND still selects/gates the period; interpolation now uses the
three raw squared-difference values. Confidence remains based on the normalised
curve. No extra audio buffer, detector window, hop or latency was added.

Independent controller + Live bank audit (not a full host/voice test):
- 27 cases: 226/452/904 Hz, 0/50/100% strength, 44.1/48/96 kHz.
- Before: four cases exceeded 15 cents, worst 95.6679 cents.
- After: all 27 passed a tightened 5-cent bound; worst 0.481602 cents.
- Before/after CSVs and exit statuses retained. Half-strength expectations are in
  semitones, not a linear interpolation of Hz. Full wet mix isolates correction.
- `tune-initial-discovery.csv` is the original early-exit, incomplete audit, explicitly
  retained separately; use the full `tune-before.csv` / `tune-after.csv` for comparison.

This is a measured estimator fix, **not** proof of premium vocal quality. No formant
preservation was added. Low-frequency bias, breath/noise smearing, strong overtone
errors and transition behaviour still need real voice/sax recordings and listening.
Existing 100–1600 Hz intended range and monophonic design remain. No claim is made
that every effect has now reached the best possible or commercial-final quality.

## Local verification

Eight portable DSP tests passed after the production changes, plus the new 27-case
TuneQuality audit (nine portable tests total). Native CMake adds both new tests.
TimeEffects passed ASan/UBSan; TuneQuality passed ASan/UBSan. Realtime allocation-hook
suite includes the four-engine reverb bank and mode switching and reports zero DSP
loop allocations (not a claim that all host/JUCE operations are allocation-free).

TimeEffects checks tempo formulas, capacity, four distinct impulse responses,
finite/decaying tails, channel isolation, repeated switching and Room parity.
A separate byte-for-byte comparison against `50a0099`'s original Reverb passed at
all three rates with changing size/decay: source/PCM hashes in `room-equivalence.txt`.

Reproduce portable tests with g++ -O2 -std=c++17 -pthread -I Source and the respective
Tests/*Tests.cpp. TuneQuality: Tools/TuneQualityAudit.cpp. Use -O1 -g
-fsanitize=address,undefined for the retained sanitizer checks. For Room parity,
compile reverb-equivalence.cpp against old/new Effects.h include roots, then cmp
stdout PCM. No local JUCE build was performed: system GUI/audio dependencies and
CMake are unavailable here. Native verification is performed by platform CI.

Native tests added to both Studio and Live: mock playhead tempo/fallback changes;
measured processed delay impulse; 12 actual-processor pitch-strength/boundary cases;
previous/next/wrap restores full snapshots; 1152×720 native editor snapshots. These
must pass on the new candidate before old native test results can be cited for it.

## Release limits

The Live nominal software delay remains ~12 ms. Neither physical roundtrip, real
instrument listening, long-run device stability nor notarisation/store distribution
is established by this work. Commercial readiness remains a separate acceptance gate.

## Verified native builds and outputs

Final source `0eeeb36` passed Windows x64 and universal macOS build/tests in
[run 40136562296](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40136562296):
all **11 CTests on each platform**, actual processor pitch and sync-delay checks,
factory navigation/state checks, Windows VST3 pluginval strictness 5, and macOS AU
validation. This is not physical-device/DAW performance certification.

- [Windows VST3 + standalone](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40136562296/artifacts/12577636793)
- [macOS universal VST3/AU + standalone, unsigned for distribution](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40136562296/artifacts/12577041668)
- [Android ARM64 debug APK, not device-tested](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40136562348/artifacts/12578095351)
- [iPhone/iPad unsigned AUv3 + containing app build output, NOT an installable IPA](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40136562290/artifacts/12578050355)

Independent saved-evidence review
[40137510231](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40137510231)
checked both full JUnit reports and all **48 actual-processor tone measurements**
(12 cases × Studio/Live × Windows/macOS). See `native-pitch-results.json` and
`native-tests.json`. `compact-preview.jpg` is a compressed preview of the actual
Windows Live editor's 1152×720 test snapshot, not generated imagery. Original PNGs
are retained in the desktop test artifacts. Snapshot is taken immediately after
preset navigation; timer-driven scale indicators can be one UI refresh behind the
already-verified parameter state. The full screenshot was visually inspected for
layout: arrows and new selectors are visible and retain the original main design.

Install Windows by replacing the entire `Xiffects Live.vst3` bundle with the DAW
closed, then rescan. macOS distribution signing/notarisation, Android stable release
signing/device tests, and iOS personal signing/provisioning remain separate steps.
The earlier personal-use source ZIP is a frozen pre-change snapshot; it is not the
source archive for these new features. The repository's current iOS helper can
stage the current source without editing the original Windows implementation.

## Recovery and provenance

This turn began with restored files over an older local Git HEAD while the remote
branch retained the prior release history. Work was preserved in a local commit
and an explicit ours merge recorded the remote ancestry; no reset or force push
was used. Historical pinned Reference/onset/Live replays remain available. Hardcoded
old artifact-review workflows are manual replays rather than being retriggered
against the new production work. No old native validation is presented as proof
for this candidate.

Across the 48 saved native steady-tone measurements, the largest absolute error
was **0.560171 cents**. This applies only to those 452/904 Hz synthetic cases and
50/100% strength settings, not every note or recording. At 48 kHz Live, 904 Hz at
full strength measured 879.755 Hz (−0.481602 cents relative to 880 Hz). The independent
old-DSP reproducer measured about 929.997 Hz in that case; it was not an old full-host
render. Existing lower-bound 100 Hz limitations are not removed by this interpolation
change. Compressor, EQ, distortion, phaser and pitch-shift architecture were retained
and regression-tested, not all rewritten or claimed to be best-in-class.
