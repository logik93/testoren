#pragma once
#include "Modulation.h"
#include <cstdint>
namespace logisth {
struct ArpStep {
    int transpose = 0, velocity = 100;
    bool hold = false;
};
struct ArpParams {
    struct GraphStep {
        int note = 8, random = 0, octave = 0, pitch = 0, velocity = 100, gate = 100, late = 0,
            harmony = 0;
        bool mute = false, reset = false;
    };
    struct GraphParams {
        bool enabled = false, retrigger = true, link = true, chordMode = false;
        int division = 5;
        float freeHz = 8;
        std::array<int, 8> lengths{16, 16, 16, 16, 16, 16, 16, 16},
            divisors{1, 1, 1, 1, 1, 1, 1, 1};
        std::array<bool, 7> degrees{true, true, true, true, true, true, true};
        std::array<int, 12> constraints{};
        std::array<GraphStep, 32> steps{};
    } graphs;
    bool clockAvailable = false, transportPlaying = true, positionValid = false;
    double ppq = 0;
    bool enabled = false, sync = true, pattern = false, latch = false;
    int mode = 0, velocityMode = 3, division = 4, octaves = 1, wrap = 0, steps = 16;
    float time = .125f, gate = .5f, swing = 0;
    double bpm = 120;
    std::array<ArpStep, 32> sequence{};
    struct Lane {
        int note = 0, octave = 0, gate = 50, repeats = 1, chance = 100;
    };
    std::array<Lane, 32> lanes{};
    std::array<int, 6> lengths{16, 16, 16, 16, 16, 16};
};
enum class EventType { NoteOn, NoteOff, Controller, PitchWheel, Aftertouch, StepVelocity };
struct MidiEvent {
    int time = 0;
    EventType type = EventType::NoteOn;
    int channel = 1, key = 60;
    float value = 0;
};

} // namespace logisth
