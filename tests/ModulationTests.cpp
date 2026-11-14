#include "Engine.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Render {
    std::vector<float> l, r, drive, phaser;
};
Render render(Params p, Source source = Source::Velocity) {
    Engine e;
    e.prepare(48000);
    e.controller(1, 1, 100, p);
    e.pitchWheel(1, .3f);
    e.aftertouch(1, -1, .75f);
    e.setStepVelocity(.8f);
    e.noteOn(72, .8f, 1, p);
    if (source == Source::Aftertouch)
        e.aftertouch(1, 72, .9f);
    Render out;
    for (auto *v : {&out.l, &out.r, &out.drive, &out.phaser})
        v->resize(8192);
    e.render(out.l.data(), out.r.data(), 8192, p, out.drive.data(), out.phaser.data());
    return out;
}
double difference(const Render &a, const Render &b) {
    double d = 0;
    for (size_t s = 0; s < a.l.size(); ++s)
        d += std::abs(a.l[s] - b.l[s]) + std::abs(a.r[s] - b.r[s]);
    return d;
}
double energy(const Render &a) {
    double d = 0;
    for (float x : a.l) {
        check(std::isfinite(x), "finite audio");
        d += double(x) * x;
    }
    return d;
}
int main() {
    try {
        Params base;
        for (auto &e : base.envelopes)
            e = EnvelopeParams{}; // Explicit nonzero envelope fixture; Init now intentionally
                                  // starts at zero.
        base.osc[1].level = 0;
        base.osc[0].retrigger = true;
        base.osc[2] = base.osc[0];
        base.osc[2].level = .4f;
        base.osc[2].semitone = 7;
        for (auto &p : base.part) {
            p.cutoff = 900;
            p.amp = {.001f, .01f, .8f, .1f};
        }
        base.sync = false;
        for (int i = 0; i < 2; ++i) {
            base.lfo[i].rate = float(3 + i);
            base.lfo[i].gain = .4f;
            base.lfo[i].routes[0] = {i == 0 ? Destination::PitchA : Destination::PitchB, .5f};
        }
        const auto baseline = render(base);
        for (int d = 1; d <= int(Destination::DistortionAmount); ++d) {
            auto p = base;
            p.misc[0].source = Source::Velocity;
            p.misc[0].routes[0] = {Destination(d), 3};
            const auto changed = render(p);
            if (Destination(d) == Destination::DistortionAmount)
                check(changed.drive[4000] > .2f, "distortion destination");
            else if (Destination(d) == Destination::PhaserFrequency)
                check(changed.phaser[4000] > .2f, "phaser destination");
            else if (difference(baseline, changed) < .001) {
                std::cerr << "inactive destination " << d << '\n';
                return 1;
            }
        }
        for (int source = 1; source < int(Source::Count); ++source) {
            auto p = base;
            p.misc[0].source = Source(source);
            p.misc[0].routes[0] = {Destination::PitchAB, 2};
            check(difference(baseline, render(p, Source(source))) > .001,
                  "selectable modulation source");
        }
        ModValues sum;
        sum.add(Routes{{{Destination::CutoffA, 5}, {Destination::PitchB, -5}}}, .8f);
        check(std::abs(sum.get(Destination::CutoffA) - .4f) < 1e-6f &&
                  std::abs(sum.get(Destination::PitchB) + .4f) < 1e-6f,
              "two bipolar routes");
        for (int solo : {1, 2}) {
            auto p = base;
            p.solo = solo;
            auto original = render(p);
            p.filterCutoff = -36;
            check(energy(render(p)) < energy(original) * .2, "global cutoff controls each part");
            p = base;
            p.solo = solo;
            p.filterResonance = .7f;
            check(difference(render(p), original) > .1, "global resonance controls each part");
        }
        check(std::abs(Engine::effectiveCutoff(1000, 12, 0, 0, 48000) - 2000) < .01,
              "global cutoff octave");
        check(std::abs(Engine::effectiveCutoff(1000, 0, 0, 12, 48000) - 2000) < .01,
              "keytrack octave");
        auto p = base;
        p.solo = 2;
        p.osc[2].level = 0;
        p.part[1].input = 1;
        check(energy(render(p)) < 1e-9, "empty B oscillators");
        p.part[1].input = 0;
        check(energy(render(p)) > .01, "cross route A into filter B");
        p.part[1].input = 3;
        check(energy(render(p)) < 1e-9, "filter input None");
        p = base;
        p.solo = 1;
        p.part[0].cutoff = 300;
        auto twelve = render(p);
        p.part[0].slope = 1;
        check(energy(render(p)) < energy(twelve) * .7, "24dB stronger attenuation");
        p = base;
        p.solo = 1;
        p.part[0].drive = .8f;
        auto cold = render(p);
        p.warmDrive = true;
        check(difference(cold, render(p)) > .01, "warm drive quality path");
        p = base;
        for (auto &o : p.osc)
            o.unison = 0;
        check(energy(render(p)) < 1e-9, "zero oscillator voices disables oscillator");
        Engine e;
        e.prepare(48000);
        p = base;
        p.monoLegato = true;
        p.portamento = .1f;
        e.noteOn(60, 1, 1, p);
        e.noteOn(72, 1, 1, p);
        check(e.activeVoices() == 1, "mono legato polyphony");
        e.noteOff(72, 1, p);
        check(e.activeVoices() == 1, "last note priority fallback");
        e.noteOff(60, 1, p);
        std::vector<float> l(48000), r(48000);
        e.render(l.data(), r.data(), 48000, p);
        check(e.activeVoices() == 0, "mono releases without stuck key");
        LfoParams lp;
        lp.free = false;
        lp.division = 10;
        check(std::abs(lfoRate(lp, true, 120) - 2) < 1e-6, "synced LFO rate");
        lp.free = true;
        lp.rate = 7;
        check(lfoRate(lp, true, 120) == 7, "Free ignores host sync");
        std::cout << "PASS: all 27 destinations, all 14 sources, A/B global filters, input "
                     "routing, slopes, warm drive, mono, LFO sync\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
