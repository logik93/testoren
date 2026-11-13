# logisth 0.6.0 — Calibrated gain staging and direct sample loading

- **Unity reference:** a centred full-scale sine reaches approximately **0.00
  dBFS peak per channel**, before the shared FX and at unity Main, with -3.01 dBFS
  unweighted RMS. The old bypassed chain measured -19.49 dBFS peak.
- The arbitrary engine attenuator and duplicate Master startup ramp are removed.
  Main is applied once after FX, and smoothing starts at its chosen value. Instant
  attacks retain their first sample. Compressor thresholds and distortion now
  receive a properly calibrated source bus; factory Main trims are retuned.
- **Drag and drop onto the large SAMPLE waveform or its header** loads the slot
  selected above it (A1/A2/B1/B2). Successful Classic imports switch to Sampler.
  Invalid files preserve the previous source and mode. A drag highlight shows
  the destination and decoding runs on the background worker.
- **No fixed sample-duration limit.** Four imported sources share a 512 MiB
  decoded-stereo-PCM budget. Serialized presets/DAW states allow up to 1 GiB.
  Waveform peaks are prepared in the background for responsive long-file zoom/pan.
- Numeric output dBFS and red overload indication are added to the mixer.

`logisth-0.6.0-calibration.zip` contains an independent full-scale 440 Hz WAV,
the rendered plug-in sine, a calibration preset and before/after CSV/report data.
Play MIDI 69 at velocity 127, with a unity DAW fader and bypassed track effects.
Regression tests cover three sample rates, gain/pan/unison, all source engines,
compressor reduction, instantaneous attacks, factory headroom, selected-slot
file drops and a 60-second embedded sample reloaded without its original file.

**Levels and FX drive are recalibrated together; old patches may sound different.**
The output retains float headroom and has no hidden limiter or note-count gain
normalization. Lower Main for layered patches. The calibration preset deliberately
targets full scale. The memory budget covers active imported PCM; decoding,
waveform/wavetable preparation and state serialization require additional memory.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual and build metadata.
Source, previews, a demo and SHA256SUMS are included. Publication requires all
platform tests to pass with none skipped. macOS 11+ binaries are ad-hoc signed
and not notarized; Windows is unsigned. macOS runtime tests run on Intel, with
both architectures checked. Linux requirements are in BUILD_INFO.json. VST2 is
not included without its separately licensed SDK.
