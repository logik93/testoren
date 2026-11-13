#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include <thread>
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
        set("g1pitch", float(slot - 6));
        p.setPatternName(slot, "Phrase " + String(slot + 1));
    }
    for (int slot = 0; slot < 12; ++slot) {
        p.recallPattern(slot);
        check(get("g1pitch") == slot - 6, "A-L automatically retain edits");
    }
    p.copyPattern(11, 3);
    p.recallPattern(3);
    check(get("g1pitch") == 5 && p.patternName(3) == "Phrase 12", "whole pattern copy");
    set("g1pitch", -24);
    set("glen3", 7);
    set("gdiv3", 3);
    p.copyGraph(3);
    p.pasteGraph(2);
    check(get("g1octave") == -8 && get("gdiv2") == 3 && get("glen7") == 7,
          "graph clipboard scales shape and linked lengths");
    set("glink", 0);
    set("glen2", 9);
    p.pasteGraph(3);
    check(get("glen2") == 9 && get("glen3") == 7, "unlinked paste retains other lengths");
    p.setChord(61, 4);
    p.state.state.setProperty("uiScope", 1, nullptr);
    TemporaryFile preset(".logisth");
    p.setPresetDirectory(preset.getFile().getParentDirectory());
    check(p.savePresetFile(preset.getFile()).wasOk(), "user preset atomic save");
    check(p.currentPresetFile() == preset.getFile(), "save tracks active preset");
    set("g1pitch", 18);
    p.setChord(61, 1);
    check(p.loadPresetFile(preset.getFile()).wasOk(), "user preset load");
    check(get("g1pitch") == -24 && p.chordMap()->keys[61].count == 4 &&
              int(p.state.state.getProperty("uiScope")) == 1,
          "preset restores graphs, chords and scope selection");
    p.recallPattern(11);
    check(get("g1pitch") == 5 && p.patternName(11) == "Phrase 12", "preset retains A-L bank");
    preset.getFile().replaceWithText("not a preset");
    check(p.loadPresetFile(preset.getFile()).failed() && get("g1pitch") == 5,
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
    set("gmuteaudio", 1);
    midi.clear();
    p.processBlock(audio, midi);
    check(audio.getMagnitude(0, 512) == 0, "MIDI-only mode silences output");
    std::cout
        << "PASS studio processor: 12 automatic pattern memories, graph clipboard, "
           "user preset files, malformed file rejection, real sum scope and grain telemetry\n";
}

int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        checkEffects();
        checkHybridProcessor();
        checkStudioProcessor();
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
        check(restored.state.getRawParameterValue("misc1dest1")->load() == 33 &&
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
            check(int(restored.state.state.getProperty("uiDark", 0)) == 1,
                  "theme is saved in plugin state");
            {
                std::unique_ptr<juce::AudioProcessorEditor> second(restored.createEditor());
                auto *other = dynamic_cast<LogisthEditor *>(second.get());
                check(other && other->isDarkTheme(), "restored editor opens in dark theme");
                compact->setDarkTheme(false);
                check(!compact->isDarkTheme() && other->isDarkTheme(),
                      "themes are independent across plugin instances");
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
            compact->snapshotPage(13);
            canvas = ui->getChildComponent(0)->findChildWithID("graph.canvas");
            check(canvas && canvas->isVisible(), "eight-graph drawing view available");
            canvas->mouseDown(eventAt({25, 125}, juce::ModifierKeys::leftButtonModifier));
            check(p.state.getRawParameterValue("g2mute")->load() == 1,
                  "graph mute row controls mute without erasing note");
            canvas->mouseDown(eventAt({25, 40}, juce::ModifierKeys::leftButtonModifier |
                                                    juce::ModifierKeys::altModifier));
            check(p.state.getRawParameterValue("g2reset")->load() == 1,
                  "Alt graph click creates motion reset marker");
            canvas->mouseDown(eventAt({25, 40}, juce::ModifierKeys::rightButtonModifier));
            valueEditor = dynamic_cast<juce::TextEditor *>(canvas->getChildComponent(0));
            check(valueEditor && valueEditor->isVisible(), "exact graph entry opens");
            valueEditor->setText("U+D");
            valueEditor->onReturnKey();
            check(p.state.getRawParameterValue("g2note")->load() == 12,
                  "graph exact entry accepts note motion names");
            canvas->mouseDown(eventAt({5, 150}, juce::ModifierKeys::leftButtonModifier));
            check(p.state.getRawParameterValue("gprevent0")->load() == 1,
                  "graph pitch constraint cycles to Block");
            canvas->mouseDown(eventAt({260, 1}, juce::ModifierKeys::leftButtonModifier));
            canvas->mouseUp(eventAt({260, 1}, 0));
            check(p.state.getRawParameterValue("glen0")->load() == 16,
                  "graph length bar sets linked length");
        }
        p.setStateInformation(uiState.getData(), int(uiState.getSize()));
        if (argc > 1) {
            juce::File folder(juce::String::fromUTF8(argv[1]));
            folder.createDirectory();
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            p.loadPreset(0);
            for (const auto *id : {"o1engine", "o2engine"}) {
                auto *param = p.state.getParameter(id);
                param->setValueNotifyingHost(param->convertTo0to1(3));
            }
            midi.clear();
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
            for (int i = 0; i < 18; ++i) {
                p.processBlock(audio, midi);
                midi.clear();
            }
            auto *logisthEditor = dynamic_cast<LogisthEditor *>(editor.get());
            check(logisthEditor != nullptr, "editor type");
            for (int page = 0; page < 14; ++page) {
                auto full = logisthEditor->snapshotPage(page);
                juce::FileOutputStream out(
                    folder.getChildFile("logisth-page-" + juce::String(page + 1) + ".png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(full, out), "full page image");
            }
            {
                auto dark = logisthEditor->snapshotPage(4);
                juce::FileOutputStream out(folder.getChildFile("logisth-dark.png"));
                out.setPosition(0);
                out.truncate();
                juce::PNGImageFormat format;
                check(format.writeImageToStream(dark, out), "dark theme image");
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
                     "80-125% scale, view state preserved, parameter bindings, independent "
                     "light/dark themes saved in state, pattern drawing and inline entry\n";
        std::cout << "PASS processor: MIDI offsets, state, malformed state, ten presets and FX\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
