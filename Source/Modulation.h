#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace logisth {
constexpr double pi = 3.14159265358979323846;
inline float clamp(float x, float lo, float hi) {
    return std::clamp(x, lo, hi);
}
enum class Destination {
    None,
    VolumeA,
    VolumeB,
    VolumeAB,
    PitchA,
    PitchB,
    PitchAB,
    PhaseA,
    PhaseB,
    PhaseAB,
    PanA,
    PanB,
    PanAB,
    CutoffA,
    CutoffB,
    CutoffAB,
    ResonanceA,
    ResonanceB,
    ResonanceAB,
    PhaserFrequency,
    MixA,
    MixB,
    MixAB,
    Lfo1Rate,
    Lfo1Gain,
    Lfo2Rate,
    Lfo2Gain,
    DistortionAmount,
    Count
};
enum class Source {
    None,
    Velocity,
    Keytrack,
    Aftertouch,
    ModWheel,
    PitchWheel,
    AmpEnvA,
    AmpEnvB,
    ModEnv1,
    ModEnv2,
    Lfo1,
    Lfo2,
    StepVelocity,
    Random,
    RandomBipolar,
    Count
};
struct Route {
    Destination destination = Destination::None;
    float amount = 0;
};
using Routes = std::array<Route, 2>;
struct Misc {
    Source source = Source::None;
    Routes routes{};
};
struct ModValues {
    std::array<float, size_t(Destination::Count)> values{};
    float get(Destination d) const {
        return values[size_t(d)];
    }
    void add(const Routes &routes, float source) {
        for (const auto &r : routes)
            if (r.destination != Destination::None)
                values[size_t(r.destination)] += source * r.amount * .1f;
    }
    float pair(Destination first, int part) const {
        return get(Destination(int(first) + part)) + get(Destination(int(first) + 2));
    }
};
struct EnvelopeParams {
    float attack = .008f, decay = .25f, sustain = .7f, release = .35f;
};
struct LfoParams {
    int wave = 0, division = 7;
    float rate = 1, gain = 1, offset = 0;
    bool free = false;
    Routes routes{};
};
// Beat lengths: straight, triplet and dotted values, indexed by the UI menu.
inline constexpr std::array<double, 16> beatLengths{.0625,
                                                    .0833333333333,
                                                    .125,
                                                    .166666666667,
                                                    .25,
                                                    .375,
                                                    .333333333333,
                                                    .5,
                                                    .75,
                                                    .666666666667,
                                                    1,
                                                    1.5,
                                                    2,
                                                    4,
                                                    8,
                                                    16};
inline double beatLength(int index) {
    return beatLengths[size_t(std::clamp(index, 0, 15))];
}
inline double lfoRate(const LfoParams &p, bool sync, double bpm) {
    return sync && !p.free ? bpm / (60 * beatLength(p.division)) : p.rate;
}
inline float lfoWave(double phase, int wave, float held, float next) {
    const float t = float(phase);
    switch (wave) {
    case 0:
        return float(std::sin(2 * pi * phase));
    case 1:
        return 1 - 4 * std::abs(t - .5f);
    case 2:
        return 2 * t - 1;
    case 3:
        return 1 - 2 * t;
    case 4:
        return t < .5f ? 1 : -1;
    case 5:
        return t < .25f ? 1 : -1;
    case 6:
        return held;
    case 7:
        return held + (next - held) * t * t * (3 - 2 * t);
    case 8:
        return .5f + .5f * float(std::sin(2 * pi * phase));
    default:
        return 1 - 2 * std::abs(t - .5f);
    }
}
} // namespace logisth
