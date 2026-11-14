#pragma once
#include "Engine.h"
#include <iostream>
#include <stdexcept>
namespace phase_test {
using namespace logisth;
using Audio = std::array<std::vector<float>, 2>;
inline void check(bool ok, const char *text) {
    if (!ok)
        throw std::runtime_error(text);
}
inline Params fixture(int slot, int engine, bool retrig, int unison, const SourceData *data,
                      int wave = 0) {
    Params p;
    for (auto &o : p.osc)
        o.level = 0;
    auto &o = p.osc[size_t(slot)];
    o.level = .2f;
    o.engine = engine;
    o.retrigger = retrig;
    o.unison = unison;
    o.wave = wave;
    o.data = data;
    o.stereo = 0;
    o.detune = 0;
    o.semitone = 0;
    for (auto &part : p.part) {
        part.filterType = 3;
        part.level = 1;
        part.amp = {0, 0, 1, 0};
    }
    return p;
}
inline Audio render(Engine &e, const Params &p, int samples, int block = 257) {
    Audio out{std::vector<float>(size_t(samples)), std::vector<float>(size_t(samples))};
    for (int at = 0; at < samples; at += block)
        e.render(out[0].data() + at, out[1].data() + at, std::min(block, samples - at), p);
    for (auto &channel : out)
        for (float x : channel)
            check(std::isfinite(x), "phase control produces finite audio");
    return out;
}
inline Audio start(Params p, double rate, int samples = 2048) {
    Engine e;
    e.prepare(rate);
    e.noteOn(69, 1, 1, p);
    return render(e, p, samples);
}
inline double delta(const Audio &a, const Audio &b, int begin = 0) {
    double sum = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (size_t i = size_t(begin); i < a[size_t(ch)].size(); ++i)
            sum += std::abs(a[size_t(ch)][i] - b[size_t(ch)][i]);
    return sum;
}
inline void run() {
    auto data = SourceData::factory();
    int onsetCases = 0, liveCases = 0;
    for (double rate : {44100., 48000., 96000.})
        for (int slot = 0; slot < 4; ++slot)
            for (int mode = 0; mode < 2; ++mode)
                for (bool retrig : {false, true})
                    for (int unison : {1, 8}) {
                        auto p = fixture(slot, mode, retrig, unison, data.get());
                        auto &phase = p.osc[size_t(slot)].phase;
                        for (int wave = 0; wave < (mode == 0 ? 8 : 1); ++wave) {
                            if (wave == 4)
                                continue; // Random noise has no periodic phase.
                            p.osc[size_t(slot)].wave = wave;
                            phase = 0;
                            auto zero = start(p, rate);
                            phase = 90;
                            check(delta(zero, start(p, rate)) > 1,
                                  "PHASE changes initial periodic audio with RETRIG both off and "
                                  "on in every slot");
                            phase = 360;
                            check(delta(zero, start(p, rate)) < 1e-7,
                                  "0 and 360 degrees are identical with either retrigger policy");
                            ++onsetCases;
                        }
                        p.osc[size_t(slot)].wave = 0;
                        phase = 0;
                        Engine moved, target, unchanged;
                        for (auto *e : {&moved, &target, &unchanged})
                            e->prepare(rate);
                        moved.noteOn(69, 1, 1, p);
                        unchanged.noteOn(69, 1, 1, p);
                        auto desired = p;
                        desired.osc[size_t(slot)].phase = 90;
                        target.noteOn(69, 1, 1, desired);
                        render(moved, p, 317);
                        render(unchanged, p, 317);
                        render(target, desired, 317);
                        auto actual = render(moved, desired, int(rate * .1));
                        auto reference = render(target, desired, int(rate * .1));
                        auto dry = render(unchanged, p, int(rate * .1));
                        check(delta(actual, dry) > 1,
                              "turning PHASE on a held note reaches Classic and Wavetable audio "
                              "without requiring RETRIG");
                        int end = int(actual[0].size());
                        for (int ch = 0; ch < 2; ++ch)
                            for (int i = end - 128; i < end; ++i)
                                check(std::abs(actual[size_t(ch)][size_t(i)] -
                                               reference[size_t(ch)][size_t(i)]) < 1e-5,
                                      "live phase converges to the requested offset without being "
                                      "applied twice");
                        if (mode == 0)
                            check(std::abs(actual[0][0] - dry[0][0]) < .01f,
                                  "phase automation begins with a small smoothed change, "
                                  "independent of unison count");
                        // Retrigger governs future note starts; toggling it must not reset a held
                        // oscillator.
                        Engine a, b;
                        a.prepare(rate);
                        b.prepare(rate);
                        a.noteOn(69, 1, 1, p);
                        b.noteOn(69, 1, 1, p);
                        render(a, p, 317);
                        render(b, p, 317);
                        auto toggle = p;
                        toggle.osc[size_t(slot)].retrigger = !retrig;
                        check(delta(render(a, toggle, 512), render(b, p, 512)) < 1e-7,
                              "RETRIG changes do not restart a held oscillator");
                        // Knob and modulation offsets add, with no duplicate start-phase term.
                        auto modulated = p;
                        modulated.osc[size_t(slot)].phase = 90;
                        modulated.misc[0].source = Source::Velocity;
                        modulated.misc[0].routes[0] = {Destination::PhaseAB, 2.5f};
                        auto direct = p;
                        direct.osc[size_t(slot)].phase = 180;
                        check(delta(start(modulated, rate), start(direct, rate)) < 1e-5,
                              "PHASE knob and phase modulation remain additive in either retrigger "
                              "mode");
                        // Repeated notes are fixed-phase only when RETRIG is enabled.
                        Engine repeat;
                        repeat.prepare(rate);
                        repeat.noteOn(69, 1, 1, p);
                        auto first = render(repeat, p, 1024);
                        repeat.noteOff(69, 1, p);
                        render(repeat, p, 256);
                        repeat.noteOn(69, 1, 1, p);
                        auto second = render(repeat, p, 1024);
                        check(retrig ? delta(first, second) < 1e-7 : delta(first, second) > 1,
                              "RETRIG retains repeatable starts while off retains random starts");
                        ++liveCases;
                    }
    // Wrap-around automation must move through two degrees, not almost a full turn.
    for (double rate : {44100., 48000., 96000.})
        for (bool retrig : {false, true}) {
            auto p = fixture(0, 0, retrig, 1, data.get());
            p.osc[0].phase = 359;
            Engine e, base, target;
            for (auto *x : {&e, &base, &target})
                x->prepare(rate);
            e.noteOn(69, 1, 1, p);
            base.noteOn(69, 1, 1, p);
            auto next = p;
            next.osc[0].phase = 1;
            target.noteOn(69, 1, 1, next);
            render(e, p, 317);
            render(base, p, 317);
            render(target, next, 317);
            auto x = render(e, next, int(rate * .1)), dry = render(base, p, int(rate * .1)),
                 goal = render(target, next, int(rate * .1));
            for (size_t i = 0; i < x[0].size(); ++i)
                check(std::abs(x[0][i] - dry[0][i]) < .008f,
                      "359-to-1 automation uses the short cyclic path");
            check(delta(x, goal, int(x[0].size()) - 128) < 1e-4,
                  "wrapped phase automation reaches its target");
        }
    // Host partitioning around the exact same automation sample cannot change audio.
    for (int mode = 0; mode < 2; ++mode)
        for (bool retrig : {false, true}) {
            auto p = fixture(0, mode, retrig, 8, data.get());
            auto q = p;
            q.osc[0].phase = 153;
            auto sequence = [&](int block) {
                Engine e;
                e.prepare(48000);
                e.noteOn(69, 1, 1, p);
                auto a = render(e, p, 317, block);
                auto b = render(e, q, 4096, block);
                for (int ch = 0; ch < 2; ++ch)
                    a[size_t(ch)].insert(a[size_t(ch)].end(), b[size_t(ch)].begin(),
                                         b[size_t(ch)].end());
                return a;
            };
            check(delta(sequence(1), sequence(257)) < 1e-7 &&
                      delta(sequence(257), sequence(4096)) < 1e-7,
                  "live PHASE automation is independent of audio block size");
        }
    std::cout << "PASS PHASE: " << onsetCases << " onset fixtures, " << liveCases
              << " live controls, RETRIG on/off, additive modulation, cyclic wrap and buffer "
                 "invariance\n";
}
} // namespace phase_test
