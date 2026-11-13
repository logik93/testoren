#pragma once
#include "Gain.h"
#include "Granular.h"
#include "Modulation.h"
#include "SourceData.h"
#include "Telemetry.h"
#include <cstdint>
namespace logisth {
struct OscParams {
    int wave = 1, unison = 1;
    float level = .65f, detune = 12, pan = 0, semitone = 0, phase = 0, stereo = .6f;
    bool retrigger = false, invert = false;
    int engine = 0, root = 60;
    float position = 0, end = 1, size = .08f, density = 16, spray = .15f, scan = 0, warp = 0;
    bool loop = true, reverse = false, envelopeEnabled = false;
    EnvelopeParams envelope{.008f, .25f, 1, .35f};
    const SourceData *data = nullptr;
    GranularParams gran;
};
struct PartParams {
    EnvelopeParams amp;
    float cutoff = 5000, resonance = .15f, drive = 0, level = .7f;
    int filterType = 0, input = 0, slope = 0;
};
struct Params {
    std::array<OscParams, 4> osc{};
    std::array<PartParams, 2> part{};
    std::array<EnvelopeParams, 2> envelopes{};
    std::array<Routes, 2> envelopeRoutes{};
    std::array<LfoParams, 2> lfo{};
    std::array<Misc, 4> misc{};
    float filterCutoff = 0, filterResonance = 0, keytrack = 0, master = .65f, portamento = 0,
          bendRange = 2;
    double bpm = 120;
    int polyphony = 16, solo = 0, portaMode = 0;
    bool warmDrive = false, sync = true, monoLegato = false;
    Params() {
        osc[1].level = .3f;
        osc[1].semitone = -12;
        osc[2].level = osc[3].level = 0;
        part[1].input = 1;
        for (auto &e : envelopes)
            e.sustain = 0;
    }
};
class Envelope {
    enum Stage { off, attack, decay, sustain, release } stage = off;
    float value = 0, releaseStep = 0;

  public:
    void reset() {
        stage = off;
        value = 0;
    }
    void on() {
        stage = attack;
        value = 0;
    }
    void releaseNote(float seconds, double sr) {
        releaseStep = value / float(std::max(1., seconds * sr));
        stage = release;
    }
    bool active() const {
        return stage != off;
    }
    float tick(const EnvelopeParams &p, double sr) {
        switch (stage) {
        case off:
            return 0;
        case attack:
            value += float(1 / std::max(1., p.attack * sr));
            if (value >= 1) {
                value = 1;
                stage = decay;
            }
            break;
        case decay:
            value -= float((1 - p.sustain) / std::max(1., p.decay * sr));
            if (value <= p.sustain) {
                value = p.sustain;
                stage = sustain;
            }
            break;
        case sustain:
            value = p.sustain;
            break;
        case release:
            value -= releaseStep;
            if (value <= 0)
                reset();
            break;
        }
        return value;
    }
};
struct Filter {
    float ic1 = 0, ic2 = 0;
    float tick(float input, float cutoff, float resonance, int type, double sr, float drive,
               bool warm) {
        const float gain = 1 + drive * 12;
        auto saturate = [&](float x) { return warm ? std::tanh(x) : x / (1 + std::abs(x)); };
        if (drive > 0)
            input = saturate(input * gain);
        const float g = std::tan(float(pi) * clamp(cutoff, 20, float(sr * .43)) / float(sr));
        const float k = 2 - 2.01f * clamp(resonance, 0, 1), a1 = 1 / (1 + g * (g + k));
        const float v1 = a1 * (ic1 + g * (input - ic2)), v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        if (drive > 0 || resonance > .97f) {
            ic1 = saturate(ic1);
            ic2 = saturate(ic2);
        }
        return type == 1 ? input - k * v1 - v2 : type == 2 ? v1 : v2;
    }
};
class Engine {
    struct LfoState {
        double phase = 0;
        float held = 0, next = 0;
    };
    struct Voice {
        int note = -1, channel = 1;
        bool held = false, sustained = false;
        uint64_t age = 0;
        float velocity = 0, pressure = -1, random = 0, pitch = 60, glideStep = 0;
        std::array<std::array<double, 8>, 4> phases{};
        std::array<Envelope, 2> amp, env;
        std::array<Envelope, 4> sourceEnv;
        std::array<float, 4> sourcePosition{};
        std::array<int, 4> sourceModes{-1, -1, -1, -1};
        std::array<std::array<SourcePlayback, 8>, 4> playback{};
        std::array<std::array<GranulizerPlayback, 8>, 4> granular{};
        std::array<std::array<std::array<Filter, 2>, 2>, 2> filters{};
        std::array<float, 2> cutoff{5000, 5000};
        std::array<LfoState, 2> lfo{};
        ModValues previous{};
        bool active() const {
            return amp[0].active() || amp[1].active();
        }
    };
    std::vector<Voice> voices = std::vector<Voice>(16);
    std::array<PlaybackPicture, 4> pictures{};
    std::array<float, 16> bend{}, wheel{}, pressure{};
    std::array<bool, 16> sustain{};
    struct Key {
        int note = -1, channel = 1;
        float velocity = 0;
        uint64_t age = 0;
    };
    std::array<Key, 128> keys{};
    std::array<LfoState, 2> freeLfo{};
    double sr = 44100;
    uint64_t counter = 0;
    uint32_t randomState = 0x67a8451;
    float master = 0, lastPitch = 60, stepVelocity = 0;
    float noise() {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return float(double(randomState) / 2147483648. - 1);
    }
    static double blep(double t, double dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1;
        }
        if (t > 1 - dt) {
            t = (t - 1) / dt;
            return t * t + t + t + 1;
        }
        return 0;
    }
    float wave(double t, double dt, int shape) {
        switch (shape) {
        case 0:
            return float(std::sin(2 * pi * t));
        case 1:
            return float(2 * t - 1 - blep(t, dt));
        case 2:
            return float((t < .5 ? 1 : -1) + blep(t, dt) - blep(std::fmod(t + .5, 1.), dt));
        case 3:
            return float(1 - 4 * std::abs(t - .5));
        case 4:
            return noise();
        case 5:
        case 6: {
            const double duty = shape == 5 ? .25 : .125;
            return float((t < duty ? 1 : -1) + blep(t, dt) - blep(std::fmod(t + 1 - duty, 1.), dt));
        }
        default:
            return float((2 * t - 1 + 1 - 4 * std::abs(t - .5)) * .5 - blep(t, dt) * .5);
        }
    }
    void releaseVoice(Voice &v, const Params &p) {
        v.held = v.sustained = false;
        for (int i = 0; i < 4; ++i)
            v.sourceEnv[size_t(i)].releaseNote(p.osc[size_t(i)].envelope.release, sr);
        for (int i = 0; i < 2; ++i) {
            v.amp[i].releaseNote(p.part[i].amp.release, sr);
            v.env[i].releaseNote(p.envelopes[i].release, sr);
        }
    }
    float tickLfo(LfoState &state, const LfoParams &lp, const Params &p, float rateMod,
                  float gainMod) {
        const float result =
            lfoWave(state.phase, lp.wave, state.held, state.next) * clamp(lp.gain + gainMod, 0, 2) +
            lp.offset;
        const double rate = std::clamp(
            lfoRate(lp, p.sync, p.bpm) * std::exp2(double(clamp(rateMod, -4, 4)) * 4), .001, 200.);
        state.phase += rate / sr;
        if (state.phase >= 1) {
            state.phase -= std::floor(state.phase);
            state.held = state.next;
            state.next = noise();
        }
        return result;
    }
    static float sourceValue(Source s, const Voice &v, const std::array<float, 2> &amps,
                             const std::array<float, 2> &envs, const std::array<float, 2> &lfos,
                             float aftertouch, float modwheel, float pitchwheel, float step) {
        switch (s) {
        case Source::None:
            return 0;
        case Source::Velocity:
            return v.velocity;
        case Source::Keytrack:
            return (v.note - 60) / 60.f;
        case Source::Aftertouch:
            return aftertouch;
        case Source::ModWheel:
            return modwheel;
        case Source::PitchWheel:
            return pitchwheel;
        case Source::AmpEnvA:
            return amps[0];
        case Source::AmpEnvB:
            return amps[1];
        case Source::ModEnv1:
            return envs[0];
        case Source::ModEnv2:
            return envs[1];
        case Source::Lfo1:
            return lfos[0];
        case Source::Lfo2:
            return lfos[1];
        case Source::StepVelocity:
            return step;
        case Source::Random:
            return v.random;
        case Source::RandomBipolar:
            return 2 * v.random - 1;
        default:
            return 0;
        }
    }

  public:
    void prepare(double sampleRate) {
        sr = std::max(8000., sampleRate);
        reset();
    }
    void reset() {
        std::fill(voices.begin(), voices.end(), Voice{});
        bend = {};
        wheel = {};
        pressure = {};
        sustain = {};
        keys = {};
        freeLfo = {};
        master = 0;
        counter = 0;
        lastPitch = 60;
        stepVelocity = 0;
    }
    void setStepVelocity(float value) {
        stepVelocity = clamp(value, 0, 1);
    }
    void syncPosition(double ppq, const Params &p) {
        if (p.sync)
            for (int i = 0; i < 2; ++i)
                if (!p.lfo[i].free) {
                    double phase = ppq / beatLength(p.lfo[i].division);
                    phase -= std::floor(phase);
                    for (auto &v : voices)
                        v.lfo[i].phase = phase;
                }
    }
    void noteOn(int note, float velocity, int channel, const Params &p) {
        if (channel < 1 || channel > 16 || note < 0 || note > 127)
            return;
        if (velocity <= 0) {
            noteOff(note, channel, p);
            return;
        }
        const bool overlap =
            std::any_of(keys.begin(), keys.end(), [](auto &k) { return k.note >= 0; });
        auto key = std::find_if(keys.begin(), keys.end(),
                                [&](auto &k) { return k.note == note && k.channel == channel; });
        if (key == keys.end())
            key = std::find_if(keys.begin(), keys.end(), [](auto &k) { return k.note < 0; });
        if (key != keys.end())
            *key = {note, channel, velocity, ++counter};
        Voice *selected = nullptr;
        const int limit = p.monoLegato ? 1 : std::clamp(p.polyphony, 1, 16);
        if (p.monoLegato) {
            selected = &voices[0];
            for (size_t i = 1; i < voices.size(); ++i)
                voices[i] = Voice{};
            if (selected->active() && overlap) {
                selected->glideStep = std::abs(float(note) - selected->pitch) /
                                      float(std::max(1., p.portamento * sr));
                selected->note = note;
                selected->channel = channel;
                selected->velocity = velocity;
                selected->held = true;
                selected->sustained = false;
                selected->age = counter;
                if (p.portamento <= 0)
                    selected->pitch = float(note);
                lastPitch = selected->pitch;
                return;
            }
        }
        for (int i = 0; i < limit && !selected; ++i)
            if (!voices[size_t(i)].active())
                selected = &voices[size_t(i)];
        if (!selected)
            selected = &*std::min_element(voices.begin(), voices.begin() + limit,
                                          [](auto &a, auto &b) { return a.age < b.age; });
        auto &v = *selected;
        v = Voice{};
        v.note = note;
        v.channel = channel;
        v.held = true;
        v.velocity = clamp(velocity, 0, 1);
        v.age = ++counter;
        v.random = (noise() + 1) * .5f;
        v.pitch = p.portamento > 0 && (overlap || p.portaMode == 1) ? lastPitch : float(note);
        lastPitch = float(note);
        v.glideStep = std::abs(float(note) - v.pitch) / float(std::max(1., p.portamento * sr));
        for (auto &part : v.filters)
            for (auto &channel : part)
                for (auto &filter : channel)
                    filter.ic1 = 1e-6f;
        for (int i = 0; i < 2; ++i) {
            v.amp[i].on();
            v.env[i].on();
            v.cutoff[i] = p.part[i].cutoff;
            v.lfo[i].held = noise();
            v.lfo[i].next = noise();
            if (p.lfo[i].free)
                v.lfo[i] = freeLfo[i];
        }
        for (size_t o = 0; o < 4; ++o) {
            v.sourceEnv[o].on();
            v.sourcePosition[o] = p.osc[o].position;
            for (auto &phase : v.phases[o])
                phase = p.osc[o].retrigger ? p.osc[o].phase / 360.f : (noise() + 1) * .5;
        }
    }
    void noteOff(int note, int channel, const Params &p) {
        if (channel < 1 || channel > 16)
            return;
        for (auto &k : keys)
            if (k.note == note && k.channel == channel)
                k = Key{};
        if (p.monoLegato && voices[0].note == note && voices[0].channel == channel) {
            auto k = std::max_element(keys.begin(), keys.end(),
                                      [](auto &a, auto &b) { return a.age < b.age; });
            if (k->note >= 0) {
                auto &v = voices[0];
                v.glideStep =
                    std::abs(float(k->note) - v.pitch) / float(std::max(1., p.portamento * sr));
                v.note = k->note;
                v.channel = k->channel;
                v.velocity = k->velocity;
                if (p.portamento <= 0)
                    v.pitch = float(k->note);
                return;
            }
        }
        for (auto &v : voices)
            if (v.note == note && v.channel == channel && v.held) {
                v.held = false;
                if (sustain[size_t(channel - 1)])
                    v.sustained = true;
                else
                    releaseVoice(v, p);
            }
    }
    void pitchWheel(int channel, float normalized) {
        if (channel >= 1 && channel <= 16)
            bend[size_t(channel - 1)] = clamp(normalized, -1, 1);
    }
    void aftertouch(int channel, int note, float value) {
        if (channel < 1 || channel > 16)
            return;
        if (note < 0)
            pressure[size_t(channel - 1)] = clamp(value, 0, 1);
        else
            for (auto &v : voices)
                if (v.channel == channel && v.note == note)
                    v.pressure = clamp(value, 0, 1);
    }
    void controller(int channel, int cc, int value, const Params &p) {
        if (channel < 1 || channel > 16)
            return;
        const auto ch = size_t(channel - 1);
        if (cc == 1)
            wheel[ch] = value / 127.f;
        if (cc == 64) {
            sustain[ch] = value >= 64;
            if (!sustain[ch])
                for (auto &v : voices)
                    if (v.channel == channel && v.sustained)
                        releaseVoice(v, p);
        }
        if (cc == 123 || cc == 120) {
            for (auto &k : keys)
                if (k.channel == channel)
                    k = Key{};
            for (auto &v : voices)
                if (v.channel == channel) {
                    if (cc == 120)
                        v = Voice{};
                    else
                        releaseVoice(v, p);
                }
        }
        if (cc == 121) {
            wheel[ch] = bend[ch] = pressure[ch] = 0;
            sustain[ch] = false;
            for (auto &v : voices)
                if (v.channel == channel && v.sustained)
                    releaseVoice(v, p);
        }
    }
    int activeVoices() const {
        int n = 0;
        for (auto &v : voices)
            n += v.active();
        return n;
    }
    void render(float *left, float *right, int count, const Params &p, float *distortion = nullptr,
                float *phaser = nullptr, float *partA = nullptr, float *partB = nullptr) {
        pictures = {};
        std::array<uint64_t, 4> pictureAges{};
        const float smoothing = float(1 - std::exp(-1 / (.01 * sr)));
        const int limit = p.monoLegato ? 1 : std::clamp(p.polyphony, 1, 16);
        for (size_t i = size_t(limit); i < voices.size(); ++i)
            voices[i] = Voice{};
        for (int s = 0; s < count; ++s) {
            float out[2]{};
            float parts[2]{};

            float fxDrive = 0, fxPhaser = 0;
            uint64_t fxAge = 0;
            for (int i = 0; i < 2; ++i)
                tickLfo(freeLfo[i], p.lfo[i], p, 0, 0);
            for (auto &v : voices) {
                if (!v.active())
                    continue;
                if (p.portamento > 0) {
                    float step = v.glideStep;
                    v.pitch += clamp(float(v.note) - v.pitch, -step, step);
                } else
                    v.pitch = float(v.note);
                std::array<float, 2> amps{}, envs{}, lfos{};
                for (int i = 0; i < 2; ++i) {
                    amps[i] = v.amp[i].tick(p.part[i].amp, sr);
                    envs[i] = v.env[i].tick(p.envelopes[i], sr);
                    const auto r = i == 0 ? Destination::Lfo1Rate : Destination::Lfo2Rate;
                    const auto g = i == 0 ? Destination::Lfo1Gain : Destination::Lfo2Gain;
                    // Feedback destinations use the preceding sample, avoiding an algebraic loop.
                    lfos[i] = tickLfo(v.lfo[i], p.lfo[i], p, v.previous.get(r), v.previous.get(g));
                }
                ModValues mods;
                for (int i = 0; i < 2; ++i) {
                    mods.add(p.envelopeRoutes[i], envs[i]);
                    mods.add(p.lfo[i].routes, lfos[i]);
                }
                for (const auto &misc : p.misc)
                    mods.add(
                        misc.routes,
                        sourceValue(misc.source, v, amps, envs, lfos,
                                    v.pressure >= 0 ? v.pressure : pressure[size_t(v.channel - 1)],
                                    wheel[size_t(v.channel - 1)], bend[size_t(v.channel - 1)],
                                    stepVelocity));
                v.previous = mods;
                if (v.age >= fxAge) {
                    fxAge = v.age;
                    fxDrive = mods.get(Destination::DistortionAmount);
                    fxPhaser = mods.get(Destination::PhaserFrequency);
                }
                float oscillators[2][2]{};
                for (int osc = 0; osc < 4; ++osc) {
                    const int part = osc / 2;
                    const auto &op = p.osc[size_t(osc)];
                    int mode = op.engine == 3 && op.gran.enabled ? 4 : op.engine;
                    if (v.sourceModes[size_t(osc)] != mode) {
                        v.sourceModes[size_t(osc)] = mode;
                        for (auto &player : v.playback[size_t(osc)])
                            player.reset();
                        for (auto &player : v.granular[size_t(osc)])
                            player.reset();
                    }
                    float sourceEnvelope = v.sourceEnv[size_t(osc)].tick(op.envelope, sr);
                    const float level =
                        clamp(op.level + mods.pair(Destination::VolumeA, part), 0, 2) *
                        (op.envelopeEnabled ? sourceEnvelope : 1.f);
                    const int n = std::clamp(op.unison, 0, 8);
                    if (n == 0 || level <= 0)
                        continue;
                    const float pitch = bend[size_t(v.channel - 1)] * p.bendRange +
                                        mods.pair(Destination::PitchA, part) * 48;
                    for (int u = 0; u < n; ++u) {
                        const float spread = n == 1 ? 0 : 2.f * u / (n - 1) - 1;
                        const double hz = 440 * std::exp2((v.pitch - 69 + pitch + op.semitone +
                                                           spread * op.detune * .01) /
                                                          12.);
                        const double dt = std::min(.45, hz / sr);
                        auto &phase = v.phases[size_t(osc)][size_t(u)];
                        double t = phase + mods.pair(Destination::PhaseA, part);
                        t -= std::floor(t);
                        std::array<float, 2> signal{};
                        float targetPosition =
                            clamp(op.position + mods.pair(Destination::PositionA, part), 0, 1);
                        auto &position = v.sourcePosition[size_t(osc)];
                        position += smoothing * (targetPosition - position);
                        if (op.engine == 0)
                            signal.fill(wave(t, dt, op.wave));
                        else if (op.data) {
                            if (op.engine == 1) {
                                t += op.warp * .15 * std::sin(2 * pi * t);
                                t -= std::floor(t);
                                signal.fill(op.data->table(t, position, dt));
                            } else {
                                double ratio = std::exp2((v.pitch - op.root + pitch + op.semitone +
                                                          spread * op.detune * .01) /
                                                         12.);
                                if (op.engine == 3 && op.gran.enabled) {
                                    signal = v.granular[size_t(osc)][size_t(u)].tick(
                                        *op.data, op.gran, sr, ratio, position, op.end, op.loop,
                                        op.reverse, v.note, op.root, p.bpm,
                                        std::exp2(
                                            clamp(mods.pair(Destination::GrainSizeA, part), -4, 4) *
                                            4));
                                } else
                                    signal = v.playback[size_t(osc)][size_t(u)].tick(
                                        *op.data, op.engine, ratio, sr, position, op.end, op.loop,
                                        op.reverse,
                                        clamp(op.size *
                                                  std::exp2(
                                                      mods.pair(Destination::GrainSizeA, part) * 4),
                                              .005f, .25f),
                                        op.density, op.spray, op.scan,
                                        op.engine == 3 &&
                                                v.playback[size_t(osc)][size_t(u)].untilGrain <= 0
                                            ? noise()
                                            : 0);
                            }
                        }
                        if (u == 0 && op.data && v.age >= pictureAges[size_t(osc)]) {
                            pictureAges[size_t(osc)] = v.age;
                            auto &view = pictures[size_t(osc)];
                            view = {};
                            view.active = true;
                            view.revision = op.data->revision;
                            double length = std::max(2., double(op.data->samples[0].size()));
                            if (op.engine == 1)
                                view.position = float(t);
                            else if (op.engine == 2) {
                                auto &player = v.playback[size_t(osc)][0];
                                view.active = !player.finished;
                                view.position =
                                    float(op.reverse ? op.end - player.cursor / (length - 1)
                                                     : position + player.cursor / (length - 1));
                            } else if (op.engine == 3 && op.gran.enabled) {
                                auto &player = v.granular[size_t(osc)][0];
                                view.position = float(player.head / (length - 1));
                                view.active =
                                    !player.finished ||
                                    std::any_of(player.grains.begin(), player.grains.end(),
                                                [](const auto &grain) { return grain.active; });
                                for (const auto &grain : player.grains)
                                    if (grain.active && view.grainCount < 16)
                                        view.grains[size_t(view.grainCount++)] =
                                            float(grain.cursor / (length - 1));
                            } else if (op.engine == 3) {
                                auto &player = v.playback[size_t(osc)][0];
                                view.position = float(position + player.elapsed * op.scan);
                                view.position -= std::floor(view.position);
                                for (const auto &grain : player.grains)
                                    if (grain.active && view.grainCount < 16)
                                        view.grains[size_t(view.grainCount++)] =
                                            float(grain.cursor / (length - 1));
                            } else
                                view.active = false;
                        }
                        const float gain = level * (op.invert ? -1.f : 1.f) / std::sqrt(float(n));
                        phase += dt;
                        phase -= std::floor(phase);
                        const float pan =
                            clamp(op.pan + spread * op.stereo + mods.pair(Destination::PanA, part),
                                  -1, 1);
                        oscillators[part][0] += signal[0] * gain * std::sqrt((1 - pan) * .5f);
                        oscillators[part][1] += signal[1] * gain * std::sqrt((1 + pan) * .5f);
                    }
                }
                for (int part = 0; part < 2; ++part) {
                    const auto &pp = p.part[size_t(part)];
                    const float key = (v.note - 60) * p.keytrack;
                    const float cutoff =
                        effectiveCutoff(pp.cutoff, p.filterCutoff,
                                        mods.pair(Destination::CutoffA, part) * 120, key, sr);
                    v.cutoff[part] += smoothing * (cutoff - v.cutoff[part]);
                    const float res = clamp(pp.resonance + p.filterResonance +
                                                mods.pair(Destination::ResonanceA, part),
                                            0, 1);
                    const float mix = clamp(pp.level + mods.pair(Destination::MixA, part), 0, 2);
                    for (int ch = 0; ch < 2; ++ch) {
                        float input = pp.input == 0   ? oscillators[0][ch]
                                      : pp.input == 1 ? oscillators[1][ch]
                                      : pp.input == 2 ? oscillators[0][ch] + oscillators[1][ch]
                                                      : 0;
                        if (pp.filterType != 3) {
                            input = v.filters[part][ch][0].tick(input, v.cutoff[part], res,
                                                                pp.filterType, sr, pp.drive,
                                                                p.warmDrive);
                            if (pp.slope == 1)
                                input = v.filters[part][ch][1].tick(input, v.cutoff[part], res,
                                                                    pp.filterType, sr, pp.drive,
                                                                    p.warmDrive);
                        }
                        float contribution = input * amps[part] * v.velocity * mix;
                        parts[part] += contribution;
                        if (p.solo == 0 || p.solo == part + 1)
                            out[ch] += contribution;
                    }
                }
            }
            master += smoothing * (p.master - master);
            left[s] = out[0] * engineHeadroom * master;
            right[s] = out[1] * engineHeadroom * master;
            if (partA)
                partA[s] = parts[0] * engineHeadroom * master;
            if (partB)
                partB[s] = parts[1] * engineHeadroom * master;
            if (distortion)
                distortion[s] = fxDrive;
            if (phaser)
                phaser[s] = fxPhaser;
        }
    }
    const std::array<PlaybackPicture, 4> &playbackPictures() const {
        return pictures;
    }
    static float effectiveCutoff(float local, float global, float modulation, float key,
                                 double sampleRate) {
        return clamp(local * std::exp2(clamp(global + modulation + key, -180, 180) / 12), 20,
                     float(sampleRate * .43));
    }
};
} // namespace logisth
