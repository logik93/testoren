#pragma once
#include "Modulation.h"
#include <cstdint>
namespace logisth {
template <class Data>
float resample(const Data &data, double at, int channel, int quality, double speed, double first,
               double last, bool wrap) {
    auto &samples = data.samples[size_t(channel)];
    if (samples.size() < 2)
        return 0;
    const double span = std::max(1., last - first);
    auto read = [&](int64_t index) {
        double x = double(index);
        if (wrap) {
            x = first + std::fmod(x - first, span);
            if (x < first)
                x += span;
        }
        x = std::clamp(x, first, last - 1);
        return samples[size_t(std::clamp(int64_t(x), int64_t(0), int64_t(samples.size() - 1)))];
    };
    int64_t i = int64_t(std::floor(at));
    float f = float(at - i);
    if (quality == 0)
        return read(i) * (1 - f) + read(i + 1) * f;
    if (quality == 1) {
        float a = read(i - 1), b = read(i), c = read(i + 1), d = read(i + 2);
        return b + .5f * f * (c - a + f * (2 * a - 5 * b + 4 * c - d + f * (3 * (b - c) + d - a)));
    }
    int half = quality >= 3 ? 16 : 8;
    double sum = 0, weight = 0, cutoff = std::min(1., 1 / std::max(1., std::abs(speed)));
    for (int k = -half + 1; k <= half; ++k) {
        double distance = double(k) - f;
        double x = pi * distance * cutoff;
        double sinc = std::abs(x) < 1e-12 ? 1 : std::sin(x) / x;
        double window =
            .42 + .5 * std::cos(pi * distance / half) + .08 * std::cos(2 * pi * distance / half);
        double w = sinc * window * cutoff;
        sum += read(i + k) * w;
        weight += w;
    }
    return std::abs(weight) > 1e-12 ? float(sum / weight) : 0;
}
struct SamplerPlayer {
    struct Grain {
        double position = 0;
        int age = 0;
        bool active = false;
    };
    std::array<Grain, 4> grains{};
    double rateCache = 0, smoothing = 0;
    double cursor = 0, untilGrain = 0, lastStart = 0, stretch = 1;
    int algorithm = -1;
    bool finished = false, started = false;
    void reset() {
        *this = SamplerPlayer{};
    }
    template <class Data>
    std::array<float, 2> tick(const Data &data, double ratio, double sr, float begin, float end,
                              bool loop, bool reverse, int mode, float factor, int interpolation) {
        if (algorithm != mode) {
            reset();
            algorithm = mode;
            stretch = std::clamp(double(factor), .125, 8.);
        }
        double length = double(data.samples[0].size());
        if (length < 2)
            return {};
        double first = begin * (length - 2),
               last = std::min(length - 1, std::max(first + 2., end * (length - 1))),
               span = std::max(1., last - first);
        if (rateCache != sr) {
            rateCache = sr;
            smoothing = 1 - std::exp(-1 / (.02 * sr));
        }
        stretch += smoothing * (std::clamp(double(factor), .125, 8.) - stretch);
        double pitch = ratio * data.sampleRate / sr;
        auto read = [&](double offset, int ch) {
            double at = reverse ? last - 1 - offset : first + offset;
            return resample(data, at, ch, interpolation, mode == 1 ? pitch / stretch : pitch, first,
                            last, loop);
        };
        if (mode <= 1) {
            if (finished)
                return {};
            if (cursor >= span) {
                if (loop) {
                    double fade = std::min(span * .25, data.sampleRate * .005);
                    cursor = fade + std::fmod(cursor - span, span - fade);
                } else {
                    finished = true;
                    return {};
                }
            }
            std::array<float, 2> out{};
            double fade = std::min(span * .25, data.sampleRate * .005);
            for (int ch = 0; ch < 2; ++ch) {
                out[size_t(ch)] = read(cursor, ch);
                if (loop && span - cursor < fade) {
                    float mix = float((span - cursor) / fade);
                    out[size_t(ch)] =
                        out[size_t(ch)] * mix + read(cursor - (span - fade), ch) * (1 - mix);
                }
            }
            cursor += pitch / (mode == 1 ? stretch : 1);
            return out;
        }
        const int window = std::max(32, int(sr * .04)), hop = window / 2;
        if (!started) {
            grains[0] = {cursor, hop, true};
            started = true;
        }
        if (untilGrain <= 0 && !finished) {
            double at = cursor;
            if (mode == 3 && cursor > 0) {
                // WSOLA: align the next window to the previous window's continuation.
                double predicted = lastStart + pitch * hop, best = -1e30;
                int search = std::min(192, int(data.sampleRate * .006)),
                    step = std::max(1, search / 24);
                auto scoreAt = [&](int shift) {
                    double score = 0, a2 = 1e-12, b2 = 1e-12, candidate = cursor + shift;
                    if (!loop && (candidate < 0 || candidate >= span))
                        return -1e30;
                    for (int point = 0; point < 32; ++point) {
                        double delta = double(point) * hop / 32;
                        auto quick = [&](double o) {
                            double x = reverse ? last - 1 - o : first + o;
                            return resample(data, x, 0, 0, pitch, first, last, loop);
                        };
                        double a = quick(predicted + delta * pitch),
                               b = quick(candidate + delta * pitch);
                        score += a * b;
                        a2 += a * a;
                        b2 += b * b;
                    }
                    return score / std::sqrt(a2 * b2) -
                           double(std::abs(shift)) / std::max(1, search) * 1e-6;
                };
                int bestShift = 0;
                for (int shift = -search; shift <= search; shift += step) {
                    double score = scoreAt(shift);
                    if (score > best) {
                        best = score;
                        bestShift = shift;
                    }
                }
                const int coarse = bestShift;
                for (int shift = std::max(-search, coarse - step);
                     shift <= std::min(search, coarse + step); ++shift) {
                    double score = scoreAt(shift);
                    if (score > best) {
                        best = score;
                        bestShift = shift;
                    }
                }
                at = cursor + bestShift;
            }
            for (auto &g : grains)
                if (!g.active) {
                    g = {at, 0, true};
                    break;
                }
            lastStart = at;
            untilGrain += hop;
        }
        std::array<float, 2> out{};
        for (auto &g : grains)
            if (g.active) {
                double w = .5 - .5 * std::cos(2 * pi * g.age / window);
                for (int ch = 0; ch < 2; ++ch)
                    out[size_t(ch)] += read(g.position, ch) * float(w);
                g.position += pitch;
                if (++g.age >= window)
                    g.active = false;
            }
        cursor += data.sampleRate / sr / stretch;
        if (cursor >= span) {
            if (loop)
                cursor = std::fmod(cursor, span);
            else {
                cursor = span;
                finished = true;
            }
        }
        --untilGrain;
        // A non-looping sample ends its windows smoothly rather than jumping to zero.
        return out;
    }
};
} // namespace logisth
