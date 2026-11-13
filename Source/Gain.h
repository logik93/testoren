#pragma once
namespace logisth {
// Keep the existing operating level of the shared FX, then restore unity on output.
// One full-scale mono oscillator is attenuated by the equal-power centre pan law
// and this internal headroom factor. At unity source/mix/main, each final channel
// must reproduce a sine at 0 dBFS peak (-3.0103 dBFS unweighted RMS).
inline constexpr float engineHeadroom = .15f;
inline constexpr float outputCalibration = 1.4142135623730951f / engineHeadroom;
// Output-only trims: reference four-note chord, velocity .8, all FX as authored.
// Quiet presets stop at unity; there is no note-count-dependent gain control.
inline constexpr float factoryMainLevels[10] = {.42f,  .628f, .0586f, .407f, .461f,
                                                .830f, .582f, .677f,  1.f,   1.f};
} // namespace logisth
