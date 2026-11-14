# wavelogik logisth 0.9.1 — Live oscillator phase control

PHASE now works independently of RETRIG in all four Classic/Wavetable players,
including held notes and host automation.

- **PHASE stays enabled with RETRIG off.** Both initial layout and periodic GUI
  refresh previously disabled the knob unless RETRIG was on.
- **The audio engine applies PHASE as a live offset.** Previously it only read
  the parameter when starting a retriggered note. Knob changes now reach held
  voices in A1, A2, B1 and B2, in both periodic engine modes.
- **RETRIG controls note starts.** On gives repeatable starts at the selected
  phase; off retains randomized base phases with the chosen offset added.
  Toggling RETRIG does not restart a held oscillator. The offset is applied once
  and remains additive with existing phase modulation.
- **Smooth cyclic changes.** Live knob changes use a short smoothing time constant
  of approximately 5 ms, updated once per oscillator rather than per unison voice.
  Zero and 360 degrees are equivalent. Crossing 359 to 1 degrees takes the short
  two-degree path. Offsets continue updating while the oscillator is muted.
- The English manual, interface previews, demo and calibration materials are
  refreshed. All 581 parameter IDs, wavelogik plug-in identities and MIT/third-party
  notices are retained from 0.9.0.

Regression tests cover 384 onset fixtures and 96 live-control configurations:
all four slots, seven periodic classic waveforms and Wavetable, RETRIG off/on,
one/eight unison voices, 44.1/48/96 kHz, phase/modulation addition, cyclic wrapping,
repeatable/random starts and buffer partitioning. Full-processor tests automate
PHASE on held notes in 16 slot/mode/retrigger combinations and check every knob's
binding/enabled state after GUI refresh. Noise is nonperiodic; Sampler/Granular
continue using their sample-position controls.

Patches saved with RETRIG off and a nonzero PHASE value now apply that stored
phase offset. The unity sine calibration and existing anti-click, reverb, theme,
scaling, MIDI, media, preset/state and button regressions remain enabled.
