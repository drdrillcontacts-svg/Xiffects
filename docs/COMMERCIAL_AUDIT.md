# Commercial-readiness audit — 2026-10-22

## Scope and baseline

Audit of the current workspace, not a redesign. Existing parameter IDs, native
UI, nine effects and state compatibility must be retained. Portable DSP, Pitch
and Realtime tests were compiled with GCC 12 and rerun successfully before edits.
The checkout contains the previous implementation as uncommitted changes over
`388be0d`; do not reset it or force-push over the previously published branch.
Historical Windows CI results are not evidence for newly changed code.

## Architecture

- `PluginProcessor`: JUCE mono/stereo wrapper, APVTS parameter definitions,
  cached atomic parameter pointers, smoothing, serial/parallel routing and state.
- `DSP/Effects.h`: independent compressor, delay, distortion, broad EQ, phaser,
  damped-comb reverb. No callback file I/O or obvious dynamic allocation.
- `DSP/Pitch.h`: decimated YIN, nearest allowed note with hysteresis, correction
  smoothing, three two-head granular shifters (tune then parallel octave/pitch).
- `Telemetry.h`: bounded SPSC audio-to-editor records; GUI cannot block audio.
- `PluginEditor` and `UI/Controls`: native JUCE controls, fixed-aspect desktop
  layout, attachments, meters, note keyboard, graph, file chooser and recipes.
- Tests: portable signal tests/allocation audit; JUCE host smoke/state/bypass test.
  CI: Linux portable tests and Windows VST3/Standalone build/host tests only.

## Findings (confirmed by code, unless labelled risk)

| Priority | Finding | Consequence / disposition |
|---|---|---|
| P0 | Constant two-stage pitch delay remains with all modules off | ~60 ms plugin delay before device/host buffering: not a stage-monitoring release |
| P0 | Host state replacement and preset load have different validation paths | Need one versioned schema, migration, duplicate/missing-ID/type/range checks |
| P0 | Preset load applies parameters individually | Audio can observe an intermediate preset; stage a coherent snapshot at a block boundary |
| P1 | Non-default factory presets are partial patches, not complete snapshots | Same preset can sound different depending on previous edits; only 3 recipes + Default |
| P1 | Compressor detector sees undelayed raw input but gain is applied after ~60 ms pitch delay | Unintended look-ahead/envelope mismatch, especially attacks/releases; needs signal-path regression test |
| P1 | YIN performs all lag comparisons on a single analysis sample | Average CPU is insufficient evidence; measure max/percentile small-block callback cost |
| P1 | Nearest-target search and retune exp coefficient are recomputed every sample even when unchanged | Avoidable hot-path work; cache only with exact invalidation and numerical regression tests |
| P1 | Detector scan/interpolation does not enforce documented upper pitch bound | Probe frequencies outside 100–1600 Hz and add a regression before changing |
| P1 | 12-bit mask plus integer note search | Cannot represent non-equal temperament or quarter tones; microtonal target data model needed |
| P1 | Eight scale choices indexed in serialized/automated parameter | Reordering/extending the existing choice can reinterpret old host automation; preserve legacy mapping |
| P1 | No hardware loopback, live-input or DAW validation in this environment | Cannot claim measured round-trip latency, no dropouts, or concert readiness |
| P2 | Grain read positions vary and follow detector period | Attacks/noise/vibrato can smear; nominal dry alignment is not phase-perfect |
| P2 | Raw spectral tests search near expected frequency only | May miss a stronger wrong octave/sideband elsewhere; add wideband error metrics and recordings |
| P2 | Fixed energy threshold/no explicit voiced onset/release state machine | Risk of unstable correction around breaths and transitions; needs labelled recordings |
| P2 | UI 1152×720 minimum and 9-column row | Desktop view not a usable phone layout; preserve visual language, add touch-specific views |
| P2 | Graph red trace is an interpolated correction estimate | Not measured output pitch, particularly at partial mix; retain truthful labelling |
| P2 | Comb reverb and hard output ceiling | Not a polished hall/transparent limiter; leave headroom and audition overload behaviour |
| P2 | macOS/AU, AUv3/iOS and Android targets absent | A portable core alone does not constitute mobile binaries or device validation |

## Proposed changes, in dependency order

1. **Safety baseline (current step):** preserve UI/IDs, reproduce defects, add
   measured pitch-path latency/acquisition and small-block timing harness; make
   only regression-tested hot-path fixes. No claim of reduced audio latency yet.
2. **Live processing:** extract a shared engine/config snapshot from the JUCE
   wrapper; prototype one-stage correction/harmony rendering and a bounded-work
   detector. Compare Studio and Live candidates on quality/latency/CPU. Do not
   simply halve grains or move work to a blocking worker thread. Any latency-mode
   change must occur with host reprepare/explicit stopped-engine transition, not
   unsignalled automation during a concert. Retain the old sound as Studio mode.
3. **Tuning model:** fixed-capacity, preallocated octave-periodic target set in
   floating-point cents, enabled flags and per-degree offset. Search adjacent
   octaves in cents, hysteresis in cents, ratios `2^(cents/1200)`. Shared immutable
   target snapshot drives keyboard AND audio. No rounding targets back to MIDI.
   Directional scales need a tested ascending/descending state with hysteresis.
4. **Compatibility:** keep legacy `TUNE_SCALE` semantics for old automation;
   add a versioned tuning-system selection, explicit IDs and state migration.
   Do not silently renumber the existing 8 choices. Legacy note switches remain
   valid; new custom targets have independent enabled/cents fields. New editor
   controls sit in a tuning drawer, not a wholesale main-layout redesign.
5. **Presets:** full versioned snapshots, factory/user IDs, categories, rename,
   duplicate/delete with confirmation, atomic file replacement, coherent audio
   publication, complete round-trip tests. Design >=50 distinct recipes across
   natural correction, hard correction, microtonal, jazz, pop, ambient, doubling,
   lead and utility uses. Validate unique parameter fingerprints, ranges and
   enabled-effect relevance; listening review is required before calling them
   professional. Do not pad the count with transpositions/name-only duplicates.
6. **Desktop gates:** Windows regression build, macOS universal VST3/AU builds,
   pluginval/host validation, automation soak, state migration and CPU profiling.
7. **Mobile adapters:** shared engine + touch view, platform-specific lifecycle,
   routes/interruption handling, session/sample-rate changes and permissions.
8. **Release:** device/DAW matrix, audio acceptance, licensing, signing, installer,
   crash diagnostics, recovery behaviour and reproducible release artifacts.

## Scale acceptance inventory — none omitted

The following are required, not claimed implemented by this audit. Each needs
explicit degree/cents data, source/provenance, 12-key transposition tests, nearest
microtonal target tests, serialization and matching UI presentation.

1. User Scale
2. Chromatic
3. Major
4. Natural Minor
5. Harmonic Minor
6. Melodic Minor (Ascending)
7. Melodic Minor (Ascending & Descending)
8. Major Justonic
9. Minor Justonic
10. Dorian
11. Mixolydian
12. Blues
13. Phrygian
14. Locrian
15. Lydian
16. Whole Tone
17. Diminished (W-H)
18. Diminished (H-W)
19. Bebop
20. Major Pentatonic
21. Minor Pentatonic
22. Major Pentatonic With flat 3rd
23. Minor Pentatonic + Blues
24. Bebop + Blues
25. Pythagorean
26. Maqam Rast (Rast-Rast)
27. Maqam Saba
28. Maqam Bayati
29. Maqam Nahawand
30. Maqam Hijaz
31. Maqam Kurd
32. Maqam Huzam
33. Bilawal (Hindustani)
34. Bilawal (Carnatic)
35. Kafi (Hindustani)
36. Kafi (Carnatic)
37. Bhairavi (Hindustani)
38. Bhairavi (Carnatic)
39. Kalyan (Hindustani)
40. Kalyan (Carnatic)
41. Khamaj (Hindustani)
42. Khamaj (Carnatic)
43. Asavari (Hindustani)
44. Asavari (Carnatic)

Definition decisions must be explicit: does “Justonic” mean a particular just
intonation ratio table? Which dominant/major Bebop variant? Is the Carnatic label
an equivalent pitch collection or a specific raga (notably Bhairavi)? Indian
ascending/descending rules and maqam regional variants cannot be reduced to a
single universally authoritative 12-TET list. Supply editable defaults with
provenance and variants; let users adjust cents, not just enable piano keys.
Equal quarter tones alone are not a complete model of Iranian/Turkish/Arabic tuning.

## Platform plan / genuine external requirements

| Platform | Proposed product | Required gate / limitation |
|---|---|---|
| Windows | VST3 + Standalone | ASIO/device tests, host matrix, signing/installer; past CI is baseline only |
| macOS | Universal VST3 + AU + Standalone | Apple SDK, Intel/ARM testing, AU validation, signing/notarization |
| iPhone/iPad | Standalone + AUv3 extension | Xcode/export/signing, entitlements, extension lifecycle, interruption/routing, real devices/hosts |
| Android | Standalone, low-latency device audio adapter | SDK/NDK/app project, permissions/audio focus/routes, device latency matrix; VST3 is not the assumed app host |

Keep engine/tuning/preset semantics common; choose/review mobile audio adapters
with platform toolchains before promising equivalent latency. Mobile UIs need
larger touch targets and module paging while retaining the black/crimson identity.
Commercial JUCE/dependency licensing and photograph/font rights must be checked;
open-source source code availability is not a commercial distribution licence.

## Measurement protocol

Report separately: (a) dry-path impulse sample delay, (b) voiced detection
acquisition/release, (c) correction settling, (d) callback p50/p99/max vs deadline,
(e) physical analog input-to-output loopback. Sum of reported plugin latency and
buffer sizes is only an estimate of (e), not a measurement. Do not add detector
acquisition blindly to steady-state audio transit delay.

For hardware measurements record device/driver/OS/DAW, sample rate, buffer sizes,
input/output safety buffers, mode, round-trip distribution, xruns and CPU load.
Measure clean bypass and each pitch mode with the same routing, avoiding DAW
latency compensation hiding the physical delay. Record at least 30 minutes of
representative live input before making concert-stability claims. No microphone,
interface, physical loopback or mobile device is exposed to this sandbox.

## Changes completed after the initial audit (same stage)

- Reproduced 1700 Hz being published as 1703.8/1715.4 Hz despite the declared
  100–1600 Hz range. Added publication bounds with a 15-cent estimator guard,
  clamping only inside that guard, preserving genuine 100/1600 Hz edge tones.
  Regression was run against baseline (failed) and fixed source (passed).
  This is not a guarantee against all octave errors from out-of-band input.
- Cached target selection when detected pitch/key/scale/manual mask are unchanged;
  cached retune coefficient when speed is unchanged. Strength and speed automation
  remain sample-wise. Reprepare invalidates all caches. A frozen reference
  controller tests equivalence at 44.1/48/96 kHz including mask changes/empty masks.
- Added an optional offline profile executable and raw before/after CSVs in
  `docs/measurements/`. Nominal measured pitch-bank delay remains ~60 ms; no live
  mode or reduced-latency claim is introduced by this stage.
- CMake Release: DSP, Pitch, CorrectionRegression, Realtime all passed locally.
  CorrectionRegression and Pitch also passed ASan/UBSan on the final source.
- Full native configure attempted again: blocked at missing `PkgConfig` and
  downstream Linux GUI/audio dependencies. No fresh Windows/macOS/mobile build
  or hardware-input test is claimed for this stage. Main UI/IDs/routing untouched.
- 44 scales, cents-based custom targets, 50 presets and platform adapters remain
  planned work, not implemented capabilities. Earlier defects outside the two
  narrow fixes above remain open until their own tests/changes land.
