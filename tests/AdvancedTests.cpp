#include "Engine.h"
#include "Equalizer.h"
#include "Parameters.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
void check(bool value, const char *text) {
    if (!value)
        throw std::runtime_error(text);
}
std::vector<float> synth(Params p, int length = 24000) {
    Engine engine;
    engine.prepare(48000);
    engine.noteOn(69, 1, 1, p);
    std::vector<float> l(size_t(length), 0.f), r(size_t(length), 0.f);
    engine.render(l.data(), r.data(), length, p);
    return l;
}
Params clean() {
    Params p;
    for (auto &o : p.osc) {
        o.level = 0;
        o.retrigger = true;
        o.detune = 0;
        o.stereo = 0;
        o.semitone = 0;
    }
    p.osc[0].level = .5f;
    p.osc[0].wave = 0;
    for (auto &part : p.part) {
        part.filterType = 3;
        part.level = 1;
        part.amp = {0, 0, 1, 0};
    }
    return p;
}
double power(const std::vector<float> &a, double hz, double sr = 48000, int begin = 2048) {
    double c = 2 * std::cos(2 * pi * hz / sr), x = 0, y = 0;
    for (size_t i = size_t(begin); i < a.size(); ++i) {
        double z = a[i] + c * x - y;
        y = x;
        x = z;
    }
    return std::max(0., x * x + y * y - c * x * y) / std::pow(double(a.size() - size_t(begin)), 2);
}
double delta(const std::vector<float> &a, const std::vector<float> &b) {
    double sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += std::abs(a[i] - b[i]);
    return sum;
}
int main() {
    try {
        for (auto &spec : parameterSpecs) {
            std::string id = spec.id;
            if (id.find("dest") != std::string::npos) {
                int count = 1;
                for (auto *p = spec.choices; *p; ++p)
                    if (*p == '|')
                        ++count;
                check(count == int(Destination::Count),
                      "every modulation menu exposes all appended FM/AM/PWM targets");
            }
        }
        auto p = clean();
        auto dry = synth(p);
        p.osc[0].fmDepth = 1;
        auto fm = synth(p);
        check(power(fm, 880) > power(dry, 880) * 100 + 1e-5, "FM generates new harmonic sidebands");
        p = clean();
        p.osc[0].amDepth = 1;
        p.osc[0].amRatio = .5f;
        auto am = synth(p);
        check(power(am, 220) > power(dry, 220) * 100 + 1e-5 &&
                  power(am, 660) > power(dry, 660) * 100 + 1e-5,
              "AM generates the expected upper and lower sidebands");
        p = clean();
        p.osc[0].wave = 2;
        auto square = synth(p);
        p.osc[0].pulseWidth = .25f;
        auto pulse = synth(p);
        check(power(pulse, 880) > power(square, 880) * 20 + 1e-5,
              "pulse width changes duty cycle and even harmonics");
        p.osc[0].pwmDepth = .2f;
        p.osc[0].pwmRate = 3;
        check(delta(pulse, synth(p)) > 10, "PWM modulates the actual pulse waveform");
        const Destination starts[]{Destination::FmA1, Destination::AmA1, Destination::WidthA1};
        for (auto first : starts)
            for (int target = 0; target < 7; ++target)
                for (int slot = 0; slot < 4; ++slot) {
                    auto a = clean();
                    a.osc[0].level = 0;
                    a.osc[size_t(slot)].level = .2f;
                    a.osc[size_t(slot)].wave = 2;
                    a.sync = false;
                    auto before = synth(a, 4096);
                    a.lfo[0].rate = 3;
                    a.lfo[0].routes[0] = {Destination(int(first) + target), 4};
                    auto after = synth(a, 4096);
                    bool affected =
                        target < 4 ? target == slot : target == 6 || target - 4 == slot / 2;
                    check(affected ? delta(before, after) > 1 : delta(before, after) < 1e-5,
                          "new modulation destinations affect exactly their selected player or "
                          "group");
                }
        std::array<std::vector<float>, 3> noise;
        for (int i = 0; i < 3; ++i) {
            p = clean();
            p.osc[0].wave = i == 0 ? 4 : 7 + i;
            noise[size_t(i)] = synth(p, 96000);
            for (float x : noise[size_t(i)])
                check(std::isfinite(x) && std::abs(x) < 4,
                      "white/pink/brown noise remains bounded");
        }
        auto band = [&](const auto &a, double start, double end) {
            double sum = 0;
            for (int i = 0; i < 32; ++i)
                sum += power(a, start + (end - start) * i / 31);
            return sum;
        };
        double white = band(noise[0], 100, 500) / band(noise[0], 5000, 10000),
               pink = band(noise[1], 100, 500) / band(noise[1], 5000, 10000),
               brown = band(noise[2], 100, 500) / band(noise[2], 5000, 10000);
        check(pink > white * 5 && brown > pink * 2,
              "new coloured noises have distinct low/high spectral slopes");
        for (int wave : {10, 11}) {
            p = clean();
            p.osc[0].wave = wave;
            check(delta(dry, synth(p)) > 1, "Organ and Folded Sine are distinct periodic waves");
        }
        SourceData data;
        data.sampleRate = 48000;
        for (auto &ch : data.samples) {
            ch.resize(48000);
            for (int i = 0; i < 48000; ++i)
                ch[size_t(i)] = .5f * float(std::sin(2 * pi * 440 * i / 48000));
        }
        for (int mode = 0; mode < 4; ++mode) {
            SamplerPlayer sampler;
            std::vector<float> out(48000);
            for (int i = 0; i < 48000; ++i)
                out[size_t(i)] = sampler.tick(data, 1, 48000, 0, 1, false, false, mode, 2, 0)[0];
            check(std::abs(sampler.cursor - (mode == 0 ? 48000 : 24000)) < 3,
                  "sampler stretching changes traversal duration by the requested factor");
            double low = 0, high = 0;
            for (int i = 0; i < 31; ++i) {
                low += power(out, 205 + i);
                high += power(out, 410 + i * 2);
            }
            check(mode == 1 ? low > high * 5 : high > low * 5,
                  "Tape changes pitch while granular/WSOLA retain the pitched grain band");
            for (int i = 0; i < 60000; ++i) {
                auto x = sampler.tick(data, 1, 48000, 0, 1, false, false, mode, 2, 0);
                check(std::isfinite(x[0]) && std::abs(x[0]) < 2,
                      "stretched one-shots finish safely");
            }
            check(sampler.finished, "stretched one-shots reach the end");
        }
        for (auto &ch : data.samples)
            for (int i = 0; i < 48000; ++i)
                ch[size_t(i)] = float(std::sin(2 * pi * 14000 * i / 48000));
        double linear = 0, sinc = 0;
        for (int i = 100; i < 10000; ++i) {
            float a = resample(data, i * 4, 0, 0, 4, 0, 47999, false),
                  b = resample(data, i * 4, 0, 3, 4, 0, 47999, false);
            linear += a * a;
            sinc += b * b;
        }
        check(sinc < linear * .01, "Sinc 32 suppresses downsampling aliases rather than only "
                                   "changing interpolation labels");
        for (double sr : {44100., 48000., 96000.}) {
            EqualizerParams eq;
            check(std::abs(Equalizer::response(eq, 12, 200, 0, 6000, 1000, sr)) < .2,
                  "Legacy bass boost no longer applies hidden full-band attenuation");
            check(Equalizer::response(eq, 12, 200, 0, 6000, 20, sr) > 11.8,
                  "Legacy shelf reaches its requested low-frequency gain");
            eq.eqMode = 1;
            eq.eqBands[5] = 6;
            check(std::abs(Equalizer::response(eq, 0, 200, 0, 6000, 1000, sr) - 6) < .001,
                  "graphic EQ provides calibrated gain at its band centre");
            Equalizer engine;
            engine.prepare(sr);
            engine.configure(eq, 0, 200, 0, 6000);
            double input = 0, output = 0;
            for (int i = 0; i < int(sr * 2); ++i) {
                float x = .1f * float(std::sin(2 * pi * 1000 * i / sr));
                auto y = engine.tick(x, x);
                if (i >= int(sr)) {
                    input += x * x;
                    output += y[0] * y[0];
                }
                check(std::isfinite(y[0]), "EQ remains finite");
            }
            check(std::abs(10 * std::log10(output / input) - 6) < .05,
                  "graphic EQ audio agrees with its plotted response");
        }
        for (double sr : {44100., 48000., 96000.}) {
            Equalizer engine;
            engine.prepare(sr);
            EqualizerParams eq;
            engine.configure(eq, 0, 200, 0, 6000);
            for (int i = 0; i < 1024; ++i) {
                float x = float(std::sin(i * .1));
                auto out = engine.tick(x, -x);
                check(out[0] == x && out[1] == -x, "flat EQ is unity in both stereo channels");
            }
            for (int block = 0; block < 200; ++block) {
                eq.eqMode = block % 2;
                eq.eqAutoGain = block % 3 == 0;
                eq.eqOutput = block % 2 ? 18 : -18;
                for (int band = 0; band < 10; ++band)
                    eq.eqBands[size_t(band)] = (block + band) % 2 ? 18.f : -18.f;
                engine.configure(eq, block % 2 ? 18.f : -18.f, block % 2 ? 20.f : 2000.f,
                                 block % 2 ? -18.f : 18.f, block % 2 ? 2000.f : 20000.f);
                for (int i = 0; i < 512; ++i) {
                    auto out = engine.tick(.01f * float(std::sin((block * 512 + i) * .1)), 0);
                    check(std::isfinite(out[0]) && std::abs(out[0]) < 100,
                          "extreme EQ automation/mode changes remain stable at all sample rates");
                }
            }
        }
        std::cout << "PASS advanced DSP: FM/AM/PWM spectra, isolated routing, coloured noises, "
                     "stretch algorithms, sinc alias rejection and calibrated Legacy/graphic EQ\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
