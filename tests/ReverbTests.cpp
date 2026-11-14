#include "Reverb.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
using Stereo = std::array<std::vector<float>, 2>;
Stereo render(int type, double rate, float decay = 2, float amount = .5f, float pitch = 12,
              float damp = .3f, int block = 257, bool tone = false, float dispersion = .6f,
              float wow = .35f) {
    CharacterReverb model;
    model.prepare(rate, type);
    ReverbControls p;
    p.reverbDecay = decay;
    p.shimmerAmount = amount;
    p.shimmerPitch = pitch;
    p.springDispersion = dispersion;
    p.vintageMod = wow;
    int length = int(rate * (tone ? 4 : 3));
    Stereo result{std::vector<float>(size_t(length)), std::vector<float>(size_t(length))};
    for (int offset = 0; offset < length; offset += block) {
        model.configure(p, .65f, damp);
        for (int i = offset; i < std::min(offset + block, length); ++i) {
            float in = tone ? .2f * float(std::sin(2 * pi * 440 * i / rate)) : (i == 0 ? 1.f : 0.f);
            auto out = model.tick(in, in * .7f);
            for (int ch = 0; ch < 2; ++ch) {
                check(std::isfinite(out[size_t(ch)]) && std::abs(out[size_t(ch)]) < 8,
                      "reverb output remains finite and bounded");
                result[size_t(ch)][size_t(i)] = out[size_t(ch)];
            }
        }
    }
    return result;
}
double energy(const Stereo &v, int begin, int end) {
    double e = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = begin; i < end; ++i)
            e += double(v[size_t(ch)][size_t(i)]) * v[size_t(ch)][size_t(i)];
    return e;
}
double difference(const Stereo &a, const Stereo &b) {
    double d = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (size_t i = 0; i < a[size_t(ch)].size(); ++i)
            d += std::abs(a[size_t(ch)][i] - b[size_t(ch)][i]);
    return d;
}
double band(const Stereo &v, double rate, double hz) {
    // Integrate the shifted band, allowing the short-window pitch shifter's
    // intentionally diffuse sidebands rather than assuming an ideal oscillator.
    double power = 0;
    int begin = int(rate * 2), length = int(rate * 2);
    for (double f = hz - 32; f <= hz + 32; f += 2) {
        double coefficient = 2 * std::cos(2 * pi * f / rate), a = 0, b = 0;
        for (int i = begin; i < begin + length; ++i) {
            double sample = v[0][size_t(i)] + coefficient * a - b;
            b = a;
            a = sample;
        }
        power += std::max(0., a * a + b * b - coefficient * a * b) / (double(length) * length);
    }
    return power;
}
int main() {
    try {
        for (double rate : {44100., 48000., 96000.}) {
            std::array<Stereo, 4> impulse;
            for (int type = 1; type <= 4; ++type) {
                auto response = render(type, rate);
                check(energy(response, int(rate * .01), int(rate * 3)) > 1e-5,
                      "every model produces a genuine reverberant impulse tail");
                check(energy(response, int(rate * 2), int(rate * 3)) <
                          energy(response, 0, int(rate)),
                      "unexcited reverberation decays instead of accumulating energy");
                check(difference(response, render(type, rate, 2, .5f, 12, .3f, 1)) < 1e-7,
                      "reverb is invariant to block boundaries and repeated parameter snapshots");
                auto shortTail = render(type, rate, .3f);
                check(energy(response, int(rate * .5), int(rate * 3)) >
                          energy(shortTail, int(rate * .5), int(rate * 3)) * 2,
                      "Decay controls the measured late tail in every new model");
                impulse[size_t(type - 1)] = std::move(response);
            }
            for (int a = 0; a < 4; ++a)
                for (int b = 0; b < a; ++b)
                    check(difference(impulse[size_t(a)], impulse[size_t(b)]) > .1,
                          "Room, Spring, Vintage and Shimmer have distinct impulse responses");
            std::cout << "PASS reverb @ " << rate
                      << ": distinct stereo IRs, decay, finite tails and buffer invariance\n";
        }
        const auto dry = render(4, 48000, 6, 0, 12, .1f, 257, true);
        const auto octave = render(4, 48000, 6, 1, 12, .1f, 257, true);
        const auto fifth = render(4, 48000, 6, 1, 7, .1f, 257, true);
        double baseline = band(dry, 48000, 880), shifted = band(octave, 48000, 880);
        std::cout << "Shimmer octave band: " << baseline << " -> " << shifted
                  << "; fifth band: " << band(fifth, 48000, 440 * std::exp2(7. / 12)) << '\n';
        check(shifted > baseline * 20 + 1e-9,
              "Shimmer actually creates pitch-shifted energy above the original tone");
        check(difference(octave, fifth) > 1,
              "Shimmer pitch changes the rendered feedback, rather than just its label");
        check(difference(render(2, 48000, 2), render(2, 48000, 2, .5f, 12, .9f)) > .1,
              "Spring damping changes the dispersive tail");
        check(difference(render(2, 48000, 2, .5f, 12, .3f, 257, false, 0),
                         render(2, 48000, 2, .5f, 12, .3f, 257, false, 1)) > .1,
              "Spring Dispersion changes the frequency-dependent all-pass response");
        check(difference(render(3, 48000, 2, .5f, 12, .3f, 257, false, .6f, 0),
                         render(3, 48000, 2, .5f, 12, .3f, 257, false, .6f, 1)) > .1,
              "Vintage wow/flutter changes its modulated delay network");
        for (int type = 1; type <= 4; ++type) {
            CharacterReverb model;
            model.prepare(96000, type);
            ReverbControls p;
            p.reverbDecay = 20;
            p.shimmerAmount = 1;
            p.shimmerPitch = 24;
            p.springDispersion = 1;
            p.vintageMod = 1;
            double earlyEnergy = 0, lateEnergy = 0;
            for (int i = 0; i < 96000 * 10; ++i) {
                if (i % 257 == 0)
                    model.configure(p, 1, 0);
                auto sample = model.tick(i == 0 ? 1.f : 0.f, 0);
                for (float x : sample) {
                    check(std::isfinite(x) && std::abs(x) < 8,
                          "maximum-decay and character extremes stay stable over a long tail");
                    if (i < 96000)
                        earlyEnergy += x * x;
                    if (i >= 96000 * 9)
                        lateEnergy += x * x;
                }
            }
            check(lateEnergy < earlyEnergy, "extreme settings retain a decaying tail");
            model.reset();
            model.configure(p, .65f, .3f);
            for (int i = 0; i < 96000 / 2; ++i) {
                auto x = model.tick(0, 0);
                check(x[0] == 0 && x[1] == 0,
                      "constant-time reset leaves exact silence and no stale delay samples");
            }
        }
        std::cout << "PASS Shimmer pitch bands, maximum-parameter stability and clean resets\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
