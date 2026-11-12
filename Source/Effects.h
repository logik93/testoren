#pragma once
#include "Modulation.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
namespace logisth {
struct EffectsParams {
    bool distortion = false, phaser = false, chorus = false, eq = false, reverb = true,
         delay = false, compressor = false, sync = true, dual = false, pingpong = false;
    int distortionType = 0, phaserDivision = 12, chorusDivision = 12, delayDivisionL = 8,
        delayDivisionR = 7;
    double bpm = 120;
    float drive = 0, distMix = 1, phaserFreq = 800, phaserSpread = .5f, phaserOffset = .2f,
          phaserWidth = 1, phaserRate = .3f, phaserGain = .5f, phaserFeedback = .3f,
          phaserMix = .5f;
    float chorusDelay = 12, chorusDepth = .25f, chorusRate = .35f, chorusFeedback = .1f,
          chorusWidth = 1, chorusMix = .25f;
    float bass = 0, bassFreq = 200, treble = 0, trebleFreq = 6000, reverbSize = .65f,
          reverbDamp = .5f, reverbPredelay = 0, reverbWidth = 1, reverbMix = .12f;
    float delayL = .375f, delayR = .5f, feedback = .35f, delaySpread = 1, delayWidth = 1,
          lowCut = 20, highCut = 18000, smear = 0, delayMix = .2f;
    float ratio = 4, threshold = -18, attack = 10, release = 150;
};
class Effects {
    struct Biquad {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float tick(float x) {
            double y = x * b0 + z1;
            z1 = x * b1 - y * a1 + z2;
            z2 = x * b2 - y * a2;
            return float(y);
        }
        void shelf(double sr, double freq, double db, bool high) {
            const double A = std::pow(10., db / 40),
                         w = 2 * pi * std::clamp(freq, 20., sr * .43) / sr, c = std::cos(w),
                         sn = std::sin(w), alpha = sn / std::sqrt(2.),
                         beta = 2 * std::sqrt(A) * alpha;
            double a0;
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
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
        }
    };
    double sr = 44100, phase = 0, chorusPhase = 0;
    size_t delayPos = 0, chorusPos = 0, prePos = 0;
    std::array<std::vector<float>, 2> delayLines, preLines;
    std::array<std::vector<float>, 4> chorusLines;
    std::array<std::array<float, 6>, 2> allpass{};
    std::array<float, 2> phaserLast{}, lowState{}, highState{}, crushHold{};
    std::array<std::array<float, 3>, 2> smearState{};
    std::array<std::array<Biquad, 2>, 2> shelves{};
    std::array<float, 512> wetL{}, wetR{};
    juce::Reverb room;
    float crushCountdown = 0, detector = 0, drive = 0, distMix = 1, delayMix = 0, reverbMix = 0;
    static float read(const std::vector<float> &data, size_t pos, double samples) {
        double at = double(pos) - samples;
        while (at < 0)
            at += data.size();
        const auto a = size_t(at) % data.size();
        const auto b = (a + 1) % data.size();
        float frac = float(at - std::floor(at));
        return data[a] + frac * (data[b] - data[a]);
    }
    static void width(float &l, float &r, float width) {
        const float mid = (l + r) * .5f, side = (l - r) * .5f * width;
        l = mid + side;
        r = mid - side;
    }

  public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        for (auto &b : delayLines)
            b.assign(size_t(sr * 16) + 4, 0);
        for (auto &b : preLines)
            b.assign(size_t(sr * .21) + 4, 0);
        for (auto &b : chorusLines)
            b.assign(size_t(sr * .1) + 4, 0);
        room.setSampleRate(sr);
        reset();
    }
    void reset() {
        for (auto *buffers : {&delayLines, &preLines})
            for (auto &b : *buffers)
                std::fill(b.begin(), b.end(), 0);
        for (auto &b : chorusLines)
            std::fill(b.begin(), b.end(), 0);
        phase = chorusPhase = 0;
        delayPos = chorusPos = prePos = 0;
        allpass = {};
        phaserLast = lowState = highState = crushHold = {};
        smearState = {};
        shelves = {};
        room.reset();
        crushCountdown = detector = drive = delayMix = reverbMix = 0;
        distMix = 1;
    }
    void process(float *l, float *r, int n, const float *driveMod, const float *phaserMod,
                 const EffectsParams &p) {
        const float smooth = float(1 - std::exp(-1 / (.01 * sr)));
        const double phaserHz = p.sync ? p.bpm / (60 * beatLength(p.phaserDivision)) : p.phaserRate;
        const double chorusHz = p.sync ? p.bpm / (60 * beatLength(p.chorusDivision)) : p.chorusRate;
        const float normalization = std::pow(10.f, -std::max({0.f, p.bass, p.treble}) / 20);
        if (p.eq)
            for (int ch = 0; ch < 2; ++ch) {
                shelves[ch][0].shelf(sr, p.bassFreq, p.bass, false);
                shelves[ch][1].shelf(sr, p.trebleFreq, p.treble, true);
            }
        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};
            if (p.distortion) {
                drive += smooth * (clamp(p.drive + driveMod[s], 0, 1) - drive);
                distMix += smooth * (p.distMix - distMix);
                float gain = 1 + drive * 30;
                const int hold = 1 + int(drive * 63);
                bool capture = crushCountdown <= 0;
                if (capture)
                    crushCountdown = float(hold);
                --crushCountdown;
                for (int ch = 0; ch < 2; ++ch) {
                    const float a = x[ch] * gain;
                    float wet = a;
                    switch (p.distortionType) {
                    case 0:
                        wet = std::tanh(a);
                        break;
                    case 1:
                        wet = std::fmod(a + 1, 4.f);
                        if (wet < 0)
                            wet += 4;
                        wet = 1 - std::abs(wet - 2);
                        break;
                    case 2:
                        wet = clamp(a, -1, 1);
                        break;
                    case 3:
                        if (capture)
                            crushHold[ch] = x[ch];
                        wet = crushHold[ch];
                        break;
                    case 4: {
                        float levels = std::pow(2.f, 16 - 14 * drive);
                        wet = std::round(x[ch] * levels) / levels;
                        break;
                    }
                    default:
                        break;
                    }
                    x[ch] += (wet - x[ch]) * distMix;
                }
            }
            if (p.phaser) {
                float wet[2]{};
                for (int ch = 0; ch < 2; ++ch) {
                    float sample = x[ch] + phaserLast[ch] * p.phaserFeedback;
                    for (int stage = 0; stage < 6; ++stage) {
                        double exponent = (stage - 2.5) * p.phaserSpread * .8 +
                                          (ch == 0 ? -1 : 1) * p.phaserOffset +
                                          std::sin(2 * pi * phase + ch * p.phaserWidth * pi * .5) *
                                              p.phaserGain * 2 +
                                          phaserMod[s] * 10;
                        float freq =
                            clamp(float(p.phaserFreq * std::exp2(exponent)), 20, float(sr * .43));
                        float g = std::tan(float(pi) * freq / float(sr)), a = (1 - g) / (1 + g);
                        float out = -a * sample + allpass[ch][stage];
                        allpass[ch][stage] = sample + a * out;
                        sample = out;
                    }
                    phaserLast[ch] = std::tanh(sample);
                    wet[ch] = sample;
                }
                width(wet[0], wet[1], p.phaserWidth);
                for (int ch = 0; ch < 2; ++ch)
                    x[ch] += (wet[ch] - x[ch]) * p.phaserMix;
                phase += phaserHz / sr;
                phase -= std::floor(phase);
            }
            if (p.chorus) {
                float wet[2]{};
                for (int stage = 0; stage < (p.dual ? 2 : 1); ++stage)
                    for (int ch = 0; ch < 2; ++ch) {
                        const int line = 2 * stage + ch;
                        const double angle = 2 * pi * chorusPhase + line * pi * .5;
                        const double time = std::clamp(
                            double(p.chorusDelay) * (1 + .8 * p.chorusDepth * std::sin(angle)), .1,
                            60.);
                        float echo = read(chorusLines[line], chorusPos, time * .001 * sr);
                        chorusLines[line][chorusPos] = x[ch] + echo * p.chorusFeedback;
                        wet[ch] += echo / (p.dual ? 2 : 1);
                    }
                width(wet[0], wet[1], p.chorusWidth);
                for (int ch = 0; ch < 2; ++ch)
                    x[ch] += (wet[ch] - x[ch]) * p.chorusMix;
                chorusPos = (chorusPos + 1) % chorusLines[0].size();
                chorusPhase += chorusHz / sr;
                chorusPhase -= std::floor(chorusPhase);
            }
            if (p.eq)
                for (int ch = 0; ch < 2; ++ch)
                    x[ch] = shelves[ch][1].tick(shelves[ch][0].tick(x[ch])) * normalization;
            l[s] = x[0];
            r[s] = x[1];
            if (p.reverb) {
                for (int ch = 0; ch < 2; ++ch) {
                    preLines[ch][prePos] = x[ch];
                    float pre = read(preLines[ch], prePos, p.reverbPredelay * .001 * sr);
                    (ch == 0 ? wetL : wetR)[s] = pre;
                }
                prePos = (prePos + 1) % preLines[0].size();
            }
        }
        if (p.reverb) {
            juce::Reverb::Parameters rp;
            rp.roomSize = p.reverbSize;
            rp.damping = p.reverbDamp;
            rp.width = p.reverbWidth;
            rp.wetLevel = 1;
            rp.dryLevel = 0;
            room.setParameters(rp);
            room.processStereo(wetL.data(), wetR.data(), n);
            for (int s = 0; s < n; ++s) {
                reverbMix += smooth * (p.reverbMix - reverbMix);
                l[s] += (wetL[s] - l[s]) * reverbMix;
                r[s] += (wetR[s] - r[s]) * reverbMix;
            }
        }
        const double dl =
            std::clamp((p.sync ? 60 / p.bpm * beatLength(p.delayDivisionL) : p.delayL) * sr, 1.,
                       double(delayLines[0].size() - 2));
        const double dr =
            std::clamp((p.sync ? 60 / p.bpm * beatLength(p.delayDivisionR) : p.delayR) * sr, 1.,
                       double(delayLines[0].size() - 2));
        const float low = float(1 - std::exp(-2 * pi * p.lowCut / sr)),
                    high =
                        float(1 - std::exp(-2 * pi * std::min(double(p.highCut), sr * .43) / sr));
        const float attack = float(std::exp(-1 / (p.attack * .001 * sr))),
                    release = float(std::exp(-1 / (p.release * .001 * sr)));
        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};
            if (p.delay) {
                float echo[2]{read(delayLines[0], delayPos, dl), read(delayLines[1], delayPos, dr)};
                for (int ch = 0; ch < 2; ++ch) {
                    highState[ch] += high * (echo[ch] - highState[ch]);
                    lowState[ch] += low * (highState[ch] - lowState[ch]);
                    echo[ch] = highState[ch] - lowState[ch];
                    for (int stage = 0; stage < 3; ++stage) {
                        float a = p.smear * .65f;
                        float y = -a * echo[ch] + smearState[ch][stage];
                        smearState[ch][stage] = echo[ch] + a * y;
                        echo[ch] = y;
                    }
                }
                const float mono = (x[0] + x[1]) * .5f;
                for (int ch = 0; ch < 2; ++ch) {
                    float input = p.pingpong ? mono * (ch == 0 ? 1.f : 1 - p.delaySpread) : x[ch];
                    float feedback = p.pingpong ? echo[1 - ch] : echo[ch];
                    delayLines[ch][delayPos] = input + feedback * p.feedback;
                }
                width(echo[0], echo[1], p.delayWidth);
                delayMix += smooth * (p.delayMix - delayMix);
                for (int ch = 0; ch < 2; ++ch)
                    x[ch] += (echo[ch] - x[ch]) * delayMix;
                delayPos = (delayPos + 1) % delayLines[0].size();
            }
            if (p.compressor) {
                float level = std::max(std::abs(x[0]), std::abs(x[1]));
                float c = level > detector ? attack : release;
                detector = level + c * (detector - level);
                float db = 20 * std::log10(std::max(1e-8f, detector));
                float reduction = std::max(0.f, db - p.threshold) * (1 - 1 / p.ratio);
                float gain = std::pow(10.f, -reduction / 20);
                x[0] *= gain;
                x[1] *= gain;
            }
            l[s] = x[0];
            r[s] = x[1];
        }
    }
};
} // namespace logisth
