# logisth 0.5.2 — User Manual

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

## 3.1 Four source engines

Every oscillator slot A1/A2/B1/B2 has its own engine menu: Classic, Wavetable,
Sampler or Granular. All four may use different engines and different audio files.
The adjacent view menu selects VOICE, SOURCE, ENV, GRAINS, MOTION or TIME without opening another window.
VOICE holds the shared tuning, level, unison, stereo and pan controls. SOURCE
holds the engine-specific controls; ENV holds that oscillator's optional ADSR.
The central Amp Env A/B remains the final envelope for its part.

Classic retains the original oscillator waveforms and retrigger behaviour.
Wavetable scans up to 256 frames using POSITION. Frames are interpolated and
precomputed spectral bands reduce aliasing at higher notes. WARP bends phase.
Choose a frame size (256/512/1024/2048 samples) before IMPORT when loading a
concatenated wavetable WAV. The factory table morphs between sine and harmonics.
Wavetable extraction uses the file's left channel. Padding is used for files
shorter than one frame; additional frames beyond 256 are ignored.

Sampler plays the imported audio at the selected ROOT MIDI note (default 60).
START and END choose a region; LOOP repeats it with a short crossfade; REVERSE
reverses playback. Without LOOP, playback ends at the selected boundary. Root
sets the reference pitch, not automatic pitch detection. Stereo files retain
both channels; mono files feed both channels. Unison and pitch bend still apply.

Granular offers two models. **Granulizer** is the default for new patches;
**Cloud** retains the eight Hann-windowed grains, size, density, scan and spray
controls from 0.4. Existing projects select Cloud automatically.

In Granulizer, GRAINS contains ATTACK, HOLD, GRAIN SPACE and WAVE SPACE.
ATTACK sets both fade-in and fade-out, so total grain duration is twice ATTACK
plus HOLD. GRAIN SPACE is the interval between grain starts relative to HOLD;
WAVE SPACE advances the source by HOLD times that percentage at every start.
With zero ATTACK and both spacings at 100%, playback reconstructs the sample
at its original pitch. Increasing GRAIN SPACE slows traversal without changing
pitch. WAVE SPACE can be negative: this reverses the order of grains while each
grain still reads forwards. REVERSE separately reverses each grain's samples.
Up to 64 grains overlap per unison voice; further starts are skipped if full.

MOTION contains alternating grain PAN, wave-spacing modulation DEPTH and SPEED,
RANDOM source-position variation, TIME HOLD (freezes traversal), LOOP and REVERSE.
TIME contains transient mode Off / Regions / Detect, transient HOLD, KEY TO,
TEMPO SYNC and BEATS. Regions uses imported WAV cue points; Detect uses a local
onset detector. A crossed marker aligns the grain and replaces its hold time.
KEY TO selects Pitch, Percent, Step or Transient. Pitch follows played notes;
the other modes keep source pitch and use keys relative to ROOT for position:
+12 semitones selects 50% of the sample, advances 12 holds, or selects region 12,
respectively. Negative position offsets clamp to the first position/region.
TEMPO SYNC fits traversal to BEATS at the current tempo without repitching grains.
START sets the initial location; automation moves the active scanner. LOOP wraps
over the sample to END; TIME HOLD keeps creating grains at the current location.

Click an oscillator's waveform strip or select SAMPLE in the rack for the larger
waveform view. Choose A1/A2/B1/B2. It draws the actual stereo amplitude envelope,
region markers, start/end and the most recent active voice's scanner, plus up to
16 active grain positions. Click to change START; wheel zooms around the cursor;
Shift-wheel pans. The display is a view of playback, not a sample editor.

Enable OSC ENVELOPE in ENV to give a slot an additional attack, decay, sustain
and release. A part's shared envelope still limits its final output. New
Position A/B/A+B and Grain size A/B/A+B modulation destinations scan frames or
sample locations and scale grain duration. Position changes are smoothed.

IMPORT accepts WAV, AIFF and FLAC at 8–96 kHz, up to 12 seconds per source.
Alternatively, drop one file onto an oscillator panel: a Classic slot switches
to Sampler, while the other engine selections are retained. Import decoding and
table preparation run on a worker, and the footer reports completion or errors.
RESET returns the slot's material to the factory glass sample/table. The waveform
strip previews the loaded material. No original media path is needed on project
recall: imported PCM data is compressed into the plug-in state, with a 64 MiB
state-input limit. Save larger libraries separately; this is a single-sample
source rather than a multisample instrument.

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

## 6. Chords and pattern arpeggiator

The CHORDS rack page and Pattern arp use a chord-programming and multi-lane
workflow inspired by Cthulhu, with logisth's own interface and implementation.
It is not a Cthulhu clone and does not import Cthulhu chord/pattern files.
The chain is MIDI input → Chords → Arpeggiator → Synth and optional MIDI output.
Each processor has its own enable square in the rack.

### Chord mapper

Choose CHORDS and enable its square. TRIGGER selects one of 128 MIDI notes.
VOICING assigns Unison, Major, Minor, Maj 7, Min 7, Dominant 7, Sus 2, Sus 4,
Power, Diminished or Augmented to that trigger. MAP ALL assigns the selected type
to all triggers, transposed to each root. Eight note fields edit the stored
voicing directly: drag or enter MIDI numbers 0–127; -1 removes a voice. Empty
triggers pass their original note through. Shared notes in overlapping chords
are reference-counted so releasing one trigger does not cut another chord short.

LEARN collects up to eight incoming MIDI notes for the selected trigger. Play
your voicing, then choose DONE. Notes are collected silently during learning;
DONE stores the first eight distinct pitches in ascending order. The complete
chord map is saved with the project. Chords can play as blocks with arp disabled,
or supply a pool of notes to the Pattern engine.

### Eight-graph arpeggiator

Select ARPEGGIATOR, choose Pattern, enable GRAPHS and the rack enable square.
The selected graph occupies one drawing area with 32 steps. The tabs are:

| Graph | Meaning |
| --- | --- |
| NOTE | Rest, held-note ranks 1–8, Up, Down, Up/Down, Down/Up, UpAndDown, DownAndUp, FingeredBottom or FingeredTop. |
| RAND SEL | Random positive or negative rank offset, wrapping in the held-note pool. |
| OCTAVE | -8 to +8 octaves. |
| PITCH | -24 to +24 semitones, with seven pitch-degree enable buttons below. |
| VEL SCALE | 0–200% of input velocity; zero is a rest. |
| GATE | 0–100% of the base step interval; zero is a rest. |
| LATE | -49% early to +49% late, using half-step look-ahead. |
| HARMONY | Adds a second note -12 to +12 semitones from the main note; zero disables it. |

Up/Down and Down/Up omit repeated extremes; UpAndDown and DownAndUp repeat them.
Fingered modes alternate the bottom or top note with the other held notes.
Pitch-degree enables use a detected chord root, including common inversions.
Root detection uses triad/seventh templates; ambiguous voicings may select a
different root from another instrument.

Drag to draw; right-click or double-click for an exact value; wheel adjusts one
unit. NOTE accepts ranks 1–8 or UP, DN, UD, DU, U+D, D+U, FLO, FHI and REST.
The row below NOTE toggles mute without erasing values. Alt-click a NOTE step
adds a motion-reset marker. Drag the top bar or use LEN to change length.
LINK shares lengths; with LINK off each graph cycles independently. DIV slows
the selected graph by 1–16 base steps. COPY / PASTE transfer a normalized shape,
length and divider; RAND randomizes the selected graph. NOTE copying includes
mute/reset flags. The pitch-class buttons cycle Pass, Block (x), Up (>) and
Down (<); redirected notes move one semitone. These constraints are global.

A–L are twelve named pattern memories. Switching slots automatically retains
current edits. Click the name to rename; COPY TO duplicates the complete current
pattern into another slot. RATE ranges from 1/512 note to 16 bars when SYNC is on,
or 0.01–512 Hz otherwise. RETR resets motion with each incoming note. With RETR
off and valid host PPQ, the clock aligns to transport. CHORD rotates the held
voicing so the selected arp note is lowest, then plays the whole voicing.
LATCH holds the released chord until a new chord arrives. Stopping the host or
disabling the arp clears generated notes. Only one pattern plays at a time.

MIDI OUT sends generated notes and supported pitch/controller/pressure messages
to the host. MIDI ONLY also silences synth output; turning it on enables MIDI OUT.
Route output in the DAW or choose an output device in Standalone settings.
Turning MIDI OUT off sends all-notes-off. MIDI routing depends on the host.
Large SysEx messages are not forwarded. Early/late timing is sample scheduled;
an early first note is clamped to the incoming note time, never before it.

### Previous Pattern engine

Turn GRAPHS off to use the 0.4 Pattern grid, with note rank, octave, velocity,
gate, repeat, chance and tie rows, independent lane lengths and two 16-step pages.
Draw or right-click for values. Existing 0.4 projects select this engine
automatically. Its pattern selector also uses the A–L memory bank. Legacy remains
a separate mode with its original controls.

### Legacy mode and compatibility

Legacy retains the ten original modes, velocity modes, 16 steps, transpose,
wrap, gate, swing, octaves and hold. The Step sequence factory sound uses it.
Older logisth states select Legacy automatically when they have no Pattern
engine parameter. ARP enable clears held notes; play fresh notes afterward.
Pattern uses note-rank/octave/gate lanes instead of Legacy transpose/wrap and
its shared gate. Legacy controls remain available in the Legacy view.

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

The mixer scope has A, B and L+R source buttons. A/B show each part's stereo
sum after its filter, amplitude envelope and Mix level, before shared effects
and Solo, scaled by MAIN. L+R shows the actual final left-plus-right output,
after effects, MAIN and MIDI ONLY. It is a sum, not an average. The scope uses
zero-crossing alignment and automatic visual scaling; the peak label gives the
unscaled peak. Selecting a source changes only the view and is saved in state.

### Output calibration (0.5.2)

One centred Classic sine, one unison voice, MIDI note 69 at velocity 127,
source Volume / Mix / Main at 100%, amplitude sustain at 100%, bypassed filters
and effects, and no modulation produces **0 dBFS peak on each output channel**.
The unweighted RMS is -3.0103 dBFS (a full-scale sine has RMS 1/sqrt(2)).
This is a unity reference, rather than a normal listening preset.

A fixed output calibration follows the shared FX, preserving their input level
and drive/compression behaviour. At the same controls the change is +19.49 dB
from 0.5.1. Factory presets have individually adjusted Main settings; the initial
Main default is now 42%. Existing user presets keep their saved fader values and
will be louder. Reduce Main for patches that overload the output.

The test reference for factory presets is a six-second C3/G3/C4/E4 chord (MIDI
48/55/60/64), velocity 102, including the attack. At 48 kHz most factory presets
peak near -6 dBFS; Grain cloud and Chord pulse remain a little quieter. All ten
are tested at 44.1, 48 and 96 kHz with at least 3 dB of headroom for that chord.
This reference does not guarantee headroom for every possible note or edit.

The mixer prints the peak of the latest processed block in dBFS. Values above
0 dBFS turn red. Constant-power panning is retained: hard panning raises the
active channel by 3.01 dB relative to centre. Two identical oscillators add
6.02 dB; eight coherent, undetuned unison voices add 9.03 dB. Keep headroom with
Mix/Main when stacking voices. There is no automatic note-count normalization
or hidden output limiter. The scope's L+R mode is a literal sum and reads +6.02
dB relative to either channel for a centred mono sine.

The release calibration ZIP includes a full-scale 440 Hz float WAV, a rendered
logisth WAV, a .logisth calibration preset and before/after CSV readings. Load
the preset, play MIDI 69 at full velocity, leave the DAW track fader at unity,
and bypass DAW effects to repeat the reference. Some RMS meters add an AES17
3 dB reference offset; these may display 0 dB RMS for this same signal.

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

The ten factory sounds are Init / analog, Wide supersaw, Sub bass, Slow atmosphere,
Pulse pluck, Step sequence, Morph glass, Sample keys, Grain cloud and Chord pulse.
The sound selector lists factory sounds and user presets from the current folder.
Its arrows step through that list. Loading replaces the current patch.

The PATCH menu provides Save as, Save current, Load file, Choose folder,
Refresh library and Delete current. Save as creates a named **.logisth** file;
Save current overwrites the active user preset. Delete asks for confirmation
and moves the file to the OS trash. The default library is logisth/Presets under
the OS application-data directory. Choose folder selects an instance's working
library; it does not relocate files. Load file can open a preset from elsewhere.
Save/load run on a worker, with completion/errors in the footer. Wait for the
completion message before closing the instance or moving a newly saved file.

Presets contain all 936 audio parameters, routing, compressed audio for all four
slots, WAV regions, chord mappings, twelve named patterns, theme and scope
selection. The original sample files are unnecessary on recall. Saving uses a
temporary file and atomic replacement. Invalid files leave the current patch
intact. State/preset files are limited to 64 MiB. These are logisth's own files;
renaming another format to .logisth does not convert it.

All 936 parameters are available for host automation. The first 514 parameter
IDs and order from 0.4 are retained, with 422 IDs appended. A/B views, rack pages,
theme and scope source do not change the patch's audio settings. DAW state
contains the same embedded material and pattern data as a user preset. Restoring
older states automatically selects the corresponding earlier granular/arp engines.
The 0.4 destination expansion still affects normalized destination automation
from earlier releases; inspect such automation when importing an older project.

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

High pitches in Sampler/Granular use interpolated resampling and may alias.
Wavetable mip bands reduce aliasing, but phase warp can introduce new harmonics.
These engines are independent implementations, not models of commercial synths.
Dense granular patches with high unison/polyphony require more CPU. Imported
material can be loud; use moderate levels. An import failure leaves the previous
material intact. A source starts again on the next note or engine/material change.

Version 0.5 adds the documented Granulizer workflow and eight-graph arp.
Native release archives are published only after their platform builds and tests pass.
See BUILD_INFO.json in each archive for source revision and validation results.


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
