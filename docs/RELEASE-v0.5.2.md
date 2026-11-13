# logisth 0.5.2 — Calibrated output level

- **Unity sine reference:** one centred full-scale sine now reaches approximately
  **0.00 dBFS peak per channel** (-3.01 dBFS unweighted RMS), previously -19.49 dBFS.
- Output correction is applied **after shared FX**, retaining their operating
  level and distortion/compression behaviour.
- Factory Main trims are balanced against a six-second four-note reference
  chord, mostly around -6 dBFS peak. The initial Main default is 42%.
- The mixer adds a numeric dBFS readout and red overload indication.
- Regression tests cover three sample rates, source/Mix/Main gain, velocity,
  stereo pan, oscillator/unison summing and unity media in Wavetable, Sampler
  and Granulizer, plus all ten factory sounds.

**Existing saved patches retain their fader values and are approximately
19.49 dB louder at identical settings. Lower Main for hot or layered patches.**
The output retains float headroom and has no hidden limiter or note-count
normalization. The calibration preset deliberately targets full scale.

`logisth-0.5.2-calibration.zip` contains a generated 0 dBFS reference sine, the
rendered plug-in sine, a calibration preset, before/after CSVs and the English
measurement report. Play MIDI 69 at velocity 127 with a unity DAW track fader and
bypassed track FX to reproduce the measurement.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the updated English manual and build
metadata. Source, previews, a demo and SHA256SUMS are included. All platform
builds and tests must pass before publication. macOS requires 11+, is ad-hoc
signed and not notarized; Windows is unsigned. macOS runtime tests run on Intel,
with both architectures checked. Linux requirements are in BUILD_INFO.json.
VST2 remains unavailable without its separately licensed SDK.
