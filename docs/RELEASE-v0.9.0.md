# wavelogik logisth 0.9.0 — Appearance tools and reverb models

- **Remembered editor size and editable 80–200% scaling.** Settings shows the
  last accepted percentage, including decimals. New/reopened windows restore
  their exact dimensions. Closing an unchanged older window retains the latest
  size chosen in another instance. Audio presets keep appearance preferences.
- **Theme files and an integrated Theme Editor.** The five factory palettes are
  written to the user's logisth/themes folder as JSON. Edit ten colour roles with
  the picker or RGB hex values, preview changes, and save custom copies. Duplicate
  names receive a suffix. The folder scans every two seconds; Open Folder and
  Rescan are available. Invalid files are ignored, file identities retain the
  selection, and unrelated folder changes preserve an unsaved draft.
- **Hall, Room, Spring, Vintage and Shimmer.** The original Hall remains, with
  four new stereo algorithms: Room early reflections/feedback diffusion, Spring
  all-pass dispersion, Vintage filtered/saturated modulated feedback, and Shimmer
  granular pitch-shifted excitation and feedback. Relevant Decay, Dispersion,
  Wow/Flutter, Shimmer amount and -12 to +24 semitone Pitch controls appear per
  type. Algorithm changes crossfade over approximately 100 ms. All model settings
  are saved in presets/DAW state; Main calibration remains unchanged.
- **Manufacturer wavelogik; instrument logisth.** Original code and documentation
  are licensed under MIT, Copyright (c) 2026 wavelogik. Third-party libraries,
  SDKs and fonts retain their upstream licenses and notices.

The manufacturer code is now Wvlg and Bundle/CLAP ID is com.wavelogik.logisth.
Hosts see a different instrument identity from earlier builds; existing host
projects may need the new instrument selected explicitly. Existing .logisth sound
files remain readable. The original 575 audio parameter IDs remain; six reverb
parameters are appended, for 581 total.

Regression checks cover persisted fractional scaling, multiple editors, all five
standard JSON files, custom colours, automatic scanning, unsaved drafts, invalid
files, stable selections and layout. Reverb tests measure distinct stereo impulse
responses, decay, shifted pitch bands, parameter extremes, rapid type changes,
reset silence, host-buffer invariance, preset persistence and conditional UI.
Existing gain, anti-click, MIDI, synthesis, arp, source/media, state, themes and
148-button checks remain enabled. Linux also loads actual VST3/CLAP binaries and
checks the manufacturer/product metadata. Platform results are attached when a
release is built and published.
