# Xiffects: personal iPhone/iPad AUv3 installation investigation

## Evidence boundary — please read first

**`AUv3.rar` was not supplied and is not present in this workspace. I have not
inspected that exact RAR, and cannot certify its contents or signature.** Upload
it for an exact-file inspection. A RAR name is not evidence of an installable app.

The recorded generated artifact was instead:

- Run `39699124885`, source `a4b6b0e12c75d27ab1b03f6c8ed8c4d1da94f124`.
- Artifact `12392655966`: `Xiffects-iOS-iPadOS-unsigned-build-only`.
- Inner archive: `Xiffects-iOS-unsigned-NOT-INSTALLABLE.zip`.
- Recorded **outer GitHub artifact** SHA-256:
  `3736b24d21444bc57c78c62015e3c2d16b5a3d8b7bb656e5461500b9d9e07ee2`.
  This is not a checksum for your RAR or necessarily the inner ZIP.
- [Original artifact](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39699124885/artifacts/12392655966).
- [Earlier package inspection](https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39700229135).

A new download attempt in this session failed at GitHub's external artifact
storage (EOF). Thus original bundle findings below come from the retained earlier
artifact inspection, not a fresh extraction. The current 13 Source/Assets files
were checked against the retained production manifest: all match. Windows source,
root CMake and UI files have not been edited during this investigation.

## 1. Complete plugin or build output?

The original artifact is an **unsigned compiled iOS build output**, containing a
standalone container and AUv3 extension. It is not a signed, installable IPA.
Structural completeness and installability are different questions.

If your `AUv3.rar` contains only `Xiffects Live.appex`, it is incomplete for normal
installation on an iPhone/iPad. The extension cannot be imported into GarageBand
through Files, installed independently, or turned into a working app by renaming
an archive to `.ipa`. Its containing application must be installed by iOS.

## 2. Where is the containing application?

The earlier inspection verified these paths in the original inner ZIP:

```text
Release/
  AUv3/
    Xiffects Live.appex/                 # separate build product
  Standalone/
    Xiffects Live.app/                   # install THIS container, after signing
      Info.plist
      PlugIns/
        Xiffects Live.appex/             # embedded extension
          Info.plist
```

The important extension is the copy **inside the `.app/PlugIns/` directory**.
The source project uses JUCE's AUv3+Standalone targets; JUCE adds the dependency
and Xcode's `XCODE_EMBED_APP_EXTENSIONS` configuration automatically. Do not
manually copy an extension into an already-signed container after signing.

## 3. iOS or macOS? What architecture?

The recorded build explicitly used:

```text
CMAKE_SYSTEM_NAME=iOS
CMAKE_OSX_ARCHITECTURES=arm64
CMAKE_OSX_DEPLOYMENT_TARGET=15.0
TARGETED_DEVICE_FAMILY=1,2
```

That is an iPhone/iPad **device** build configuration, not the macOS Universal
artifact. Earlier extracted plist checks confirmed device families `[1, 2]`,
microphone usage description and the embedded extension. However, that inspection
did NOT record the iOS Mach-O platform load commands. Therefore device arm64 is
verified as the build setting, not a fresh binary finding about your RAR.

On an actual extracted archive, use `inspect_bundle.py` below to distinguish
`LC_BUILD_VERSION` platform 2 (iOS device), 7 (iOS Simulator) and 1 (macOS).
**Arm64 alone does not distinguish them.** A macOS `.component` or `.vst3` does
not work in GarageBand for iOS.

## 4. Is it configured as an AUv3 effect GarageBand can load?

The source configuration is for an audio effect, not a synth or MIDI effect:

```text
IS_SYNTH=FALSE
NEEDS_MIDI_INPUT=FALSE
IS_MIDI_EFFECT=FALSE
PLUGIN_MANUFACTURER_CODE=Xiva
PLUGIN_CODE=Xifl
```

At JUCE 8.0.6 these flags select `kAudioUnitType_Effect` (`aufx`). JUCE's AUv3
plist generator creates:

```text
NSExtensionPointIdentifier = com.apple.AudioUnit-UI
NSExtensionPrincipalClass = <generated JUCE AU factory class>
NSExtensionAttributes.AudioComponents[0]:
    type          = aufx
    subtype       = Xifl
    manufacturer  = Xiva
    sandboxSafe   = true
    tags          = [Effects]
```

The earlier artifact inspection checked an Audio Unit extension point and its
embedding, but did not save all AudioComponents/factory-class fields. The above
full registration is **expected from inspected source/JUCE templates**, not a
claim that every field in your RAR was read. Do not manually guess or rename the
factory class; it must match the factory compiled into the extension.

This is the appropriate AUv3 category for GarageBand's **effect slots**. It is
not expected in the Audio Unit instrument picker. **Actual enumeration, loading,
audio processing and mobile UI behaviour in GarageBand have NOT been tested.**
macOS `auval` success is not iOS GarageBand validation.

Source references inspected: JUCE 8.0.6 `extras/Build/CMake/JUCEUtils.cmake`
(AU type selection, extension bundle ID, embedding), and
`extras/Build/juce_build_tools/utils/juce_PlistOptions.cpp` (AUv3 registration).

## 5. What signing is missing?

The original workflow explicitly used `CODE_SIGNING_ALLOWED=NO` at configuration
and build time and did not set up developer provisioning. It must be treated as
**not development-signed for your devices**. The exact signature/profile contents
of your RAR remain unexamined.

A personal device installation requires:

1. An Apple Development certificate with its matching private key accessible to
   Xcode in your own Mac's Keychain. Do not send either here.
2. A signable, unique container bundle ID and a separate extension ID under the
   same team. The extension ID must be prefixed by the containing app's ID.
3. Valid development provisioning for BOTH bundles, with your iPhone/iPad
   registered, matching identifiers/entitlements, and unexpired certificates/profiles.
4. Correct signing of the embedded extension and then the containing application.
   Xcode should do this as part of the dependency/embedding build.

No App Store distribution is required for this development-install workflow.
A free Personal Team can be tried if Xcode supports provisioning the requested
app/extension capabilities. Do not assume it will approve every capability;
if Xcode rejects it, resolve that account/capability restriction or use an Apple
Developer Program team. Free provisioning is typically short-lived (commonly
seven days), not permanent personal installation. Check the generated profiles.

A successful `codesign --verify` alone is not sufficient: an ad-hoc signature may
verify its hash but still not authorise installation on an iOS device.

## 6. Precise personal installation steps (Mac + physical device)

### Prerequisites

- A Mac with full Xcode and an iOS SDK/device-support version compatible with your
  actual phone/tablet OS. Command Line Tools alone are insufficient.
- CMake 3.22+, Git and Python 3; internet access for JUCE 8.0.6 on first configure.
- Your Apple account added in Xcode Settings > Accounts.
- A paired, trusted iPhone/iPad running iOS/iPadOS 15 or newer.
- On iOS/iPadOS 16+, enable Developer Mode when prompted, normally under
  Settings > Privacy & Security > Developer Mode, then complete the restart prompts.

### Generate a fresh, signable Xcode project without touching Windows

Extract the accompanying **source ZIP**, enter its top-level directory, then:

```sh
xcode-select -p
xcrun --sdk iphoneos --show-sdk-path
cmake --version
python3 Platforms/iOS/PersonalUse/generate_xcode.py \
  --bundle-id com.youruniqueid.xiffects \
  --output "$HOME/Xiffects-iOS-Personal"
open "$HOME/Xiffects-iOS-Personal/build/Xiffects.xcodeproj"
```

Replace `com.youruniqueid.xiffects` with an identifier you can register/sign.
The output directory must not already exist. You may additionally supply
`--team YOURTEAMID` using your public 10-character Team ID; otherwise choose the
team in Xcode afterwards. No secret is needed by this script.

The helper copies the source, changes **only that copy's Live bundle ID**, and
runs the existing project's iOS CMake configuration with automatic development
signing allowed. It does not edit the original CMake, Windows code or UI.
Do NOT reuse the CI cache with `CODE_SIGNING_ALLOWED=NO`.

### Xcode signing and run

1. Select scheme **XiffectsLive_Standalone**, not the desktop/Studio target.
   Select your actual connected iPhone or iPad, not a Simulator or “Any iOS Device”.
2. In Signing & Capabilities, check **Automatically manage signing** and select
   the SAME Team for **XiffectsLive_Standalone** and **XiffectsLive_AUv3**.
3. Confirm the container ID is your chosen ID. With JUCE's current naming rule,
   an ID ending `.xiffects` gives an extension ID ending `.xiffects.xiffectsAUv3`.
   Check the generated settings; do not independently rename the extension to
   an unrelated identifier. The original ID ending `.live` derives `.live.liveAUv3`.
4. Let Xcode register the devices and generate profiles. Resolve ALL signing
   errors. Do not remove necessary signing checks to obtain a green build.
5. Confirm the standalone target embeds `XiffectsLive_AUv3` in its App Extensions
   build phase. Build/Run the standalone scheme on your device. This should
   install the containing app together with its extension if provisioning succeeds.
6. Follow any developer-trust prompts (Settings > General > VPN & Device Management
   if required). Launch the app as a smoke test and allow microphone access if
   testing live input. A successful standalone launch does not prove AUv3 hosting.
7. Repeat installation for the iPad, registering/provisioning that device as well.

### GarageBand test

1. Open GarageBand on that device; create/open a project with an Audio Recorder
   track or another track supporting effect slots. Use a short recorded/imported
   clip first, so microphone monitoring does not confuse the host-loading test.
2. Open **Track Controls > Plug-ins & EQ > Edit**, tap an available effect-slot
   **+**, then **Audio Unit Extensions** (wording can vary by GarageBand version).
3. Look for **Xiffects Live** (possibly with the manufacturer name Xivan).
   It is an effect, not an AU instrument. If GarageBand exposes an “Enable iOS
   Effect Plug-ins” setting, enable it; its location varies with iOS version.
4. Insert it and verify the editor opens without a crash. Play the clip. In the
   Effects page select Delay, enable it, and set an obvious wet mix/time, or choose
   the `Long Answer` factory preset. Verify audible processing and bypass behaviour.
5. Save and reopen the GarageBand project; check restored parameter values/audio.
   The preset dropdown's label itself is not stored as audio state.
6. Test microphone monitoring separately using headphones and conservative levels.
   Record the device model/OS, GarageBand version, bundle IDs and test outcomes.

If it is absent: confirm the containing app actually installed and is still
provisioned, the embedded extension exists, and the plist declares an `aufx` Audio
Unit. Relaunch GarageBand after installation. If it appears but fails to open,
collect the device console/crash log from Xcode and inspect the extension's load,
signing and OS-compatibility errors. Repeatedly rebuilding unsigned files is not
a fix. Registration success is separate from audio and touch-layout correctness.

## 7. Source package and inspection tools

The provided source ZIP contains the original CMake project, Source, Assets,
Tests and Tools plus this guide and two iOS-only helper scripts. JUCE 8.0.6 is
fetched by CMake; it is NOT vendored in the ZIP, so the first configure needs
internet. This is complete application/project source with a declared dependency,
not a pre-generated `.xcodeproj` or a signed IPA.

`generate_xcode.py` has been tested in **stage-only** mode; original source hashes
were unchanged. It has NOT been generated, compiled, signed or installed on a Mac
in this session. The original project's unsigned iOS build did pass previously;
that does not verify this new personal-signing workflow end to end.

For an extracted archive or your newly signed build, run on your Mac:

```sh
python3 Platforms/iOS/PersonalUse/inspect_bundle.py "/path/to/extracted/Release" \
  --output "$HOME/xiffects-ios-inspection.json"
```

This reads real plists, executable hashes and Mach-O platform load commands. On
macOS it also invokes `codesign` and decodes embedded provisioning metadata.
It deliberately omits device UDIDs and certificate blobs. Review the report before
sharing it; bundle/team identifiers are still included. Synthetic parser tests
passed for iOS device, iOS Simulator and macOS Mach-O data; **that is not a binary
or GarageBand test of your RAR**.

**Current verdict:** original artifact = structurally complete unsigned iOS build
output, not an installable personal-use plugin. Exact `AUv3.rar` verdict, valid
personal signing, installation, GarageBand discovery, audio and UI tests = pending.
