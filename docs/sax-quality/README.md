# Insert transparency / UI candidate

## Scope and rationale

- Fresh Live used an enabled EQ at -2/+1.5/+2/+3 dB. This demonstrably colours an inserted track; it is not proof of the cause of the user's specific sax complaint.
- New Live instances: effects off, EQ flat, output 0 dB. Tune Mix defaults to 100% (inactive until enabled); Strength remains the correction amount. Saved parameter values and the 50 complete factory recipes are unchanged. A separate `Clean - No effects` menu action is available for existing instances; it resets parameters, not instantaneously flushed delay/reverb buffers. Smoothing still applies.
- Live compressor detector now follows the actual pitch-bank output rather than the input ahead of its delay. Studio retains its previous detector and defaults.
- Live pitch voices use four-point cubic reads, reducing fractional-delay error versus linear on the tested tones. Studio keeps linear reads. Windows, buffers and nominal software delay are unchanged. Cubic interpolation is not formant preservation, oversampling, or a guarantee of natural sax timbre/absence of aliasing.
- Header byline removed, original hero image/baked byline preserved, exact programmer credit added beneath. Interface Size menu uses ASCII; module circle is an explicit Unicode codepoint.

## Validation

Ten portable suites passed locally using `g++ -std=c++17 -O2 -pthread -ISource`: Interpolation, DSP, Pitch, Onset, LivePitch, CorrectionRegression, Realtime, Scale, TimeEffects, TuneQuality. Raw output is alongside this document. These are algorithm tests, not native product approval.

Native tests added to the real Live processor: fresh-default latency-compensated null and delayed compressor-model comparison, mono/stereo at 44.1/48/96 kHz with varying block sizes. Existing tests retain 50-preset/state restoration, 48 platform/product pitch measurements, UI and host validation. Final native run **40144341822 succeeded**: all 12 CTest suites on both Windows and macOS, Windows pluginval strictness 5 and macOS auval passed. Saved-output review and screenshot extraction are recorded below.

Test inputs are synthetic, in normal unclipped range. The output safety ceiling remains; transparency above 0 dBFS is not claimed. No physical roundtrip measurement, real sax/voice recording comparison, listening approval or device certification has been performed for this candidate. Prior delivery test results do not validate these new changes.

## Additional checks and initial failure

ASan/UBSan passed LivePitch and Interpolation. Studio shifter same-input PCM is byte-identical to 6450bde at three rates, including changing ratio: SHA256 `6278c2d0c59e0a63868230f963abffb0e2cf517478ea7622146d4b2ac2b65657`. This is an independent shifter comparison, not a full Studio host render. Factory-bank regeneration produced no diff; hero bytes are unchanged.

Initial native run 40143729998 built macOS and passed its first fresh-insert null case with maximum error 0, but failed an excessively strict `OUTPUT == 0` assertion after Clean reset. JUCE normalised float round-trips require a rounding tolerance; test revision 9a1e04a checks within 0.0001 dB. Production DSP was not changed to mask this assertion. Raw diagnostic annotations are retained (long first annotation was truncated by GitHub; the failure-bearing final chunk is complete). Final native run: 40144341822, success. Initial Windows job also passed; the initial overall run failed due to the macOS assertion described above.

## Downloads

- Windows VST3/Standalone: https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40144341822/artifacts/12579979393
- macOS universal (unsigned): https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40144341822/artifacts/12579659611
- iOS/iPadOS (unsigned compile, not an installable signed IPA): https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40143730025/artifacts/12579817825
- Android ARM64 (debug, not device tested): https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40143729999/artifacts/12580007642

Desktop revision: 9a1e04a; mobile revision: b3d49c1. Production Source/Assets/CMake are identical between these revisions; only test assertion tolerance and diagnostic workflow changed. Mobile builds passed, not physical-device/audio tests. Existing projects retain their saved values: select **Clean - No effects** to audition the neutral reset after parameter smoothing. Enabling effects still changes sound intentionally; preserving parameter recipes is not a promise of byte-identical processed Live audio after the DSP fixes.

## Reviewed measurements

Saved-output review **40145235835 succeeded**. Both 12-suite XMLs, all 48 processor pitch rows, all 24 insert/sidechain rows and the native Windows screenshot were retrieved. The screenshot was viewed, not merely generated: header byline removed, hero byline retained, programmer credit visible beneath, existing layout retained. It shows the Velvet Room preset after navigation testing, not the fresh Clean defaults.

- Fresh insert, 3 sample rates × mono/stereo × 2 OS: **maximum absolute null error 0** after software-delay compensation for these synthetic inputs.
- Compressor cases: maximum absolute error **1.48982e-08** against the delayed-signal golden compressor model across both OS.
- Pitch regression: all 48 cases within 5 cents; worst absolute error **0.560171 cents** (synthetic steady tones, not sax timbre quality).
- Windows VST3 binary **5,567,488 bytes** (~5.57 MB), below the accepted 50 MB allowance. SHA256 `57d707eaab3c3e263bc898c085f4bbcadbded81a093e24f3b7d741b18b7a5c0f`.
- Nominal software delays remain **531 / 578 / 1154 samples** at 44.1 / 48 / 96 kHz. These are not physical input/output latency measurements.

First review 40145166345 succeeded but GitHub dropped its eleventh per-step notice; the final review uses fewer notices and all 24 rows were recovered. No DSP change was involved.
