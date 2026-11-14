# logisth console

The complete console occupies one 1240 × 860 canvas, with no vertical scrolling.
Its initial size adapts to the monitor. Drag the lower-right corner to resize
proportionally from 80% to 150%.

SETTINGS opens in the central rack and offers Light, Midnight Blue, Cobalt,
Graphite and Forest, plus About. Menus, editors, keyboard and controls follow the
palette. The selected appearance persists as a shared user preference; changing
sounds keeps it and audio settings remain untouched. Midnight Blue is the default.
Vector drawing and embedded Open Sans keep the design consistent across systems.

- Top: factory/user presets and PATCH file menu, part A/B, Solo, Poly, Sync and Settings; below, two oscillators
  and the selected part's amplitude envelope.
- Left: the selected part's filter and shared Filter Control A+B.
- Centre: the arp and seven effects, selected by name, each with a separate enable square.
- Right: A/B/L+R oscilloscope, Mix A/B, Main and stereo meters.
- Bottom: two Mod Env, two LFO, four Misc sources, wheels, keyboard and glide.

Each modulation block has two destinations and amounts. Destination menus use
three grouped columns. Hover for tooltips and the bottom value readout.
Numeric values support units such as kHz, ms and percent. Sync displays musical
divisions instead of free time/rate controls. Arp steps use 01–08 and 09–16 views.

The current engine exposes 575 audio parameters. Tests check access, bounds, click
targets, view changes, bindings, scaling and independent theme persistence.
See [the English user manual](MANUAL.md) for full operation.

The oscillator header selects engine and only its relevant pages: VOICE/ENV for
Classic, WAVETABLE for the table engine, SOURCE for Sampler, and GRAINS for
Granular. MOTION/TIME are available only in the Granulizer model. File drops and
IMPORT load audio into that slot. CHORDS is the ninth rack page. Pattern arp
uses a seven-row drawing canvas with independent lane lengths, exact cell entry,
playheads and two 16-step views; Legacy retains the original arp page. MIDI OUT
is in the top header. Both themes use the same layout.

SAMPLE is the tenth rack page. For Sampler and Granular it shows the actual PCM
waveform, live cursor and grain markers. For Wavetable it shows a perspective
stack, highlights the selected cycle and displays its frame number. WT POS,
dragging the view or wheeling selects a frame. The header's AUTO/length menu
controls frame slicing on import. Graphs arp has been removed; Legacy and the
seven-row Pattern grid remain. The mixer scope is always visible.

Version 0.5.1 increases embedded font sizes by 18%, with an 11 px base minimum
and 28 px popup rows. The console uses one shared layout-state comparison for
refresh and layout; unchanged audio/UI state does not hide and rebuild controls.
Regression tests exercise 100 toggle interaction cases across timer ticks in both parts
and both themes, checking both directions of each switch. The cached layout
previously omitted granular model state used by the refresh comparison, causing
a permanent mismatch with default patches.

Version 0.6 adds file dropping onto the SAMPLE waveform and header, with a
highlighted selected-slot destination. File decoding and peak preparation are
performed on the import worker. The mixer includes a dBFS readout and red
overload indication. The fixed 12-second sample limit is removed; active
imported PCM uses a 512 MiB aggregate memory budget.

The comment box below the preset selector edits up to 4096 characters of preset
metadata and is saved with both .logisth presets and DAW state. New instances
select Init / saw with Main 65%; restoring an existing sound keeps its saved level.
WT Pos / Start targets include individual A1/A2/B1/B2 plus A, B and A+B groups.
The ADSR curve uses actual attack/decay/release durations and a 0.5-second held
segment; zero-length stages are vertical rather than forced to diagonal lines.
