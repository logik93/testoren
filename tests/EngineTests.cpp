#include "Engine.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
float energy(const std::vector<float> &a) {
    float e = 0;
    for (auto x : a) {
        require(std::isfinite(x), "non-finite audio");
        e += x * x;
    }
    return e;
}
int main() {
    try {
        for (double sr : {44100., 48000., 96000.}) {
            Engine engine;
            engine.prepare(sr);
            Params p;
            std::vector<float> l(512), r(512);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) == 0, "silence before notes");
            engine.noteOn(69, 1, 1, p);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) > .0001f, "note must sound");
            engine.controller(1, 64, 127, p);
            engine.noteOff(69, 1, p);
            for (int i = 0; i < 100; ++i)
                engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 1, "sustain pedal");
            engine.controller(1, 64, 0, p);
            for (int i = 0; i < 250; ++i)
                engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 0, "release ends voice");
            require(energy(l) < 1e-12f, "release reaches silence");
            for (int n = 0; n < 40; ++n)
                engine.noteOn(40 + n, .8f, 1, p);
            require(engine.activeVoices() == 16, "bounded voice stealing");
            for (auto &o : p.osc) {
                o.unison = 8;
                o.level = 1;
            }
            for (int wave = 0; wave < 8; ++wave) {
                for (auto &o : p.osc)
                    o.wave = wave;
                for (auto &part : p.part) {
                    part.resonance = 1;
                    part.cutoff = 20000;
                }
                engine.pitchWheel(1, 1);
                engine.render(l.data(), r.data(), 512, p);
                require(std::isfinite(energy(l)), "stress finite");
            }
            engine.controller(1, 120, 0, p);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) == 0, "all sound off");
            // A sine at MIDI 69 must cross zero approximately 440 times/second.
            engine.reset();
            p = Params{};
            p.osc[0].wave = 0;
            p.osc[0].level = 1;
            p.osc[1].level = 0;
            p.part[0].cutoff = 20000;
            engine.noteOn(69, 1, 1, p);
            std::vector<float> a(static_cast<size_t>(sr)), b(static_cast<size_t>(sr));
            engine.render(a.data(), b.data(), int(sr), p);
            int crossings = 0;
            for (size_t i = 1; i < a.size(); ++i)
                if (a[i - 1] < 0 && a[i] >= 0)
                    ++crossings;
            require(std::abs(crossings - 440) <= 2, "A4 tuning");
            std::cout << "PASS sample rate " << sr
                      << ": MIDI, sustain, release, polyphony, waveforms, tuning\n";
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
