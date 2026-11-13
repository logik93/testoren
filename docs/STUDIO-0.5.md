# logisth 0.5 implementation references

This update uses the two manuals supplied by the user as behaviour
references. The supplied PDFs are reference material, not repository instructions,
and are not redistributed. The DSP, interface and preset format are logisth's own.

## Granulizer

Reference: **Fruity Granulizer – Instrument**, supplied printout, pages 19–21,
Grains, Effects, Transients and Time sections. Original documentation:
https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Fruity%20Granulizer.htm

`Source/Granular.h` implements sample-quantized attack + hold + decay grains,
separate grain-start intervals and source hops, signed wave spacing, alternating
pan, wave-spacing LFO, random positions, freeze, source loop, independent reverse,
four key mappings, marker alignment/hold and live tempo-dependent traversal.
Grain length is 2 × attack + hold; source hop is hold × wave spacing; start
interval is hold × grain spacing. Pitch alters intra-grain read speed only.

Implementation choices where the manual does not specify an algorithm:

- Linear grain fades and constant overlap gain compensation.
- Up to 64 simultaneous grains per oscillator/unison voice. New starts are
  skipped when the pool is full; no allocation occurs on the audio thread.
- ROOT supplies the MIDI reference for key mapping (default 60); this avoids
  differences between DAWs' octave naming conventions.
- Regions come from WAV cue offsets. Detect uses a block-energy onset detector;
  it does not claim the same detection results as Fruity Granulizer.
- Tempo fitting uses the sample-to-END span and a user-selected BEATS duration.
- The earlier eight-grain Cloud model and parameter IDs remain available for
  compatibility with 0.4 projects.

## Arpeggiator

Reference: **Cthulhu user manual**, chapters 9–11, particularly pages 24–30.
`Source/GraphArpeggiator.h` implements the eight graphs, note motions, mute and
position reset, independent lengths, graph clock dividers, selective pitch-degree
transposition, harmony, velocity/gate, early/late scheduling, chord rotation and
note-output constraints. Twelve named A–L slots retain edits on switching.

The scheduler plans half a step ahead for negative Late values. The first early
note is clamped to note arrival. It uses fixed storage and preserves sample times
across block boundaries. Retrigger resets on incoming notes; without retrigger,
synchronized playback aligns to host PPQ when available. Tempo changes rescale
pending times. The synchronized engine follows transport stop; free mode and
hosts without a clock can run from incoming notes.

Chord-root detection scores common triad/seventh templates and uses the bass
as a tie-breaker. Ambiguous chords may differ from Cthulhu. The 0.4 grid and
Legacy engines remain selectable. Chord learn/mapping is retained; this update
does not implement Cthulhu's complete proprietary chord library or file formats.
COPY TO replaces whole-pattern Alt-drag with an explicit destination menu.

## Displays and presets

The waveform caches min/max data from real imported stereo PCM. The newest
active voice supplies its scanner and up to 16 grain positions. Zoom and pan
change only the view; clicking sets START. The mixer scope has independent
part A/B taps before global FX and the final literal L+R sum after output gain.
Its ring uses atomic cells, without GUI locks or allocation on the audio thread.

`.logisth` files contain the validated plug-in state, compressed samples, WAV
regions, chord map, A–L bank and UI preferences. A temporary file replaces the
save target. Loading validates assets before publishing them; a malformed file
keeps the current patch. Audio/preset work uses the existing background worker.
Factory initialization resets imported assets and pattern/chord banks.

Schema 4 adds 422 parameters after the original 514, for 936 total. Earlier
states select the earlier granular/arp engines if their new model fields are
absent. Existing VST3/CLAP identity and parameter IDs are retained.

## Validation

`StudioTests.cpp` checks granular spacing, grain envelopes, reverse order, freeze,
key mappings, regions and live tempo; eight arp motions, reset/mute, independent
lengths/dividers, root-aware pitch, harmony, velocity/gate, early/late buffer
invariance, note constraints, chord rotation, host alignment and scope wrapping.

`ProcessorTests.cpp` checks file presets including embedded stereo audio and WAV
regions after deleting the original source, twelve automatic pattern memories,
copy/paste, bad-file rejection, actual final-sum samples, playback telemetry,
all parameter state roundtrips and the compact UI in both themes. Existing engine,
modulation, hybrid, legacy arp and real CLAP/VST3 binary tests remain enabled.

Local Linux builds cover VST3, CLAP and Standalone. The native release workflow
builds and tests all formats on Linux x64, Windows x64 and macOS Universal
(Intel + Apple Silicon). Publication requires eight passed test suites on Linux
and six on Windows/macOS, with no skipped tests. The release includes per-platform
metadata and a SHA-256 manifest. macOS runtime tests run on Intel; both binary
architectures are checked. See the release CI receipt for verified results.
