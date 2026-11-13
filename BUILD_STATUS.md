# logisth 0.4.0 — Native build and release status

[Download the release](https://github.com/logik93/testoren/releases/tag/v0.4.0).
All three native builds passed and their archives were uploaded and verified.

| Platform | Tests passed | Native formats | Requirements / signing |
| --- | --- | --- | --- |
| Linux x86_64 | 7/7 | VST3, CLAP, Standalone | glibc 2.35+, unsigned |
| Windows x86_64 | 5/5 | VST3, CLAP, Standalone | Static MSVC runtime, unsigned |
| macOS Universal | 5/5 | VST3, CLAP, Standalone | macOS 11+, ad-hoc signed, not notarized |

Zero failed or skipped tests. Every macOS executable contains both arm64 and
x86_64 code; runtime tests ran on Intel, not Apple Silicon hardware.

Release source: `d4ac6e7d19ed5a105dc45b8ce894dae67b346dde`, tag `v0.4.0`.
[Native CI run](https://github.com/logik93/testoren/actions/runs/43335179365).
The publication receipt is on branch `ci-release-v0.4.0` in `status.json`.
Archive checksums and binary/source identities are recorded in SHA256SUMS and
build-info-*.json. All 13 release assets were verified after upload.

Tests cover the four engines in all four source slots at 44.1/48/96 kHz,
modulation, filters, arp scheduling, independent lanes, ratchets, probability,
latch, chord overlap, embedded audio, concurrent source replacement, old state
migration, malformed state rejection, MIDI output, editor bindings, drawing,
inline entry and both themes. Linux additionally loads real CLAP/VST3 binaries.
Standalone engine ASan/UBSan tests also passed locally before publication.

The release includes the English PDF/Markdown manual, source, screenshots and
an eight-second four-engine demo. Review old normalised destination automation
after expanding the destination choices. DAW-specific routing and compatibility
need verification in the target host. VST2 requires a separately licensed SDK
and is not included. No sample-exact Sylenth1 or Cthulhu compatibility is claimed.
