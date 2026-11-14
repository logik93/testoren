#pragma once
#include "Modulation.h"
#include <vector>
namespace logisth {
struct ReverbControls {
    int reverbType = 0; // Hall, Room, Spring, Vintage, Shimmer
    float reverbDecay = 3.5f, shimmerAmount = .45f, shimmerPitch = 12, springDispersion = .6f,
          vintageMod = .35f, reverbDiffusion = .8f;
};
// Original feedback-delay-network implementations. All storage is allocated in
// prepare(); resetting a delay only invalidates its written span, never clears a
// large buffer or allocates in the audio callback.
class CharacterReverb {
    struct Line {
        std::vector<float> data;
        size_t pos = 0, filled = 0;
        void prepare(size_t size) {
            data.assign(std::max(size_t(4), size), 0);
            reset();
        }
        void reset() {
            pos = filled = 0;
        }
        void push(float x) {
            data[pos] = x;
            if (++pos == data.size())
                pos = 0;
            filled = std::min(filled + 1, data.size());
        }
        float read(double delay) const {
            delay = std::clamp(delay, 1., double(data.size() - 2));
            if (delay > double(filled))
                return 0;
            double at = double(pos) - delay;
            if (at < 0)
                at += data.size();
            size_t a = size_t(at), b = (a + 1) % data.size();
            return data[a] + float(at - a) * (data[b] - data[a]);
        }
        float allpass(float x, double delay, float gain) {
            float y = read(delay) - gain * x;
            push(x + gain * y);
            return y;
        }
    };
    std::array<Line, 16> lines;
    std::array<std::array<Line, 8>, 2> diffusion;
    std::array<Line, 2> early, pitch;
    std::array<float, 16> dampState{}, dcState{}, feedback{}, feedbackTarget{};
    std::array<std::array<float, 4>, 16> dispersion{};
    std::array<float, 2> previousWet{};
    std::array<double, 16> lengths{};
    double sr = 44100, phase = 0, pitchPhase = 0, pitchStep = 0, size = 1, sizeTarget = 1;
    float damping = 1, dcCoefficient = 0, smoothing = 0, shimmer = 0, character = 0, modulation = 0;
    float dampingTarget = 1, shimmerTarget = 0, characterTarget = 0, modulationTarget = 0;
    double pitchStepTarget = 0;
    float diffusionAmount = .8f, diffusionTarget = .8f;
    int type = 1;
    bool configured = false;

  public:
    void prepare(double sampleRate, int algorithm) {
        sr = std::max(8000., sampleRate);
        type = algorithm;
        for (auto &l : lines)
            l.prepare(size_t(sr * .36) + 4);
        for (auto &channel : diffusion)
            for (auto &l : channel)
                l.prepare(size_t(sr * .022) + 4);
        for (auto &l : early)
            l.prepare(size_t(sr * .12) + 4);
        for (auto &l : pitch)
            l.prepare(size_t(sr * .11) + 4);
        smoothing = float(1 - std::exp(-1 / (.02 * sr)));
        dcCoefficient = float(1 - std::exp(-2 * pi * 5 / sr));
        reset();
    }
    void reset() {
        for (auto &l : lines)
            l.reset();
        for (auto &c : diffusion)
            for (auto &l : c)
                l.reset();
        for (auto &l : early)
            l.reset();
        for (auto &l : pitch)
            l.reset();
        dampState = {};
        dcState = {};
        dispersion = {};
        previousWet = {};
        phase = pitchPhase = 0;
        configured = false;
    }
    void configure(const ReverbControls &p, float roomSize, float damp) {
        static constexpr double times[5][16] = {
            {.0211, .0277, .0331, .0391, .0437, .0517, .0599, .0673, .0739, .0839, .0971, .1097,
             .1279, .1451, .1637, .1879},
            {.0113, .0149, .0181, .0211, .0269, .0301, .0337, .0371, .0413, .0479, .0533, .0617,
             .0679, .0733, .0797, .0899},
            {.0251, .0293, .0329, .0371, .0413, .0479, .0521, .0577, .0637, .0719, .0797, .0839,
             .0919, .1013, .1139, .1277},
            {.0197, .0297, .0331, .0371, .0411, .0437, .0479, .0531, .0619, .0713, .0799, .0877,
             .0971, .1097, .1213, .1373},
            {.0271, .0371, .0433, .0499, .0559, .0613, .0671, .0719, .0833, .0971, .1099, .1193,
             .1319, .1451, .1613, .1793}};
        for (int i = 0; i < 16; ++i)
            lengths[size_t(i)] = times[type][i] * sr;
        sizeTarget = .5 + 1.3 * clamp(roomSize, 0, 1);
        const bool initialise = !configured;
        if (initialise)
            size = sizeTarget;
        float cutoff = float(18000 * std::pow(700. / 18000., clamp(damp, 0, 1)));
        if (type == 3)
            cutoff = std::min(cutoff, 7000.f);
        dampingTarget = float(1 - std::exp(-2 * pi * std::min(double(cutoff), sr * .43) / sr));
        for (int i = 0; i < 16; ++i)
            feedbackTarget[size_t(i)] =
                float(std::exp(-6.907755278982137 * lengths[size_t(i)] / sr * sizeTarget /
                               clamp(p.reverbDecay, .2f, 20)));
        shimmerTarget = clamp(p.shimmerAmount, 0, 1);
        characterTarget = clamp(p.springDispersion, 0, 1);
        modulationTarget = clamp(p.vintageMod, 0, 1);
        diffusionTarget = clamp(p.reverbDiffusion, 0, 1);
        pitchStepTarget = (std::exp2(double(clamp(p.shimmerPitch, -12, 24)) / 12) - 1) / (.08 * sr);
        if (initialise) {
            diffusionAmount = diffusionTarget;
            damping = dampingTarget;
            shimmer = shimmerTarget;
            character = characterTarget;
            modulation = modulationTarget;
            pitchStep = pitchStepTarget;
            feedback = feedbackTarget;
            configured = true;
        }
    }
    std::array<float, 2> tick(float l, float r) {
        diffusionAmount += smoothing * (diffusionTarget - diffusionAmount);
        damping += smoothing * (dampingTarget - damping);
        shimmer += smoothing * (shimmerTarget - shimmer);
        character += smoothing * (characterTarget - character);
        modulation += smoothing * (modulationTarget - modulation);
        pitchStep += smoothing * (pitchStepTarget - pitchStep);
        for (int i = 0; i < 16; ++i)
            feedback[size_t(i)] += smoothing * (feedbackTarget[size_t(i)] - feedback[size_t(i)]);
        size += smoothing * (sizeTarget - size);
        float input[2]{l, r};
        for (int ch = 0; ch < 2; ++ch) {
            early[size_t(ch)].push(input[ch]);
            for (int stage = 0; stage < (type == 2 ? 4 : 8); ++stage) {
                const double seconds = .0013 + stage * .0017 + ch * .00023;
                input[ch] = diffusion[size_t(ch)][size_t(stage)].allpass(
                    input[ch], seconds * sr, .35f + diffusionAmount * .35f);
            }
        }
        float shifted[2]{};
        if (type == 4 && shimmer > 0) {
            for (int ch = 0; ch < 2; ++ch) {
                pitch[size_t(ch)].push(std::tanh(previousWet[size_t(ch)] + input[ch] * .5f));
                for (int grain = 0; grain < 4; ++grain) {
                    double at = pitchPhase + grain * .25;
                    at -= std::floor(at);
                    double window = .5 - .5 * std::cos(2 * pi * at);
                    shifted[ch] +=
                        float(window * .5) * pitch[size_t(ch)].read(sr * (.005 + .08 * (1 - at)));
                }
            }
            pitchPhase += pitchStep;
            pitchPhase -= std::floor(pitchPhase);
        }
        float taps[16]{}, sum = 0;
        for (int i = 0; i < 16; ++i) {
            double movement = type == 3
                                  ? std::sin(phase * (1 + i * .071) + i) * modulation * sr * .00025
                              : type == 4 || type == 0 ? std::sin(phase + i) * sr * .000025
                                                       : 0;
            float sample = lines[size_t(i)].read(lengths[size_t(i)] * size + movement);
            if (type == 2)
                for (int stage = 0; stage < 4; ++stage) {
                    float a = .35f + character * .5f + (stage - 1.5f) * .025f;
                    float y = -a * sample + dispersion[size_t(i)][size_t(stage)];
                    dispersion[size_t(i)][size_t(stage)] = sample + a * y;
                    sample = y;
                }
            dampState[size_t(i)] += damping * (sample - dampState[size_t(i)]);
            dcState[size_t(i)] += dcCoefficient * (dampState[size_t(i)] - dcState[size_t(i)]);
            taps[i] = dampState[size_t(i)] - dcState[size_t(i)];
            sum += taps[i];
        }
        static constexpr float left[16]{1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1},
            right[16]{1, 1, -1, -1, 1, 1, -1, -1, -1, -1, 1, 1, -1, -1, 1, 1};
        static constexpr float outL[16]{1, -1, 1, -1, 1, -1, 1, -1, -1, 1, -1, 1, -1, 1, -1, 1},
            outR[16]{1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1};
        std::array<float, 2> wet{};
        for (int i = 0; i < 16; ++i) {
            float reflected = sum * .125f - taps[i]; // Orthogonal Householder feedback.
            if (type == 3)
                reflected = std::tanh(reflected * 1.4f) / 1.4f;
            float injection = (input[0] * left[i] + input[1] * right[i]) * .13f;
            float gain = feedback[size_t(i)];
            if (type == 4) {
                gain *= 1 - shimmer * .35f;
                injection += (shifted[0] * left[i] + shifted[1] * right[i]) * shimmer * .12f;
            }
            lines[size_t(i)].push(injection + reflected * gain);
            wet[0] += taps[i] * outL[i] * .25f;
            wet[1] += taps[i] * outR[i] * .25f;
        }
        if (type == 1) {
            for (int tap = 0; tap < 5; ++tap) {
                double seconds = (.007 + tap * .0053) * size;
                float gain = .22f / (1 + tap * .3f);
                wet[0] += early[size_t(tap % 2)].read(seconds * sr) * gain;
                wet[1] += early[size_t(1 - tap % 2)].read((seconds + .0019) * sr) * gain;
            }
        }
        previousWet = wet;
        phase += 2 * pi * .19 / sr;
        if (phase > 2 * pi)
            phase -= 2 * pi;
        return wet;
    }
};
} // namespace logisth
