# Local configure diagnosis (Reference source unchanged)

Observed on Debian 12 x64:
- `command -v pkg-config` found no executable.
- `/usr/include/alsa`, `/usr/include/X11`, `/usr/include/freetype2` are absent.
- `/var/cache/apt/archives` contains no cached development package archives.
- Fresh native configure fails at `FindPkgConfig`: `PKG_CONFIG_EXECUTABLE` missing.
- Caller: JUCE 8.0.6 `extras/Build/CMake/JUCEModuleSupport.cmake:391`,
  `find_package(PkgConfig REQUIRED)` in Linux system dependency discovery.

This is missing native development dependencies, compounded by this sandbox's
outbound allowlist (normal Debian package hosts are not allowed), not evidence
of a broken DSP algorithm or an incorrect Windows CMake target. A Python package
named `pkgconfig` does not supply ALSA/X11 headers and is not a complete fix.
Do not fake PKG_CONFIG_EXECUTABLE or remove JUCE dependency checks.

On a normally networked Debian/Ubuntu development machine the principled fix is:

```sh
sudo apt-get install build-essential pkg-config cmake ninja-build \
  libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev
cmake -S . -B build-reference-native -DXIFFECTS_BUILD_REFERENCE_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference-native --parallel 2
```

That package installation was NOT executed here against prohibited hosts. Local
native execution remains blocked. Windows Actions is a separate real Windows
runner and does not need Linux pkg-config/ALSA/X11; it is not a claim that Linux
has been repaired. Raw current configure stdout/stderr are stored beside this file.

## Publishing the existing Reference branch

The remote branch still pointed to earlier session history `d2338da`, while the
local Reference was recorded at `26e8bb3`. Both histories were retained with a
content-preserving `ours` merge on the same fixed branch. No force-push, branch
switch, Source/Assets replacement or DSP merge was performed. Production bytes
are verified against `baseline-files.json` and `git diff 26e8bb3 -- Source Assets`.

The first Windows verification failed before configuration. Windows checkout
line-ending conversion was addressed by `git config --global core.autocrlf false`
BEFORE checkout; exact source SHA-256 verification then passed on the subsequent
Windows run. This is a runner setup correction, not a change to production files.
