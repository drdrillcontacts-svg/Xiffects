# Factory bank and platform build candidate

This work does **not** claim zero latency or production/mobile store readiness.
The Live audio architecture remains the measured ~12 ms nominal software path.
The earlier Studio/Reference and verified Live candidate can still be rebuilt
from their pinned workflows/commits. New source changes are separate development.

## Factory bank

50 deterministic complete snapshots in `Assets/factory_presets.json`, exposed in
the native preset menu. Ten sound families: intimate sax, sax lead, natural voice,
tuned voice, space, echo, octave, harmony, motion and texture. No random recipe
creation at runtime. Each selection explicitly restores all 52 parameters, including
module enables, key/scale, all 12 manual permissions, output and power. Selecting a
factory recipe intentionally replaces the previous manual note mask.

`Tests/FactoryBankTests.h` tests all 50 on the actual processor at 44.1/48/96 kHz:
parameter completeness/ranges, unique recipes/names, host state restoration and
bounded finite audio. These tests are not real sax/voice listening approval.
Recipes include substantial differences in routing, time, pitch and gain, but
still require instrument-specific listening and level refinement before release.

## Scales

44 labelled choices, including Custom/Manual, are defined once in `DSP/Scales.h`.
Some traditional names are synonyms (e.g. Romanian Minor/Ukrainian Dorian), not
44 unique mathematical masks. All transpositions and manual-mask intersections
are tested. Custom allows the existing 12 note switches to define an arbitrary
12-TET pitch-class set. Empty masks release correction.

These are **12-tone equal temperament** maps; the Persian-labelled map is explicitly
12-TET. They do not implement authentic microtonal dastgah/maqam tuning. Per-note
cents, reference tuning and a full cultural tuning editor are still outstanding.
Do not market this as complete regional/microtonal scale support.

The original 8-value `TUNE_SCALE` parameter retains its range and indices, avoiding
normalised automation reinterpretation. New `TUNE_SCALE_EXT` is 0 for legacy mode,
or 1–36 for scale entries 8–43. Choosing a legacy scale in the UI clears the extended
override. Legacy host/file states missing the new selector are migrated to 0 so
a previously selected extended scale cannot silently override the restored old preset. Hosts can automate both; when nonzero, the extended selector takes priority.

## Native mobile implementation and limits

The iOS/iPadOS build uses the same real JUCE processor, with an AUv3 extension
embedded in a standalone container, microphone usage description, iPhone+iPad
family and arm64 / iOS 15 minimum configuration. Unsigned compilation is **not** an
installable IPA, TestFlight release or proof of host/device operation. Distribution
requires the developer's Apple signing/provisioning process; no credentials are
requested or embedded here.

Android has a JUCE 8.0.6 Projucer/Gradle native standalone audio project with recording
permission, minimum API 26 and ARM64 debug configuration. It is not a VST3/AU for
Android hosts. This foreground standalone app must be tested for permission denial,
routing, device changes, interruptions, feedback and underruns on physical devices.
Debug builds are not Play Store releases or stable product signing identities.

Both mobile platforms use a dedicated native touch layout: effects/module pages,
tuning with larger note buttons, and a monitor page with real telemetry/pitch/waveform.
The desktop artwork, colours and desktop layout remain. Phone portrait and larger
tablet layouts are the design target; arbitrary host view sizes, landscape, safe
areas, accessibility and touch-device interaction are not yet physically validated.
No feature is replaced by a mock audio UI.

## Build/test evidence

Portable tests are saved in `portable-tests.txt`. Platform-specific run outcomes
and deliverable links are recorded below only after their jobs complete. Desktop
Windows VST3/standalone, macOS universal VST3/AU/standalone, unsigned iOS AUv3/container
and Android ARM64 debug are separate outputs and must not be confused.

Unsigned macOS artifacts are not Developer ID signed/notarised installers. Both
architectures are compiled; tests run only on the CI runner's architecture.
AU validation and VST3 validation are host checks, not certification in every DAW.

## Installation / developer use

- Windows: extract the artifact, copy the **whole** `Xiffects Live.vst3` bundle
  from `VST3/` into `C:\Program Files\Common Files\VST3`, then rescan. Standalone
  is in `Standalone/`. It is a Windows x64 VST3, not VST2 or a 32-bit plugin.
- macOS: extract the artifact and then its inner ZIP (preserves bundle symlinks).
  Copy the VST3 to `~/Library/Audio/Plug-Ins/VST3/` and the AU `.component` to
  `~/Library/Audio/Plug-Ins/Components/`. These are universal arm64+x86_64 binaries,
  but unsigned/not-notarised for distribution; macOS security policy may block them.
  A public release needs Developer ID signing/notarisation, not blanket security disabling.
- iOS/iPadOS: the artifact is **unsigned build evidence, not an installable IPA**.
  Use the CMake Xcode project with your own Apple development team/provisioning to
  install and test the container and its AUv3. Do not present this as a TestFlight app.
- Android: if an APK build passes, it is an ARM64/API26+ **debug** application,
  signed with an ephemeral development key. Not a store/release signing identity;
  different build runs may not update an older installation in place. Export user
  presets before any uninstall. Microphone/headphone routing still needs device testing.

Do not replace the older Reference in existing sessions for a comparison. Select
Xiffects Live explicitly. Pitch-stage software delay remains approximately 12 ms;
zero-delay dry mode and a substantially different active-pitch architecture are
still unfinished. No personal device configuration is being used as a substitute
for reducing the product's own delay.

## Verified candidate outputs

Production source: `a4b6b0e` (later commits only change build/review tooling and
reports). Each following run checked out a commit with these same Source/Assets
bytes; `production-hashes.json` records the final files. This is a development
snapshot, not a new signed commercial release.

| Platform | Output | Verification / download |
|---|---|---|
| Windows x64 | Live VST3 + standalone | [Download](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699124874/artifacts/12392402232). Nine CTests including both actual processors' 50-preset tests; pluginval strictness 5 passed. |
| macOS arm64 + x86_64 | Live VST3 + AU + standalone | [Download unsigned bundle](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699124874/artifacts/12392407296). Nine CTests and native `auval -v aufx Xifl Xiva` passed. Intel architecture compiled, not separately run. macOS VST3 not separately host-validated. |
| iOS + iPadOS | AUv3 embedded in standalone container | [Unsigned build evidence](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699124885/artifacts/12392655966). Build passed; no installable IPA or device/host run. |
| Android ARM64 | Native standalone debug APK | [Download development APK](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699179967/artifacts/12392876452). Native C++ and APK build/signing passed; not run on a device. |

GitHub sign-in may be required to download. Final desktop run
[39699124874](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699124874)
is successful on both operating systems. No audio algorithm or nominal-latency
reduction beyond the earlier Live bank is claimed by this release-bank work.

### Build failures retained, not relabelled as successful

- Initial Android project-generator compilation failed due to missing `curl/curl.h`.
  The host build dependencies now include libcurl development headers; diagnostic
  run 39697897679 retained the explicit failure. This was a build-host dependency,
  not a phone DSP result.
- Android run 39698491014 reached Gradle but failed its custom debug signing
  configuration because no development keystore existed. Final workflow explicitly
  creates an ephemeral debug key and uses Bash pipefail so a failed Gradle process
  cannot be hidden by `tee`. APK artifact globs cover the generated JUCE build variant.
- The first universal macOS run built and passed processor tests but AU lookup
  failed before refreshing the component registry. Saved-bundle diagnostic
  39698350695 then passed `auval`; the final workflow refreshes the registry and
  independently passes it again. That earlier failed run is not cited as a pass.
- A previous remote-log retrieval diagnostic failed; build diagnostics are now
  saved and summarised from the runner directly. No failed step was ignored to
  obtain a final package.

The earlier reference documents and artifact links are historical baselines.
They do not certify this new factory-bank/platform snapshot. Full physical audio,
long-term stability, real voice/sax listening and signing/store release remain open.

### Independent final package inspection

[Inspection run 39700229135](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39700229135)
passed against downloaded final artifacts, not assumed build paths. It verified:
Windows PE x64 VST3; all three macOS executables contain **both** arm64 and x86_64;
Android APK includes `lib/arm64-v8a/libjuce_jni.so` and RECORD_AUDIO permission;
iOS container declares microphone use and device families 1+2, with an AUv3
extension actually embedded under the app's PlugIns folder. Both desktop JUnit
reports contain all nine passing tests. Binary/APK SHA-256 values and full focused
results are in `package-inspection-summary.json`. This static inspection does not
add any claim of physical-device audio testing.

Final local portable run passed all seven tests; Scales also passed ASan/UBSan.
JUCE is pinned to 8.0.6. All build commands, SDK/runner setup and validation commands
are in the corresponding workflows; CI logs retain the build environment. Android
currently uses target API 34 / minimum API 26 and is **not** a claim of meeting
current Play Store submission requirements.
