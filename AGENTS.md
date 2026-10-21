# Agent instructions for Xiffects

## Product goal
Build a polished soprano saxophone multi-effect audio plug-in branded:
- Product: Xiffects
- Tagline: UNIQUE SOUNDS BY XIVAN
- Visual style: premium minimal charcoal/black with crimson accents; legible controls; avoid a nearly-black unusable UI.

## Required modules
Octaver, Compressor, Reverb, Delay, Distortion, EQ, Pitch Shifter, Phaser, Auto Tune.

## Auto Tune requirements
- Musical key and scale selection.
- A chromatic note map with individually toggleable notes.
- Correction strength and retune speed.
- Eventually implement real-time pitch detection and correction, not just a visual simulation.

## Engineering constraints
- C++17 and JUCE, CMake build.
- Never allocate memory, lock a mutex, or perform file/network I/O in `processBlock`.
- Keep DSP blocks independently testable.
- Parameter changes must be automatable and state must be saved/restored.
- Add tests for DSP blocks before integrating them into the main processor.
- Keep GUI and DSP separate.
- Do not claim an effect is implemented until it actually changes audio correctly.
- Update README with real build/test steps and known limitations.
