# logisth 0.6.0

**A two-part polyphonic subtractive synthesizer with a compact single-window console.**
VST3, CLAP and Standalone for Windows, macOS and Linux. Built with JUCE and an
independent synthesis engine; not affiliated with LennarDigital.

![logisth dark interface](docs/images/logisth-dark.png)

## New in 0.6.0

- Calibrated source bus: one centred full-scale sine reaches 0 dBFS peak per
  channel before effects and at unity Main. Shared effects receive that same
  reference; there is no hidden attenuator, reciprocal boost or startup fade.
- Factory Main trims provide headroom; the mixer displays output dBFS.
- Drop audio directly onto the large SAMPLE waveform or its header to load
  the selected A1/A2/B1/B2 slot. Successful Classic drops select Sampler;
  invalid files leave the source and mode intact.
- No fixed sample-duration limit. Active imported stereo PCM has a 512 MiB
  aggregate memory budget; portable state/presets allow up to 1 GiB.
- Long-file waveform peaks are computed on the import worker for fast zoom/pan.
- [Calibration report](docs/CALIBRATION.md), independent reference WAV, rendered
  sine and calibration preset are included with the release.

Levels and effect drive have been recalibrated together. Existing patches may
sound different; use the new factory presets and check Main when loading older
sounds. Parameter IDs remain stable; preserving old sound is not the objective.

## Fixed in 0.5.1

- Buttons and menus stay stable between GUI refreshes; Retrig, Invert and the
  arp enable switch respond to each click.
- The arp playhead clears on disable in all three engines; latch, sustain and
  scheduled notes are checked by regression tests.
- Larger embedded text, a minimum 11 px base font and taller popup menu rows.
- Press/refresh/release tests cover both parts and both themes, plus audio
  checks for retrigger phase and inversion polarity.

## What's new in 0.5.0

- Granulizer model with separate grain/wave spacing, attack/hold, alternating
  pan, wave-spacing LFO, randomness, transient regions/detection, key mapping,
  time hold, reverse and tempo stretching. Previous Cloud model remains available.
- Large sample waveform with a live scanner and grain positions, zoom and seek.
- Mixer oscilloscope: part A, part B or actual final L+R sum.
- Eight independent arp graphs, signed early/late timing, clock dividers,
  pitch-degree masks, note constraints, chord rotation and twelve named A–L patterns.
- User preset library: save/load/delete .logisth files with embedded audio,
  chords, all patterns and UI preferences. 936 automatable audio parameters.
- Compact single-window interface with light/dark themes.

[English manual](docs/MANUAL.md) · [PDF manual](docs/logisth-Manual-English.pdf) ·
[Implementation references](docs/STUDIO-0.5.md) ·
[Download 0.6.0](https://github.com/logik93/logisth/releases/tag/v0.6.0)

Native archives are built and tested on Windows, macOS and Linux. Each package
contains the English manual and build metadata. Publication requires all platform
builds and tests to pass.
Cthulhu and Fruity Granulizer manuals supply behaviour references. logisth uses
its own implementation and graphics and does not import their proprietary presets.

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
