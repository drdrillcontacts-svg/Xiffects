# Final UI and manuals delivery — 25 October 2026

Production / native test revision: **90b6ecf**.

## Changes

- Programmer credit: regular 11 px font, wider tracking and muted grey to visually match the baked hero byline. The original image and its lettering are unchanged; its original font file is not supplied, so exact typeface identity cannot be established from the bitmap.
- Reverb and Delay now have dedicated compact header panels. Their old decorative waveform marks are omitted rather than painted underneath selectors. Sync, division, BPM, title and enable button occupy separate rectangles.
- Settings menu restores About, with an English product description and the exact requested artist/programmer Instagram IDs. Product copy is centralised in `Source/UI/ProductInfo.h`.
- Two seven-page embedded-font PDF manuals, English and Persian, with editable source, pinned Python dependencies and a build script. Persian mixed-direction text uses explicit RTL base direction and per-line shaping.
- No DSP, processor, parameter, preset-bank, hero-image or latency changes relative to the previous delivery.

## Verified

**Desktop run 40146100036 succeeded** on Windows and macOS: 12 CTest suites each; Windows pluginval strictness 5; macOS system auval. New native UI assertions check compact/standard/large header control separation and non-bold, reduced-size credit text. Existing tests cover mono/stereo and variable blocks, bypass delay, 50 complete preset recipes and state restoration, note/pitch cases, tempo sync and processor null/sidechain comparisons.

Ten portable suites also passed locally with g++ 12.2.0. DSP allocation checks are integrated algorithm tests, not an exhaustive measurement of every possible host callback.

**iOS/iPadOS run 40146099957 succeeded** (unsigned build only). **Android run 40146099997 succeeded** (ARM64 debug). Compilation is not physical-device audio or GarageBand validation.

PDF checks: 7 pages per language; all 14 pages rendered; nonempty text and text bounds checked. English cover and Persian mixed-direction pages visually reviewed. The first mixed-direction rendering issue was fixed before delivery. Manuals cover installation, all modules, automation/state, delay divisions/fallback, tuning, practical troubleshooting and limitations.

## Downloads

- [Windows VST3 + Standalone](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40146100036/artifacts/12580653270)
- [macOS universal unsigned](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40146100036/artifacts/12580402932)
- [iOS / iPadOS unsigned build only](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40146099957/artifacts/12581256611)
- [Android ARM64 debug, not device-tested](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/40146099997/artifacts/12581211727)
- [Persian PDF](../../Manuals/Xiffects-Live-User-Manual-FA.pdf)
- [English PDF](../../Manuals/Xiffects-Live-User-Manual-EN.pdf)
- [Both PDFs as ZIP](../../Manuals/Xiffects-Live-User-Manuals.zip)

## Explicit limits

This is a completed requested delivery, not a guarantee of zero defects in every host or configuration. Not performed: exhaustive host/device matrix, long-duration physical live-performance soak, actual sax/voice listening comparison, physical roundtrip measurement, native About-dialog click-through on each device or security signing/notarisation. The native screenshot covers the main editor, not an open About dialog. Existing reports are retained rather than silently replaced.

## Final saved-output review

Review run **40146820165 succeeded**. Both 12-suite XML reports, 48 native pitch rows and 24 native insert/sidechain rows were recovered and checked. Fresh-input null maximum error remains **0** for the synthetic normal-range cases; compressor golden-model maximum error is **1.48982e-08**. Windows VST3 binary is **5,571,072 bytes**, SHA256 `886cc2bc9e3fcdded160b7504b9985d511f660c508a6817e1c0bb4c5a23e606c`.

`compact-preview.jpg` is the actual Windows native editor at 80% after the navigation test selects Velvet Room. It was visually inspected: credit weight/size/tracking reduced, original hero byline intact, no decorative waveform sticking out beneath the time-effect selectors, BPM separated from the title. Original PNGs are in the Tests artifacts. This screenshot is not the Clean starting state.
