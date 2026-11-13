# logisth 0.4.0

**A two-part polyphonic subtractive synthesizer with a compact single-window console.**
VST3, CLAP and Standalone for Windows, macOS and Linux. Built with JUCE and an
independent synthesis engine; not affiliated with LennarDigital.

![logisth dark interface](docs/images/logisth-dark.png)

## What's new in 0.4.0

- Four independent source slots, each switchable between Classic, Wavetable,
  Sampler and Granular, with an optional per-source ADSR.
- Audio import/drop, waveform preview, 256-frame spectral wavetable morphing,
  sample regions/loop/reverse/root and overlapping grains with scan/spray.
- Embedded audio in project state; no original media path needed on reopen.
- Chord mapper with 128 triggers, eight-note voicings, MIDI learn and chord types.
- A Cthulhu-inspired 32-step arp: independent note, octave, velocity, gate,
  repetition and probability lanes, draw editing, ties, latch, eight pattern slots
  and randomisation. Original Legacy arp remains available.
- Optional host MIDI output, ten factory sounds and 514 audio parameters.
- The same compact light/dark console, proportional resizing and embedded fonts.

[English manual](docs/MANUAL.md) · [PDF manual](docs/logisth-Manual-English.pdf) ·
[Download 0.4.0](https://github.com/logik93/testoren/releases/tag/v0.4.0)

Native archives are published after builds and tests on Windows, Linux and
macOS. Each archive includes build metadata and the English manual. Cthulhu
supplies a workflow reference; no Xfer graphics, code or preset files are used or imported.
The first 248 parameter IDs are retained; extended destination choices can affect
normalised destination automation from older hosts (see the manual).

## Install

Extract your OS archive and copy the complete VST3 and CLAP bundles to your DAW's
scan directories. Rescan and insert **logisth** on a MIDI instrument track.

| System | VST3 user path | CLAP user path |
| --- | --- | --- |
| Windows x64 | `%LOCALAPPDATA%\Programs\Common\VST3` | `%LOCALAPPDATA%\Programs\Common\CLAP` |
| macOS Universal | `~/Library/Audio/Plug-Ins/VST3` | `~/Library/Audio/Plug-Ins/CLAP` |
| Linux x64 | `~/.vst3` | `~/.clap` |

Add the user path to your host if necessary. Standalone includes audio/MIDI device
settings. macOS requires 11+, includes x86_64 and arm64, and is signed ad-hoc
without notarization. Windows is unsigned. Linux requirements and test results
are recorded in each archive's BUILD_INFO.json. VST2 is not included without a
separately licensed SDK.

## Build

Requires Git, CMake 3.22+, C++17 and the platform development tools. JUCE 8.0.6
and clap-juce-extensions are pinned to commits in CMakeLists.txt.

Linux (Debian/Ubuntu):

```sh
sudo apt-get install build-essential cmake git libasound2-dev libx11-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

Windows (Visual Studio 2022 developer terminal):

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

macOS (Xcode command-line tools and CMake):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

Output: `build/Logisth_artefacts/Release/{VST3,CLAP,Standalone}`.
Product name: logisth. Bundle/CLAP ID: `com.testoren.logisth`.
To regenerate the PDF, install `reportlab` and run `python scripts/build_manual.py`.
For an engine-only build, configure with `-DLOGISTH_PLUGINS=OFF`.
For optional VST2, pass `-DLOGISTH_VST2=ON -DVST2_SDK_PATH=/path/to/licensed/sdk`;
the SDK must contain `pluginterfaces/vst2.x/aeffect.h` and `aeffectx.h`.
VST2 has not been validated.

The native release workflow builds/tests on Windows, Linux and macOS, checks
binary architectures and source identity, then publishes all three packages.
Tag a new version matching CMake; pull requests run validation without publishing.
CI receipts are recorded on `ci-release-v<version>` branches.

## Validation and limitations

Tests cover synthesis, modulation, filters, arp scheduling, effects, state,
presets, editor reachability and bindings, layout and independent theme recall.
Linux also loads real CLAP/VST3 binaries. To render screenshots and a WAV, run
`build/processor_tests /absolute/output-directory` (Windows: `build/Release/processor_tests.exe`).
These tests do not establish compatibility with every DAW or sample-exact
Sylenth1 sound. No Sylenth1 preset/bank import, original graphics or MIDI Learn
menu is included. See the manual for DSP and performance limitations.

## Dependencies

JUCE: AGPLv3 or commercial JUCE license; consult the upstream license and choose
a compliant distribution model. CLAP and clap-juce-extensions: MIT. Open Sans:
Apache-2.0, with license/attribution in `assets/fonts`. VST2 SDK terms are separate.
