#pragma once
namespace logisth {
// Unity per output channel for one centred, full-scale mono source.
// This compensates the equal-power pan law at the engine boundary, before FX.
// No arbitrary headroom attenuation or reciprocal post-FX boost is applied.
inline constexpr float engineOutputCalibration = 1.4142135623730951f;
// Output-only trims: reference four-note chord, velocity .8, all FX as authored.
// Init is the raw saw starting patch and deliberately opens at the requested Main 65%.
// The dense distorted bass uses a lower peak trim; quieter presets stop at unity.
// No note-count-dependent gain control is applied.
inline constexpr float factoryMainLevels[10] = {.65f,  .628f, .2f,   .407f, .346f,
                                                .830f, .582f, .593f, 1.f,   1.f};
} // namespace logisth
