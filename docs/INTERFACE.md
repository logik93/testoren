# logisth console

The complete console occupies one 1240 × 860 canvas, with no vertical scrolling.
Its initial size adapts to the monitor. Drag the lower-right corner to resize
proportionally from 80% to 150%.

THEME selects LIGHT or DARK. Menus, value editors, keyboard and controls follow
the palette. The setting is saved per instance without changing audio parameters.
Vector drawing and embedded Open Sans keep the design consistent across systems.

- Top: factory/user presets and PATCH file menu, part A/B, Solo, Poly, Sync and Theme; below, two oscillators
  and the selected part's amplitude envelope.
- Left: the selected part's filter and shared Filter Control A+B.
- Centre: the arp and seven effects, selected by name, each with a separate enable square.
- Right: A/B/L+R oscilloscope, Mix A/B, Main and stereo meters.
- Bottom: two Mod Env, two LFO, four Misc sources, wheels, keyboard and glide.

Each modulation block has two destinations and amounts. Destination menus use
three grouped columns. Hover for tooltips and the bottom value readout.
Numeric values support units such as kHz, ms and percent. Sync displays musical
divisions instead of free time/rate controls. Arp steps use 01–08 and 09–16 views.

The original 514 audio parameter IDs remain, with 422 new IDs appended. Tests check access, bounds, click
targets, view changes, bindings, scaling and independent theme persistence.
See [the English user manual](MANUAL.md) for full operation.

The oscillator header selects engine and VOICE/SOURCE/ENV/GRAINS/MOTION/TIME views. File drops and
IMPORT load audio into that slot. CHORDS is the ninth rack page. Pattern arp
uses a seven-row drawing canvas with independent lane lengths, exact cell entry,
playheads and two 16-step views; Legacy retains the original arp page. MIDI OUT
is in the top header. Both themes use the same layout.

SAMPLE is the tenth rack page with source selection, actual waveform, live cursor
and grain markers. Pattern + GRAPHS shows one of eight 32-step graphs, independent
length/divider, mute/reset, pitch masks and A-L named memories in the same rack.
GRAPHS off retains the earlier Pattern grid. The mixer scope is always visible.
