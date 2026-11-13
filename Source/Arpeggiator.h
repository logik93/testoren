#pragma once
#include "ArpTypes.h"
#include "GraphArpeggiator.h"
#include <cstdint>
namespace logisth {
class Arpeggiator {
    GraphArpeggiator graph;
    bool graphMode = false;
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
    std::array<Key, 128> repeatNotes{};
    int repeatCount = 0, repeatsLeft = 0;
    double untilRepeat = -1, repeatPeriod = 1, repeatGate = 1;
    int currentStep = -1;
    bool latched = false, latchWasEnabled = false;
    uint32_t roll() {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        return random;
    }
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
                for (int octave = 0; octave < (p.pattern ? 1 : std::clamp(p.octaves, 1, 4));
                     ++octave)
                    if (key.note + 12 * octave < 128) {
                        sorted[count] = key;
                        sorted[count++].note += 12 * octave;
                    }
        if (!count)
            return;
        std::sort(sorted.begin(), sorted.begin() + count, [&](auto &a, auto &b) {
            return !p.pattern && p.mode == 7 ? a.age < b.age || (a.age == b.age && a.note < b.note)
                                             : a.note < b.note;
        });
        const int seq = step % std::clamp(p.pattern ? p.lengths[2] : p.steps, 1, 32);
        auto laneAt = [&](int lane) -> const ArpParams::Lane & {
            return p.lanes[size_t(step % std::clamp(p.lengths[size_t(lane)], 1, 32))];
        };
        const auto &noteLane = laneAt(0);
        const auto &octaveLane = laneAt(1);
        const auto &gateLane = laneAt(3);
        const auto &repeatLane = laneAt(4);
        const auto &chanceLane = laneAt(5);
        currentStep = step;
        const auto &spec = p.sequence[size_t(seq)];
        const int pos = p.wrap > 0 ? position % p.wrap : position;
        int index = pos % count;
        switch (p.pattern ? -1 : p.mode) {
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
        if (p.pattern)
            index = noteLane.note % count;
        const bool accepted = !p.pattern || int(roll() % 100) < chanceLane.chance;
        std::array<Key, 128> next{};
        int nextCount = 0;
        const int desired = !p.pattern && p.mode == 9 ? std::min(count, 128) : 1;
        for (int i = 0; i < desired; ++i) {
            auto key = sorted[size_t(!p.pattern && p.mode == 9 ? i : index)];
            if (p.pattern)
                key.note += octaveLane.octave * 12;
            else if (p.mode >= 8)
                key.note += spec.transpose;
            if (key.note < 0 || key.note > 127 || spec.velocity == 0 || !accepted)
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
        const int repeats = p.pattern ? std::clamp(repeatLane.repeats, 1, 4) : 1;
        repeatPeriod = interval / repeats;
        repeatGate =
            std::max(1., std::round(repeatPeriod * (p.pattern ? gateLane.gate / 100. : p.gate)));
        bool tie =
            p.pattern ? p.sequence[size_t(step % std::clamp(p.lengths[3], 1, 32))].hold : spec.hold;
        untilOff = tie && repeats == 1 ? -1 : repeatGate;
        repeatNotes = next;
        repeatCount = nextCount;
        repeatsLeft = repeats - 1;
        untilRepeat = repeatsLeft > 0 ? repeatPeriod : -1;
        ++step;
        ++position;
    }

  public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        graph.prepare(sampleRate);
        reset();
    }
    void reset() {
        graph.reset();
        graphMode = false;
        keys = {};
        playing = {};
        sustain = {};
        playingCount = step = position = 0;
        untilStep = 0;
        untilOff = -1;
        counter = 0;
        enabled = false;
        repeatsLeft = 0;
        untilRepeat = -1;
        currentStep = -1;
        latched = false;
    }
    int graphHead(int lane) const {
        return graphMode ? graph.playhead(lane) : -1;
    }
    int playhead() const {
        return graphMode ? graph.playhead(0) : currentStep;
    }
    template <class Emit> void begin(const ArpParams &p, Emit &&emit) {
        bool next = p.pattern && p.graphs.enabled;
        if (next != graphMode) {
            stop(0, emit);
            graphMode = next;
            enabled = false;
        }
        if (graphMode) {
            graph.begin(p, emit);
            return;
        }
        if (enabled != p.enabled) {
            off(0, emit);
            for (int ch = 1; ch <= 16; ++ch) {
                emit(MidiEvent{0, EventType::Controller, ch, 120, 0});
                emit(MidiEvent{0, EventType::Controller, ch, 64, 0});
            }
            keys = {};
            sustain = {};
            repeatsLeft = 0;
            untilRepeat = -1;
            latched = false;
            latchWasEnabled = false;
            currentStep = -1;
            step = position = 0;
            untilStep = 0;
            enabled = p.enabled;
        }
    }
    template <class Emit> void stop(int time, Emit &&emit) {
        if (graphMode) {
            graph.stop(time, emit);
            return;
        }
        off(time, emit);
        repeatsLeft = 0;
        untilRepeat = -1;
        currentStep = -1;
        latched = false;
        keys = {};
        sustain = {};
        untilStep = 0;
        step = position = 0;
        for (int ch = 1; ch <= 16; ++ch)
            emit(MidiEvent{time, EventType::Controller, ch, 123, 0});
    }
    template <class Emit> void input(MidiEvent event, const ArpParams &p, Emit &&emit) {
        if (graphMode) {
            graph.input(event, p, emit);
            return;
        }
        if (!p.enabled) {
            emit(event);
            return;
        }
        if (event.type == EventType::NoteOn && event.value > 0) {
            if (p.latch && latched) {
                keys = {};
                latched = false;
                repeatsLeft = 0;
                untilRepeat = -1;
            }
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
            if (p.latch) {
                for (auto &k : keys)
                    if (k.note == event.key && k.channel == event.channel)
                        k.down = false;
                latched = std::none_of(keys.begin(), keys.end(),
                                       [](const auto &k) { return k.note >= 0 && k.down; });
                return;
            }
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
        if (graphMode) {
            graph.advance(start, end, p, emit);
            return;
        }
        if (!p.enabled)
            return;
        if (latchWasEnabled && !p.latch) {
            for (auto &key : keys)
                if (key.note >= 0 && !key.down && !sustain[size_t(key.channel - 1)])
                    key = Key{};
            if (!anyKeys()) {
                off(start, emit);
                repeatsLeft = 0;
                untilRepeat = -1;
            }
            latched = false;
        }
        latchWasEnabled = p.latch;
        for (int s = start; s < end; ++s) {
            if (untilOff >= 0) {
                untilOff -= 1;
                if (untilOff <= 0)
                    off(s, emit);
            }
            if (repeatsLeft > 0 && anyKeys()) {
                if (--untilRepeat <= 0) {
                    off(s, emit);
                    playing = repeatNotes;
                    playingCount = repeatCount;
                    for (int i = 0; i < playingCount; ++i)
                        emit(MidiEvent{s, EventType::NoteOn, playing[i].channel, playing[i].note,
                                       playing[i].velocity});
                    if (playingCount)
                        emit(MidiEvent{s, EventType::StepVelocity, 1, 0, playing[0].velocity});
                    untilOff = repeatGate;
                    untilRepeat += repeatPeriod;
                    --repeatsLeft;
                }
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
