# wavelogik logisth 0.9.2 — Source modulation, sampler stretching and two EQs

## Equalizer

Legacy now applies calibrated bass/treble shelves without hidden full-band
attenuation when a shelf is boosted. Graphic adds ten ISO bands from 31.5 Hz
to 16 kHz, each with ±18 dB gain, faders and draggable response points. Double-click
a point to reset its band. Both modes have explicit Auto Gain (off by default)
and ±18 dB output trim; the displayed curve includes these gains. Changes and
mode transitions are smoothed. Auto Gain compensates estimated response peaks;
it is not a limiter or loudness normalization.

## Oscillators and sampler

- MOD ENV 1/2 A/D/S/R and both routing slots start at zero in fresh instances.
- New Classic waves: Pink noise, Brown noise, Organ and Folded sine. Existing
  Noise is now labelled White noise; all twelve wave choices remain available.
- Each Classic/Wavetable player adds FM index/ratio (sinusoidal phase modulation).
  Every engine adds AM depth/ratio. Pulse variants add base width, PWM depth/rate.
- FM Index, AM Depth and Pulse Width modulation destinations independently address
  A1/A2/B1/B2, part A/B, or A+B, including both LFOs and envelope/Misc routing.
- Sampler PLAYBACK adds Off, Tape, Granular and WSOLA stretching with 0.125–8×
  duration factors. Tape links time/pitch; the overlapping-window modes preserve
  played-note pitch independently of traversal speed. WSOLA aligns adjacent
  windows by waveform correlation. Extreme stretching/transients can retain
  time-domain window artefacts.
- Linear, Cubic, Sinc 16 and Sinc 32 resampling; sinc modes suppress downsampling
  aliases. Algorithm/quality switches crossfade. Controls appear only in the
  applicable source pages; PWM is specific to pulse waves.

## Themes

- SAVE AS... chooses the exact JSON file and folder; USE FOLDER changes the
  scanned directory. RENAME saves/renames a custom theme; standard files are protected.
- SETTINGS / STYLE selects font family, Flat/Gradient/Grid background and
  Arc/Solid/Minimal knobs. Dedicated knob/track colours extend the palette to 12.
- All settings are saved in version-2 JSON themes; version-1 themes remain readable.
  All five packaged standard templates use version 2. Fonts other than embedded
  Open Sans must be installed on the destination computer.
- Unsaved theme drafts survive minimizing and editor destruction/recreation within
  the same instrument instance. Save a theme for persistence across host sessions.
- Automatic directory polling is every two minutes; RESCAN remains immediate.

## Reverb and validation

Hall is rebuilt, and all five algorithms use denser 16-line feedback networks,
with eight input diffusers per channel (four for Spring), adjustable Diffusion,
fuller low-frequency tails and four-window Shimmer pitch shifting. Existing
Room, Spring, Vintage and Shimmer character controls remain available.

635 automatable audio parameters; 59 destination entries. New IDs are appended.
Legacy EQ boosts and rebuilt reverbs change sound; sound design takes priority
over matching previous wet patches. Check Main/headroom when revisiting presets.

Regression checks cover FM/AM sidebands, pulse-width harmonics, isolated routes,
coloured-noise spectra, stretch duration/pitch, sinc alias rejection, EQ response,
real processor/GUI bindings and graph gestures, preset/host-state round-trips,
theme save paths/rename/font/styles/drafts, two-minute scanning, and five reverb
impulse responses at 44.1/48/96 kHz. Unity-sine calibration, anti-click, phase,
MIDI/arp, media, controls and layout regressions remain enabled. Native builds
run 10 suites on Linux and 8 on Windows/macOS.

VST3, CLAP and Standalone: Windows x64, Linux x64 and macOS Universal
(Intel + Apple Silicon). Packages include the updated English manual, theme
files and MIT/third-party notices. VST2 requires a separately licensed SDK and
is not included. macOS bundles are ad-hoc signed; Windows binaries are unsigned.
