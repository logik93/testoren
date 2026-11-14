# logisth 0.8.0 — Individual WT control, clean startup and persistent themes

WT position can now target each player separately, new instances open a clean saw
patch at Main 65%, and presets can carry editable notes. A compact Settings/About
panel adds five persistent colour themes without changing the audio patch.

- **Individual WT Pos / Start targets: A1, A2, B1 and B2.** A and B still control
  both players in their section; A+B controls all four. Both LFOs and both routing
  slots support every target. Individual and grouped offsets add before clamping.
  The same targets also control start/scanner position in Sampler and Granular.
- **Init / saw is selected automatically in new instances:** one classic saw,
  bypassed filters, no FX, arp or modulation, neutral tuning and Main **65%**.
  Presets and DAW projects restore their saved Main value. The other nine authored
  factory sounds retain their existing settings and output trims.
- **Editable preset comments** sit below the preset selector. Up to 4096 Unicode
  characters are embedded in .logisth files and DAW state. Comment edits notify
  the host that project state changed without generating audio automation. A
  delayed edit event from a previous preset cannot overwrite the new preset's note.
- **SETTINGS and About stay inside the central rack.** Themes are **Midnight Blue**
  (default black/blue), **Cobalt**, **Graphite**, **Forest** and **Light**. The chosen
  theme is saved as a user preference, recalled by new instances and retained when
  sounds change. Open instances share appearance changes; comment text retains
  contrast in both light and dark themes.
- **ADSR preview geometry now uses actual segment times.** Attack 0 ms draws a
  vertical rise; a longer Attack takes more horizontal time. Zero decay/release
  stages also draw vertical transitions. The artificial minimum-width diagonal
  segments are removed. The held portion illustrates 0.5 seconds.
- The English manual, interface guide, screenshots and calibration report are
  updated. Native archives include the theme images referenced by their README.

Regression checks cover all seven individual/grouped WT targets through both
LFOs and route slots, Init's saw harmonic spectrum, saved Main restoration,
Unicode comments and delayed edit events, metadata-only host notifications,
five themes persisted to disk and recalled in new/open instances, comment contrast
and zero-time ADSR geometry. Existing 100-cent detune, 148 button interactions,
preset/media validation, long-sample loading, scope, filters, FX and Legacy/Pattern
arp checks remain enabled. The calibrated sine still reaches 0 dBFS per channel
at unity controls. At Main 65%, the raw Init reference chord stays below full
scale; the nine authored sounds keep at least 3 dB of reference-chord headroom.

All 575 audio parameter IDs remain. Four new destination choices are appended;
existing numeric target IDs are retained. Normalized host destination automation
may need review after the expanded choice range. Appearance is now a user
preference rather than a preset colour; older preset theme properties are ignored.

Native packages contain **VST3, CLAP and Standalone** for Windows x64, Linux x64
and macOS Universal (Intel + Apple Silicon), the English manual, screenshots and
build metadata. Source, a demo, calibration audio/preset/report and SHA256SUMS
accompany the release. Publication requires all platform tests to pass with none
skipped. macOS 11+ binaries are ad-hoc signed and not notarized; Windows is unsigned.
macOS runtime tests run on Intel, with both binary architectures checked. Linux
requirements are in BUILD_INFO.json. VST2 is not included without its separately
licensed SDK.
