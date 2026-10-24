# Insert transparency / UI candidate

## Scope and rationale

- Fresh Live used an enabled EQ at -2/+1.5/+2/+3 dB. This demonstrably colours an inserted track; it is not proof of the cause of the user's specific sax complaint.
- New Live instances: effects off, EQ flat, output 0 dB. Tune Mix defaults to 100% (inactive until enabled); Strength remains the correction amount. Saved parameter values and the 50 complete factory recipes are unchanged. A separate `Clean - No effects` menu action is available for existing instances; it resets parameters, not instantaneously flushed delay/reverb buffers. Smoothing still applies.
- Live compressor detector now follows the actual pitch-bank output rather than the input ahead of its delay. Studio retains its previous detector and defaults.
- Live pitch voices use four-point cubic reads, reducing fractional-delay error versus linear on the tested tones. Studio keeps linear reads. Windows, buffers and nominal software delay are unchanged. Cubic interpolation is not formant preservation, oversampling, or a guarantee of natural sax timbre/absence of aliasing.
- Header byline removed, original hero image/baked byline preserved, exact programmer credit added beneath. Interface Size menu uses ASCII; module circle is an explicit Unicode codepoint.

## Validation

Ten portable suites passed locally using `g++ -std=c++17 -O2 -pthread -ISource`: Interpolation, DSP, Pitch, Onset, LivePitch, CorrectionRegression, Realtime, Scale, TimeEffects, TuneQuality. Raw output is alongside this document. These are algorithm tests, not native product approval.

Native tests added to the real Live processor: fresh-default latency-compensated null and delayed compressor-model comparison, mono/stereo at 44.1/48/96 kHz with varying block sizes. Existing tests retain 50-preset/state restoration, 48 platform/product pitch measurements, UI and host validation. New native run is pending at the time of this note.

Test inputs are synthetic, in normal unclipped range. The output safety ceiling remains; transparency above 0 dBFS is not claimed. No physical roundtrip measurement, real sax/voice recording comparison, listening approval or device certification has been performed for this candidate. Prior delivery test results do not validate these new changes.
