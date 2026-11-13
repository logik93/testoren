#include "Chords.h"
#include "Engine.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
std::vector<float> render(Params p, int note = 60, int samples = 16384, double sr = 48000,
                          int block = 257) {
    Engine engine;
    engine.prepare(sr);
    engine.noteOn(note, 1, 1, p);
    std::vector<float> l(size_t(samples), 0), r(size_t(samples), 0);
    for (int offset = 0; offset < samples; offset += block)
        engine.render(l.data() + offset, r.data() + offset, std::min(block, samples - offset), p);
    for (auto x : l)
        check(std::isfinite(x) && std::abs(x) < 8, "finite bounded hybrid audio");
    return l;
}
double difference(const std::vector<float> &a, const std::vector<float> &b) {
    double n = 0;
    for (size_t i = 0; i < a.size(); ++i)
        n += std::abs(a[i] - b[i]);
    return n;
}
double energy(const std::vector<float> &a) {
    double n = 0;
    for (auto x : a)
        n += x * x;
    return n;
}
int main() {
    try {
        auto data = SourceData::factory();
        Params p;
        for (auto &o : p.osc) {
            o.level = 0;
            o.data = data.get();
        }
        for (auto &part : p.part) {
            part.filterType = 3;
            part.amp = {0, 0, 1, .01f};
        }
        p.osc[0].level = .7f;
        p.osc[0].retrigger = true;
        p.master = 1;
        for (int engine : {0, 1}) {
            p.osc[0].engine = engine;
            p.osc[0].phase = 90;
            const auto reference = render(p);
            check(difference(reference, render(p)) < 1e-6,
                  "Retrig repeats the selected start phase");
            p.osc[0].phase = 180;
            check(difference(reference, render(p)) > 1, "Retrig phase changes audible waveform");
            p.osc[0].phase = 90;
            p.osc[0].invert = true;
            const auto inverted = render(p);
            double error = 0;
            for (size_t i = 0; i < reference.size(); ++i)
                error += std::abs(reference[i] + inverted[i]);
            check(error < 1e-5, "Invert reverses audio polarity in Classic and Wavetable engines");
            p.osc[0].invert = false;
        }
        p.osc[0].engine = 0;
        p.osc[0].phase = 0;
        for (double sr : {44100., 48000., 96000.})
            for (int slot = 0; slot < 4; ++slot)
                for (int mode = 0; mode < 4; ++mode) {
                    Params q = p;
                    q.osc[0].level = 0;
                    q.osc[size_t(slot)].level = .7f;
                    q.osc[size_t(slot)].engine = mode;
                    q.osc[size_t(slot)].retrigger = true;
                    q.osc[size_t(slot)].semitone = 0;
                    auto a = render(q, 60, 8192, sr), b = render(q, 60, 8192, sr, 8192);
                    check(energy(a) > .0001, "every engine works in every oscillator slot");
                    check(difference(a, b) < .00001,
                          "hybrid output invariant to buffer partitioning");
                }
        p.osc[0].engine = 1;
        auto wavetable = render(p);
        p.osc[0].position = 1;
        check(difference(wavetable, render(p)) > 1, "wavetable frame morph changes timbre");
        p.osc[0].position = 0;
        p.osc[0].warp = .6f;
        check(difference(wavetable, render(p)) > 1, "phase warp changes timbre");
        for (int destination = int(Destination::PositionA); destination < int(Destination::Count);
             ++destination) {
            Params q = p;
            q.osc[0].warp = 0;
            q.osc[2] = q.osc[0];
            q.osc[2].level = .4f;
            q.osc[0].engine = q.osc[2].engine = destination < int(Destination::GrainSizeA) ? 1 : 3;
            auto dry = render(q);
            q.misc[0].source = Source::Velocity;
            q.misc[0].routes[0] = {Destination(destination), 3};
            check(difference(dry, render(q)) > .01,
                  "all six position and grain-size modulation destinations");
        }
        p.osc[0].warp = 0;
        p.misc[0].source = Source::Velocity;
        p.misc[0].routes[0] = {Destination::PositionAB, 5};
        check(difference(wavetable, render(p)) > 1, "position modulation reaches wavetable frames");
        p.misc = {};
        p.osc[0].engine = 2;
        p.osc[0].loop = false;
        auto sample = render(p);
        p.osc[0].reverse = true;
        check(difference(sample, render(p)) > 1, "reverse playback");
        p.osc[0].reverse = false;
        p.osc[0].end = .05f;
        auto one = render(p, 60, 16384);
        double tail = 0;
        for (size_t i = 8000; i < one.size(); ++i)
            tail += std::abs(one[i]);
        check(tail < 1e-6, "one-shot stops at selected end");
        p.osc[0].loop = true;
        check(energy(render(p, 60, 16384)) > energy(one) * 2, "loop repeats selected region");
        p.osc[0].end = 1;
        p.osc[0].engine = 3;
        auto grain = render(p);
        p.osc[0].spray = 1;
        p.osc[0].scan = .8f;
        check(difference(grain, render(p)) > .1, "grain spray and scan alter playback");
        p.osc[0].spray = .15f;
        p.osc[0].scan = 0;
        p.misc[0].source = Source::Velocity;
        p.misc[0].routes[0] = {Destination::GrainSizeAB, 5};
        check(difference(grain, render(p)) > .1, "grain-size modulation");
        p.misc = {};
        p.osc[0].engine = 0;
        p.osc[0].envelopeEnabled = true;
        p.osc[0].envelope = {0, .01f, 0, .01f};
        auto env = render(p);
        tail = 0;
        for (size_t i = 4000; i < env.size(); ++i)
            tail += std::abs(env[i]);
        check(tail < 1e-6, "individual oscillator envelope gates source");
        SourcePlayback player;
        for (int i = 0; i < 500; ++i)
            player.tick(*data, 2, 1, 48000, 0, 1, true, false, .08f, 16, 0, 0, 0);
        auto other = std::make_shared<SourceData>(*data);
        other->samples[0].resize(20);
        other->samples[1].resize(20);
        auto swapped = player.tick(*other, 2, 1, 48000, 0, 1, true, false, .08f, 16, 0, 0, 0);
        check(std::isfinite(swapped[0]) && player.cursor < 2,
              "asset replacement resets playback cursor");
        ChordMap map;
        map.set(60, 1);
        map.set(64, 2);
        ChordMapper mapper;
        std::vector<MidiEvent> notes;
        auto emit = [&](MidiEvent event) { notes.push_back(event); };
        mapper.input({0, EventType::NoteOn, 1, 60, 1}, true, map, emit);
        mapper.input({1, EventType::NoteOn, 1, 64, 1}, true, map, emit);
        notes.clear();
        mapper.input({2, EventType::NoteOff, 1, 60, 0}, true, map, emit);
        for (auto e : notes)
            check(!(e.type == EventType::NoteOff && (e.key == 64 || e.key == 67)),
                  "overlapping chords retain shared notes");
        notes.clear();
        mapper.input({3, EventType::NoteOff, 1, 64, 0}, true, map, emit);
        check(notes.size() == 3, "last chord releases all remaining notes");
        ArpParams ap;
        ap.enabled = true;
        ap.pattern = true;
        ap.sync = false;
        ap.time = .1f;
        ap.lengths = {3, 2, 4, 5, 3, 2};
        for (int i = 0; i < 32; ++i) {
            ap.lanes[size_t(i)].note = i % 3;
            ap.lanes[size_t(i)].octave = i % 2;
            ap.lanes[size_t(i)].repeats = 2;
        }
        auto sequence = [&](int block) {
            Arpeggiator arp;
            arp.prepare(1000);
            std::vector<MidiEvent> result;
            int absolute = 0;
            auto out = [&](MidiEvent event) {
                event.time += absolute;
                result.push_back(event);
            };
            arp.begin(ap, out);
            for (int key : {60, 64, 67})
                arp.input({0, EventType::NoteOn, 1, key, 1}, ap, out);
            for (absolute = 0; absolute < 1000; absolute += block)
                arp.advance(0, std::min(block, 1000 - absolute), ap, out);
            return result;
        };
        auto a = sequence(1000), b = sequence(31);
        check(a.size() == b.size(), "pattern event count block invariant");
        std::vector<MidiEvent> on;
        for (size_t i = 0; i < a.size(); ++i) {
            check(a[i].time == b[i].time && a[i].type == b[i].type && a[i].key == b[i].key,
                  "pattern scheduling invariant");
            if (a[i].type == EventType::NoteOn)
                on.push_back(a[i]);
        }
        check(on.size() == 20 && on[1].time == 50 && on[2].key == 76 && on[4].key == 67 &&
                  on[6].key == 72,
              "ratchets and independent note/octave periods");
        ap.lanes[0].chance = 37;
        ap.lanes[1].chance = 61;
        auto probabilityA = sequence(1000), probabilityB = sequence(31);
        check(probabilityA.size() == probabilityB.size(),
              "probability count independent of block size");
        for (size_t i = 0; i < probabilityA.size(); ++i)
            check(probabilityA[i].time == probabilityB[i].time &&
                      probabilityA[i].type == probabilityB[i].type &&
                      probabilityA[i].key == probabilityB[i].key,
                  "probability scheduling independent of block size");
        ap.lanes[0].chance = 0;
        ap.lanes[1].chance = 0;
        auto rest = sequence(31);
        for (auto e : rest)
            check(e.type != EventType::NoteOn, "zero probability is a rest");
        ap.lanes[0].chance = 100;
        ap.lanes[1].chance = 100;
        ap.latch = true;
        Arpeggiator arp;
        arp.prepare(1000);
        notes.clear();
        arp.begin(ap, emit);
        arp.input({0, EventType::NoteOn, 1, 60, 1}, ap, emit);
        arp.advance(0, 10, ap, emit);
        arp.input({10, EventType::NoteOff, 1, 60, 0}, ap, emit);
        notes.clear();
        arp.advance(10, 300, ap, emit);
        check(std::any_of(notes.begin(), notes.end(),
                          [](auto e) { return e.type == EventType::NoteOn && e.time >= 100; }),
              "latch continues after release");
        arp.stop(300, emit);
        notes.clear();
        arp.advance(300, 600, ap, emit);
        check(notes.empty(), "stop clears latched and repeated events");
        std::cout << "PASS: four engines in all four slots at 44.1/48/96 kHz, frame morph, warp, "
                     "sample loop/end/reverse, grain modulation, source envelope, shared chord "
                     "notes, polymetric lanes, ratchets, probability, latch, block invariance\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
