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
        for (int mode = 0; mode < 2; ++mode) {
            auto params = ArpParams{};
            params.enabled = true;
            params.pattern = mode > 0;
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
                     "spacing/envelope/order/freeze/key/transient/tempo, "
                     "Legacy and Pattern disable/sustain/latch, and scope\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
