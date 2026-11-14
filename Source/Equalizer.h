#pragma once
#include "Modulation.h"
#include <complex>
#include <cstdint>
namespace logisth {
struct EqualizerParams {
    int eqMode = 0;
    bool eqAutoGain = false;
    float eqOutput = 0;
    std::array<float, 10> eqBands{};
};
class Equalizer {
    struct Filter {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float tick(float x) {
            double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return float(y);
        }
        void set(double sr, double frequency, double db, bool shelf, bool high = false) {
            double A = std::pow(10., db / 40),
                   w = 2 * pi * std::clamp(frequency, 15., sr * .45) / sr, c = std::cos(w),
                   s = std::sin(w), a0;
            if (!shelf) {
                double alpha = s / (2 * 1.41421356237);
                b0 = 1 + alpha * A;
                b1 = -2 * c;
                b2 = 1 - alpha * A;
                a0 = 1 + alpha / A;
                a1 = -2 * c;
                a2 = 1 - alpha / A;
            } else {
                double beta = s * std::sqrt(2 * A);
                if (high) {
                    b0 = A * ((A + 1) + (A - 1) * c + beta);
                    b1 = -2 * A * ((A - 1) + (A + 1) * c);
                    b2 = A * ((A + 1) + (A - 1) * c - beta);
                    a0 = (A + 1) - (A - 1) * c + beta;
                    a1 = 2 * ((A - 1) - (A + 1) * c);
                    a2 = (A + 1) - (A - 1) * c - beta;
                } else {
                    b0 = A * ((A + 1) - (A - 1) * c + beta);
                    b1 = 2 * A * ((A - 1) - (A + 1) * c);
                    b2 = A * ((A + 1) - (A - 1) * c - beta);
                    a0 = (A + 1) + (A - 1) * c + beta;
                    a1 = -2 * ((A - 1) + (A + 1) * c);
                    a2 = (A + 1) + (A - 1) * c - beta;
                }
            }
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
        }
        double magnitude(double frequency, double sr) const {
            auto z = std::polar(1., -2 * pi * frequency / sr);
            return std::abs((b0 + b1 * z + b2 * z * z) / (1. + a1 * z + a2 * z * z));
        }
    };
    std::array<std::array<Filter, 12>, 2> filters{};
    std::array<float, 14> current{}, target{};
    double sr = 44100;
    float coefficient = 0, sampleCoefficient = 0, mode = 0, modeTarget = 0, gain = 1,
          gainTarget = 1;
    uint32_t counter = 0;
    bool initial = false;
    void update() {
        for (auto &ch : filters) {
            ch[0].set(sr, current[2], current[0], true);
            ch[1].set(sr, current[3], current[1], true, true);
            for (int i = 0; i < 10; ++i)
                ch[size_t(i + 2)].set(sr, frequencies[size_t(i)], current[size_t(i + 4)], false);
        }
    }

  public:
    inline static constexpr std::array<double, 10> frequencies{31.5, 63,   125,  250,  500,
                                                               1000, 2000, 4000, 8000, 16000};
    void prepare(double rate) {
        sr = rate;
        coefficient = float(1 - std::exp(-32 / (.015 * sr)));
        sampleCoefficient = float(1 - std::exp(-1 / (.015 * sr)));
        reset();
    }
    void reset() {
        filters = {};
        current = {};
        target = {};
        counter = 0;
        initial = false;
        mode = modeTarget = 0;
        gain = gainTarget = 1;
    }
    void configure(const EqualizerParams &p, float bass, float bassFreq, float treble,
                   float trebleFreq) {
        target[0] = bass;
        target[1] = treble;
        target[2] = bassFreq;
        target[3] = trebleFreq;
        for (int i = 0; i < 10; ++i)
            target[size_t(i + 4)] = p.eqBands[size_t(i)];
        modeTarget = p.eqMode == 0 ? 0.f : 1.f;
        double compensation = 0;
        if (p.eqAutoGain) {
            for (int i = 0; i < 96; ++i) {
                double hz = 20 * std::pow(std::min(20000., sr * .45) / 20, i / 95.);
                compensation = std::max(
                    compensation, response(p, bass, bassFreq, treble, trebleFreq, hz, sr, false));
            }
        }
        gainTarget = float(std::pow(10., (p.eqOutput - compensation) / 20));
        if (!initial) {
            current = target;
            mode = modeTarget;
            gain = gainTarget;
            update();
            initial = true;
        }
    }
    std::array<float, 2> tick(float l, float r) {
        if (counter++ % 32 == 0) {
            bool changed = false;
            for (size_t i = 0; i < current.size(); ++i) {
                float d = target[i] - current[i];
                if (std::abs(d) > 1e-5f) {
                    current[i] += coefficient * d;
                    changed = true;
                } else
                    current[i] = target[i];
            }
            if (changed)
                update();
        }
        const float smooth = sampleCoefficient;
        mode += smooth * (modeTarget - mode);
        gain += smooth * (gainTarget - gain);
        std::array<float, 2> out{l, r};
        for (int ch = 0; ch < 2; ++ch) {
            float a = filters[size_t(ch)][1].tick(filters[size_t(ch)][0].tick(out[size_t(ch)])),
                  b = out[size_t(ch)];
            for (int i = 2; i < 12; ++i)
                b = filters[size_t(ch)][size_t(i)].tick(b);
            out[size_t(ch)] = (a + (b - a) * mode) * gain;
        }
        return out;
    }
    static double response(const EqualizerParams &p, float bass, float bassFreq, float treble,
                           float trebleFreq, double hz, double sr, bool output = true) {
        double mag = 1;
        Filter f;
        if (p.eqMode == 0) {
            f.set(sr, bassFreq, bass, true);
            mag *= f.magnitude(hz, sr);
            f.set(sr, trebleFreq, treble, true, true);
            mag *= f.magnitude(hz, sr);
        } else
            for (int i = 0; i < 10; ++i) {
                f.set(sr, frequencies[size_t(i)], p.eqBands[size_t(i)], false);
                mag *= f.magnitude(hz, sr);
            }
        return 20 * std::log10(std::max(1e-12, mag)) + (output ? p.eqOutput : 0);
    }
};
} // namespace logisth
