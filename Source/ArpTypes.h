#pragma once
#include "Modulation.h"
#include <cstdint>
namespace logisth {
struct ArpStep {
    int transpose = 0, velocity = 100;
    bool hold = false;
};
struct ArpParams {
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
