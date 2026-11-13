#pragma once
#include "ArpTypes.h"
namespace logisth {
class GraphArpeggiator {
    struct Key {
        int note = -1, channel = 1;
        float velocity = 0;
        bool down = false;
    };
    struct Plan {
        bool active = false;
        double due = 0, period = 1;
        uint64_t tick = 0, motion = 0;
    };
    std::array<Key, 128> keys{}, playing{};
    std::array<Plan, 4> plans{};
    std::array<bool, 16> sustain{};
    std::array<int, 8> playheads{};
    int playingCount = 0;
    double sampleRate = 44100, untilPlan = 0, untilOff = -1;
    bool enabled = false, initial = true, latched = false, previousLatch = false,
         hostAligned = false;
    double previousPeriod = 0;
    uint64_t tick = 0, motion = 0;
    uint32_t random = 0x154897;
    uint32_t roll() {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        return random;
    }
    bool held() const {
        return std::any_of(keys.begin(), keys.end(), [](const auto &k) { return k.note >= 0; });
    }
    static int index(uint64_t at, int lane, const ArpParams &p) {
        int length = std::clamp(p.graphs.lengths[size_t(p.graphs.link ? 0 : lane)], 1, 32);
        return int((at / uint64_t(std::clamp(p.graphs.divisors[size_t(lane)], 1, 16))) %
                   uint64_t(length));
    }
    static int value(uint64_t at, int lane, const ArpParams &p) {
        const auto &s = p.graphs.steps[size_t(index(at, lane, p))];
        switch (lane) {
        case 0:
            return s.note;
        case 1:
            return s.random;
        case 2:
            return s.octave;
        case 3:
            return s.pitch;
        case 4:
            return s.velocity;
        case 5:
            return s.gate;
        case 6:
            return s.late;
        default:
            return s.harmony;
        }
    }
    template <class Emit> void off(int time, Emit &&emit) {
        for (int i = 0; i < playingCount; ++i)
            emit(MidiEvent{time, EventType::NoteOff, playing[size_t(i)].channel,
                           playing[size_t(i)].note, 0});
        playingCount = 0;
        untilOff = -1;
        emit(MidiEvent{time, EventType::StepVelocity, 1, 0, 0});
    }
    void restart() {
        plans = {};
        tick = motion = 0;
        untilPlan = 0;
        initial = true;
        hostAligned = false;
    }
    template <class Emit> void play(int time, const Plan &plan, const ArpParams &p, Emit &&emit) {
        off(time, emit);
        for (int lane = 0; lane < 8; ++lane)
            playheads[size_t(lane)] = index(plan.tick, lane, p);
        std::array<Key, 128> sorted{};
        int count = 0;
        for (auto k : keys)
            if (k.note >= 0)
                sorted[size_t(count++)] = k;
        if (!count)
            return;
        std::sort(sorted.begin(), sorted.begin() + count, [](const Key &a, const Key &b) {
            return a.note < b.note || (a.note == b.note && a.channel < b.channel);
        });
        const auto &selection = p.graphs.steps[size_t(playheads[0])];
        int choice = value(plan.tick, 0, p), gate = value(plan.tick, 5, p),
            velocity = value(plan.tick, 4, p);
        if (choice < 0 || selection.mute || gate <= 0 || velocity <= 0)
            return;
        int ordinal = 0, position = int(plan.motion % uint64_t(std::max(1, count * 2)));
        if (choice <= 7)
            ordinal = choice;
        else
            switch (choice) {
            case 8:
                ordinal = int(plan.motion % uint64_t(count));
                break;
            case 9:
                ordinal = count - 1 - int(plan.motion % uint64_t(count));
                break;
            case 10:
            case 11: {
                int span = std::max(1, 2 * count - 2), at = int(plan.motion % uint64_t(span));
                ordinal = at < count ? at : span - at;
                if (choice == 11)
                    ordinal = count - 1 - ordinal;
                break;
            }
            case 12:
            case 13:
                ordinal = position < count ? position : 2 * count - 1 - position;
                if (choice == 13)
                    ordinal = count - 1 - ordinal;
                break;
            case 14:
                ordinal = plan.motion % 2 == 0
                              ? 0
                              : 1 + int((plan.motion / 2) % uint64_t(std::max(1, count - 1)));
                break;
            default:
                ordinal = plan.motion % 2 == 0
                              ? count - 1
                              : int((plan.motion / 2) % uint64_t(std::max(1, count - 1)));
                break;
            }
        int randomness = value(plan.tick, 1, p);
        if (randomness != 0)
            ordinal += (randomness < 0 ? -1 : 1) * int(roll() % uint32_t(std::abs(randomness) + 1));
        ordinal = ((ordinal % count) + count) % count;
        int octave = value(plan.tick, 2, p) * 12, shift = value(plan.tick, 3, p);
        std::array<int, 128> pitches{};
        for (int i = 0; i < count; ++i)
            pitches[size_t(i)] = sorted[size_t(i)].note;
        int root = chordRoot(pitches.data(), count);
        static constexpr int degrees[12] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6};
        auto transpose = [&](int note) {
            int degree = degrees[(note - root + 120) % 12];
            return note + octave + (p.graphs.degrees[size_t(degree)] ? shift : 0);
        };
        auto add = [&](Key key, int note) {
            if (note < 0 || note > 127 || playingCount >= 128)
                return;
            int constraint = p.graphs.constraints[size_t(note % 12)];
            if (constraint == 1)
                return;
            if (constraint == 2)
                ++note;
            if (constraint == 3)
                --note;
            if (note < 0 || note > 127)
                return;
            for (int i = 0; i < playingCount; ++i)
                if (playing[size_t(i)].note == note && playing[size_t(i)].channel == key.channel)
                    return;
            key.note = note;
            key.velocity = clamp(key.velocity * velocity * .01f, 0, 1);
            playing[size_t(playingCount++)] = key;
            emit(MidiEvent{time, EventType::NoteOn, key.channel, key.note, key.velocity});
        };
        auto selected = sorted[size_t(ordinal)];
        int target = transpose(selected.note);
        add(selected, target);
        if (p.graphs.chordMode)
            for (int i = 0; i < count; ++i) {
                auto key = sorted[size_t(i)];
                int note = key.note;
                while (note < selected.note)
                    note += 12;
                note = transpose(note);
                while (note < target)
                    note += 12;
                add(key, note);
            }
        int harmony = value(plan.tick, 7, p);
        if (harmony != 0)
            add(selected, target + harmony);
        untilOff = playingCount ? std::max(1., std::round(plan.period * gate * .01)) : -1;
        emit(MidiEvent{time, EventType::StepVelocity, 1, 0,
                       playingCount ? clamp(velocity * .01f, 0, 1) : 0});
    }

  public:
    static int chordRoot(const int *notes, int count) {
        if (count <= 0)
            return 0;
        std::array<bool, 12> present{};
        for (int i = 0; i < count; ++i)
            if (notes[i] >= 0)
                present[size_t(notes[i] % 12)] = true;
        static constexpr int templates[][4] = {{0, 4, 7, -1}, {0, 3, 7, -1}, {0, 3, 6, -1},
                                               {0, 4, 8, -1}, {0, 2, 7, -1}, {0, 5, 7, -1},
                                               {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 4, 7, 10},
                                               {0, 3, 6, 9},  {0, 3, 6, 10}};
        int bestRoot = notes[0] % 12, bestScore = -10000;
        for (int root = 0; root < 12; ++root)
            if (present[size_t(root)])
                for (const auto &shape : templates) {
                    int score = 0;
                    std::array<bool, 12> model{};
                    for (int degree : shape)
                        if (degree >= 0)
                            model[size_t((root + degree) % 12)] = true;
                    for (int pc = 0; pc < 12; ++pc) {
                        if (model[size_t(pc)] && present[size_t(pc)])
                            score += 5;
                        else if (model[size_t(pc)])
                            score -= 7;
                        else if (present[size_t(pc)])
                            score -= 2;
                    }
                    if (root == notes[0] % 12)
                        ++score;
                    if (score > bestScore) {
                        bestScore = score;
                        bestRoot = root;
                    }
                }
        return bestRoot;
    }
    template <class Emit>
    void alignHost(int start, const ArpParams &p, double period, Emit &&emit) {
        off(start, emit);
        plans = {};
        double beats = std::exp2(double(std::clamp(p.graphs.division, 0, 13) - 7));
        double now = p.ppq + double(start) / sampleRate * std::max(1., p.bpm) / 60;
        double exact = std::clamp(now / beats, 0., 1e12), whole = std::floor(exact),
               phase = (exact - whole) * period;
        uint64_t current = uint64_t(whole);
        double due = value(current, 6, p) * period * .01 - phase;
        if (due >= -.0001)
            plans[0] = {true, std::max(0., due), period, current, current};
        tick = current + 1;
        motion = tick;
        initial = false;
        untilPlan = period * .5 - phase;
        if (untilPlan < 0) {
            double upcoming = period + value(tick, 6, p) * period * .01 - phase;
            if (upcoming >= 0)
                plans[1] = {true, upcoming, period, tick, motion};
            ++tick;
            ++motion;
            untilPlan += period;
        }
        hostAligned = true;
    }
    void prepare(double sr) {
        sampleRate = std::max(1., sr);
        reset();
    }
    void reset() {
        keys = {};
        playing = {};
        plans = {};
        sustain = {};
        playheads.fill(-1);
        playingCount = 0;
        untilOff = -1;
        enabled = false;
        latched = previousLatch = false;
        random = 0x154897;
        previousPeriod = 0;
        restart();
    }
    int playhead(int lane) const {
        return playheads[size_t(std::clamp(lane, 0, 7))];
    }
    template <class Emit> void stop(int time, Emit &&emit) {
        off(time, emit);
        for (int ch = 1; ch <= 16; ++ch) {
            emit(MidiEvent{time, EventType::Controller, ch, 64, 0});
            emit(MidiEvent{time, EventType::Controller, ch, 123, 0});
        }
        keys = {};
        sustain = {};
        latched = false;
        restart();
        playheads.fill(-1);
    }
    template <class Emit> void begin(const ArpParams &p, Emit &&emit) {
        if (enabled != p.enabled) {
            stop(0, emit);
            enabled = p.enabled;
        }
    }
    template <class Emit> void input(MidiEvent e, const ArpParams &p, Emit &&emit) {
        if (!p.enabled) {
            emit(e);
            return;
        }
        if (e.channel < 1 || e.channel > 16)
            return;
        if (e.type == EventType::NoteOn && e.value > 0) {
            if (latched) {
                keys = {};
                latched = false;
            }
            auto key = std::find_if(keys.begin(), keys.end(), [&](auto &k) {
                return k.note == e.key && k.channel == e.channel;
            });
            if (key == keys.end())
                key = std::find_if(keys.begin(), keys.end(), [](auto &k) { return k.note < 0; });
            if (key != keys.end())
                *key = {e.key, e.channel, e.value, true};
            if (p.graphs.retrigger) {
                off(e.time, emit);
                restart();
            }
        } else if (e.type == EventType::NoteOff || (e.type == EventType::NoteOn && e.value <= 0)) {
            for (auto &k : keys)
                if (k.note == e.key && k.channel == e.channel) {
                    k.down = false;
                    if (!p.latch && !sustain[size_t(e.channel - 1)])
                        k = Key{};
                }
            latched = p.latch && held() && std::none_of(keys.begin(), keys.end(), [](auto &k) {
                          return k.note >= 0 && k.down;
                      });
            if (!held())
                off(e.time, emit);
        } else if (e.type == EventType::Controller && e.key == 64) {
            sustain[size_t(e.channel - 1)] = e.value >= 64;
            if (e.value < 64 && !p.latch) {
                for (auto &k : keys)
                    if (k.channel == e.channel && !k.down)
                        k = Key{};
                if (!held())
                    off(e.time, emit);
            }
        } else if (e.type == EventType::Controller && (e.key == 120 || e.key == 123)) {
            for (auto &k : keys)
                if (k.channel == e.channel)
                    k = Key{};
            off(e.time, emit);
            emit(e);
        } else
            emit(e);
    }
    template <class Emit> void advance(int start, int end, const ArpParams &p, Emit &&emit) {
        if (!p.enabled || (p.sync && p.clockAvailable && !p.transportPlaying))
            return;
        if (previousLatch && !p.latch) {
            for (auto &k : keys)
                if (k.note >= 0 && !k.down && !sustain[size_t(k.channel - 1)])
                    k = Key{};
            latched = false;
            if (!held())
                off(start, emit);
        }
        previousLatch = p.latch;
        double period = p.sync ? sampleRate * 60 / std::max(1., p.bpm) *
                                     std::exp2(double(std::clamp(p.graphs.division, 0, 13) - 7))
                               : sampleRate / std::max(.01f, p.graphs.freeHz);
        period = std::max(1., period);
        if (previousPeriod > 0 && std::abs(previousPeriod - period) > .00001) {
            double ratio = period / previousPeriod;
            untilPlan *= ratio;
            if (untilOff >= 0)
                untilOff *= ratio;
            for (auto &plan : plans)
                if (plan.active) {
                    plan.due *= ratio;
                    plan.period *= ratio;
                }
        }
        previousPeriod = period;
        if (!p.graphs.retrigger && p.sync && p.clockAvailable && p.positionValid && !hostAligned)
            alignHost(start, p, period, emit);
        for (int at = start; at < end; ++at) {
            if (untilOff >= 0 && --untilOff <= 0)
                off(at, emit);
            if (untilPlan <= 0) {
                int noteIndex = index(tick, 0, p);
                if (p.graphs.steps[size_t(noteIndex)].reset &&
                    tick % uint64_t(std::clamp(p.graphs.divisors[0], 1, 16)) == 0)
                    motion = 0;
                for (auto &plan : plans)
                    if (!plan.active) {
                        plan = {true,
                                std::max(0., (initial ? 0. : period * .5) +
                                                 value(tick, 6, p) * period * .01),
                                period, tick, motion};
                        break;
                    }
                ++tick;
                ++motion;
                untilPlan += initial ? period * .5 : period;
                initial = false;
            }
            for (auto &plan : plans)
                if (plan.active) {
                    if (plan.due <= 0) {
                        play(at, plan, p, emit);
                        plan.active = false;
                    } else
                        --plan.due;
                }
            --untilPlan;
        }
    }
};
} // namespace logisth
