# logisth 0.5.1 — Responsive controls and clearer text

This maintenance release fixes missed clicks reported in 0.5.0.

- **Buttons and menus:** the GUI no longer hides and rebuilds its controls on
  every 24 Hz refresh. Layout and refresh now share the same state comparison,
  including all four granular model selectors and the graph length-link switch.
  Previously the refresh included granular-model state that the cached layout
  omitted; the default model values made the two disagree continuously. The
  shared comparison keeps mouse presses and menu interactions stable.
- **Retrig and Invert:** tested using mouse-down, timer refresh and mouse-up on
  both parts, in both themes. Audio tests check retrigger phase repeatability
  and inversion polarity in Classic and Wavetable modes.
- **Arpeggiator:** disabling Legacy or the previous Pattern engine also clears
  the stale playhead. Tests cover switching off all three engines with latch
  and sustain, cancellation of future steps, and normal MIDI bypass afterward.
- **Typography:** embedded font sizes increased by 18%, with an 11 px base
  minimum; popup rows increased to 28 px.

The button test reproduces the repeated visibility changes in 0.5.0 and passes
with the fix. Existing parameters, preset format and plug-in identity are retained.
All four source engines, waveform/grain display, A/B/L+R scope, eight arp graphs
and portable user presets remain available.

Downloads contain **VST3, CLAP and Standalone** for Windows x64, Linux x64 and
macOS Universal (Intel + Apple Silicon), plus the English PDF/Markdown manual,
source, screenshots, an audio demo and SHA256SUMS.

Publication requires all native builds and tests to pass, with none skipped.
Linux also loads the actual VST3 and CLAP binaries. macOS runtime tests run on
Intel; both binary architectures are verified. macOS requires 11+, is ad-hoc
signed and not notarized; Windows is unsigned. Linux requirements appear in
BUILD_INFO.json. VST2 is not included without its separately licensed SDK.
