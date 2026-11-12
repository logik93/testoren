#pragma once
#include "Modulation.h"
#include <cstdint>
namespace logisth {
struct ArpStep {
    int transpose = 0, velocity = 100;
    bool hold = false;
};
struct ArpParams {
    bool enabled = false, sync = true;
    int mode = 0, velocityMode = 3, division = 4, octaves = 1, wrap = 0, steps = 16;
    float time = .125f, gate = .5f, swing = 0;
    double bpm = 120;
    std::array<ArpStep, 16> sequence{};
};
enum class EventType { NoteOn, NoteOff, Controller, PitchWheel, Aftertouch, StepVelocity };
struct MidiEvent {
    int time = 0;
    EventType type = EventType::NoteOn;
    int channel = 1, key = 60;
    float value = 0;
};
class Arpeggiator {
    struct Key {
        int note = -1, channel = 1;
        float velocity = 0;
        uint64_t age = 0;
        bool down = false;
    };
    std::array<Key, 128> keys{}, playing{};
    std::array<bool, 16> sustain{};
    int playingCount = 0, step = 0, position = 0;
    double untilStep = 0, untilOff = -1, sr = 44100;
    uint64_t counter = 0;
    uint32_t random = 0x57219;
    float lastVelocity = 1;
    bool enabled = false;
    bool anyKeys() const {
        return std::any_of(keys.begin(), keys.end(), [](auto &k) { return k.note >= 0; });
    }
    template <class Emit> void off(int time, Emit &&emit) {
        for (int i = 0; i < playingCount; ++i)
            emit(MidiEvent{time, EventType::NoteOff, playing[i].channel, playing[i].note, 0});
        playingCount = 0;
        untilOff = -1;
        emit(MidiEvent{time, EventType::StepVelocity, 1, 0, 0});
    }
    template <class Emit> void trigger(int time, const ArpParams &p, Emit &&emit) {
        std::array<Key, 512> sorted{};
        int count = 0;
        for (const auto &key : keys)
            if (key.note >= 0)
                for (int octave = 0; octave < std::clamp(p.octaves, 1, 4); ++octave)
                    if (key.note + 12 * octave < 128) {
                        sorted[count] = key;
                        sorted[count++].note += 12 * octave;
                    }
        if (!count)
            return;
        std::sort(sorted.begin(), sorted.begin() + count, [&](auto &a, auto &b) {
            return p.mode == 7 ? a.age < b.age || (a.age == b.age && a.note < b.note)
                               : a.note < b.note;
        });
        const int seq = step % std::clamp(p.steps, 1, 16);
        const auto &spec = p.sequence[size_t(seq)];
        const int pos = p.wrap > 0 ? position % p.wrap : position;
        int index = pos % count;
        switch (p.mode) {
        case 1:
            index = count - 1 - index;
            break;
        case 2:
        case 3: {
            int span = std::max(1, 2 * count - 2);
            int t = pos % span;
            index = t < count ? t : span - t;
            if (p.mode == 3)
                index = count - 1 - index;
            break;
        }
        case 4:
        case 5: {
            int t = pos % (2 * count);
            index = t < count ? t : 2 * count - 1 - t;
            if (p.mode == 5)
                index = count - 1 - index;
            break;
        }
        case 6:
            random ^= random << 13;
            random ^= random >> 17;
            random ^= random << 5;
            index = int(random % uint32_t(count));
            break;
        case 8:
        case 9:
            index = 0;
            break;
        default:
            break;
        }
        std::array<Key, 128> next{};
        int nextCount = 0;
        const int desired = p.mode == 9 ? std::min(count, 128) : 1;
        for (int i = 0; i < desired; ++i) {
            auto key = sorted[size_t(p.mode == 9 ? i : index)];
            if (p.mode >= 8)
                key.note += spec.transpose;
            if (key.note < 0 || key.note > 127 || spec.velocity == 0)
                continue;
            const float sv = spec.velocity / 127.f;
            switch (p.velocityMode) {
            case 0:
                break;
            case 1:
                key.velocity = lastVelocity;
                break;
            case 2:
                key.velocity = sv;
                break;
            case 3:
                key.velocity *= sv;
                break;
            case 4:
                key.velocity = lastVelocity * sv;
                break;
            }
            next[size_t(nextCount++)] = key;
        }
        // New notes precede old note-offs at a tied step so mono legato can glide.
        for (int i = 0; i < nextCount; ++i) {
            bool tied = false;
            for (int j = 0; j < playingCount; ++j)
                tied |= next[i].note == playing[j].note && next[i].channel == playing[j].channel;
            if (!tied)
                emit(MidiEvent{time, EventType::NoteOn, next[i].channel, next[i].note,
                               next[i].velocity});
        }
        for (int j = 0; j < playingCount; ++j) {
            bool tied = false;
            for (int i = 0; i < nextCount; ++i)
                tied |= next[i].note == playing[j].note && next[i].channel == playing[j].channel;
            if (!tied)
                emit(MidiEvent{time, EventType::NoteOff, playing[j].channel, playing[j].note, 0});
        }
        playing = next;
        playingCount = nextCount;
        emit(MidiEvent{time, EventType::StepVelocity, 1, 0, nextCount ? spec.velocity / 127.f : 0});
        const double period =
            p.sync ? 60 / p.bpm * beatLength(p.division) * sr : std::round(p.time * sr);
        const double interval = std::max(1., period * (step % 2 ? 1 - p.swing : 1 + p.swing));
        untilStep += interval;
        untilOff = spec.hold ? -1 : std::max(1., std::round(interval * p.gate));
        ++step;
        ++position;
    }

  public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        reset();
    }
    void reset() {
        keys = {};
        playing = {};
        sustain = {};
        playingCount = step = position = 0;
        untilStep = 0;
        untilOff = -1;
        counter = 0;
        enabled = false;
    }
    template <class Emit> void begin(const ArpParams &p, Emit &&emit) {
        if (enabled != p.enabled) {
            off(0, emit);
            for (int ch = 1; ch <= 16; ++ch) {
                emit(MidiEvent{0, EventType::Controller, ch, 120, 0});
                emit(MidiEvent{0, EventType::Controller, ch, 64, 0});
            }
            keys = {};
            sustain = {};
            step = position = 0;
            untilStep = 0;
            enabled = p.enabled;
        }
    }
    template <class Emit> void stop(int time, Emit &&emit) {
        off(time, emit);
        keys = {};
        sustain = {};
        untilStep = 0;
        step = position = 0;
        for (int ch = 1; ch <= 16; ++ch)
            emit(MidiEvent{time, EventType::Controller, ch, 123, 0});
    }
    template <class Emit> void input(MidiEvent event, const ArpParams &p, Emit &&emit) {
        if (!p.enabled) {
            emit(event);
            return;
        }
        if (event.type == EventType::NoteOn && event.value > 0) {
            bool wasEmpty = !anyKeys();
            auto key = std::find_if(keys.begin(), keys.end(), [&](auto &k) {
                return k.note == event.key && k.channel == event.channel;
            });
            if (key == keys.end())
                key = std::find_if(keys.begin(), keys.end(), [](auto &k) { return k.note < 0; });
            if (key != keys.end())
                *key = {event.key, event.channel, event.value, ++counter, true};
            lastVelocity = event.value;
            if (wasEmpty) {
                step = position = 0;
                untilStep = 0;
            }
            return;
        }
        if (event.type == EventType::NoteOff ||
            (event.type == EventType::NoteOn && event.value <= 0)) {
            for (auto &k : keys)
                if (k.note == event.key && k.channel == event.channel) {
                    if (sustain[size_t(k.channel - 1)])
                        k.down = false;
                    else
                        k = Key{};
                }
            if (!anyKeys())
                off(event.time, emit);
            return;
        }
        if (event.type == EventType::Controller && event.key == 64) {
            sustain[size_t(event.channel - 1)] = event.value >= 64;
            if (event.value < 64) {
                for (auto &k : keys)
                    if (k.channel == event.channel && !k.down)
                        k = Key{};
                if (!anyKeys())
                    off(event.time, emit);
            }
            return;
        }
        if (event.type == EventType::Controller && (event.key == 120 || event.key == 123)) {
            for (auto &k : keys)
                if (k.channel == event.channel)
                    k = Key{};
            off(event.time, emit);
        }
        emit(event);
    }
    template <class Emit> void advance(int start, int end, const ArpParams &p, Emit &&emit) {
        if (!p.enabled)
            return;
        for (int s = start; s < end; ++s) {
            if (untilOff >= 0) {
                untilOff -= 1;
                if (untilOff <= 0)
                    off(s, emit);
            }
            if (anyKeys()) {
                if (untilStep <= 0)
                    trigger(s, p, emit);
                untilStep -= 1;
            }
        }
    }
};
} // namespace logisth
