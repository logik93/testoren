#pragma once
#include "Engine.h"
#include <iostream>
#include <stdexcept>
namespace envelope_click_test {
using namespace logisth;
inline void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void stages(double rate) {
    Envelope control;
    control.on();
    check(control.tick({0, 0, 0, 0}, rate) == 0,
          "all-zero control ADSR skips the peak instead of emitting an impulse");
    control.on();
    check(control.tick({0, 0, .37f, 0}, rate) == .37f,
          "zero attack/decay enters the actual sustain on the same sample");
    control.releaseNote(0, rate);
    check(control.tick({0, 0, .37f, 0}, rate) == 0 && !control.active(),
          "modulation envelope retains an instantaneous zero release");
    control.on();
    check(std::abs(control.tick({.01f, .02f, .5f, .03f}, rate) - 1.f / float(.01f * rate)) < 1e-7f,
          "nonzero modulation attack retains its exact unsmoothed value");

    // A constant carrier isolates envelope edges from the waveform's own edges.
    // Test audible behaviour, without duplicating the smoothing implementation.
    for (float sustain : {0.f, .37f, 1.f}) {
        AudioEnvelope amp;
        EnvelopeParams p{0, 0, sustain, 0};
        amp.on();
        float previous = 0;
        int tailLimit = int(std::ceil(.0011 * rate));
        for (int i = 0; i < 512; ++i) {
            float value = amp.tick(p, rate);
            check(value >= 0 && value <= sustain + 1e-6f,
                  "amplitude edge never overshoots its requested sustain");
            check(value >= previous - 1e-6f && value - previous <= .05f,
                  "zero attack has a bounded monotonic amplitude edge");
            if (i == 0)
                check(value == 0, "a new note starts at silence regardless of carrier phase");
            if (i >= tailLimit)
                check(std::abs(value - sustain) < 1e-6f,
                      "anti-click smoothing reaches exact unity within 1.1 ms");
            previous = value;
        }
        amp.releaseNote(0, rate);
        check(amp.tick(p, rate) == previous,
              "zero release preserves the amplitude at the note-off boundary");
        for (int i = 0; i < tailLimit; ++i) {
            float value = amp.tick(p, rate);
            check(value >= 0 && value <= previous + 1e-6f && previous - value <= .05f,
                  "zero release fades monotonically without a hard amplitude cut");
            previous = value;
        }
        check(previous == 0 && !amp.active(), "release tail reaches exact silence and retires");
    }
    for (int length : {1, 2, 7, 17, 49}) {
        AudioEnvelope amp;
        EnvelopeParams p{0, 0, 1, 0};
        amp.on();
        for (int i = 0; i < length; ++i)
            amp.tick(p, rate);
        amp.releaseNote(0, rate);
        for (int i = 0; i < int(rate * .002); ++i) {
            float value = amp.tick(p, rate);
            check(value >= 0 && value <= 1, "sub-millisecond notes stay finite and bounded");
        }
        check(!amp.active() && amp.tick(p, rate) == 0,
              "very short notes finish their bounded tail without a stuck voice");
    }
}
inline Params fixture(int slot, int engine, int wave, int phase, const SourceData *data) {
    Params p;
    for (auto &osc : p.osc)
        osc.level = 0;
    auto &osc = p.osc[size_t(slot)];
    osc.level = 1;
    osc.semitone = 0;
    osc.stereo = 0;
    osc.engine = std::min(engine, 3);
    osc.gran.enabled = engine == 4;
    osc.wave = wave;
    osc.phase = float(phase);
    osc.retrigger = phase >= 0;
    osc.data = data;
    for (auto &part : p.part) {
        part.filterType = 3;
        part.level = 1;
        part.amp = {0, 0, 1, 0};
    }
    return p;
}
inline void voices(double rate, const SourceData *data) {
    int cases = 0;
    for (int mode = 0; mode < 5; ++mode) {
        auto p = fixture(0, mode, 0, 90, data);
        Engine engine;
        engine.prepare(rate);
        engine.noteOn(69, 1, 1, p);
        std::array<float, 2048> left{}, right{};
        engine.render(left.data(), right.data(), int(left.size()), p);
        check(std::any_of(left.begin(), left.end(), [](float v) { return std::abs(v) > .001f; }),
              "nonzero-sustain fixtures produce audio in every engine and both grain models");
    }
    for (int slot = 0; slot < 4; ++slot)
        for (int mode = 0; mode < 5; ++mode)
            for (int wave = 0; wave < (mode == 0 ? 8 : 1); ++wave)
                for (int phase : {0, 90, 217, -1})
                    for (bool sourceEnvelope : {false, true}) {
                        auto p = fixture(slot, mode, wave, phase, data);
                        if (sourceEnvelope) {
                            p.osc[size_t(slot)].envelopeEnabled = true;
                            p.osc[size_t(slot)].envelope = {0, 0, 0, 0};
                        } else
                            for (auto &part : p.part)
                                part.amp = {0, 0, 0, 0};
                        Engine engine;
                        engine.prepare(rate);
                        engine.noteOn(69, 1, 1, p);
                        std::array<float, 257> left{}, right{};
                        for (int block = 0; block < 3; ++block) {
                            if (block == 1)
                                engine.noteOff(69, 1, p);
                            engine.render(left.data(), right.data(), int(left.size()), p);
                            for (size_t i = 0; i < left.size(); ++i)
                                check(left[i] == 0 && right[i] == 0,
                                      "all-zero part/source ADSR produces digital silence in "
                                      "every oscillator slot, waveform, engine and phase");
                        }
                        check(engine.activeVoices() == 0, "zero-ADSR notes retire after note-off");
                        ++cases;
                    }

    // Compare a released note with a continuously held, identically phased
    // carrier. Noise/saw discontinuities are legitimate waveform content, so
    // testing raw adjacent-sample differences would misidentify them as clicks.
    for (int slot = 0; slot < 4; ++slot)
        for (int wave = 0; wave < 8; ++wave)
            for (bool sourceEnvelope : {false, true}) {
                auto p = fixture(slot, 0, wave, 90, data);
                p.osc[size_t(slot)].envelopeEnabled = sourceEnvelope;
                p.osc[size_t(slot)].envelope = {0, 0, 1, 0};
                Engine released, held;
                released.prepare(rate);
                held.prepare(rate);
                released.noteOn(69, 1, 1, p);
                held.noteOn(69, 1, 1, p);
                std::array<float, 512> left{}, right{}, reference{}, refRight{};
                released.render(left.data(), right.data(), 317, p);
                held.render(reference.data(), refRight.data(), 317, p);
                check(left[0] == 0 && right[0] == 0, "phase-independent silent note onset");
                released.noteOff(69, 1, p);
                released.render(left.data(), right.data(), 512, p);
                held.render(reference.data(), refRight.data(), 512, p);
                check(left[0] == reference[0] && right[0] == refRight[0],
                      "note-off boundary follows the carrier instead of abruptly muting it");
                float previous = 1;
                int tailLimit = int(std::ceil(rate * .0011));
                for (int i = 0; i < 512; ++i) {
                    if (i >= tailLimit)
                        check(left[size_t(i)] == 0 && right[size_t(i)] == 0,
                              "zero release completes within 1.1 ms in both channels");
                    if (std::abs(reference[size_t(i)]) > .02f) {
                        float gain = left[size_t(i)] / reference[size_t(i)];
                        check(gain >= -1e-6f && gain <= previous + 1e-5f,
                              "release envelope attenuates the actual carrier monotonically");
                        previous = gain;
                    }
                }
                check(released.activeVoices() == 0, "smoothed release retires the engine voice");
            }
    std::cout << "PASS anti-click @ " << rate << ": " << cases
              << " silent ADSR fixtures, 64 waveform release comparisons, short-note tails\n";
}
inline void run() {
    auto data = SourceData::factory();
    for (double rate : {44100., 48000., 96000.}) {
        stages(rate);
        voices(rate, data.get());
    }
}
} // namespace envelope_click_test
