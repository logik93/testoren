#pragma once
#include "PluginProcessor.h"
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace gain_test {
inline void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void set(LogisthProcessor &p, const juce::String &id, float value) {
    auto *parameter = p.state.getParameter(id);
    require(parameter != nullptr, "gain fixture parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
inline void cleanSine(LogisthProcessor &p) {
    p.loadPreset(0);
    for (int i = 1; i <= 4; ++i) {
        auto id = "o" + juce::String(i);
        set(p, id + "level", i == 1 ? 1.f : 0.f);
        set(p, id + "voices", 1);
        set(p, id + "wave", 0);
        set(p, id + "octave", 0);
        set(p, id + "tune", 0);
        set(p, id + "fine", 0);
        set(p, id + "detune", 0);
        set(p, id + "stereo", 0);
        set(p, id + "pan", 0);
        set(p, id + "retrig", 1);
        set(p, id + "phase", 0);
    }
    for (int i = 1; i <= 2; ++i) {
        auto id = "p" + juce::String(i);
        set(p, id + "type", 3);
        set(p, id + "ampa", 0);
        set(p, id + "ampd", 0);
        set(p, id + "amps", 1);
        set(p, id + "ampr", .01f);
        set(p, "env" + juce::String(i) + "amt1", 0);
    }
    for (auto id : {"diston", "phaseron", "choruson", "eqon", "reverbon", "delayon", "compon",
                    "arpon", "chordon"})
        set(p, id, 0);
    set(p, "mixA", 1);
    set(p, "mixB", 1);
    set(p, "master", 1);
}
struct Reading {
    std::array<double, 2> peak{}, rms{};
    double dbPeak() const {
        return 20 * std::log10(std::max(1e-12, std::max(peak[0], peak[1])));
    }
};
inline Reading measure(LogisthProcessor &p, double rate, std::initializer_list<int> notes = {69},
                       float velocity = 1, int seconds = 2,
                       juce::AudioBuffer<float> *capture = nullptr, bool settled = true) {
    p.prepareToPlay(rate, 512);
    juce::AudioBuffer<float> block(2, 512);
    juce::MidiBuffer midi;
    for (int key : notes)
        midi.addEvent(juce::MidiMessage::noteOn(1, key, velocity), 0);
    if (capture) {
        capture->setSize(2, int(rate) * seconds);
        capture->clear();
    }
    Reading result;
    std::array<double, 2> squares{};
    int samples = 0;
    for (int offset = 0; offset < int(rate) * seconds; offset += 512) {
        int n = std::min(512, int(rate) * seconds - offset);
        block.setSize(2, n, false, false, true);
        p.processBlock(block, midi);
        midi.clear();
        if (capture)
            for (int ch = 0; ch < 2; ++ch)
                capture->copyFrom(ch, offset, block, ch, 0, n);
        for (int s = 0; s < n; ++s) {
            if (settled && offset + s < int(rate))
                continue; // settle envelopes, FX and gain smoothing
            ++samples;
            for (int ch = 0; ch < 2; ++ch) {
                double value = block.getSample(ch, s);
                require(std::isfinite(value), "calibration output finite");
                result.peak[size_t(ch)] = std::max(result.peak[size_t(ch)], std::abs(value));
                squares[size_t(ch)] += value * value;
            }
        }
    }
    for (int ch = 0; ch < 2; ++ch)
        result.rms[size_t(ch)] = std::sqrt(squares[size_t(ch)] / std::max(1, samples));
    return result;
}
inline void run(bool verify, const juce::File &folder = {}) {
    juce::String report = "case,sample_rate,peak_L,peak_R,rms_L,rms_R,peak_dBFS\n";
    auto row = [&](const char *name, double rate, const Reading &r) {
        report += juce::String(name) + "," + juce::String(rate, 0);
        for (double x : r.peak)
            report += "," + juce::String(x, 9);
        for (double x : r.rms)
            report += "," + juce::String(x, 9);
        report += "," + juce::String(r.dbPeak(), 6) + "\n";
        std::cout << name << " @ " << rate << ": " << std::fixed << std::setprecision(4)
                  << r.dbPeak() << " dBFS peak; " << 20 * std::log10(std::max(1e-12, r.rms[0]))
                  << " dBFS RMS\n";
    };
    for (double rate : {44100., 48000., 96000.}) {
        LogisthProcessor p;
        cleanSine(p);
        juce::AudioBuffer<float> audio;
        auto r = measure(p, rate, {69}, 1, 2, folder == juce::File{} ? nullptr : &audio);
        row("unity_sine", rate, r);
        if (verify) {
            require(std::abs(r.dbPeak()) < .002, "unity sine peaks at 0 dBFS per channel");
            require(std::abs(r.rms[0] - std::sqrt(.5)) < 1e-4, "unity sine RMS is -3.0103 dBFS");
            require(std::abs(r.peak[0] - r.peak[1]) < 1e-7, "centered mono sine channels agree");
        }
        if (folder != juce::File{} && rate == 48000) {
            folder.createDirectory();
            // Reference and rendered signals use float WAV, preserving levels above full scale.
            for (bool reference : {false, true}) {
                auto file = folder.getChildFile(reference ? "reference-sine-0dBFS.wav"
                                                          : "logisth-sine.wav");
                juce::AudioBuffer<float> wav(2, 48000);
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < 48000; ++s)
                        wav.setSample(ch, s,
                                      reference
                                          ? float(std::sin(2 * logisth::pi * 440 * s / 48000.))
                                          : audio.getSample(ch, 48000 + s));
                auto stream = file.createOutputStream();
                stream->setPosition(0);
                stream->truncate();
                juce::WavAudioFormat format;
                std::unique_ptr<juce::AudioFormatWriter> writer(
                    format.createWriterFor(stream.get(), 48000, 2, 32, {}, 0));
                require(bool(writer), "calibration WAV writer");
                stream.release();
                require(writer->writeFromAudioSampleBuffer(wav, 0, wav.getNumSamples()),
                        "calibration WAV export");
            }
            require(
                p.savePresetFile(folder.getChildFile("Calibration - sine 0 dBFS.logisth")).wasOk(),
                "calibration preset export");
        }
    }
    // Linear faders, MIDI velocity, pan law and coherent summing must stay predictable.
    {
        LogisthProcessor p;
        auto test = [&](const char *name, double expected) {
            auto result = measure(p, 48000);
            row(name, 48000, result);
            if (verify)
                require(std::abs(result.dbPeak() - expected) < .003,
                        "documented gain/pan/summing calibration");
            return result;
        };
        cleanSine(p);
        set(p, "o1phase", 90);
        juce::AudioBuffer<float> onset;
        auto instant = measure(p, 48000, {69}, 1, 2, &onset);
        row("instant_attack", 48000, instant);
        if (verify) {
            require(onset.getSample(0, 0) == 0 && onset.getSample(1, 0) == 0,
                    "zero attack starts at silence with a nonzero-phase carrier");
            for (int s = 49; s < 150; ++s) {
                float reference = float(std::cos(2 * logisth::pi * 440 * s / 48000.));
                require(std::abs(onset.getSample(0, s) - reference) < 1e-5f &&
                            std::abs(onset.getSample(1, s) - reference) < 1e-5f,
                        "per-voice anti-click edge reaches unity within 1 ms without master fade");
            }
            require(std::abs(instant.dbPeak()) < .002, "anti-click edge retains unity steady gain");
        }
        for (const auto *id : {"master", "mixA", "o1level"}) {
            cleanSine(p);
            set(p, id, .5f);
            test(id, -6.0206);
        }
        cleanSine(p);
        auto velocity = measure(p, 48000, {69}, .5f);
        row("half_velocity", 48000, velocity);
        if (verify)
            require(std::abs(
                        velocity.dbPeak() -
                        20 * std::log10(juce::MidiMessage::noteOn(1, 69, .5f).getFloatVelocity())) <
                        .003,
                    "MIDI velocity remains linear");
        cleanSine(p);
        set(p, "o1pan", -1);
        auto left = test("pan_left", 3.0103);
        if (verify)
            require(left.peak[1] == 0, "hard-left pan silences right channel");
        cleanSine(p);
        set(p, "o1pan", 1);
        auto right = test("pan_right", 3.0103);
        if (verify)
            require(right.peak[0] == 0, "hard-right pan silences left channel");
        cleanSine(p);
        set(p, "compon", 1);
        set(p, "compthreshold", -12);
        set(p, "compratio", 4);
        set(p, "compattack", .1f);
        set(p, "comprelease", 100);
        auto compressed = measure(p, 48000);
        row("compressor_at_unity", 48000, compressed);
        if (verify)
            require(
                compressed.dbPeak() < -8 && compressed.dbPeak() > -10,
                "compressor receives the calibrated engine level and applies expected reduction");
        cleanSine(p);
        set(p, "o1level", 0);
        set(p, "o3level", 1);
        test("part_B_unity", 0);
        cleanSine(p);
        set(p, "o2level", 1);
        test("two_coherent_oscillators", 6.0206);
        cleanSine(p);
        set(p, "o1voices", 8);
        test("eight_coherent_unison", 9.0309);
        // Source-file calibration checks Wavetable, Sampler and Granulizer separately.
        for (int mode : {1, 2, 3}) {
            juce::TemporaryFile sample(".wav");
            auto stream = sample.getFile().createOutputStream();
            juce::WavAudioFormat format;
            std::unique_ptr<juce::AudioFormatWriter> writer(
                format.createWriterFor(stream.get(), 48000, 1, 32, {}, 0));
            require(bool(writer), "unit source writer");
            stream.release();
            int n = mode == 1 ? 2048 : 48001;
            juce::AudioBuffer<float> data(1, n);
            for (int i = 0; i < n; ++i)
                data.setSample(
                    0, i,
                    float(std::sin(2 * logisth::pi * (mode == 1 ? i / 2048. : 440 * i / 48000.))));
            require(writer->writeFromAudioSampleBuffer(data, 0, n), "unit source WAV export");
            writer.reset();
            cleanSine(p);
            require(p.loadSource(0, sample.getFile()).wasOk(), "unit source import");
            set(p, "o1engine", float(mode));
            set(p, "o1root", 69);
            set(p, "o1gmodel", 1);
            set(p, "o1gattack", 0);
            set(p, "o1ghold", 20);
            auto result = measure(p, 48000);
            row(mode == 1   ? "unit_wavetable"
                : mode == 2 ? "unit_sampler"
                            : "unit_granulizer",
                48000, result);
            if (verify)
                require(std::abs(result.dbPeak()) < .02, "unit media retains full-scale output");
        }
    }
    static const char *names[] = {"Init",     "Supersaw",  "Bass",    "Atmosphere", "Pluck",
                                  "Sequence", "Wavetable", "Sampler", "Grains",     "Chord pulse"};
    for (double rate : {44100., 48000., 96000.})
        for (int i = 0; i < 10; ++i) {
            LogisthProcessor p;
            p.loadPreset(i);
            auto result = measure(p, rate, {48, 55, 60, 64}, .8f, 6, nullptr, false);
            row(names[i], rate, result);
            if (verify)
                require(result.dbPeak() < (i == 0 ? 0 : -3),
                        i == 0 ? "raw Init saw at Main 65 percent stays below full scale for the "
                                 "reference chord"
                               : "authored factory reference chords keep at least 3 dB of output "
                                 "headroom");
        }
    if (folder != juce::File{}) {
        folder.createDirectory();
        require(folder.getChildFile("measurements.csv").replaceWithText(report, false, false, "\n"),
                "calibration report export");
    }
}
} // namespace gain_test
