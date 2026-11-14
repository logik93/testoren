# logisth 0.8.1 — Zero-time ADSR click fix

This maintenance release fixes clicks caused by zero-time amplitude envelopes
without reducing the calibrated steady-state output level.

- **All-zero ADSR now produces silence.** Zero Attack previously emitted one
  sample at full envelope gain before zero Decay reached Sustain. At Sustain
  zero, this created a single-sample impulse. Zero-time stages now consume no
  audio samples and enter the requested Sustain directly.
- **Smooth note edges, including Release 0.** Part A/B and enabled per-oscillator
  amplitude envelopes use an approximately 1 ms anti-click smoothing window.
  A new note starts at silence; note-off follows the current carrier before a
  short fade to exact silence. The window also smooths nonzero amplitude-envelope
  transitions and adds a bounded tail of approximately 1 ms. It has unity settled
  gain and no overshoot. Modulation envelopes keep unsmoothed control values.
- **Gain calibration is retained.** A full-scale sine at unity source/Mix/Main,
  full MIDI velocity, one centred unison voice, bypassed filters and effects
  still reaches 0 dBFS peak and -3.0103 dBFS RMS per channel after settling.
- The English manual, PDF, calibration measurements, demo and anti-click
  regression notes are updated. Native archives include the anti-click report.

New regression checks cover 1,152 silent-output fixtures across all four slots,
eight classic waves, wavetable, sampler and both granular models; fixed/random
start phases; part and oscillator ADSR; and 44.1/48/96 kHz. Another 192 tests
compare released notes with identical continuously held carriers. Full-processor
tests place MIDI events inside host buffers and compare results at buffer sizes
1/31/257/512. Short-note tails, voice retirement, steady gain, existing presets,
modulation, arp, UI, state and actual Linux plug-in binaries remain tested.

All 575 audio parameter IDs and plug-in identities remain unchanged. Patches
with hard amplitude gates now have the short anti-click edge described above.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual, screenshots and
build metadata. Source, a demo, calibration audio/preset/report and SHA256SUMS
accompany the release. Publication requires all platform tests to pass without
skips. macOS 11+ binaries are ad-hoc signed and not notarized; Windows is unsigned.
macOS runtime tests run on Intel, with both binary architectures checked. Linux
requirements are in BUILD_INFO.json. VST2 is not included without its separately
licensed SDK.
