# logisth 0.3.0 — User Manual

A two-part polyphonic subtractive synthesizer for VST3, CLAP and standalone use.
This manual describes logisth's own controls and behaviour. logisth is an independent
instrument and is not affiliated with LennarDigital. It does not load Sylenth1
presets or reproduce its sound exactly.

## Contents

1. Installation and first sound
2. The single-window console
3. Oscillators and amplitude envelopes
4. Filters and shared Filter Control
5. Modulation
6. Arpeggiator
7. Effects rack
8. Mixer and performance
9. Presets, automation and project recall
10. Sound-design examples
11. Troubleshooting and technical notes

## 1. Installation and first sound

Download the archive for your operating system from the GitHub release. Each
native archive contains VST3, CLAP and Standalone folders. Extract it first.
Copy the entire .vst3 bundle and the .clap file or bundle into a plug-in directory
scanned by your DAW. Keep the internal structure of bundles intact.

| System | User VST3 directory | User CLAP directory |
| --- | --- | --- |
| Windows | %LOCALAPPDATA%\Programs\Common\VST3 | %LOCALAPPDATA%\Programs\Common\CLAP |
| macOS | ~/Library/Audio/Plug-Ins/VST3 | ~/Library/Audio/Plug-Ins/CLAP |
| Linux | ~/.vst3 | ~/.clap |

If your DAW does not scan these user directories, add them to its scan paths or
use its documented system directory. Rescan plug-ins, insert logisth on a MIDI
instrument track, select Init / analog from SOUND LIBRARY, and play a note.
The on-screen keyboard also generates notes. Start at a moderate listening level.

For Standalone, launch the executable or app and choose audio and MIDI devices
in its audio settings. The standalone wrapper provides its own device settings;
those are separate from the synth panel.

Windows builds target x64. macOS Universal bundles contain Intel x86_64 and Apple
Silicon arm64 code and require macOS 11 or later. The minimum Linux glibc version
is recorded in BUILD_INFO.json. macOS bundles have an ad-hoc signature, without
Apple notarization; Windows binaries have no publisher signature. Follow your
OS's normal review/open procedure for downloaded applications you trust.

VST2 is not included: building it requires a separately licensed VST2 SDK.

## 2. The single-window console

All sections occupy one console. No page needs vertical scrolling. Select A or B
at the top to edit that part's two oscillators, filter and amplitude envelope.
This selects the editing view; it does not mute the other part. Use SOLO for
listening to one part. The central rack changes its controls when you select a
rack name. The small square beside each name independently enables that section.
Selecting a rack page does not enable or disable its processing.

The upper row contains the preset selector, previous/next preset buttons, A/B,
SOLO, POLY, SYNC and THEME. Filters are on the left, the rack in the centre, and
the mixer on the right. Mod Env, LFO and Misc routing remain visible below them.
The bottom row contains wheels, the keyboard and mono/portamento controls.

Drag the bottom-right resize handle to scale the console from 80% to 150%.
The nominal size is 1240 by 860 pixels. Initial size adapts to the display.
THEME switches between LIGHT and DARK. Each instance saves its own theme with
the plug-in state. The theme changes appearance without changing audio parameters.

Drag knobs and faders to adjust values. Numeric fields can be dragged or edited.
Examples of accepted values include 2.5k for 2500 Hz, 350ms and 65%.
Double-click a control to restore its default; modulation amount controls return
to zero. Hover for a tooltip and the parameter/value readout at the bottom.
Disabled phase knobs become available when their oscillator's RETRIG is enabled.

## 3. Oscillators and amplitude envelopes

There are four oscillators: A1 and A2 feed source A; B1 and B2 feed source B.
Each oscillator has a waveform menu, 0–8 unison voices, volume, octave, note,
fine tuning, phase, detune, stereo width, pan, retrigger and invert.

| Control | Purpose |
| --- | --- |
| Wave | Selects the oscillator waveform. |
| Voices | Sets the unison voice count. Zero silences this oscillator. |
| Volume | Sets its level before filtering and the part envelope. |
| Octave / Note / Fine | Coarse octave, semitone and cent tuning. |
| Phase / Retrig | Retrig starts notes at the selected phase; otherwise phase runs without this reset. |
| Detune | Spreads unison pitches. Increase gradually for a wider ensemble. |
| Stereo | Spreads unison voices across the stereo field. |
| Pan | Positions the oscillator output left or right. |
| Invert | Inverts oscillator polarity; it can affect cancellation when sources mix. |

The amplitude envelope for each part has attack, decay, sustain and release.
Attack rises from silence, decay approaches sustain, and release fades after
note-off. Sustain is a level; the other three are times. The line above the
faders illustrates the current envelope. Amp Env A and B remain independent.

## 4. Filters and shared Filter Control

Each part has an input selector, type, slope, cutoff, resonance and drive.
Input A uses A1+A2, B uses B1+B2, A+B uses both, and None disconnects the input.
Consequently, a part's filter can process the other oscillator source. Its output
still passes through its own amplitude envelope and Mix level.

Choose low-pass, high-pass, band-pass or bypass. Slope selects 12 or 24 dB per
octave. Cutoff is the local frequency. Resonance emphasises the cutoff region;
high resonance can self-oscillate while a note is active. Drive adds saturation.

The shared FILTER CONTROL affects both A and B while retaining their local
settings. CUTOFF is an offset in semitones: +12 doubles both cutoff frequencies,
-12 halves them. RESO adds a shared resonance offset. KEYTRACK ties cutoff to
played pitch; at full tracking, an octave higher doubles cutoff. WARM changes
the filter saturation characteristic. There is no third filter in this section.
Modulation is combined with these settings before the effective cutoff is limited
to a valid frequency for the current sample rate.

## 5. Modulation

Mod Env 1 and 2 each provide an ADSR envelope and two destination/amount pairs.
LFO 1 and 2 also provide two pairs each. Misc 1 and Misc 2 each contain two source
blocks, with two pairs per block. There are 16 routes in total. The source menu
in a Misc block chooses the signal used by both routes in that block.

Choose a destination, then move its small amount knob away from zero. Amount
ranges from -10 to +10; negative values reverse the modulation. None or amount
zero makes a route inactive. Routes to the same destination are added together.
The destination popup groups oscillators, filters and modulation/effects into
three columns.

Destinations include Volume, Pitch, Phase and Pan for A, B or A+B; Cutoff and
Resonance for A, B or A+B; Mix A, B or A+B; Phaser Frequency; LFO 1/2 Rate and
Gain; and Distortion Amount. Modulating A+B affects both parts together.

Misc sources include Velocity, Keytrack, Aftertouch, Mod Wheel, Pitch Wheel,
Amp Env A/B, Mod Env 1/2, LFO 1/2, Step Velocity and random sources. Polyphonic
pressure takes precedence over channel pressure for the affected note. Random
values are generated at note-on. Step Velocity follows the active arp step and
returns to zero after its gate ends.

Each LFO has waveform, Rate, Gain, Offset and FREE. With SYNC enabled and FREE
off, RATE is a rhythmic division at the host tempo. With FREE enabled it uses
its Hz rate and does not restart on each note. Otherwise notes restart the LFO.
Gain scales its output; Offset shifts it. The global fallback tempo is 120 BPM
when no host tempo is available.

Modulation depth is specific to logisth. At amount +10 and source value +1,
pitch adds 48 semitones, cutoff adds 120 semitones, phase adds one cycle, and
pan adds one full side. Volume, Mix, Resonance, Gain and Distortion add one
linear unit. LFO Rate multiplies by 16 and Phaser Frequency by 1024. Smaller
amounts give smaller changes; start low. Effects are global, so FX modulation
uses the newest active voice. LFO feedback modulation uses the previous sample.

## 6. Arpeggiator

Select ARPEGGIATOR in the rack and enable its square. Hold a note or chord.
MODE selects a directional pattern, ordered playing or a step-sequencer mode.
The ten modes include up/down variations and Step Seq / Step Chord.
Step Seq plays one note per step; Step Chord processes the held chord.

TIME uses a rhythmic division with SYNC, or a free-running time otherwise.
GATE controls note length within the step, SWING offsets alternating steps,
OCT sets the octave span, WRAP controls pattern wrapping, and LENGTH sets the
number of active steps, up to 16. VELOCITY selects how key and step velocities
contribute; Step+Key and Step+Hold multiply their normalised values in logisth.

The step grid has PITCH, VELOCITY and HOLD rows. Use 01–08 and 09–16 to view all
steps. PITCH transposes in semitones in the step modes; it is inactive in other
modes. Velocity zero is a rest. HOLD ties a step into the next one. A changing
pitch in Mono Legato can glide between tied notes. Values outside the active
length remain stored for later use.

Enabling the arp clears held notes; play them again. Sustain is supported.
Host stop/seek and enable changes clear scheduled notes. Events are scheduled
at sample offsets rather than only at block boundaries.

## 7. Effects rack

Signal order is fixed: Distortion → Phaser → Chorus → EQ → Reverb → Delay →
Compressor → Main. Click a name to edit it and use the adjacent square to bypass
or enable it. All seven effects are global after the A/B mix.

| Section | Controls and use |
| --- | --- |
| Distortion | Overdrive, Foldback, Clip, Decimate and Bitcrush; Amount and Dry/Wet set character and blend. |
| Phaser | Six-stage stereo phasing with centre frequency, spread, L/R offset, width, LFO, feedback and Dry/Wet. Its frequency is a modulation destination. |
| Chorus | Delay, depth, rate, feedback, dual mode, width and Dry/Wet thicken a sound. |
| Equalizer | Bass and treble shelves with frequency controls and optional positive-gain compensation. |
| Reverb | Size, damping, predelay, width and Dry/Wet shape the room tail. |
| Delay | Independent left/right times or divisions, feedback, ping-pong, spread, width, low/high cut, smear and Dry/Wet. |
| Compressor | Stereo-linked threshold, ratio, attack and release control dynamics. |

When SYNC is on, rate/time fields that support synchronization become division
menus. Turning SYNC off exposes their independent Hz/time values again. Bypass
can freeze a section's internal memory; a tail may resume when re-enabled.
Delay memory is limited to 16 seconds per channel, including very slow synced
divisions. Compressor attack/release and some effect delays use milliseconds;
envelope times use seconds internally. The panel displays suitable units.

## 8. Mixer and performance

MIX A and MIX B set the part levels; MAIN controls output after all effects.
Independent L/R meters show post-effects output. There is no hard output limiter:
loud patches, resonance and effects may exceed 0 dBFS. Reduce MAIN as needed.

POLY sets up to 16 simultaneous notes. Each note may contain several unison
voices. SOLO isolates A or B. MONO / LEGATO uses last-note priority and returns
to the previous held note when the newest one is released. POLY is disabled in
mono mode. GLIDE sets portamento time; Normal mode glides for overlapping notes,
while Slide also permits glide between separate notes.

BEND / st sets pitch-wheel range in semitones. The on-screen BEND wheel springs
back to the centre after dragging. MOD remains where you set it. Route Mod Wheel
through a Misc block to hear its effect. MIDI pitch bend and modulation messages
also update these controls. The keyboard covers five octaves.

## 9. Presets, automation and project recall

The six factory sounds are Init / analog, Wide supersaw, Sub bass, Slow atmosphere,
Pulse pluck and Step sequence. Choose a sound in SOUND LIBRARY or use the arrow
buttons. Loading a sound replaces the current patch; save the project first if
you want to retain edits. The preset selection is a factory sound selector, not
a user bank manager. Use your DAW's plug-in preset/state system to save patches.

All 248 audio parameters are available for host automation. A/B view changes,
rack selection and theme do not alter their values. The theme is saved as UI
metadata rather than an additional audio parameter. DAW state contains all audio
parameters, modulation routes and the 16-step pattern. Reopening an instance
restores its stored theme. Parameter IDs remain unchanged from logisth 0.2.0.

There is no Sylenth1 bank import or MIDI Learn menu. Legacy prototype states
receive a best-effort migration of filter and modulation settings; changes to
DSP mean that identical sound is not guaranteed.

## 10. Sound-design examples

For a wide lead, load Wide supersaw. Raise unison detune gradually, open shared
CUTOFF, and add a little Delay. Use MONO / LEGATO with a short GLIDE for connected
melodies. Lower MAIN if the combined voices become too loud.

For an envelope sweep, load Init / analog, lower Filter A cutoff, select Cutoff A
as Mod Env 1's first destination, and add a small positive amount. Set attack
near zero, moderate decay, low sustain and short release. Increase the amount
until the filter opens sufficiently at the start of each note.

For vibrato, choose Mod Wheel in a Misc source block and route it to LFO 1 Gain.
Set LFO 1 to a sine wave in FREE mode at a few Hz, set its base Gain to zero,
then route LFO 1 to Pitch A+B with a small amount. The wheel opens the vibrato
depth. Keep amounts low to avoid large pitch excursions.

For a sequence, load Step sequence and hold a note. Adjust PITCH and VELOCITY,
set a zero velocity for a rest, and enable HOLD to tie steps. Select 09–16 and
increase LENGTH to use the second half. Automate the shared filter cutoff to
move both parts together without replacing their local settings.

## 11. Troubleshooting and technical notes

No sound: check the audio/MIDI device or DAW routing, oscillator Voices and
Volume, filter input, envelope sustain, Mix levels and MAIN. Check SOLO and
whether the arp needs fresh notes. A long attack or low cutoff can hide a note.

No modulation: check the source is active, destination is not None, and amount
is nonzero. The mod wheel needs an assigned route. A source can be zero during
part of a note; Step Velocity needs an active arp gate.

Missing control: A/B selects the part, a rack name selects effect controls, and
SYNC/FREE selects time versus rhythmic controls. RETRIG enables phase editing.
The second group of arp steps is under 09–16.

High CPU: reduce unison voices, polyphony or active effects. Some abrupt changes
to routing, waveforms and delay times can click. Selected continuous controls
are smoothed, but not every transition is crossfaded.

Build validation includes audio at 44.1/48/96 kHz, filters, modulation, arp timing,
state restoration, preset output and editor bindings/layout/theme persistence.
Linux tests additionally load the actual CLAP and VST3 binaries. A VST3 factory
smoke test does not replace testing in your particular DAW. Native macOS tests
run on Intel; Universal architecture checks establish that arm64 code is present,
not that it has been auditioned on Apple Silicon hardware.

The filter is an independent SVF design; reverb uses JUCE Reverb. Modulation
scales and some sequencer behaviour are implementation choices. No sample-exact
comparison with Sylenth1 has been performed. Third-party font notices are included
in assets/fonts. The original Sylenth1 manual and graphics are not redistributed.
