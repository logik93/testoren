#include "Arpeggiator.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
std::vector<MidiEvent> run(ArpParams p, int block) {
    Arpeggiator arp;
    arp.prepare(1000);
    std::vector<MidiEvent> events;
    int absolute = 0;
    auto emit = [&](MidiEvent e) {
        e.time += absolute;
        events.push_back(e);
    };
    arp.begin(p, emit);
    for (int note : {60, 64, 67})
        arp.input({0, EventType::NoteOn, 1, note, .8f}, p, emit);
    for (absolute = 0; absolute < 1000; absolute += block)
        arp.advance(0, std::min(block, 1000 - absolute), p, emit);
    return events;
}
int main() {
    try {
        ArpParams p;
        p.enabled = true;
        p.sync = false;
        p.time = .1f;
        p.steps = 4;
        auto events = run(p, 1000);
        std::vector<MidiEvent> notes;
        for (auto e : events)
            if (e.type == EventType::NoteOn)
                notes.push_back(e);
        check(notes.size() == 10 && notes[0].time == 0 && notes[1].time == 100 &&
                  notes[0].key == 60 && notes[1].key == 64 && notes[2].key == 67,
              "up sequence and exact timing");
        bool off50 = false;
        for (auto e : events)
            off50 |= e.type == EventType::NoteOff && e.time == 50;
        check(off50, "gate timing");
        for (int mode = 0; mode < 10; ++mode) {
            p.mode = mode;
            auto a = run(p, 1000), b = run(p, 37);
            check(a.size() == b.size(), "block invariant event count");
            for (size_t i = 0; i < a.size(); ++i)
                check(a[i].time == b[i].time && a[i].key == b[i].key && a[i].type == b[i].type &&
                          a[i].value == b[i].value,
                      "block invariant event order");
        }
        p.mode = 8;
        p.sequence[0].transpose = 12;
        p.sequence[0].hold = true;
        p.sequence[1].transpose = 7;
        p.sequence[2].velocity = 0;
        events = run(p, 1000);
        notes.clear();
        for (auto e : events)
            if (e.type == EventType::NoteOn)
                notes.push_back(e);
        check(notes[0].key == 72 && notes[1].key == 67 && notes[2].time == 300,
              "step transpose/rest");
        for (auto e : events)
            check(!(e.type == EventType::NoteOff && e.time == 50), "hold extends gate");
        p = ArpParams{};
        p.enabled = true;
        p.sync = false;
        p.time = .1f;
        p.swing = .5f;
        events = run(p, 37);
        notes.clear();
        for (auto e : events)
            if (e.type == EventType::NoteOn)
                notes.push_back(e);
        check(notes[1].time == 150 && notes[2].time == 200, "swing preserves pair length");
        Arpeggiator arp;
        arp.prepare(1000);
        std::vector<MidiEvent> live;
        auto emit = [&](MidiEvent e) { live.push_back(e); };
        arp.begin(p, emit);
        arp.input({0, EventType::NoteOn, 1, 60, 1}, p, emit);
        arp.advance(0, 10, p, emit);
        arp.input({10, EventType::Controller, 1, 64, 127}, p, emit);
        arp.input({10, EventType::NoteOff, 1, 60, 0}, p, emit);
        arp.advance(10, 200, p, emit);
        arp.input({200, EventType::Controller, 1, 64, 0}, p, emit);
        size_t end = live.size();
        arp.advance(200, 1000, p, emit);
        check(live.size() == end, "pedal release stops arp");
        p.sync = true;
        p.division = 10;
        p.bpm = 120;
        p.swing = 0;
        events = run(p, 37);
        notes.clear();
        for (auto e : events)
            if (e.type == EventType::NoteOn)
                notes.push_back(e);
        check(notes[1].time == 500, "tempo sync");
        std::cout << "PASS: 10 arp modes, timing, variable blocks, gate, hold, rest, transpose, "
                     "swing, sustain, host tempo\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
