#include "Arpeggiator.h"
#include "Granular.h"
#include "Telemetry.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
std::vector<MidiEvent> sequence(ArpParams p, int block = 1000, int frames = 1000,
                                std::initializer_list<int> chord = {60, 64, 67}) {
    Arpeggiator arp;
    arp.prepare(1000);
    std::vector<MidiEvent> result;
    int base = 0;
    auto out = [&](MidiEvent e) {
        e.time += base;
        result.push_back(e);
    };
    arp.begin(p, out);
    for (int key : chord)
        arp.input({0, EventType::NoteOn, 1, key, .8f}, p, out);
    for (base = 0; base < frames; base += block)
        arp.advance(0, std::min(block, frames - base), p, out);
    return result;
}
std::vector<MidiEvent> ons(const std::vector<MidiEvent> &input) {
    std::vector<MidiEvent> output;
    for (auto e : input)
        if (e.type == EventType::NoteOn)
            output.push_back(e);
    return output;
}
int main() {
    try {
        SourceData data;
        data.sampleRate = 1000;
        data.samples[0].resize(10001);
        data.samples[1].resize(10001);
        for (int i = 0; i < 10001; ++i)
            data.samples[0][size_t(i)] = data.samples[1][size_t(i)] = float(std::sin(i * .03));
        GranularParams g;
        g.enabled = true;
        g.attack = 0;
        g.hold = .01f;
        g.grainSpacing = 1;
        g.waveSpacing = 1;
        GranulizerPlayback normal;
        for (int i = 0; i < 300; ++i) {
            auto value = normal.tick(data, g, 1000, 1, 0, 1, false, false, 60, 60, 120);
            check(std::abs(value[0] - data.samples[0][size_t(i)]) < 1e-5,
                  "100 percent grain/wave spacing reconstructs normal playback");
        }
        GranulizerPlayback slow, pitched;
        auto slower = g;
        slower.grainSpacing = 2;
        for (int i = 0; i < 100; ++i) {
            slow.tick(data, slower, 1000, 1, 0, 1, true, false, 60, 60, 120);
            pitched.tick(data, g, 1000, 2, 0, 1, true, false, 60, 60, 120);
        }
        check(slow.generated == 5 && pitched.generated == 10 && std::abs(slow.head - 50) < .01 &&
                  std::abs(pitched.head - 100) < .01,
              "grain spacing and source traversal independent of pitch");
        GranulizerPlayback backwards;
        auto reverseOrder = g;
        reverseOrder.waveSpacing = -1;
        backwards.tick(data, reverseOrder, 1000, 1, .5f, 1, true, false, 60, 60, 120);
        double firstCursor = backwards.grains[0].cursor, head = backwards.head;
        backwards.tick(data, reverseOrder, 1000, 1, .5f, 1, true, false, 60, 60, 120);
        check(backwards.grains[0].cursor > firstCursor && head < 5000,
              "negative wave spacing reverses grain order, not grain samples");
        GranulizerPlayback frozen;
        auto freeze = g;
        freeze.freeze = true;
        for (int i = 0; i < 100; ++i)
            frozen.tick(data, freeze, 1000, 1, .25f, 1, true, false, 60, 60, 120);
        check(std::abs(frozen.head - 2499.75) < .01 && frozen.generated == 10,
              "time hold freezes scanner while grains continue");
        GranulizerPlayback envelope;
        auto env = g;
        env.attack = .003f;
        env.hold = .004f;
        env.grainSpacing = 4;
        for (int i = 0; i < 10; ++i)
            envelope.tick(data, env, 1000, 1, 0, 1, true, false, 60, 60, 120);
        check(!envelope.grains[0].active && envelope.generated == 1,
              "attack and decay each add their duration to hold");
        GranulizerPlayback percentage;
        auto percent = g;
        percent.keyTo = 1;
        percentage.tick(data, percent, 1000, 8, 0, 1, true, false, 72, 60, 120);
        check(std::abs(percentage.grains[0].cursor - (9999 * .5 + 1)) < .01,
              "key-to-percent maps root plus twelve to midpoint, without repitching");
        data.regions = {0, 100, 200};
        GranulizerPlayback transient;
        auto trans = g;
        trans.keyTo = 3;
        trans.transients = 1;
        trans.transientHold = .02f;
        transient.tick(data, trans, 1000, 1, 0, 1, true, false, 61, 60, 120);
        check(std::abs(transient.grains[0].cursor - 101) < .01 &&
                  std::abs(transient.grains[0].hold - 20) < .01,
              "region key mapping and transient grain hold");
        GranulizerPlayback backwardRegion;
        auto backwardTransient = trans;
        backwardTransient.keyTo = 0;
        backwardTransient.hold = .3f;
        backwardTransient.waveSpacing = -1;
        backwardRegion.tick(data, backwardTransient, 1000, 1, .025f, 1, true, false, 60, 60, 120);
        check(std::abs(backwardRegion.grains[0].cursor - 201) < .01,
              "backward scan aligns to nearest crossed region");
        GranulizerPlayback alternating;
        auto pan = g;
        pan.pan = 1;
        for (int i = 0; i < 10; ++i)
            alternating.tick(data, pan, 1000, 1, 0, 1, true, false, 60, 60, 120);
        auto right = alternating.tick(data, pan, 1000, 1, 0, 1, true, false, 60, 60, 120);
        check(right[0] == 0 && std::abs(right[1]) > .01,
              "odd and even grains alternate stereo sides");
        GranulizerPlayback tempo;
        auto synced = g;
        synced.sync = true;
        synced.beats = 4;
        for (int i = 0; i < 100; ++i)
            tempo.tick(data, synced, 1000, 1, 0, 1, true, false, 60, 60, 120);
        double at120 = tempo.head;
        for (int i = 0; i < 100; ++i)
            tempo.tick(data, synced, 1000, 1, 0, 1, true, false, 60, 60, 240);
        check(std::abs(tempo.head - at120 - at120 * 2) < .1,
              "live tempo changes stretch traversal independently of pitch");
        auto p = ArpParams{};
        p.enabled = true;
        p.pattern = true;
        p.graphs.enabled = true;
        p.sync = false;
        p.graphs.freeHz = 10;
        auto up = ons(sequence(p));
        check(up.size() == 10 && up[0].key == 60 && up[1].key == 64 && up[2].key == 67 &&
                  up[3].key == 60,
              "note graph up motion");
        for (int shape = 8; shape <= 15; ++shape) {
            for (auto &step : p.graphs.steps)
                step.note = shape;
            auto a = sequence(p, 1000), b = sequence(p, 29);
            check(a.size() == b.size(), "graph sequence count invariant");
            for (size_t i = 0; i < a.size(); ++i)
                check(a[i].time == b[i].time && a[i].type == b[i].type && a[i].key == b[i].key,
                      "all eight note shapes invariant to buffer partition");
        }
        p = ArpParams{};
        p.enabled = true;
        p.pattern = true;
        p.graphs.enabled = true;
        p.sync = false;
        p.graphs.freeHz = 10;
        p.graphs.steps[2].reset = true;
        auto reset = ons(sequence(p));
        check(reset[2].key == 60, "position reset restarts motion on marked step");
        p.graphs.steps[2].reset = false;
        p.graphs.steps[1].mute = true;
        auto mute = ons(sequence(p));
        check(mute[1].time == 200 && p.graphs.steps[1].note == 8,
              "step mute preserves note selection");
        p.graphs.steps[1].mute = false;
        p.graphs.link = false;
        p.graphs.lengths[2] = 2;
        p.graphs.divisors[2] = 2;
        p.graphs.steps[1].octave = 1;
        auto octave = ons(sequence(p));
        check(octave[0].key == 60 && octave[1].key == 64 && octave[2].key == 79 &&
                  octave[3].key == 72,
              "independent graph lengths and clock divisors");
        for (auto &step : p.graphs.steps) {
            step.octave = 0;
            step.pitch = -1;
        }
        p.graphs.degrees.fill(false);
        p.graphs.degrees[2] = true;
        auto selective = ons(sequence(p, 1000, 400, {55, 60, 64}));
        check(selective[0].key == 55 && selective[1].key == 60 && selective[2].key == 63,
              "pitch enable third uses chord root despite inversion");
        p.graphs.degrees.fill(true);
        for (auto &step : p.graphs.steps) {
            step.pitch = 0;
            step.harmony = 7;
            step.velocity = 50;
            step.gate = 25;
        }
        auto harmony = sequence(p);
        auto hs = ons(harmony);
        check(hs[0].key == 60 && hs[1].key == 67 && std::abs(hs[0].value - .4f) < .001f,
              "harmony and velocity scale");
        check(std::any_of(harmony.begin(), harmony.end(),
                          [](auto e) { return e.type == EventType::NoteOff && e.time == 25; }),
              "gate percent controls duration");
        for (auto &step : p.graphs.steps) {
            step.harmony = 0;
            step.velocity = 100;
            step.gate = 100;
            step.late = 0;
        }
        p.graphs.steps[1].late = -25;
        p.graphs.steps[2].late = 25;
        auto timing = ons(sequence(p));
        check(timing[1].time == 75 && timing[2].time == 225,
              "look-ahead schedules early and late steps");
        auto timingSplit = ons(sequence(p, 31));
        check(timingSplit.size() == timing.size(), "late graph buffer event count");
        for (size_t i = 0; i < timing.size(); ++i)
            check(timingSplit[i].time == timing[i].time, "early/late timing block invariant");
        for (auto &step : p.graphs.steps) {
            step.late = 0;
            step.note = 0;
        }
        p.graphs.constraints[0] = 2;
        check(ons(sequence(p))[0].key == 61, "note prevention redirects up one semitone");
        p.graphs.constraints[0] = 1;
        check(ons(sequence(p)).empty(), "note prevention block is silence");
        p.graphs.constraints[0] = 0;
        for (auto &step : p.graphs.steps)
            step.note = 1;
        p.graphs.chordMode = true;
        auto chord = ons(sequence(p, 1000, 100));
        check(chord.size() == 3 && chord[0].key == 64 && chord[1].key == 72 && chord[2].key == 67,
              "chord mode rotates full voicing above selected arp note");
        p.graphs.chordMode = false;
        p.graphs.retrigger = false;
        p.sync = true;
        p.bpm = 150;
        p.graphs.division = 5;
        p.clockAvailable = true;
        p.transportPlaying = true;
        p.positionValid = true;
        p.ppq = .125;
        auto aligned = ons(sequence(p, 17, 200));
        check(!aligned.empty() && aligned[0].time == 50,
              "non-retrigger clock aligns with host PPQ");
        p.transportPlaying = false;
        check(ons(sequence(p)).empty(), "synchronised graphs respect stopped host transport");
        for (int mode = 0; mode < 3; ++mode) {
            auto params = ArpParams{};
            params.enabled = true;
            params.pattern = mode > 0;
            params.graphs.enabled = mode == 2;
            params.sync = false;
            params.latch = true;
            Arpeggiator arp;
            arp.prepare(1000);
            std::vector<MidiEvent> events;
            auto emit = [&](MidiEvent e) { events.push_back(e); };
            arp.begin(params, emit);
            arp.input({0, EventType::NoteOn, 1, 60, 1}, params, emit);
            arp.input({0, EventType::Controller, 1, 64, 127}, params, emit);
            arp.input({0, EventType::NoteOff, 1, 60, 0}, params, emit);
            arp.advance(0, 30, params, emit);
            check(arp.playhead() >= 0, "arp starts before disable test");
            params.enabled = false;
            events.clear();
            arp.begin(params, emit);
            check(arp.playhead() == -1, "disable clears playhead in every arp engine");
            arp.advance(0, 1000, params, emit);
            check(ons(events).empty(),
                  "disable cancels latched/sustained steps and pending repeats");
            check(std::any_of(events.begin(), events.end(),
                              [](auto e) {
                                  return e.type == EventType::Controller && e.key == 64 &&
                                         e.value == 0;
                              }),
                  "disable releases sustain for downstream instruments");
            events.clear();
            arp.input({0, EventType::NoteOn, 1, 67, .7f}, params, emit);
            arp.input({20, EventType::NoteOff, 1, 67, 0}, params, emit);
            check(ons(events).size() == 1 && ons(events)[0].key == 67,
                  "disabled arp passes fresh notes through normally");
        }
        ScopeBuffer scope;
        for (int i = 0; i < 10000; ++i)
            scope.push(float(i), -float(i), 0);
        scope.commit();
        std::array<float, 512> a{}, b{}, sum{};
        scope.read(0, a.data(), 512);
        scope.read(1, b.data(), 512);
        scope.read(2, sum.data(), 512);
        check(a[0] == 9488 && a[511] == 9999 && b[0] == -9488 && sum[511] == 0,
              "scope channels remain independent and wrap safely");
        scope.clear();
        scope.read(0, a.data(), 512);
        check(a[511] == 0, "scope reset clears visible history");
        std::cout << "PASS studio: documented granular "
                     "spacing/envelope/order/freeze/key/transient/tempo, eight arp note shapes, "
                     "masks, clock divisions, root-aware pitch, gate/velocity/harmony, early/late "
                     "look-ahead, constraints, chord rotation, host sync and scope\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
