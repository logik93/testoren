# logisth 0.7.0 — Wavetable frames and focused oscillator controls

- **WT POS selects one complete periodic wave from a wavetable bank.** The selected
  frame repeats at the played note's pitch; the entire audio file is not played as
  a long oscillator cycle, and adjacent frames are not silently blended. The
  factory bank now contains **128 waves**, with up to 256 imported frames supported.
- The central source window displays a **perspective wave stack, highlighted
  playing frame and one-based frame number**. Drag to select a frame or use the
  mouse wheel to move one frame at a time. During playback the display follows
  the latest voice's modulated WT position. Position smoothing is independent of
  unison count, so adding voices no longer changes the speed of WT POS movement.
- **Auto import reads Serum WAV `clm ` cycle-length metadata**, otherwise using
  2048 samples per cycle. Short power-of-two single-cycle files are detected.
  Manual cycle lengths of 128, 256, 512, 1024, 2048 and 4096 samples override Auto.
  Wavetables use the file's left channel; Sampler and Granular keep stereo audio.
- **Graphs arp has been removed** from the editor, scheduler and host parameter
  list. **Legacy and Pattern remain**, with their previous controls, the seven-row
  Pattern grid, A–L memories, latch, MIDI output and global MIDI-only operation.
- Oscillator view menus show only pages relevant to the selected source engine.
  Sampler no longer exposes Granulizer pages or periodic-oscillator phase/retrigger
  controls. Cloud hides Granulizer-only Motion and Time pages; Classic hides
  file-only source controls. All four oscillator slots follow the same rules.
- The **English PDF/Markdown manual**, interface guide, screenshots and calibration
  measurements are updated. Existing waveform sample drops, portable presets,
  long-sample loading, filters, effects, modulation and the A/B/L+R scope remain.

Validation covers selecting and rendering all 128 frames, WAV/CLM import, manual
cycle-length override, single-cycle files, live frame telemetry, unison-independent
position smoothing, all four mode-specific menus and mouse selection in the WT
window. **148 button interaction cases** exercise press/refresh/release in both
parts and themes. Legacy and Pattern are checked for disable/latch/sustain release.
The calibrated sine, all source engines and factory headroom are verified at
44.1, 48 and 96 kHz; portable presets and a 60-second dropped sample are reloaded
without relying on the original audio file.

This release exposes **575 automatable parameters**. The removed Graphs block is
not reserved; Graphs automation and the sound of older patches are not preserved.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual and build metadata.
Source, previews, a demo, calibration audio/preset/report and SHA256SUMS accompany
the release. Publication requires every platform's tests to pass with none skipped.
macOS 11+ binaries are ad-hoc signed and not notarized; Windows is unsigned.
macOS runtime tests run on Intel, with both binary architectures checked. Linux
requirements are in BUILD_INFO.json. VST2 is not included without its separately
licensed SDK.
