# logisth 0.5.0 — Granular studio, graph arpeggiator and user presets

A complete update for all four source slots inside the same compact LIGHT/DARK console.

- **Granulizer:** independent grain and wave spacing, attack/hold/fade, reverse grain order, alternating pan, wave-spacing LFO, randomness, transient regions/detection, key mapping, time hold and live tempo stretching. The previous Cloud model remains available.
- **Sample view:** actual stereo waveform with a moving scanner, active grain positions, WAV region markers, zoom, pan and start-position editing.
- **Mixer scope:** choose part A, part B or the literal final L+R sum.
- **Eight arp graphs:** Note, Rand Sel, Octave, Pitch, Velocity Scale, Gate, Late and Harmony, with independent lengths and clock dividers, mute/reset markers, degree masks, note constraints and chord rotations.
- **Twelve named A–L patterns:** edits retained on switching, graph copy/paste/randomize and whole-pattern copying. Previous Pattern and Legacy engines are retained.
- **User preset library:** save/load `.logisth` files with embedded stereo samples, region markers, chord maps, all patterns and UI preferences. Folder selection and move-to-trash are included.
- **936 automatable parameters:** the original 514 IDs and their order are retained. Older projects select the appropriate earlier granular/arp models automatically.

Downloads include native **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual in PDF/Markdown,
source, interface previews, an audio demo and SHA256SUMS. Each native archive
contains build and test metadata.

The Fruity Granulizer and Cthulhu manuals were behaviour references. All DSP and
graphics are independently implemented. Proprietary preset formats are not
imported, and sound/detection results are not guaranteed to match those products.

Imports accept WAV/AIFF/FLAC, 8–96 kHz and up to 12 seconds per source. Granulizer
supports up to 64 overlapping grains per oscillator/unison voice. Dense granular
patches require more CPU. One pattern plays at a time; host MIDI routing varies.

All three platform builds must pass before the native archives are published:
eight test suites on Linux, six on Windows/macOS, none skipped. Linux tests also
load the actual VST3 and CLAP binaries. macOS runtime tests run on Intel; arm64
is architecture-checked. Windows is unsigned; macOS requires 11+, is ad-hoc signed
and not notarized. Linux glibc requirements are recorded in BUILD_INFO.json.
VST2 is not included because its separately licensed SDK is unavailable.
