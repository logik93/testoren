# logisth 0.3.0 — A new console, light and dark

logisth 0.3.0 introduces a minimal single-window interface with every synthesis
section organised around a central effects and arpeggiator rack.

- Switch between LIGHT and DARK; each instance recalls its own theme.
- Edit parts A/B, their oscillators, envelopes and filters in the same frame.
- Keep shared Filter Control, mixer and the full modulation section within reach.
- Resize the complete vector console from 80% to 150%.
- Use grouped routing menus, editable values with units and embedded fonts.
- Read the new English user manual, included as PDF and Markdown.

The audio engine and all 248 parameter IDs are retained from 0.2.0. Existing
logisth projects keep their audio settings. This is an independent instrument;
Sylenth1 presets and exact sound compatibility are not supported.

Native archives contain VST3, CLAP and Standalone. Additional downloads include
the English manual, source, light/dark screenshots, a WAV demo and SHA256SUMS.
VST2 still requires a separately licensed SDK and is not included.

Windows binaries are unsigned. macOS Universal requires macOS 11+, uses ad-hoc
signing and is not notarized. Consult BUILD_INFO.json for architecture, Linux
glibc requirements and validation results. macOS tests run on an Intel runner;
Apple Silicon code is included and architecture-checked, not hardware-auditioned.
