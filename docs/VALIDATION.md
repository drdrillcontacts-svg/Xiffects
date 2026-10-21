# Validation record and release gates

## Observed locally (2026-10-21)

- GCC 12 / C++17, CMake portable Release build: DSP, Pitch and Realtime tests pass.
- Pitch tests built separately with `-fsanitize=address,undefined`: pass.
- Independent spectrum measurement: 440 Hz shifted by 0.25, 0.5, 1.0594631,
  2 and 4 has fundamental within 8 cents at 44.1/48/96 kHz.
- Detuned 452 Hz corrected to chromatic A4: resynthesized fundamental within
  10 cents after settling (100% strength/mix, 12 ms retune).
- Ordinary allocation audit: zero allocations during prepared DSP loop.
- Linux full configuration is blocked by missing pkg-config/audio/GUI packages;
  a successful portable build is not a successful VST3 build.

## Windows CI

Changes are pushed to `arena/dc90c04d-xiffects`, with full JUCE build and host tests.
Consult the exact commit's Actions run; never infer success from an earlier run.
Only successful runs publish downloadable artifacts. Results are reported in the
conversation when observed, rather than treating workflow configuration as proof.

## Still required before commercial release

- Listen to real soprano sax recordings: attacks, legato, vibrato, breath noise,
  low dynamics, sustained overtones, note boundaries, silence/noise transitions.
- Compare pitch tracking against annotated fundamental tracks; quantify octave
  errors, cents error and acquisition/release times. Stress note-mask automation.
- Audition all extreme pitch intervals and wet/dry combinations. Assess aliasing,
  transient smearing and partial-mix comb filtering. Decide on a lower-latency
  mode versus improved spectral/formant-preserving algorithms.
- Validate VST3 with pluginval and test in multiple DAWs, including automation,
  project reload, bypass compensation, offline export and transport restarts.
- CPU profiling at 32/64-sample blocks and high sample rates; audio callback
  allocation hooks in a real host, denormal checks, long-run feedback stability.
- Review native GUI at supported window sizes / Windows display scaling,
  screen-reader and keyboard usability; compare generated snapshot to reference.
- Exercise preset save/load cancellation, invalid XML, permission failures and
  editor destruction during an asynchronous file chooser.
- Obtain original photograph and approved typography/logo assets and confirm
  distribution rights. Review current hero crop and layout with the owner.
- Replace/upgrade the hard safety ceiling and basic comb reverb as appropriate;
  do not advertise transparent limiting or a commercial pitch-correction engine.
