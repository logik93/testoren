#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void checkEffects() {
    using namespace logisth;
    Effects effects;
    effects.prepare(48000);
    auto render = [&](EffectsParams params, bool modulate = false) {
        effects.reset();
        std::vector<float> output;
        output.reserve(24000);
        std::array<float, 512> l{}, r{}, dm{}, pm{};
        for (int block = 0; block < 48; ++block) {
            for (int i = 0; i < 512; ++i) {
                l[i] = .35f * std::sin(float(2 * pi * (block * 512 + i) * 440 / 48000));
                r[i] = l[i] * .7f;
                dm[i] = modulate ? .4f : 0;
                pm[i] = modulate ? .5f : 0;
            }
            effects.process(l.data(), r.data(), 512, dm.data(), pm.data(), params);
            for (auto x : l) {
                check(std::isfinite(x) && std::abs(x) < 64, "finite effect output");
                output.push_back(x);
            }
        }
        return output;
    };
    auto delta = [](const auto &a, const auto &b) {
        double result = 0;
        for (size_t i = 4096; i < a.size(); ++i)
            result += std::abs(a[i] - b[i]);
        return result;
    };
    EffectsParams clean;
    clean.reverb = false;
    clean.sync = false;
    const auto dry = render(clean);
    for (int type = 0; type < 5; ++type) {
        auto p = clean;
        p.distortion = true;
        p.drive = .7f;
        p.distortionType = type;
        check(delta(dry, render(p)) > .01, "distortion type sounds");
    }
    for (int fx = 0; fx < 7; ++fx) {
        auto p = clean;
        switch (fx) {
        case 0:
            p.distortion = true;
            p.drive = .4f;
            break;
        case 1:
            p.phaser = true;
            break;
        case 2:
            p.chorus = true;
            break;
        case 3:
            p.eq = true;
            p.bass = 9;
            break;
        case 4:
            p.reverb = true;
            break;
        case 5:
            p.delay = true;
            p.delayL = .035f;
            p.delayR = .04f;
            break;
        case 6:
            p.compressor = true;
            p.threshold = -30;
            break;
        }
        check(delta(dry, render(p)) > .01, "effect section is functional");
    }
    auto p = clean;
    p.eq = true;
    check(delta(dry, render(p)) < .0001, "flat EQ is transparent");
    p = clean;
    p.compressor = true;
    p.ratio = 1;
    check(delta(dry, render(p)) < .0001, "ratio 1 is transparent");
    p = clean;
    p.phaser = true;
    check(delta(render(p), render(p, true)) > .01, "phaser frequency modulation reaches effect");
    p = clean;
    p.distortion = true;
    check(delta(render(p), render(p, true)) > .01, "distortion modulation reaches effect");
    std::cout
        << "PASS: seven FX, five distortion types, EQ/compressor transparency, FX modulation\n";
}
int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        checkEffects();
        LogisthProcessor p;
        p.setRateAndBufferSizeDetails(48000, 512);
        p.prepareToPlay(48000, 512);
        juce::AudioBuffer<float> audio(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 128);
        p.processBlock(audio, midi);
        for (int s = 0; s < 128; ++s)
            check(audio.getSample(0, s) == 0, "sample-accurate note offset");
        check(audio.getMagnitude(0, 128, 384) > 0, "MIDI produces sound");
        auto *parameter = p.state.getParameter("p1cutoff");
        parameter->setValueNotifyingHost(parameter->convertTo0to1(1234));
        juce::MemoryBlock saved;
        p.getStateInformation(saved);
        LogisthProcessor restored;
        restored.setStateInformation(saved.getData(), int(saved.getSize()));
        check(std::abs(restored.state.getRawParameterValue("p1cutoff")->load() - 1234) < .01f,
              "state round trip");
        const char invalid[] = "invalid";
        restored.setStateInformation(invalid, sizeof(invalid));
        check(std::abs(restored.state.getRawParameterValue("p1cutoff")->load() - 1234) < .01f,
              "malformed state ignored");
        // Every new route, effect and sequencer parameter survives state restoration.
        for (const auto &spec : logisth::parameterSpecs) {
            auto *param = p.state.getParameter(spec.id);
            param->setValueNotifyingHost(.37f);
        }
        p.getStateInformation(saved);
        restored.setStateInformation(saved.getData(), int(saved.getSize()));
        for (const auto &spec : logisth::parameterSpecs)
            check(std::abs(p.state.getRawParameterValue(spec.id)->load() -
                           restored.state.getRawParameterValue(spec.id)->load()) < .001f,
                  "all parameter state round trip");
        juce::ValueTree legacy("AsterionState");
        auto legacyParam = [&](const char *id, float value) {
            juce::ValueTree item("PARAM");
            item.setProperty("id", id, nullptr);
            item.setProperty("value", value, nullptr);
            legacy.addChild(item, -1, nullptr);
        };
        legacyParam("p1cutoff", 333);
        legacyParam("p1env", 36);
        legacyParam("lfo1depth", 12);
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), saved);
        restored.setStateInformation(saved.getData(), int(saved.getSize()));
        check(std::abs(restored.state.getRawParameterValue("env1amt1")->load() - 3) < .001f,
              "legacy envelope routing import");
        check(restored.state.getRawParameterValue("lfo1dest1")->load() == 15,
              "legacy LFO routing import");
        legacy = juce::ValueTree("LogisthState");
        legacyParam("misc1dest1", 999);
        legacyParam("p1cutoff", 99999);
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), saved);
        restored.setStateInformation(saved.getData(), int(saved.getSize()));
        check(restored.state.getRawParameterValue("misc1dest1")->load() == 27 &&
                  restored.state.getRawParameterValue("p1cutoff")->load() == 20000,
              "state values are validated");
        for (int preset = 0; preset < 6; ++preset) {
            p.reset();
            p.loadPreset(preset);
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
            float maximum = 0;
            for (int b = 0; b < 300; ++b) {
                if (b == 180)
                    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
                p.processBlock(audio, midi);
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < 512; ++s) {
                        float v = audio.getSample(ch, s);
                        check(std::isfinite(v) && std::abs(v) < 64, "bounded preset audio");
                        maximum = std::max(maximum, std::abs(v));
                    }
            }
            check(maximum > .001f, "preset sounds");
        }
        if (argc > 1) {
            juce::File folder(juce::String::fromUTF8(argv[1]));
            folder.createDirectory();
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            auto *logisthEditor = dynamic_cast<LogisthEditor *>(editor.get());
            check(logisthEditor != nullptr, "editor type");
            for (int page = 0; page < 4; ++page) {
                auto full = logisthEditor->snapshotPage(page);
                juce::FileOutputStream out(
                    folder.getChildFile("logisth-page-" + juce::String(page + 1) + ".png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(full, out), "full page image");
            }
            juce::FileOutputStream imageOut(folder.getChildFile("logisth.png"));
            check(imageOut.openedOk(), "image file");
            imageOut.setPosition(0);
            imageOut.truncate();
            juce::PNGImageFormat png;
            check(png.writeImageToStream(image, imageOut), "editor renders");
            p.reset();
            p.loadPreset(1);
            juce::AudioBuffer<float> demo(2, 48000 * 5);
            demo.clear();
            for (int b = 0; b < demo.getNumSamples(); b += 512) {
                const int count = std::min(512, demo.getNumSamples() - b);
                audio.setSize(2, count, false, false, true);
                if (b == 0)
                    for (int note : {48, 55, 60, 64})
                        midi.addEvent(juce::MidiMessage::noteOn(1, note, .6f), 0);
                if (b <= 96000 && b + count > 96000)
                    for (int note : {48, 55, 60, 64})
                        midi.addEvent(juce::MidiMessage::noteOff(1, note), 96000 - b);
                p.processBlock(audio, midi);
                for (int ch = 0; ch < 2; ++ch)
                    demo.copyFrom(ch, b, audio, ch, 0, count);
            }
            auto stream = folder.getChildFile("logisth-demo.wav").createOutputStream();
            check(bool(stream), "WAV stream");
            stream->setPosition(0);
            stream->truncate();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> writer(
                wav.createWriterFor(stream.get(), 48000, 2, 24, {}, 0));
            check(bool(writer), "WAV writer");
            stream.release();
            check(writer->writeFromAudioSampleBuffer(demo, 0, demo.getNumSamples()), "WAV render");
        }
        std::cout << "PASS processor: MIDI offsets, state, malformed state, six presets and FX\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
