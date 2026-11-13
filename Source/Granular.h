#pragma once
#include "SourceData.h"
namespace logisth {
struct GranularParams {
    bool enabled = false, freeze = false, sync = false;
    float attack = .01f, hold = .03f, grainSpacing = 1, waveSpacing = 1, pan = 0, depth = 0,
          frequency = 1, randomness = 0, transientHold = .03f, beats = 4;
    int transients = 0, keyTo = 0;
};
struct GranulizerPlayback {
    struct Grain {
        double cursor = 0, age = 0, attack = 0, hold = 1;
        float pan = 0;
        bool active = false;
    };
    std::array<Grain, 64> grains{};
    double head = 0, untilGrain = 0, lfo = 0, lastPosition = 0;
    uint64_t revision = 0;
    bool started = false, finished = false;
    uint32_t rng = 0x537ac109;
    uint64_t generated = 0;
    float random() {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return float(double(rng) / 2147483648. - 1);
    }
    void reset() {
        *this = GranulizerPlayback{};
    }
    std::array<float, 2> tick(const SourceData &data, const GranularParams &p, double sr,
                              double pitch, float position, float end, bool loop, bool reverse,
                              int key, int root, double bpm, float sizeMod = 1) {
        const double length = double(data.samples[0].size());
        if (length < 2)
            return {};
        if (revision != data.revision) {
            reset();
            revision = data.revision;
        }
        double hold = std::max(1., std::round(double(p.hold * sizeMod) * sr)),
               attack = std::max(0., std::round(double(p.attack) * sr));
        double interval = std::max(1., hold * std::max(.05f, p.grainSpacing));
        double speed = (p.keyTo == 0 ? pitch : 1.) * data.sampleRate / sr;
        const auto &markers = p.transients == 1 ? data.regions : data.transients;
        double first = 0, last = std::clamp(double(end), 0., 1.) * (length - 1);
        if (last <= first)
            last = length - 1;
        double span = std::max(1., last - first);
        if (!started) {
            head = std::clamp(double(position), 0., 1.) * (length - 2);
            int offset = std::max(0, key - root);
            if (p.keyTo == 1)
                head = std::clamp(double(position) + (key - root) / 24., 0., 1.) * (length - 2);
            if (p.keyTo == 2)
                head = std::clamp(std::clamp(double(position), 0., 1.) * (length - 2) +
                                      offset * hold * data.sampleRate / sr,
                                  0., length - 2);
            if (p.keyTo == 3 && !markers.empty())
                head = markers[size_t(std::clamp(offset, 0, int(markers.size()) - 1))];
            if (p.waveSpacing < 0 && position == 0)
                head = length - 2;
            started = true;
            lastPosition = position;
        }
        if (position != lastPosition) {
            head = std::clamp(head + (position - lastPosition) * (length - 2), 0., last - 1);
            finished = false;
        }
        lastPosition = position;
        std::array<float, 2> output{};
        if (untilGrain <= 0 && !finished) {
            double at = head;
            double waveHold = hold * data.sampleRate / sr;
            if (p.randomness > 0)
                at += random() * p.randomness * span * .5;
            at = std::clamp(at, 0., length - 2);
            double durationHold = hold;
            double wave = p.waveSpacing + p.depth * std::sin(lfo * 6.283185307179586);
            if (p.sync)
                wave *=
                    span / std::max(1., double(p.beats) * 60 / std::max(1., bpm) * data.sampleRate);
            double advance = p.freeze ? 0. : waveHold * wave;
            if (p.transients > 0 && !markers.empty()) {
                auto marker = advance >= 0 ? std::lower_bound(markers.begin(), markers.end(),
                                                              int64_t(std::ceil(head)))
                                           : std::upper_bound(markers.begin(), markers.end(),
                                                              int64_t(std::floor(head)));
                if (advance < 0 && marker != markers.begin())
                    --marker;
                if (marker != markers.end() && *marker >= std::min(head, head + advance) &&
                    *marker <= std::max(head, head + advance)) {
                    at = double(*marker);
                    durationHold = std::max(1., std::round(double(p.transientHold) * sr));
                }
            }
            for (auto &grain : grains)
                if (!grain.active) {
                    grain = {at,  0, attack, durationHold, (generated % 2 ? 1.f : -1.f) * p.pan,
                             true};
                    break;
                }
            ++generated;
            untilGrain += interval;
            if (!p.freeze) {
                head += advance;
                if (loop) {
                    head = first + std::fmod(head - first, span);
                    if (head < first)
                        head += span;
                } else if (head < 0 || head >= last)
                    finished = true;
            }
        }
        for (auto &grain : grains)
            if (grain.active) {
                double duration = grain.attack * 2 + grain.hold;
                float envelope = 1;
                if (grain.attack > 0 && grain.age < grain.attack)
                    envelope = float(grain.age / grain.attack);
                else if (grain.attack > 0 && grain.age >= grain.attack + grain.hold)
                    envelope = float((duration - grain.age) / grain.attack);
                for (int ch = 0; ch < 2; ++ch) {
                    float panGain =
                        ch == 0 ? std::min(1.f, 1 - grain.pan) : std::min(1.f, 1 + grain.pan);
                    output[size_t(ch)] +=
                        data.sample(grain.cursor, ch) * std::clamp(envelope, 0.f, 1.f) * panGain;
                }
                grain.cursor += reverse ? -speed : speed;
                if (loop) {
                    grain.cursor = std::fmod(grain.cursor, span);
                    if (grain.cursor < 0)
                        grain.cursor += span;
                }
                if (++grain.age >= duration ||
                    (!loop && (grain.cursor < 0 || grain.cursor >= last)))
                    grain.active = false;
            }
        // Constant overlap compensation for stationary settings, without per-grain gain pumping.
        float gain = float(1 / std::max(1., (attack + hold) / interval));
        for (auto &x : output)
            x *= gain;
        --untilGrain;
        lfo += std::max(.001f, p.frequency) / sr;
        lfo -= std::floor(lfo);
        return output;
    }
};
} // namespace logisth
