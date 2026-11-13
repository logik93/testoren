#pragma once
#include "Arpeggiator.h"
namespace logisth {
struct ChordMap {
    struct Chord {
        int count = 0;
        std::array<int, 8> notes{};
    };
    std::array<Chord, 128> keys{};
    void set(int trigger, int type) {
        trigger = std::clamp(trigger, 0, 127);
        static constexpr int intervals[][8] = {
            {0},       {0, 4, 7}, {0, 3, 7},  {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 4, 7, 10},
            {0, 2, 7}, {0, 5, 7}, {0, 7, 12}, {0, 3, 6},     {0, 4, 8}};
        static constexpr int counts[] = {1, 3, 3, 4, 4, 4, 3, 3, 3, 3, 3};
        auto &c = keys[size_t(std::clamp(trigger, 0, 127))];
        c = {};
        type = std::clamp(type, 0, 10);
        for (int i = 0; i < counts[type]; ++i)
            if (trigger + intervals[type][i] <= 127)
                c.notes[size_t(c.count++)] = trigger + intervals[type][i];
    }
};
class ChordMapper {
    struct Held {
        bool active = false;
        ChordMap::Chord chord{};
    };
    std::array<std::array<Held, 128>, 16> held{};
    std::array<std::array<int, 128>, 16> references{};
    bool enabled = false;

  public:
    void reset() {
        held = {};
        references = {};
        enabled = false;
    }
    template <class Emit> void input(MidiEvent e, bool on, const ChordMap &map, Emit &&emit) {
        if (e.channel < 1 || e.channel > 16)
            return;
        if (enabled != on) {
            for (int ch = 1; ch <= 16; ++ch)
                emit(MidiEvent{e.time, EventType::Controller, ch, 123, 0});
            reset();
            enabled = on;
        }
        if (!on) {
            emit(e);
            return;
        }
        auto &channel = held[size_t(e.channel - 1)];
        auto &refs = references[size_t(e.channel - 1)];
        if (e.type == EventType::Controller && (e.key == 120 || e.key == 123)) {
            channel = {};
            refs = {};
            emit(e);
            return;
        }
        if (e.type != EventType::NoteOn && e.type != EventType::NoteOff) {
            emit(e);
            return;
        }
        if (e.key < 0 || e.key > 127)
            return;
        auto &h = channel[size_t(e.key)];
        auto release = [&] {
            if (h.active)
                for (int i = 0; i < h.chord.count; ++i) {
                    int note = h.chord.notes[size_t(i)];
                    if (--refs[size_t(note)] == 0)
                        emit(MidiEvent{e.time, EventType::NoteOff, e.channel, note, 0});
                }
            h = {};
        };
        if (e.type == EventType::NoteOff || e.value <= 0) {
            release();
            return;
        }
        release();
        h.active = true;
        h.chord = map.keys[size_t(e.key)];
        if (h.chord.count == 0) {
            h.chord.count = 1;
            h.chord.notes[0] = e.key;
        }
        for (int i = 0; i < h.chord.count; ++i) {
            int note = h.chord.notes[size_t(i)];
            if (refs[size_t(note)]++ == 0)
                emit(MidiEvent{e.time, EventType::NoteOn, e.channel, note, e.value});
        }
    }
};
} // namespace logisth
