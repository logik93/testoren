#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;
using namespace logisth;
AudioProcessorValueTreeState::ParameterLayout LogisthProcessor::layout() {
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto &s : parameterSpecs) {
        String name = String(s.group) + " / " + s.label;
        if (*s.choices) {
            auto choices = StringArray::fromTokens(s.choices, "|", "");
            layout.add(std::make_unique<AudioParameterChoice>(ParameterID{s.id, 1}, name, choices,
                                                              int(s.initial)));
        } else if (s.integer)
            layout.add(std::make_unique<AudioParameterInt>(ParameterID{s.id, 1}, name, int(s.low),
                                                           int(s.high), int(s.initial)));
        else
            layout.add(std::make_unique<AudioParameterFloat>(
                ParameterID{s.id, 1}, name, NormalisableRange<float>(s.low, s.high, 0, s.skew),
                s.initial,
                AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) {
                    return String(v, std::abs(v) >= 100                   ? 0
                                     : std::abs(v) > 0 && std::abs(v) < 1 ? 3
                                                                          : 2);
                })));
    }
    return layout;
}
LogisthProcessor::LogisthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "LogisthState", layout()) {
    for (size_t i = 0; i < parameterCount; ++i)
        raw[i] = state.getRawParameterValue(parameterSpecs[i].id);
}
Params LogisthProcessor::snapshot() const {
    Params p;
    for (int i = 0; i < 4; ++i) {
        size_t at = size_t(Param::o1wave) + size_t(i) * 12;
        auto next = [&] { return raw[at++]->load(); };
        auto &o = p.osc[size_t(i)];
        o.wave = int(next());
        o.unison = int(next());
        o.level = next();
        float octave = next();
        float note = next();
        float fine = next();
        o.semitone = octave * 12 + note + fine * .01f;
        o.phase = next();
        o.detune = next();
        o.stereo = next();
        o.pan = next();
        o.retrigger = next() > 0;
        o.invert = next() > 0;
    }
    for (int i = 0; i < 2; ++i) {
        size_t at = size_t(Param::p1input) + size_t(i) * 10;
        auto next = [&] { return raw[at++]->load(); };
        auto &part = p.part[size_t(i)];
        part.input = int(next());
        part.filterType = int(next());
        part.slope = int(next());
        part.cutoff = next();
        part.resonance = next();
        part.drive = next();
        part.amp = {next(), next(), next(), next()};
        part.level = get(i == 0 ? Param::mixA : Param::mixB);
    }
    auto readRoutes = [&](size_t at) {
        Routes result{};
        for (auto &route : result) {
            route.destination = Destination(int(raw[at++]->load()));
            route.amount = raw[at++]->load();
        }
        return result;
    };
    for (int i = 0; i < 2; ++i) {
        size_t at = size_t(Param::env1a) + size_t(i) * 8;
        auto next = [&] { return raw[at++]->load(); };
        p.envelopes[i] = {next(), next(), next(), next()};
        p.envelopeRoutes[i] = readRoutes(at);
        at = size_t(Param::lfo1wave) + size_t(i) * 10;
        auto &l = p.lfo[i];
        l.wave = int(next());
        l.rate = next();
        l.division = int(next());
        l.gain = next();
        l.offset = next();
        l.free = next() > 0;
        l.routes = readRoutes(at);
    }
    for (int i = 0; i < 4; ++i) {
        size_t at = size_t(Param::misc1source) + size_t(i) * 5;
        p.misc[i].source = Source(int(raw[at]->load()));
        p.misc[i].routes = readRoutes(at + 1);
    }
    p.filterCutoff = get(Param::filtercutoff);
    p.filterResonance = get(Param::filterres);
    p.keytrack = get(Param::keytrack);
    p.warmDrive = get(Param::warmdrive) > 0;
    p.master = 1;
    p.portamento = get(Param::portamento);
    p.portaMode = int(get(Param::portamode));
    p.bendRange = get(Param::bendrange);
    p.polyphony = int(get(Param::polyphony));
    p.monoLegato = get(Param::monolegato) > 0;
    p.sync = get(Param::sync) > 0;
    p.solo = int(get(Param::solo));
    p.bpm = bpm;
    return p;
}
EffectsParams LogisthProcessor::effectsSnapshot() const {
    EffectsParams p;
    p.distortion = get(Param::diston) > 0;
    p.phaser = get(Param::phaseron) > 0;
    p.chorus = get(Param::choruson) > 0;
    p.eq = get(Param::eqon) > 0;
    p.reverb = get(Param::reverbon) > 0;
    p.delay = get(Param::delayon) > 0;
    p.compressor = get(Param::compon) > 0;
    p.sync = get(Param::sync) > 0;
    p.dual = get(Param::chorusdual) > 0;
    p.pingpong = get(Param::delaypingpong) > 0;
    p.distortionType = int(get(Param::disttype));
    p.phaserDivision = int(get(Param::phaserdivision));
    p.chorusDivision = int(get(Param::chorusdivision));
    p.delayDivisionL = int(get(Param::delaydivL));
    p.delayDivisionR = int(get(Param::delaydivR));
    p.drive = get(Param::drive);
    p.distMix = get(Param::distmix);
    p.phaserFreq = get(Param::phaserfreq);
    p.phaserSpread = get(Param::phaserspread);
    p.phaserOffset = get(Param::phaseroffset);
    p.phaserWidth = get(Param::phaserwidth);
    p.phaserRate = get(Param::phaserrate);
    p.phaserGain = get(Param::phasergain);
    p.phaserFeedback = get(Param::phaserfeedback);
    p.phaserMix = get(Param::phasermix);
    p.chorusDelay = get(Param::chorusdelay);
    p.chorusDepth = get(Param::chorusdepth);
    p.chorusRate = get(Param::chorusrate);
    p.chorusFeedback = get(Param::chorusfeedback);
    p.chorusWidth = get(Param::choruswidth);
    p.chorusMix = get(Param::chorus);
    p.bass = get(Param::eqbass);
    p.bassFreq = get(Param::eqbassfreq);
    p.treble = get(Param::eqtreble);
    p.trebleFreq = get(Param::eqtreblefreq);
    p.reverbSize = get(Param::reverbsize);
    p.reverbDamp = get(Param::reverbdamp);
    p.reverbPredelay = get(Param::reverbpredelay);
    p.reverbWidth = get(Param::reverbwidth);
    p.reverbMix = get(Param::reverb);
    p.delayL = get(Param::delaytime);
    p.delayR = get(Param::delayright);
    p.feedback = get(Param::feedback);
    p.delaySpread = get(Param::delayspread);
    p.delayWidth = get(Param::delaywidth);
    p.lowCut = get(Param::delaylowcut);
    p.highCut = get(Param::delayhighcut);
    p.smear = get(Param::delaysmear);
    p.delayMix = get(Param::delay);
    p.ratio = get(Param::compratio);
    p.threshold = get(Param::compthreshold);
    p.attack = get(Param::compattack);
    p.release = get(Param::comprelease);
    p.bpm = bpm;
    return p;
}
ArpParams LogisthProcessor::arpSnapshot() const {
    ArpParams p;
    p.enabled = get(Param::arpon) > 0;
    p.sync = get(Param::sync) > 0;
    p.mode = int(get(Param::arpmode));
    p.velocityMode = int(get(Param::arpvelocity));
    p.division = int(get(Param::arpdivision));
    p.octaves = int(get(Param::arpoctaves));
    p.wrap = int(get(Param::arpwrap));
    p.steps = int(get(Param::arpsteps));
    p.time = get(Param::arptime);
    p.gate = get(Param::arpgate);
    p.swing = get(Param::arpswing);
    for (int i = 0; i < 16; ++i) {
        size_t at = size_t(Param::step1transpose) + size_t(i) * 3;
        p.sequence[i] = {int(raw[at]->load()), int(raw[at + 1]->load()), raw[at + 2]->load() > 0};
    }
    p.bpm = bpm;
    return p;
}
void LogisthProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
    effects.prepare(sampleRate);
    arp.prepare(sampleRate);
    reset();
}
void LogisthProcessor::reset() {
    engine.reset();
    effects.reset();
    arp.reset();
    masterSmooth = 0;
    wasPlaying = false;
    expectedSample = -1;
    peak = 0;
    channelPeaks[0] = 0;
    channelPeaks[1] = 0;
    voices = 0;
}
void LogisthProcessor::processBlock(AudioBuffer<float> &audio, MidiBuffer &midi) {
    ScopedNoDenormals noDenormals;
    audio.clear();
    const int n = audio.getNumSamples();
    if (n <= 0 || audio.getNumChannels() < 2)
        return;
    bool stop = false, seek = false;
    if (auto *head = getPlayHead())
        if (auto position = head->getPosition()) {
            if (auto tempo = position->getBpm())
                if (std::isfinite(*tempo) && *tempo > 0)
                    bpm = jlimit(20., 400., *tempo);
            stop = wasPlaying && !position->getIsPlaying();
            if (auto samples = position->getTimeInSamples()) {
                seek = wasPlaying && position->getIsPlaying() && expectedSample >= 0 &&
                       *samples != expectedSample;
                expectedSample = *samples + n;
            }
            wasPlaying = position->getIsPlaying();
        }
    keyboard.processNextMidiBuffer(midi, 0, n, true);
    const auto p = snapshot();
    const auto fx = effectsSnapshot();
    const auto ap = arpSnapshot();
    const float masterTarget = get(Param::master);
    const float smoothing = float(1 - std::exp(-1 / (.01 * getSampleRate())));
    int rendered = 0, scheduled = 0;
    float maximum = 0;
    std::array<float, 2> maxima{};
    auto renderTo = [&](int end) {
        while (rendered < end) {
            int count = jmin(512, end - rendered);
            auto *l = audio.getWritePointer(0) + rendered;
            auto *r = audio.getWritePointer(1) + rendered;
            engine.render(l, r, count, p, driveMod.data(), phaserMod.data());
            effects.process(l, r, count, driveMod.data(), phaserMod.data(), fx);
            for (int s = 0; s < count; ++s) {
                masterSmooth += smoothing * (masterTarget - masterSmooth);
                for (auto *ch : {l, r}) {
                    float x = ch[s] * masterSmooth;
                    ch[s] = std::isfinite(x) ? x : 0;
                    maximum = jmax(maximum, std::abs(ch[s]));
                    const size_t channel = ch == l ? 0 : 1;
                    maxima[channel] = jmax(maxima[channel], std::abs(ch[s]));
                }
            }
            rendered += count;
        }
    };
    auto emit = [&](MidiEvent event) {
        renderTo(jlimit(rendered, n, event.time));
        switch (event.type) {
        case EventType::NoteOn:
            engine.noteOn(event.key, event.value, event.channel, p);
            break;
        case EventType::NoteOff:
            engine.noteOff(event.key, event.channel, p);
            break;
        case EventType::Controller:
            engine.controller(event.channel, event.key, int(event.value), p);
            break;
        case EventType::PitchWheel:
            engine.pitchWheel(event.channel, event.value);
            break;
        case EventType::Aftertouch:
            engine.aftertouch(event.channel, event.key, event.value);
            break;
        case EventType::StepVelocity:
            engine.setStepVelocity(event.value);
            break;
        }
    };
    arp.begin(ap, emit);
    if (stop || seek)
        arp.stop(0, emit);
    int changes = uiChanges.exchange(0);
    if (changes & 1)
        arp.input(MidiEvent{0, EventType::PitchWheel, 1, 0, pitchPosition.load()}, ap, emit);
    if (changes & 2)
        arp.input(MidiEvent{0, EventType::Controller, 1, 1, wheelPosition.load() * 127}, ap, emit);
    for (const auto metadata : midi) {
        int offset = jlimit(scheduled, n, metadata.samplePosition);
        arp.advance(scheduled, offset, ap, emit);
        scheduled = offset;
        auto m = metadata.getMessage();
        MidiEvent event;
        event.time = offset;
        event.channel = m.getChannel();
        if (m.isNoteOn()) {
            event.type = EventType::NoteOn;
            event.key = m.getNoteNumber();
            event.value = m.getFloatVelocity();
        } else if (m.isNoteOff()) {
            event.type = EventType::NoteOff;
            event.key = m.getNoteNumber();
        } else if (m.isController()) {
            event.type = EventType::Controller;
            event.key = m.getControllerNumber();
            event.value = float(m.getControllerValue());
        } else if (m.isPitchWheel()) {
            event.type = EventType::PitchWheel;
            event.value = (m.getPitchWheelValue() - 8192) / 8192.f;
        } else if (m.isAftertouch()) {
            event.type = EventType::Aftertouch;
            event.key = m.getNoteNumber();
            event.value = m.getAfterTouchValue() / 127.f;
        } else if (m.isChannelPressure()) {
            event.type = EventType::Aftertouch;
            event.key = -1;
            event.value = m.getChannelPressureValue() / 127.f;
        } else
            continue;
        if (event.type == EventType::PitchWheel)
            pitchPosition = event.value;
        if (event.type == EventType::Controller && event.key == 1)
            wheelPosition = event.value / 127.f;
        arp.input(event, ap, emit);
    }
    arp.advance(scheduled, n, ap, emit);
    renderTo(n);
    midi.clear();
    peak = maximum;
    channelPeaks[0] = maxima[0];
    channelPeaks[1] = maxima[1];
    voices = engine.activeVoices();
}
void LogisthProcessor::getStateInformation(MemoryBlock &dest) {
    auto tree = state.copyState();
    tree.setProperty("schema", 2, nullptr);
    copyXmlToBinary(*tree.createXml(), dest);
}
void LogisthProcessor::setStateInformation(const void *data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml)
        return;
    auto restore = [&](const ValueTree &source) {
        ValueTree safe("LogisthState");
        safe.setProperty("schema", 2, nullptr);
        for (const auto &spec : parameterSpecs) {
            float value = spec.initial;
            for (auto child : source)
                if (child["id"].toString() == spec.id) {
                    value = float(child["value"]);
                    break;
                }
            if (!std::isfinite(value))
                value = spec.initial;
            value = jlimit(spec.low, spec.high, value);
            if (spec.integer || *spec.choices)
                value = std::round(value);
            ValueTree child("PARAM");
            child.setProperty("id", spec.id, nullptr);
            child.setProperty("value", value, nullptr);
            safe.addChild(child, -1, nullptr);
        }
        state.replaceState(safe);
    };
    if (xml->hasTagName(state.state.getType())) {
        restore(ValueTree::fromXml(*xml));
        return;
    }
    // Import the previous prototype's state without retaining its product identity.
    if (!xml->hasTagName("AsterionState"))
        return;
    const auto old = ValueTree::fromXml(*xml);
    ValueTree migrated("LogisthState");
    auto oldValue = [&](const String &id, float fallback) {
        for (auto child : old)
            if (child["id"].toString() == id)
                return float(child["value"]);
        return fallback;
    };
    for (const auto &spec : parameterSpecs) {
        ValueTree parameter("PARAM");
        parameter.setProperty("id", spec.id, nullptr);
        parameter.setProperty("value", oldValue(spec.id, spec.initial), nullptr);
        migrated.addChild(parameter, -1, nullptr);
    }
    auto set = [&](const String &id, float value) {
        for (auto child : migrated)
            if (child["id"].toString() == id)
                child.setProperty("value", value, nullptr);
    };
    for (int i = 1; i <= 2; ++i) {
        String part = "p" + String(i), env = "env" + String(i);
        for (auto s : {"a", "d", "s", "r"})
            set(env + s, oldValue(part + "flt" + s, String(s) == "s" ? 0.f : .25f));
        set(env + "amt1", oldValue(part + "env", 24) / 12);
        set(i == 1 ? "mixA" : "mixB", oldValue(part + "level", .7f));
    }
    set("lfo1dest1", 15);
    set("lfo1amt1", oldValue("lfo1depth", 0) / 12);
    set("lfo2dest1", 6);
    set("lfo2amt1", oldValue("lfo2depth", 0) / 4.8f);
    set("diston", oldValue("drive", 0) > 0 ? 1 : 0);
    set("choruson", oldValue("chorus", 0) > 0 ? 1 : 0);
    set("delayon", oldValue("delay", 0) > 0 ? 1 : 0);
    restore(migrated);
}
void LogisthProcessor::loadPreset(int index) {
    for (auto *parameter : getParameters()) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
        parameter->endChangeGesture();
    }
    auto set = [&](const String &id, float value) {
        auto *parameter = state.getParameter(id);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    };
    if (index == 1) {
        set("o1voices", 7);
        set("o1detune", 18);
        set("o2voices", 5);
        set("o2tune", 0);
        set("o2level", .5f);
        set("choruson", 1);
        set("chorus", .25f);
        set("delayon", 1);
        set("delay", .3f);
        set("reverb", .25f);
    }
    if (index == 2) {
        set("o1wave", 2);
        set("o2wave", 0);
        set("p1cutoff", 500);
        set("p1res", .55f);
        set("env1amt1", 4);
        set("p1ampa", .002f);
        set("p1amps", .45f);
        set("env1d", .15f);
        set("diston", 1);
        set("drive", .25f);
        set("reverbon", 0);
    }
    if (index == 3) {
        set("o1voices", 5);
        set("o3level", .5f);
        set("o3voices", 7);
        set("o3tune", 12);
        for (int i = 1; i <= 2; ++i) {
            auto id = "p" + String(i);
            set(id + "ampa", 1.2f);
            set(id + "ampr", 2.5f);
            set(id + "cutoff", 1800);
        }
        set("lfo1dest1", 15);
        set("lfo1amt1", .7f);
        set("lfo1free", 1);
        set("lfo1rate", .2f);
        set("choruson", 1);
        set("chorus", .6f);
        set("reverb", .4f);
    }
    if (index == 4) {
        set("o1wave", 2);
        set("o1voices", 3);
        set("p1cutoff", 1200);
        set("env1amt1", 4);
        set("p1amps", 0);
        set("p1ampd", .22f);
        set("p1ampr", .12f);
        set("env1d", .12f);
        set("delayon", 1);
        set("delay", .3f);
    }
    if (index == 5) {
        set("arpon", 1);
        set("arpmode", 8);
        set("arpsteps", 8);
        for (int i = 1; i <= 8; ++i) {
            set("step" + String(i) + "transpose", i % 2 ? 0 : 12);
            set("step" + String(i) + "velocity", 70 + i * 6);
        }
        set("p1cutoff", 900);
        set("env1amt1", 4);
        set("env1d", .12f);
        set("p1ampd", .2f);
        set("p1amps", .2f);
        set("delayon", 1);
        set("delay", .2f);
    }
}
AudioProcessorEditor *LogisthProcessor::createEditor() {
    return new LogisthEditor(*this);
}
AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new LogisthProcessor();
}
