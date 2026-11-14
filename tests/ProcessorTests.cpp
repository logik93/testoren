#include "GainCalibration.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ThemeSettings.h"
#include <iostream>
#include <stdexcept>
#include <thread>
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void checkEnvelopeMidiEdges() {
    using namespace juce;
    for (double rate : {44100., 48000., 96000.})
        for (int wave = 0; wave < 8; ++wave)
            for (float sustain : {0.f, 1.f}) {
                AudioBuffer<float> reference;
                for (int blockSize : {1, 31, 257, 512}) {
                    // Fresh instances share the same initial RNG state, so even
                    // the noise waveform can be compared sample for sample.
                    LogisthProcessor p;
                    gain_test::cleanSine(p);
                    gain_test::set(p, "o1wave", float(wave));
                    gain_test::set(p, "o1phase", 90);
                    gain_test::set(p, "p1amps", sustain);
                    gain_test::set(p, "p1ampr", 0);
                    gain_test::set(p, "p2ampr", 0);
                    p.prepareToPlay(rate, blockSize);
                    AudioBuffer<float> output(2, 1024);
                    output.clear();
                    AudioBuffer<float> block(2, blockSize);
                    for (int offset = 0; offset < output.getNumSamples(); offset += blockSize) {
                        int length = std::min(blockSize, output.getNumSamples() - offset);
                        block.setSize(2, length, false, false, true);
                        MidiBuffer midi;
                        if (offset <= 37 && offset + length > 37)
                            midi.addEvent(MidiMessage::noteOn(1, 69, 1.f), 37 - offset);
                        if (offset <= 301 && offset + length > 301)
                            midi.addEvent(MidiMessage::noteOff(1, 69), 301 - offset);
                        p.processBlock(block, midi);
                        for (int ch = 0; ch < 2; ++ch)
                            output.copyFrom(ch, offset, block, ch, 0, length);
                    }
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < output.getNumSamples(); ++i) {
                            float value = output.getSample(ch, i);
                            check(std::isfinite(value), "anti-click processor output is finite");
                            if (sustain == 0 || i <= 37 || i > 301 + int(std::ceil(.0011 * rate)))
                                check(
                                    value == 0,
                                    "zero ADSR and sample-accurate smoothed MIDI edges are silent");
                            if (blockSize != 1)
                                check(std::abs(value - reference.getSample(ch, i)) < 1e-6f,
                                      "ADSR edges are invariant across host buffer sizes");
                        }
                    if (blockSize == 1)
                        reference.makeCopyOf(output);
                }
            }
    std::cout << "PASS processor anti-click: all waves, three rates, four host buffers, MIDI "
                 "on/off at samples 37/301\n";
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
void checkHybridProcessor() {
    using namespace juce;
    using namespace logisth;
    TemporaryFile temporary(".wav");
    {
        WavAudioFormat wav;
        auto output = temporary.getFile().createOutputStream();
        StringPairArray metadata;
        metadata.set("NumCuePoints", "2");
        metadata.set("Cue0Identifier", "1");
        metadata.set("Cue0Offset", "0");
        metadata.set("Cue1Identifier", "2");
        metadata.set("Cue1Offset", "2048");
        std::unique_ptr<AudioFormatWriter> writer(
            wav.createWriterFor(output.get(), 48000, 2, 24, metadata, 0));
        check(bool(writer), "hybrid test WAV writer");
        output.release();
        AudioBuffer<float> sample(2, 4096);
        for (int i = 0; i < sample.getNumSamples(); ++i) {
            sample.setSample(0, i, .7f * std::sin(float(2 * pi * i / 128)));
            sample.setSample(1, i, .3f * std::cos(float(2 * pi * i / 128)));
        }
        check(writer->writeFromAudioSampleBuffer(sample, 0, sample.getNumSamples()),
              "hybrid test WAV file");
    }
    LogisthProcessor original;
    auto result = original.loadSource(0, temporary.getFile(), 512);
    if (result.failed())
        throw std::runtime_error(result.getErrorMessage().toStdString());
    auto source = original.sourceData(0);
    check(source->samples[0].size() == 4096 && source->frames == 8 && source->frameSize == 512,
          "import and frame slicing");
    check(source->samples[0] != source->samples[1], "stereo import preserved");
    check(source->regions == std::vector<int64_t>{0, 2048}, "WAV cue regions imported");
    original.prepareToPlay(48000, 512);
    auto *engineParam = original.state.getParameter("o1engine");
    engineParam->setValueNotifyingHost(engineParam->convertTo0to1(2));
    std::atomic<bool> importing{true}, finite{true};
    std::thread audioWorker([&] {
        AudioBuffer<float> audio(2, 512);
        MidiBuffer events;
        events.addEvent(MidiMessage::noteOn(1, 60, 1.f), 0);
        int blocks = 0;
        while (importing.load() || blocks < 20) {
            original.processBlock(audio, events);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    if (!std::isfinite(audio.getSample(ch, i)))
                        finite = false;
            ++blocks;
        }
    });
    bool importOk = true;
    for (int i = 0; i < 16; ++i) {
        original.clearSource(0);
        importOk &= original.loadSource(0, temporary.getFile(), 512).wasOk();
        original.serviceAssets();
    }
    importing = false;
    audioWorker.join();
    check(importOk && finite.load(), "concurrent source replacement during rendering");
    original.setChord(61, 4);
    original.setChordNote(61, 3, 74);
    auto set = [](LogisthProcessor &p, const char *id, float value) {
        auto *parameter = p.state.getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    set(original, "o1engine", 2);
    set(original, "pat1note", 3);
    original.storePattern(2);
    set(original, "pat1note", 1);
    MemoryBlock state;
    original.getStateInformation(state);
    temporary.getFile().deleteFile();
    LogisthProcessor restored;
    restored.setStateInformation(state.getData(), int(state.getSize()));
    check(restored.sourceData(0)->samples == source->samples &&
              restored.sourceData(0)->frameSize == 512,
          "embedded sample restored after source file removed");
    check(restored.sourceData(0)->regions == source->regions, "region metadata state roundtrip");
    TemporaryFile portablePreset(".logisth");
    check(restored.savePresetFile(portablePreset.getFile()).wasOk(), "save embedded user preset");
    restored.clearSource(0);
    check(restored.loadPresetFile(portablePreset.getFile()).wasOk() &&
              restored.sourceData(0)->samples == source->samples &&
              restored.sourceData(0)->regions == source->regions,
          "user preset restores embedded audio and WAV regions without original file");
    check(restored.chordMap()->keys[61].notes == original.chordMap()->keys[61].notes,
          "custom chord voicing roundtrip");
    restored.recallPattern(2);
    check(restored.state.getRawParameterValue("pat1note")->load() == 3,
          "eight-slot pattern bank roundtrip");
    check(restored.state.getRawParameterValue("o1engine")->load() == 2, "source engine host state");
    auto tree = restored.state.copyState();
    ValueTree malformed("SOURCE");
    malformed.setProperty("slot", 0, nullptr);
    MemoryBlock invalid("bad", 3);
    malformed.setProperty("audio", var(invalid), nullptr);
    tree.addChild(malformed, -1, nullptr);
    MemoryBlock corrupt;
    AudioProcessor::copyXmlToBinary(*tree.createXml(), corrupt);
    restored.setStateInformation(corrupt.getData(), int(corrupt.getSize()));
    check(restored.sourceData(0)->samples == source->samples,
          "malformed embedded source rejected without losing current sample");
    restored.loadPreset(0);
    set(restored, "arpon", 1);
    set(restored, "arppattern", 1);
    set(restored, "arpout", 1);
    set(restored, "chordon", 1);
    restored.prepareToPlay(48000, 512);
    AudioBuffer<float> audio(2, 512);
    MidiBuffer midi;
    midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 32);
    restored.processBlock(audio, midi);
    bool at32 = false;
    for (auto metadata : midi)
        at32 |= metadata.getMessage().isNoteOn() && metadata.samplePosition == 32 &&
                metadata.getMessage().getNoteNumber() == 60;
    check(at32, "generated chord/arp MIDI reaches host at sample offset");
    restored.learnChord(62);
    midi.clear();
    for (int note : {57, 60, 64, 67})
        midi.addEvent(MidiMessage::noteOn(1, note, .8f), 0);
    restored.processBlock(audio, midi);
    restored.finishLearn();
    check(restored.chordMap()->keys[62].count == 4 && restored.chordMap()->keys[62].notes[0] == 57,
          "MIDI learning captures chord");
    set(restored, "arpout", 0);
    midi.clear();
    restored.processBlock(audio, midi);
    check(!midi.isEmpty(), "output disable sends panic");
    midi.clear();
    restored.processBlock(audio, midi);
    check(midi.isEmpty(), "MIDI output opt-out after panic");
    ValueTree legacy("LogisthState");
    legacy.setProperty("schema", 2, nullptr);
    for (size_t i = 0; i < 248; ++i) {
        ValueTree value("PARAM");
        value.setProperty("id", parameterSpecs[i].id, nullptr);
        value.setProperty("value", parameterSpecs[i].initial, nullptr);
        legacy.addChild(value, -1, nullptr);
    }
    MemoryBlock oldState;
    AudioProcessor::copyXmlToBinary(*legacy.createXml(), oldState);
    restored.setStateInformation(oldState.getData(), int(oldState.getSize()));
    check(restored.state.getRawParameterValue("arppattern")->load() == 0 &&
              restored.state.getRawParameterValue("o1engine")->load() == 0 &&
              restored.sourceData(0) == SourceData::factory(),
          "0.3 state restores classic and legacy with factory fallback");
    auto duplicate = legacy.createCopy();
    ValueTree badMap("CHORDMAP");
    for (int i = 0; i < 2; ++i) {
        ValueTree chord("CHORD");
        chord.setProperty("key", 60, nullptr);
        chord.setProperty("notes", i == 0 ? "0 1 2 3 4 5 6 7" : "8 9 10 11 12 13 14 15", nullptr);
        badMap.addChild(chord, -1, nullptr);
    }
    duplicate.addChild(badMap, -1, nullptr);
    MemoryBlock badChord;
    AudioProcessor::copyXmlToBinary(*duplicate.createXml(), badChord);
    auto goodMap = restored.chordMap();
    restored.setStateInformation(badChord.getData(), int(badChord.getSize()));
    check(restored.chordMap() == goodMap, "duplicate chord entries rejected atomically");
    std::cout
        << "PASS hybrid processor: stereo import, frame size, embedded samples, custom chords, "
           "pattern bank, malformed source rejection, sample-accurate MIDI out, chord learning, "
           "concurrent media replacement, legacy state, invalid chord rejection\n";
}

void checkStudioProcessor() {
    using namespace juce;
    LogisthProcessor p;
    auto set = [&](const String &id, float value) {
        auto *param = p.state.getParameter(id);
        check(param != nullptr, "studio parameter exists");
        param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    auto get = [&](const String &id) { return p.state.getRawParameterValue(id)->load(); };
    for (int slot = 0; slot < 12; ++slot) {
        p.recallPattern(slot);
        set("pat1gate", float(slot + 10));
        p.setPatternName(slot, "Phrase " + String(slot + 1));
    }
    for (int slot = 0; slot < 12; ++slot) {
        p.recallPattern(slot);
        check(get("pat1gate") == slot + 10, "A-L automatically retain Pattern edits");
    }
    p.copyPattern(11, 3);
    p.recallPattern(3);
    check(get("pat1gate") == 21 && p.patternName(3) == "Phrase 12", "whole pattern copy");
    set("pat1gate", 24);
    p.setChord(61, 4);
    p.state.state.setProperty("uiScope", 1, nullptr);
    TemporaryFile preset(".logisth");
    p.setPresetDirectory(preset.getFile().getParentDirectory());
    check(p.savePresetFile(preset.getFile()).wasOk(), "user preset atomic save");
    check(p.currentPresetFile() == preset.getFile(), "save tracks active preset");
    set("pat1gate", 18);
    p.setChord(61, 1);
    check(p.loadPresetFile(preset.getFile()).wasOk(), "user preset load");
    check(get("pat1gate") == 24 && p.chordMap()->keys[61].count == 4 &&
              int(p.state.state.getProperty("uiScope")) == 1,
          "preset restores Pattern steps, chords and scope selection");
    p.recallPattern(11);
    check(get("pat1gate") == 21 && p.patternName(11) == "Phrase 12", "preset retains A-L bank");
    preset.getFile().replaceWithText("not a preset");
    check(p.loadPresetFile(preset.getFile()).failed() && get("pat1gate") == 21,
          "invalid preset leaves active patch intact");
    check(p.savePresetFile(preset.getFile().withFileExtension("txt")).failed(),
          "preset extension validation");
    p.loadPreset(0);
    set("o1engine", 3);
    set("o1gmodel", 1);
    p.prepareToPlay(48000, 512);
    AudioBuffer<float> audio(2, 512);
    MidiBuffer midi;
    midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
    p.processBlock(audio, midi);
    std::array<float, 512> sum{}, a{};
    p.scope.read(2, sum.data(), 512);
    p.scope.read(0, a.data(), 512);
    bool activeA = false;
    for (int i = 0; i < 512; ++i) {
        check(std::abs(sum[size_t(i)] - audio.getSample(0, i) - audio.getSample(1, i)) < 1e-6,
              "oscilloscope sum matches actual output sample for sample");
        activeA |= std::abs(a[size_t(i)]) > 1e-5;
    }
    auto picture = p.playback[0].read();
    check(activeA && picture.active && picture.grainCount > 0 && picture.position > 0,
          "granular playback telemetry and part scope carry real audio");
    set("midionly", 1);
    midi.clear();
    p.processBlock(audio, midi);
    check(audio.getMagnitude(0, 512) == 0, "MIDI-only mode silences output");
    std::cout
        << "PASS studio processor: 12 automatic pattern memories, "
           "user preset files, malformed file rejection, real sum scope and grain telemetry\n";
}

void checkWavetableAndModeViews() {
    using namespace juce;
    using namespace logisth;
    LogisthProcessor p;
    TemporaryFile file(".wav");
    auto writeBank = [&](int frames, int size, bool metadata) {
        {
            WavAudioFormat wav;
            auto output = file.getFile().createOutputStream();
            output->setPosition(0);
            output->truncate();
            std::unique_ptr<AudioFormatWriter> writer(
                wav.createWriterFor(output.get(), 48000, 1, 24, {}, 0));
            check(bool(writer), "wavetable fixture writer");
            output.release();
            AudioBuffer<float> audio(1, frames * size);
            for (int f = 0; f < frames; ++f)
                for (int i = 0; i < size; ++i)
                    audio.setSample(0, f * size + i,
                                    float(std::sin(2 * pi * i / size) * (f + 1) / frames));
            check(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()),
                  "128-cycle bank fixture");
        }
        if (metadata) {
            FileOutputStream stream(file.getFile());
            stream.setPosition(file.getFile().getSize());
            stream.write("clm ", 4);
            const String data = "<!>" + String(size).paddedLeft('0', 4) + " 00000000";
            stream.writeInt(data.getNumBytesAsUTF8());
            stream.write(data.toRawUTF8(), data.getNumBytesAsUTF8());
            if (data.getNumBytesAsUTF8() & 1)
                stream.writeByte(0);
            auto end = stream.getPosition();
            stream.setPosition(4);
            stream.writeInt(int(end - 8));
        }
    };
    writeBank(128, 2048, false);
    check(p.loadSource(0, file.getFile()).wasOk(), "Auto imports a standard wavetable WAV");
    check(p.sourceData(0)->frames == 128 && p.sourceData(0)->frameSize == 2048,
          "standard WAV resolves to 128 cycles of 2048 points");
    writeBank(128, 256, true);
    check(p.loadSource(0, file.getFile()).wasOk() && p.sourceData(0)->frames == 128 &&
              p.sourceData(0)->frameSize == 256,
          "Auto reads Serum CLM frame length instead of treating the whole file as a cycle");
    check(p.loadSource(1, file.getFile(), 512).wasOk() && p.sourceData(1)->frames == 64,
          "manual frame length overrides file metadata");
    writeBank(1, 128, false);
    check(p.loadSource(2, file.getFile()).wasOk() && p.sourceData(2)->frames == 1 &&
              p.sourceData(2)->frameSize == 128,
          "short single-cycle WAV is not padded with a silent tail");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *console = ui->getChildComponent(0);
    auto set = [&](const String &id, float value) {
        auto *parameter = p.state.getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        Thread::sleep(55);
        Timer::callPendingTimersSynchronously();
    };
    for (int part = 0; part < 2; ++part) {
        auto *button = dynamic_cast<Button *>(console->findChildWithID("part." + String(part)));
        check(button != nullptr, "part selector available");
        button->onClick();
        for (int slot = part * 2; slot < part * 2 + 2; ++slot) {
            auto *menu =
                dynamic_cast<ComboBox *>(console->findChildWithID("source.view." + String(slot)));
            check(menu != nullptr, "mode-specific source menu available");
            String prefix = "o" + String(slot + 1);
            set(prefix + "engine", 3);
            set(prefix + "gmodel", 1);
            menu->setSelectedId(6, sendNotificationSync);
            check(console->findChildWithID(prefix + "gtrans")->isVisible(),
                  "Granulizer timing controls available in Granulizer");
            for (int mode : {2, 1, 0}) {
                set(prefix + "engine", float(mode));
                check(menu->indexOfItemId(4) < 0 && menu->indexOfItemId(5) < 0 &&
                          menu->indexOfItemId(6) < 0,
                      "Sampler, Wavetable and Classic hide granular pages");
                check(!console->findChildWithID(prefix + "gtrans")->isVisible() &&
                          !console->findChildWithID(prefix + "gattack")->isVisible(),
                      "changing engine dismisses stale Granulizer controls");
                check(mode != 0 || menu->indexOfItemId(2) < 0,
                      "Classic hides file-only source controls");
            }
            set(prefix + "engine", 3);
            set(prefix + "gmodel", 0);
            check(menu->indexOfItemId(4) >= 0 && menu->indexOfItemId(5) < 0 &&
                      menu->indexOfItemId(6) < 0,
                  "Cloud hides Granulizer-only Motion and Time pages");
        }
    }
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    editor->snapshotPage(9);
    auto *selector = dynamic_cast<ComboBox *>(console->findChildWithID("sample.slot"));
    selector->setSelectedId(1, sendNotificationSync);
    auto *view = console->findChildWithID("sample.waveform");
    auto at = Point<float>(float(view->getWidth() - 12), 169);
    MouseEvent event(Desktop::getInstance().getMainMouseSource(), at,
                     ModifierKeys(ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, view, view,
                     Time::getCurrentTime(), at, Time::getCurrentTime(), 1, false);
    view->mouseDown(event);
    check(p.sourceData(0)->frameIndex(p.state.getRawParameterValue("o1position")->load()) == 127,
          "dragging the WT window selects the final frame");
    MouseWheelDetails wheel;
    wheel.deltaY = -.1f;
    view->mouseWheelMove(event, wheel);
    check(p.sourceData(0)->frameIndex(p.state.getRawParameterValue("o1position")->load()) == 126,
          "WT window wheel steps exactly one frame");
    check(p.state.getParameter("arpgraphs") == nullptr &&
              console->findChildWithID("graph.canvas") == nullptr,
          "only Legacy and Pattern arp remain available");
    std::cout << "PASS wavetable: 128-frame WAV/CLM import, frame selection, mode-specific menus\n";
}
void checkWavetableRoutingAndDetune() {
    using namespace juce;
    using namespace logisth;
    LogisthProcessor p;
    std::unique_ptr<AudioProcessorEditor> editor(p.createEditor());
    auto *console = editor->getChildComponent(0);
    auto widget = [&](const String &id, bool menu) -> Component * {
        auto *field = console->findChildWithID(id);
        check(field != nullptr, "oscillator/routing field exists");
        for (auto *child : field->getChildren())
            if (menu ? dynamic_cast<ComboBox *>(child) != nullptr
                     : dynamic_cast<Slider *>(child) != nullptr)
                return child;
        throw std::runtime_error("missing parameter widget");
    };
    for (int slot = 0; slot < 4; ++slot) {
        gain_test::cleanSine(p);
        String prefix = "o" + String(slot + 1);
        gain_test::set(p, "o1level", 0);
        gain_test::set(p, prefix + "level", 1);
        gain_test::set(p, prefix + "voices", 2);
        auto *slider = static_cast<Slider *>(widget(prefix + "detune", false));
        slider->setValue(100, sendNotificationSync);
        check(slider->getTextFromValue(100).contains("ct"), "detune value identifies cents");
        AudioBuffer<float> audio;
        gain_test::measure(p, 48000, {69}, 1, 2, &audio);
        auto magnitude = [&](double frequency) {
            double real = 0, imaginary = 0;
            for (int i = 48000; i < 96000; ++i) {
                double phase = 2 * pi * frequency * i / 48000;
                double value = audio.getSample(0, i);
                real += value * std::cos(phase);
                imaginary += value * std::sin(phase);
            }
            return std::hypot(real, imaginary) * 2 / 48000;
        };
        for (int side : {-1, 1}) {
            auto full = magnitude(440 * std::exp2(side / 12.));
            auto half = magnitude(440 * std::exp2(side / 24.));
            check(full > .65 && full > half * 8,
                  "setting detune to 100 ct in the GUI renders voices at plus/minus one semitone "
                  "in every oscillator");
        }
        for (int lfo = 1; lfo <= 2; ++lfo)
            for (int route = 1; route <= 2; ++route) {
                gain_test::cleanSine(p);
                gain_test::set(p, "o1level", 0);
                gain_test::set(p, prefix + "level", 1);
                gain_test::set(p, prefix + "engine", 1);
                gain_test::set(p, prefix + "position", .5f);
                gain_test::set(p, "sync", 0);
                String source = "lfo" + String(lfo);
                gain_test::set(p, source + "rate", 8);
                gain_test::set(p, source + "amt" + String(route), 5);
                auto *menu = static_cast<ComboBox *>(widget(source + "dest" + String(route), true));
                const int target = int(Destination::PositionA) + slot / 2;
                const int item = target + 1;
                check(
                    menu->getItemText(menu->indexOfItemId(item)).startsWith("WT Pos / Start"),
                    "LFO menu explicitly identifies WT position without changing destination IDs");
                menu->setSelectedId(item, sendNotificationSync);
                p.prepareToPlay(48000, 128);
                AudioBuffer<float> block(2, 128);
                MidiBuffer midi;
                midi.addEvent(MidiMessage::noteOn(1, 60, 1.f), 0);
                int lowest = 128, highest = -1;
                for (int i = 0; i < 100; ++i) {
                    p.processBlock(block, midi);
                    midi.clear();
                    auto picture = p.playback[size_t(slot)].read();
                    check(picture.active, "LFO route produces active wavetable playback");
                    int frame = p.sourceData(slot)->frameIndex(picture.position);
                    lowest = std::min(lowest, frame);
                    highest = std::max(highest, frame);
                }
                check(lowest < 32 && highest > 95, "choosing either LFO route in the GUI scans the "
                                                   "playing WT frames in every oscillator");
            }
    }
    for (int slot = 1; slot <= 4; ++slot)
        static_cast<Slider *>(widget("o" + String(slot) + "detune", false))
            ->setValue(100, sendNotificationSync);
    MemoryBlock saved;
    p.getStateInformation(saved);
    LogisthProcessor restored;
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    for (int slot = 1; slot <= 4; ++slot)
        check(restored.state.getRawParameterValue("o" + String(slot) + "detune")->load() == 100,
              "100-cent detune survives project/preset state restoration without truncation");
    std::cout << "PASS LFO/WT routing and detune: both LFO route menus, all four oscillators, "
                 "100-cent audio sidebands and state round-trip\n";
}
void checkStartupCommentsAndThemes() {
    using namespace juce;
    using namespace logisth;
    LogisthProcessor p;
    auto get = [&](const String &id) { return p.state.getRawParameterValue(id)->load(); };
    check(p.currentPresetName() == "Init / saw" && get("master") == .65f,
          "new instances select the clean saw preset at Main 65 percent");
    check(get("o1wave") == 1 && get("o1engine") == 0 && get("o1voices") == 1 &&
              get("o2level") == 0 && get("o3level") == 0 && get("o4level") == 0,
          "startup has exactly one classic saw oscillator");
    for (auto id : {"arpon", "chordon", "diston", "phaseron", "choruson", "eqon", "reverbon",
                    "delayon", "compon", "env1amt1", "env2amt1"})
        check(get(id) == 0, "startup saw is free of FX, arp and modulation");
    check(get("p1type") == 3 && get("p2type") == 3, "startup filters are bypassed");
    AudioBuffer<float> tone;
    gain_test::measure(p, 48000, {69}, 1, 2, &tone);
    auto magnitude = [&](int harmonic) {
        double real = 0, imaginary = 0;
        for (int i = 48000; i < 96000; ++i) {
            double phase = 2 * pi * 440 * harmonic * i / 48000;
            real += tone.getSample(0, i) * std::cos(phase);
            imaginary += tone.getSample(0, i) * std::sin(phase);
        }
        return std::hypot(real, imaginary);
    };
    double fundamental = magnitude(1);
    check(fundamental > 100 && std::abs(magnitude(2) / fundamental - .5) < .02 &&
              std::abs(magnitude(3) / fundamental - 1. / 3) < .02,
          "startup audio retains the unfiltered saw harmonic spectrum");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    auto *notes = dynamic_cast<TextEditor *>(console->findChildWithID("preset.comment"));
    check(notes != nullptr, "editable preset comment field is present");
    const String comment =
        String::fromUTF8("A1 scan / b\xc5\x82\xc4\x99kit / \xce\xa9 / \xf0\x9f\x8e\xb9");
    struct MetadataProbe : AudioProcessorListener {
        int audioChanges = 0, metadataChanges = 0;
        void audioProcessorParameterChanged(AudioProcessor *, int, float) override {
            ++audioChanges;
        }
        void audioProcessorChanged(AudioProcessor *, const ChangeDetails &details) override {
            metadataChanges += details.nonParameterStateChanged;
        }
    } probe;
    p.addListener(&probe);
    notes->setText(comment, false);
    notes->onTextChange();
    p.removeListener(&probe);
    check(p.presetComment() == comment, "editing the comment updates preset metadata");
    check(probe.metadataChanges == 1 && probe.audioChanges == 0,
          "comment edits notify the DAW about project changes without generating audio automation");
    gain_test::set(p, "master", .29f);
    gain_test::set(p, "lfo1dest1", float(Destination::PositionB2));
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "commented user preset saves");
    p.setPresetComment("different");
    gain_test::set(p, "master", .9f);
    check(p.loadPresetFile(preset.getFile()).wasOk() && p.presetComment() == comment &&
              std::abs(get("master") - .29f) < .00001 &&
              get("lfo1dest1") == float(Destination::PositionB2),
          "user preset round-trip retains comment, Main and individual WT routing");
    MemoryBlock project;
    p.getStateInformation(project);
    LogisthProcessor restored;
    restored.setStateInformation(project.getData(), int(project.getSize()));
    check(restored.presetComment() == comment &&
              std::abs(restored.state.getRawParameterValue("master")->load() - .29f) < .00001,
          "project restoration keeps saved Main rather than applying the new-instance default");
    p.setPresetComment(String::repeatedString("x", 6000));
    check(p.presetComment().length() == 4096, "preset comments have a bounded size");
    p.loadPreset(0);
    notes->onTextChange();
    check(p.presetComment().isEmpty(),
          "a delayed previous-preset comment callback cannot replace new preset notes");
    check(p.presetComment().isEmpty() && get("master") == .65f,
          "Init resets its own notes and Main without carrying another preset's comment");
    auto *settings = dynamic_cast<Button *>(console->findChildWithID("ui.settings"));
    check(settings != nullptr, "settings button exists");
    settings->onClick();
    auto *panel = console->findChildWithID("settings.panel");
    auto *theme = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme"));
    check(panel->isVisible() && theme && theme->getNumItems() == 5,
          "five themes share the settings panel");
    notes->setText("Readable preset comment", false);
    notes->onTextChange();
    for (int choice = 0; choice < 5; ++choice) {
        const float before = get("master");
        theme->setSelectedId(choice + 1, sendNotificationSync);
        check(editor->selectedTheme() == choice && ThemeSettings::current() == choice,
              "every theme is selectable through the settings widget");
        check(ThemeSettings::reload() == choice && get("master") == before,
              "theme persists on disk without modifying audio settings");
        auto textImage = notes->createComponentSnapshot(notes->getLocalBounds());
        int readable = 0;
        for (int y = 3; y < textImage.getHeight() - 3; ++y)
            for (int x = 5; x < textImage.getWidth() - 5; ++x) {
                float brightness = textImage.getPixelAt(x, y).getBrightness();
                readable += choice == 0 ? brightness < .5f : brightness > .6f;
            }
        check(readable > 40, "existing preset-comment text keeps contrast when switching between "
                             "dark and light themes");
        auto image = editor->snapshotTheme(choice);
        check(image.isValid() && editor->selectedTheme() == choice,
              "every theme and About panel renders without changing the selection");
        LogisthProcessor next;
        std::unique_ptr<AudioProcessorEditor> fresh(next.createEditor());
        auto *other = dynamic_cast<LogisthEditor *>(fresh.get());
        check(other && other->selectedTheme() == choice &&
                  next.state.getRawParameterValue("master")->load() == .65f,
              "a fresh instance recalls the chosen theme and independently starts at Main 65");
        p.loadPreset(1);
        check(editor->selectedTheme() == choice && ThemeSettings::current() == choice,
              "loading a sound does not overwrite the appearance preference");
        check(p.loadPresetFile(preset.getFile()).wasOk() && ThemeSettings::current() == choice,
              "user-preset loading also keeps the current theme");
    }
    {
        LogisthProcessor peer;
        std::unique_ptr<AudioProcessorEditor> peerUi(peer.createEditor());
        auto *peerEditor = dynamic_cast<LogisthEditor *>(peerUi.get());
        editor->setTheme(2);
        check(peerEditor && peerEditor->selectedTheme() == 2,
              "changing the shared appearance updates existing editors immediately");
    }
    std::cout
        << "PASS startup/comments/themes: clean saw spectrum, Main defaults/restoration, "
           "Unicode preset notes, individual routing, five persistent themes and Settings/About\n";
}
void checkPointerInteraction() {
    using namespace juce;
    LogisthProcessor p;
    p.loadPreset(0);
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    ui->setSize(1240, 860);
    auto *console = ui->getChildComponent(0);
    auto tick = [] {
        Thread::sleep(55);
        Timer::callPendingTimersSynchronously();
    };
    auto event = [](Component &c, bool down) {
        auto at = c.getLocalBounds().getCentre().toFloat();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys(down ? ModifierKeys::leftButtonModifier : 0), 1, 0, 0, 0, 0,
                          &c, &c, Time::getCurrentTime(), at, Time::getCurrentTime(), 1, false);
    };
    struct Probe : ComponentListener {
        int changes = 0;
        void componentVisibilityChanged(Component &) override {
            ++changes;
        }
    };
    auto click = [&](Button &button, Component &field, bool wait) {
        Probe probe;
        field.addComponentListener(&probe);
        static_cast<Component &>(button).mouseDown(event(button, true));
        check(button.isDown(), "mouse down reaches enabled button");
        if (wait)
            tick();
        field.removeComponentListener(&probe);
        check(probe.changes == 0,
              "idle refresh must not hide/show a held button or its popup parent");
        check(button.isDown(), "refresh must preserve pressed button state until mouse up");
        // Paint the down state, as a native peer would, to avoid JUCE's synthetic flash timer.
        button.createComponentSnapshot(button.getLocalBounds());
        static_cast<Component &>(button).mouseUp(event(button, false));
    };
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    int tested = 0;
    for (bool dark : {false, true}) {
        editor->setDarkTheme(dark);
        for (int part = 0; part < 2; ++part) {
            for (auto *child : console->getChildren())
                if (auto *button = dynamic_cast<Button *>(child))
                    if (button->getButtonText() == (part == 0 ? "A" : "B") && child->getY() < 80)
                        click(*button, *button, false);
            tick();
            for (const auto &spec : logisth::parameterSpecs) {
                auto *field = console->findChildWithID(spec.id);
                if (!field || !field->isVisible() || !field->isEnabled())
                    continue;
                for (auto *child : field->getChildren())
                    if (auto *button = dynamic_cast<ToggleButton *>(child))
                        if (button->isVisible()) {
                            float before = p.state.getRawParameterValue(spec.id)->load();
                            click(*button, *field, true);
                            check(p.state.getRawParameterValue(spec.id)->load() != before,
                                  "every held-and-released toggle changes its parameter on first "
                                  "click");
                            tick();
                            click(*button, *field, true);
                            check(p.state.getRawParameterValue(spec.id)->load() == before,
                                  "second click restores toggle without extra clicks");
                            tick();
                            ++tested;
                        }
            }
        }
    }
    // All four model selectors, including host changes, must settle to one stable layout.
    for (int model : {0, 1}) {
        for (int i = 1; i <= 4; ++i) {
            auto *param = p.state.getParameter("o" + String(i) + "gmodel");
            param->setValueNotifyingHost(param->convertTo0to1(float(model)));
        }
        tick();
        Probe probe;
        auto *field = console->findChildWithID("p2type");
        check(field && field->isVisible(), "visible filter menu selected for stability check");
        field->addComponentListener(&probe);
        tick();
        tick();
        field->removeComponentListener(&probe);
        check(probe.changes == 0,
              "menu parent stays stable across refreshes for either grain model");
    }
    check(tested >= 60, "buttons tested on both parts and in both themes");
    std::cout << "PASS pointer interaction: " << tested
              << " toggles, press/refresh/release and immediate on/off, both parts/themes, stable "
                 "menu parents\n";
}

void checkLongSourceAndDrop() {
    using namespace juce;
    using namespace logisth;
    check(fitsSampleMemory(sampleMemoryBudget / 8) && !fitsSampleMemory(sampleMemoryBudget / 8 + 1),
          "sample allocation bounded by decoded bytes, not seconds");
    check(!fitsSampleMemory(UINT64_MAX) && !fitsSampleMemory(2, sampleMemoryBudget - 15),
          "sample frame/aggregate memory bounds reject overflow");
    TemporaryFile sample(".wav");
    const int frames = 48000 * 60; // one minute, deliberately beyond the previous 12-second limit
    {
        auto stream = sample.getFile().createOutputStream();
        WavAudioFormat wav;
        std::unique_ptr<AudioFormatWriter> writer(
            wav.createWriterFor(stream.get(), 48000, 1, 32, {}, 0));
        check(bool(writer), "long sample writer");
        stream.release();
        AudioBuffer<float> block(1, 48000);
        for (int i = 0; i < 48000; ++i)
            block.setSample(0, i, .25f * std::sin(float(2 * pi * i / 100)));
        for (int second = 0; second < 60; ++second)
            check(writer->writeFromAudioSampleBuffer(block, 0, 48000), "long sample fixture write");
    }
    LogisthProcessor p;
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    editor->snapshotPage(8);
    auto *console = ui->getChildComponent(0);
    auto *view = console->findChildWithID("sample.waveform");
    auto *drop = dynamic_cast<FileDragAndDropTarget *>(view);
    auto *selector = dynamic_cast<ComboBox *>(console->findChildWithID("sample.slot"));
    check(drop && view->isVisible() && selector, "central sample view accepts file drops");
    selector->setSelectedId(3,
                            sendNotificationSync); // B1; Part A is still the selected editing part
    auto *frame = dynamic_cast<ComboBox *>(console->findChildWithID("source.frame.2"));
    check(frame != nullptr, "drop uses source frame selector");
    frame->setSelectedId(4, dontSendNotification); // 512
    StringArray files{sample.getFile().getFullPathName()};
    check(drop->isInterestedInFileDrag(files), "WAV accepted in waveform");
    check(!drop->isInterestedInFileDrag({"/tmp/readme.txt"}) &&
              !drop->isInterestedInFileDrag({files[0], files[0]}),
          "waveform rejects unsupported or multiple files");
    auto oldA = p.sourceData(0);
    drop->fileDragEnter(files, 40, 60);
    view->createComponentSnapshot(view->getLocalBounds());
    drop->filesDropped(files, 40, 60);
    for (int i = 0; i < 1500 && !p.sourceStatus().startsWith("Loaded "); ++i)
        Thread::sleep(10);
    auto data = p.sourceData(2);
    check(data->samples[0].size() == size_t(frames) && data->frameSize == 512,
          "one-minute sample loads asynchronously into selected B1 slot");
    check(p.sourceData(0) == oldA && p.state.getRawParameterValue("o3engine")->load() == 2,
          "successful waveform drop selects Sampler only for target Classic slot");
    check(data->samples[0] == data->samples[1] && !data->waveformPeaks.empty(),
          "long mono source becomes stereo and receives background waveform cache");
    for (auto bounds : {std::pair<size_t, size_t>{0, 2000},
                        {73, 2001},
                        {1024, 4096},
                        {size_t(frames) - 701, size_t(frames)}}) {
        std::array<float, 2> slow{};
        for (size_t i = bounds.first; i < bounds.second; ++i) {
            slow[0] = std::min(slow[0], data->samples[0][i]);
            slow[1] = std::max(slow[1], data->samples[0][i]);
        }
        check(data->rangePeaks(0, bounds.first, bounds.second) == slow,
              "cached waveform extrema match actual PCM at chunk boundaries");
    }
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "long sample preset saves");
    check(sample.getFile().deleteFile(), "original long sample removed before preset reload");
    p.clearSource(2);
    check(p.loadPresetFile(preset.getFile()).wasOk() && p.sourceData(2)->samples == data->samples,
          "long embedded sample reloads after deleting original media");
    auto intact = p.sourceData(2);
    auto malformed = p.state.copyState();
    MemoryOutputStream compressed;
    {
        GZIPCompressorOutputStream zip(compressed);
        zip.writeDouble(48000);
        zip.writeInt(128);
        zip.writeInt(256);
        zip.writeString("truncated");
        zip.writeFloat(.5f);
    }
    ValueTree broken("SOURCE");
    broken.setProperty("slot", 0, nullptr);
    broken.setProperty("audio", var(compressed.getMemoryBlock()), nullptr);
    malformed.addChild(broken, -1, nullptr);
    MemoryBlock invalid;
    AudioProcessor::copyXmlToBinary(*malformed.createXml(), invalid);
    check(!p.applyState(invalid.getData(), int(invalid.getSize())) && p.sourceData(2) == intact,
          "truncated streaming PCM rejects without replacing any current source");
    TemporaryFile bad(".wav");
    bad.getFile().replaceWithText("invalid audio");
    selector->setSelectedId(2, sendNotificationSync); // A2 remains Classic
    drop->filesDropped({bad.getFile().getFullPathName()}, 40, 60);
    for (int i = 0; i < 500 && p.sourceStatus() == "Loading " + bad.getFile().getFileName() + "...";
         ++i)
        Thread::sleep(10);
    check(p.sourceData(1) == SourceData::factory() &&
              p.state.getRawParameterValue("o2engine")->load() == 0,
          "failed waveform drop leaves source and engine selection intact");
    std::cout << "PASS long samples and waveform drop: 60-second import, selected-slot routing, "
                 "frame size, mono/stereo, background peaks, portable preset reload, invalid-file "
                 "recovery and memory boundaries\n";
}

int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        juce::TemporaryFile themePreferences(".json");
        logisth::ThemeSettings::useStorageFileForTests(themePreferences.getFile());
        if (argc > 1 && juce::String(argv[1]) == "--customization-check") {
            checkStartupCommentsAndThemes();
            return 0;
        }
        if (argc > 2 &&
            (juce::String(argv[1]) == "--gain-report" || juce::String(argv[1]) == "--gain-check")) {
            gain_test::run(juce::String(argv[1]) == "--gain-check",
                           juce::File(juce::String::fromUTF8(argv[2])));
            return 0;
        }
        checkEnvelopeMidiEdges();
        gain_test::run(true);
        checkEffects();
        checkHybridProcessor();
        checkStudioProcessor();
        checkPointerInteraction();
        checkWavetableAndModeViews();
        checkWavetableRoutingAndDetune();
        checkStartupCommentsAndThemes();
        checkLongSourceAndDrop();
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
        check(restored.state.getRawParameterValue("misc1dest1")->load() ==
                      int(logisth::Destination::Count) - 1 &&
                  restored.state.getRawParameterValue("p1cutoff")->load() == 20000,
              "state values are validated");
        for (int preset = 0; preset < 10; ++preset) {
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
        juce::MemoryBlock uiState;
        p.getStateInformation(uiState);
        {
            std::array<float, logisth::parameterCount> before{};
            for (size_t i = 0; i < before.size(); ++i)
                before[i] = p.state.getRawParameterValue(logisth::parameterSpecs[i].id)->load();
            std::unique_ptr<juce::AudioProcessorEditor> ui(p.createEditor());
            auto *compact = dynamic_cast<LogisthEditor *>(ui.get());
            check(compact != nullptr, "compact editor exists");
            auto error = compact->validateLayout();
            if (error.isNotEmpty())
                throw std::runtime_error(error.toStdString());
            compact->setDarkTheme(true);
            check(compact->isDarkTheme(), "dark theme selected");
            check(compact->validateLayout().isEmpty(), "dark theme layout");
            juce::MemoryBlock darkState;
            p.getStateInformation(darkState);
            restored.setStateInformation(darkState.getData(), int(darkState.getSize()));
            check(logisth::ThemeSettings::current() == 1,
                  "theme is saved independently of the audio preset");
            {
                std::unique_ptr<juce::AudioProcessorEditor> second(restored.createEditor());
                auto *other = dynamic_cast<LogisthEditor *>(second.get());
                check(other && other->isDarkTheme(),
                      "restored editor follows the selected user theme");
                compact->setDarkTheme(false);
                for (int attempt = 0; attempt < 40 && other->isDarkTheme(); ++attempt) {
                    juce::Thread::sleep(25);
                    juce::Timer::callPendingTimersSynchronously();
                }
                check(!compact->isDarkTheme() && !other->isDarkTheme(),
                      "appearance preference propagates to existing plugin instances");
            }
            check(compact->validateLayout().isEmpty(), "light theme layout after switching");
            ui->setSize(992, 688);
            check(compact->validateLayout().isEmpty(), "compact layout at 80 percent");
            ui->setSize(1550, 1075);
            check(compact->validateLayout().isEmpty(), "compact layout at 125 percent");
            for (size_t i = 0; i < before.size(); ++i)
                check(before[i] ==
                          p.state.getRawParameterValue(logisth::parameterSpecs[i].id)->load(),
                      "changing UI views preserves every parameter");
            auto findWidget = [&](const char *id) -> juce::Component * {
                auto *field = ui->getChildComponent(0)->findChildWithID(id);
                check(field != nullptr, "parameter component exists");
                for (auto *child : field->getChildren())
                    if (child->isVisible() && (dynamic_cast<juce::Slider *>(child) ||
                                               dynamic_cast<juce::ComboBox *>(child) ||
                                               dynamic_cast<juce::ToggleButton *>(child)))
                        return child;
                throw std::runtime_error("missing parameter widget");
            };
            auto *cutoff = dynamic_cast<juce::Slider *>(findWidget("p1cutoff"));
            check(cutoff != nullptr, "cutoff widget");
            cutoff->setValue(cutoff->getValueFromText("2.5k"), juce::sendNotificationSync);
            check(std::abs(p.state.getRawParameterValue("p1cutoff")->load() - 2500) < .01f,
                  "frequency editing controls parameter");
            for (const auto *id : {"o1level", "o1position", "o1warp", "o2level", "o2position"}) {
                auto *sourceControl = dynamic_cast<juce::Slider *>(findWidget(id));
                check(sourceControl && std::abs(sourceControl->getValue() -
                                                p.state.getRawParameterValue(id)->load()) < 1e-5,
                      "source widget displays matching audio parameter");
            }
            auto *mix = dynamic_cast<juce::Slider *>(findWidget("mixA"));
            check(mix != nullptr, "mix widget");
            mix->setValue(mix->getValueFromText("42%"), juce::sendNotificationSync);
            check(std::abs(p.state.getRawParameterValue("mixA")->load() - .42f) < .00001f,
                  "percentage editing controls parameter");
            auto *chorusTime = dynamic_cast<juce::Slider *>(findWidget("chorusdelay"));
            check(chorusTime != nullptr, "effect delay widget");
            chorusTime->setValue(chorusTime->getValueFromText("12ms"), juce::sendNotificationSync);
            check(std::abs(p.state.getRawParameterValue("chorusdelay")->load() - 12) < .0001f,
                  "millisecond parameter editing preserves units");
            check(std::abs(chorusTime->getValueFromText("0.012s") - 12) < .0001f,
                  "seconds converted to stored milliseconds");
            auto *ampTime = dynamic_cast<juce::Slider *>(findWidget("p1ampa"));
            check(ampTime != nullptr, "envelope attack widget");
            check(std::abs(ampTime->getValueFromText("8ms") - .008) < .00001f,
                  "millisecond editing converted to envelope seconds");
            auto *type = dynamic_cast<juce::ComboBox *>(findWidget("p1type"));
            check(type != nullptr, "filter menu");
            type->setSelectedId(3, juce::sendNotificationSync);
            check(p.state.getRawParameterValue("p1type")->load() == 2,
                  "filter menu controls parameter");
            auto *sync = dynamic_cast<juce::ToggleButton *>(findWidget("sync"));
            check(sync != nullptr, "sync switch");
            sync->setToggleState(false, juce::sendNotificationSync);
            check(p.state.getRawParameterValue("sync")->load() == 0, "toggle controls parameter");
            auto *centralMenu = dynamic_cast<juce::ComboBox *>(findWidget("arpvelocity"));
            auto &skin = dynamic_cast<LogisthLook &>(ui->getLookAndFeel());
            check(centralMenu &&
                      centralMenu->findColour(juce::ComboBox::textColourId) == skin.screenText,
                  "light theme rack menu contrast");
            compact->snapshotPage(4);
            check(centralMenu->findColour(juce::ComboBox::textColourId) == skin.screenText,
                  "dark theme rack menu contrast");
            auto *canvas = ui->getChildComponent(0)->findChildWithID("pattern.canvas");
            check(canvas && canvas->isVisible(), "pattern drawing canvas available");
            auto eventAt = [&](juce::Point<float> at, int modifiers, int clicks = 1) {
                return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), at,
                                        juce::ModifierKeys(modifiers), 1, 0, 0, 0, 0, canvas,
                                        canvas, juce::Time::getCurrentTime(), at,
                                        juce::Time::getCurrentTime(), clicks, false);
            };
            canvas->mouseDown(eventAt({31, 0}, juce::ModifierKeys::leftButtonModifier));
            canvas->mouseDrag(eventAt({57, 18}, juce::ModifierKeys::leftButtonModifier));
            canvas->mouseUp(eventAt({57, 18}, 0));
            check(p.state.getRawParameterValue("pat2note")->load() == 7 &&
                      p.state.getRawParameterValue("pat3note")->load() == 0,
                  "drawing horizontally edits independent pattern cells");
            canvas->mouseDown(eventAt({31, 0}, juce::ModifierKeys::rightButtonModifier));
            auto *valueEditor = dynamic_cast<juce::TextEditor *>(canvas->getChildComponent(0));
            check(valueEditor && valueEditor->isVisible(), "exact pattern entry opens inline");
            valueEditor->setText("2");
            valueEditor->onReturnKey();
            check(p.state.getRawParameterValue("pat2note")->load() == 1,
                  "one-based note entry controls zero-based note index");
            check(ui->getChildComponent(0)->findChildWithID("graph.canvas") == nullptr &&
                      p.state.getParameter("arpgraphs") == nullptr,
                  "Graphs is removed from the editor and parameter model");
        }
        p.setStateInformation(uiState.getData(), int(uiState.getSize()));
        if (argc > 1) {
            juce::File folder(juce::String::fromUTF8(argv[1]));
            folder.createDirectory();
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            auto previewAudio = [&](int mode) {
                p.loadPreset(0);
                for (int slot = 1; slot <= 2; ++slot) {
                    juce::String prefix = "o" + juce::String(slot);
                    for (auto setting :
                         {std::pair<juce::String, float>{prefix + "engine", float(mode)},
                          {prefix + "position", mode == 1 ? .42f : 0.f},
                          {prefix + "gmodel", 1.f}}) {
                        auto *parameter = p.state.getParameter(setting.first);
                        parameter->setValueNotifyingHost(parameter->convertTo0to1(setting.second));
                    }
                }
                p.prepareToPlay(48000, 512);
                midi.clear();
                midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
                for (int i = 0; i < 18; ++i) {
                    p.processBlock(audio, midi);
                    midi.clear();
                }
            };
            auto *logisthEditor = dynamic_cast<LogisthEditor *>(editor.get());
            check(logisthEditor != nullptr, "editor type");
            for (int page = 0; page < 14; ++page) {
                previewAudio(page < 4 ? 0 : page == 4 || page == 9 ? 1 : page == 8 ? 2 : 3);
                auto full = logisthEditor->snapshotPage(page);
                juce::FileOutputStream out(
                    folder.getChildFile("logisth-page-" + juce::String(page + 1) + ".png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(full, out), "full page image");
            }
            {
                previewAudio(1);
                auto dark = logisthEditor->snapshotPage(9);
                juce::FileOutputStream out(folder.getChildFile("logisth-dark.png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(dark, out), "dark theme image");
            }
            previewAudio(0);
            p.setPresetComment("Clean saw / build your sound here, then save your own notes.");
            std::unique_ptr<juce::AudioProcessorEditor> gallery(p.createEditor());
            auto *galleryEditor = dynamic_cast<LogisthEditor *>(gallery.get());
            check(galleryEditor != nullptr, "theme gallery editor");
            for (int theme = 0; theme < 5; ++theme) {
                auto preview = galleryEditor->snapshotTheme(theme);
                juce::FileOutputStream out(
                    folder.getChildFile("logisth-theme-" + juce::String(theme) + ".png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(preview, out), "theme gallery image");
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
        std::cout << "PASS compact UI: all parameters reachable, no overlaps, clickable controls, "
                     "80-125% scale, view state preserved, parameter bindings, persistent shared "
                     "themes, pattern drawing and inline entry\n";
        std::cout << "PASS processor: MIDI offsets, state, malformed state, ten presets and FX\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
