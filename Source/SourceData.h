#pragma once
#include "AssetLimits.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace logisth {
// Immutable after publication. Decoding and spectral processing happen off the audio thread.
struct SourceData {
    static uint64_t nextRevision() {
        static std::atomic<uint64_t> counter{0};
        return ++counter;
    }
    uint64_t revision = nextRevision();
    double sampleRate = 44100;
    std::string name = "Factory / glass";
    std::array<std::vector<float>, 2> samples;
    std::vector<int64_t> regions, transients;
    int frameSize = 2048, frames = 1;
    std::array<std::vector<float>, 11> tables;
    static void fft(std::vector<std::complex<float>> &a, bool inverse) {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1) {
            float angle = float((inverse ? 2 : -2) * 3.14159265358979323846 / double(len));
            std::complex<float> wlen(std::cos(angle), std::sin(angle));
            for (size_t i = 0; i < n; i += len) {
                std::complex<float> w(1, 0);
                for (size_t j = 0; j < len / 2; ++j) {
                    auto u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
        if (inverse)
            for (auto &x : a)
                x /= float(n);
    }
    void analyseTransients() {
        buildWaveformPeaks();
        transients.clear();
        transients.push_back(0);
        float previous = 0;
        size_t separation = size_t(sampleRate * .012);
        for (size_t at = 0; at + 64 < samples[0].size(); at += 64) {
            float peak = 0;
            for (size_t i = 0; i < 64; ++i)
                peak = std::max(peak, std::abs(samples[0][at + i]));
            if (peak > .025f && peak > previous * 2.2f &&
                at - size_t(transients.back()) > separation)
                transients.push_back(int64_t(at));
            previous = std::max(peak, previous * .92f);
        }
    }
    std::vector<std::array<float, 4>> waveformPeaks;
    void buildWaveformPeaks() {
        waveformPeaks.assign((samples[0].size() + 255) / 256, {});
        for (size_t i = 0; i < samples[0].size(); ++i)
            for (size_t ch = 0; ch < 2; ++ch) {
                auto &peak = waveformPeaks[i / 256];
                peak[ch * 2] = std::min(peak[ch * 2], samples[ch][i]);
                peak[ch * 2 + 1] = std::max(peak[ch * 2 + 1], samples[ch][i]);
            }
    }
    std::array<float, 2> rangePeaks(int channel, size_t first, size_t last) const {
        std::array<float, 2> result{};
        channel = std::clamp(channel, 0, 1);
        last = std::min(last, samples[size_t(channel)].size());
        for (size_t i = std::min(first, last); i < last;) {
            if (i % 256 == 0 && i + 256 <= last && i / 256 < waveformPeaks.size()) {
                const auto &p = waveformPeaks[i / 256];
                result[0] = std::min(result[0], p[size_t(channel * 2)]);
                result[1] = std::max(result[1], p[size_t(channel * 2 + 1)]);
                i += 256;
            } else {
                float x = samples[size_t(channel)][i++];
                result[0] = std::min(result[0], x);
                result[1] = std::max(result[1], x);
            }
        }
        return result;
    }
    static bool validFrameSize(int size) {
        return size >= 128 && size <= 4096 && (size & (size - 1)) == 0;
    }
    void buildTables(int size = 2048) {
        if (!validFrameSize(size))
            size = 2048;
        frameSize = size;
        frames = std::clamp(int(samples[0].size() / size_t(size)), 1, 256);
        for (auto &band : tables)
            band.resize(size_t(frames * size));
        std::vector<std::complex<float>> spectrum(size), work(size);
        for (int f = 0; f < frames; ++f) {
            for (int i = 0; i < size; ++i) {
                size_t at = size_t(f * size + i);
                spectrum[size_t(i)] = at < samples[0].size() ? samples[0][at] : 0;
            }
            fft(spectrum, false);
            spectrum[0] = 0; // No DC in a periodic oscillator.
            for (int band = 0; band < 11; ++band) {
                work = spectrum;
                int cutoff = std::max(1, (size / 2 - 1) >> band);
                for (int k = cutoff + 1; k < size - cutoff; ++k)
                    work[size_t(k)] = 0;
                fft(work, true);
                for (int i = 0; i < size; ++i)
                    tables[size_t(band)][size_t(f * size + i)] = work[size_t(i)].real();
            }
        }
    }
    // WT POS selects one complete periodic cycle; it never advances through the PCM file.
    int frameIndex(float position) const {
        if (!std::isfinite(position))
            position = 0;
        return std::clamp(int(std::lround(std::clamp(position, 0.f, 1.f) * (frames - 1))), 0,
                          frames - 1);
    }
    float framePosition(int frame) const {
        return frames > 1 ? float(std::clamp(frame, 0, frames - 1)) / float(frames - 1) : 0;
    }
    float frameSample(int frame, double phase, int band = 0) const {
        if (tables[0].empty() || !std::isfinite(phase))
            return 0;
        phase -= std::floor(phase);
        double x = phase * frameSize;
        int i = int(x), j = (i + 1) % frameSize;
        float fraction = float(x - i);
        const auto &v = tables[size_t(std::clamp(band, 0, 10))];
        size_t offset = size_t(std::clamp(frame, 0, frames - 1) * frameSize);
        return v[offset + size_t(i)] * (1 - fraction) + v[offset + size_t(j)] * fraction;
    }
    float table(double phase, float position, double increment) const {
        const int frame = frameIndex(position);
        double level = std::clamp(std::log2(std::max(1., increment * frameSize * 2)), 0., 10.);
        int band = std::min(10, int(level) + 1), next = std::min(10, band + 1);
        float a = frameSample(frame, phase, band), b = frameSample(frame, phase, next);
        return a + (b - a) * float(level - std::floor(level));
    }
    float sample(double position, int channel) const {
        const auto &v = samples[size_t(std::clamp(channel, 0, 1))];
        if (v.size() < 2)
            return 0;
        if (position < 0 || position >= double(v.size() - 1))
            return 0;
        auto i = size_t(position);
        float frac = float(position - double(i));
        return v[i] * (1 - frac) + v[i + 1] * frac;
    }
    static std::shared_ptr<const SourceData> factory() {
        static auto data = [] {
            auto d = std::make_shared<SourceData>();
            d->samples[0].resize(44100);
            d->samples[1].resize(44100);
            for (int i = 0; i < 44100; ++i) {
                double t = i / 44100.;
                float x = float((std::sin(2 * 3.141592653589793 * 261.625565 * t) +
                                 .4 * std::sin(2 * 3.141592653589793 * 523.25113 * t)) *
                                std::exp(-4 * t) * .6);
                d->samples[0][size_t(i)] = d->samples[1][size_t(i)] = x;
            }
            // A separate, 128-cycle harmonic factory wavetable.
            auto original = d->samples;
            d->samples[0].resize(128 * 2048);
            for (int f = 0; f < 128; ++f)
                for (int i = 0; i < 2048; ++i) {
                    float x = 0;
                    for (int h = 1; h <= 32; ++h) {
                        double gain = (1. - f / 127.) * (h == 1 ? 1. : 0.) + f / 127. / h;
                        x += float(std::sin(2 * 3.141592653589793 * i * h / 2048.) * gain * .6);
                    }
                    d->samples[0][size_t(f * 2048 + i)] = x;
                }
            d->buildTables();
            d->samples = std::move(original);
            d->analyseTransients();
            return std::shared_ptr<const SourceData>(d);
        }();
        return data;
    }
};
struct SourcePlayback {
    struct Grain {
        double cursor = 0, age = 0, duration = 1;
        bool active = false;
    };
    std::array<Grain, 8> grains{};
    double cursor = 0, elapsed = 0, untilGrain = 0;
    int mode = -1;
    const SourceData *source = nullptr;
    uint64_t sourceRevision = 0;
    bool finished = false;
    void reset() {
        *this = SourcePlayback{};
    }
    std::array<float, 2> tick(const SourceData &data, int selected, double ratio, double sr,
                              float position, float end, bool loop, bool reverse, float size,
                              float density, float spray, float scan, float random) {
        if (mode != selected || source != &data || sourceRevision != data.revision) {
            reset();
            mode = selected;
            source = &data;
            sourceRevision = data.revision;
        }
        const double length = double(data.samples[0].size());
        double first = position * (length - 2), last = std::max(first + 2., end * (length - 1));
        last = std::min(length - 1, last);
        double span = std::max(1., last - first);
        double speed = ratio * data.sampleRate / sr;
        if (selected == 2) {
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
            double at = reverse ? last - 1 - cursor : first + cursor;
            std::array<float, 2> out{};
            for (int ch = 0; ch < 2; ++ch) {
                out[size_t(ch)] = data.sample(at, ch);
                double fade = std::min(span * .25, data.sampleRate * .005);
                if (loop && span - cursor < fade) {
                    float mix = float((span - cursor) / fade);
                    double head = reverse ? last - 1 - (cursor - (span - fade))
                                          : first + (cursor - (span - fade));
                    out[size_t(ch)] = out[size_t(ch)] * mix + data.sample(head, ch) * (1 - mix);
                }
            }
            cursor += speed;
            return out;
        }
        std::array<float, 2> out{};
        if (untilGrain <= 0) {
            for (auto &g : grains)
                if (!g.active) {
                    double centre = position + elapsed * scan + random * spray * .25;
                    centre -= std::floor(centre);
                    g = {centre * (length - 2), 0, std::max(8., size * sr), true};
                    break;
                }
            untilGrain += sr / std::max(1.f, density);
        }
        int active = 0;
        for (auto &g : grains)
            if (g.active) {
                float window =
                    float(.5 - .5 * std::cos(2 * 3.141592653589793 * g.age / g.duration));
                for (int ch = 0; ch < 2; ++ch)
                    out[size_t(ch)] += data.sample(g.cursor, ch) * window;
                g.cursor += (reverse ? -speed : speed);
                g.cursor = std::fmod(g.cursor, length - 1);
                if (g.cursor < 0)
                    g.cursor += length - 1;
                ++active;
                if (++g.age >= g.duration)
                    g.active = false;
            }
        float gain = 1 / std::sqrt(float(std::max(1, active)));
        for (auto &x : out)
            x *= gain;
        --untilGrain;
        elapsed += 1 / sr;
        return out;
    }
};
} // namespace logisth
