# logisth 0.7.1 — 100-cent detune and clear LFO wavetable routing

Unison detune now reaches 100 cents in every oscillator, and the modulation menus
explicitly identify the existing wavetable-position routing.

- **Detune extends from 0–50 to 0–100 cents** in A1, A2, B1 and B2. The outer
  unison voices reach one semitone below and above the played pitch at 100 ct.
  A single unison voice remains at the played pitch. Values display the **ct**
  unit, and settings up to 100 ct survive preset/project state restoration.
- **LFO 1 and LFO 2 list WT Pos / Start A, B and A+B**, in the new
  **Source position / grains** menu section. A controls A1/A2, B controls B1/B2,
  and A+B controls all four. Both destination/amount pairs of each LFO work in
  Hz, tempo-sync and Free modes, with positive or negative modulation amounts.
- The central wavetable window follows the playing frame after modulation, while
  the WT POS knob retains its base value. For a starting patch use a multi-frame
  wavetable, WT POS 50%, LFO Gain 100%, and a WT Pos / Start route at Amount +5.
  The same destination controls sample start/scanning in Sampler and Granular.
- The **English PDF/Markdown manual** includes the LFO setup and expanded detune
  range. Existing destination IDs and the 575-parameter layout are retained.

Regression checks verify both LFOs, both route slots, every oscillator and A/B/A+B
isolation in Hz/sync/free modes, including reversed modulation and audio-block
invariance. GUI-driven tests measure actual audio components at plus/minus one
semitone when detune is set to 100 ct, then save and reload all four detune values.
They also select the LFO routes through the real parameter widgets and verify
that the playing wavetable frame moves. Existing calibration, presets, button,
filter, FX, sample-loading and Legacy/Pattern arp checks remain enabled.

The wider detune range changes normalized host automation: an old normalized
value of 0.5 now means 50 ct rather than 25 ct. Check existing detune automation
when reopening earlier sessions. Preset/project values stored in cents retain
those numeric values.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual and build metadata.
Source, screenshots, a demo, calibration audio/preset/report and SHA256SUMS are
included. Publication requires all platform tests to pass with none skipped.
macOS 11+ binaries are ad-hoc signed and not notarized; Windows is unsigned.
macOS runtime tests run on Intel, with both binary architectures checked. Linux
requirements are in BUILD_INFO.json. VST2 is not included without its separately
licensed SDK.
