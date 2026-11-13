# logisth 0.4.0 — Four engines and chord patterns

Each of the four source slots now switches between **Classic, Wavetable,
Sampler and Granular** inside the same compact LIGHT/DARK console.

- Per-source ADSR, tuning/unison, file import/drop and waveform previews.
- Interpolated wavetable morphing across up to 256 frames, with spectral mip bands and phase warp.
- Stereo sample playback with root, start/end region, loop crossfade and reverse.
- Overlapping Hann grains with size, density, position, spray, scan and reverse.
- Embedded, compressed audio in project state; original media files are not required on recall.
- A chord mapper with 128 triggers, eight-note voicings, chord types and MIDI learning.
- A Cthulhu-inspired 32-step pattern editor: independent lane lengths, draw editing,
  note rank, octave, velocity, gate, repeats, probability, ties and latch.
- Eight stored pattern slots, randomisation, host MIDI output and ten factory sounds.
- 514 automatable parameters, grouped menus and the original Legacy arp.

Downloads include native **VST3, CLAP and Standalone**, the English manual in
PDF/Markdown, source, screenshots, a four-engine audio demo and SHA256SUMS.
The demo switches engine every two seconds: Classic, Wavetable, Sampler, Granular.

The original 248 parameter IDs remain in order. Older states fall back to Classic
and Legacy. The destination menu has grown: review old destination automation
stored as normalised values, since extra choices can change those positions.

Imports accept WAV/AIFF/FLAC, 8–96 kHz and up to 12 seconds per slot. Wavetable
extraction uses the left channel; Sampler/Granular retain stereo. This is a
single-sample source per slot, with one pattern active at a time. Cthulhu is a
workflow reference; its graphics, code and preset formats are not included.

Native archives are published only after all three platform builds and tests
succeed. Windows is unsigned; macOS Universal (Intel + Apple Silicon) requires
macOS 11+, is ad-hoc signed and not notarized. macOS runtime tests run on Intel;
arm64 code is architecture-checked, not hardware-auditioned. Linux glibc and
validation details are recorded in BUILD_INFO.json. VST2 is not included without
a separately licensed SDK. Host MIDI routing and DAW compatibility vary by host.
