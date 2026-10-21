# Xiffects — Unique Sounds by Xivan

A JUCE/C++ starter project for a soprano saxophone multi-effect plug-in. The project targets VST3 and Standalone on Windows and uses CMake + GitHub Actions.

## Included in this starter

- Dark charcoal / crimson UI concept and Xiffects branding
- Nine module panels: Octaver, Compressor, Reverb, Delay, Distortion, EQ, Pitch Shifter, Phaser, Auto Tune
- Host-automatable parameter tree (APVTS)
- Auto Tune key/scale selectors and 12 manual note toggles
- Correction Strength, Retune Speed and Mix controls
- GitHub Actions Windows build workflow

## Important status note

This is **starter source code**, not a finished commercial audio effect. The initial DSP currently passes audio through with output trim. The effect controls and parameter/state plumbing are scaffolded, but the actual algorithms for octaving, compression, reverberation, delay, distortion, EQ, pitch shifting, phasing, and pitch detection/correction still need implementation and testing. The hero area uses abstract artwork rather than embedding the reference photo.

## Build locally (Windows)

Requirements:
- Visual Studio 2022 with C++ desktop development tools
- CMake 3.22+
- Internet access on first configure (CMake fetches JUCE 8.0.6)

From a Developer PowerShell in the repository folder:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
```

The VST3 and Standalone targets will be under the build directory. Load the VST3 in a compatible host such as REAPER or Cubase for testing.

## GitHub

1. Create an empty repository named `Xiffects`.
2. Upload these files or push this folder using Git.
3. Open the **Actions** tab. The workflow builds the project on Windows for pushes and pull requests.
4. Download build artifacts only after adding an `upload-artifact` step to the workflow if you want CI binaries saved.

## Suggested development roadmap

1. Make the layout responsive and add the actual saxophonist image asset.
2. Implement and unit-test each DSP block separately.
3. Build a real pitch detector + note correction algorithm; the current Auto Tune UI is not pitch correction DSP.
4. Add preset serialization and preset browsing.
5. Add automated plugin validation and artifact packaging.
6. Test in multiple DAWs and at multiple sample rates/block sizes.

## References

- JUCE repository: https://github.com/juce-framework/JUCE
- JUCE plugin tutorial: https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/
