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
    for (int i = 0; i < 4; ++i)
        publishSource(i, SourceData::factory());
    for (auto &position : graphPlayheads)
        position = -1;
    auto map = std::make_shared<ChordMap>();
    for (int i = 0; i < 128; ++i)
        map->set(i, 1);
    publishChords(map);
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
        size_t ext = size_t(Param::o1engine) + size_t(i) * 16;
        auto sourceNext = [&] { return raw[ext++]->load(); };
        o.engine = int(sourceNext());
        o.position = sourceNext();
        o.end = sourceNext();
        o.root = int(sourceNext());
        o.loop = sourceNext() > 0;
        o.reverse = sourceNext() > 0;
        o.size = sourceNext() * .001f;
        o.density = sourceNext();
        o.spray = sourceNext();
        o.scan = sourceNext();
        o.warp = sourceNext();
        o.envelopeEnabled = sourceNext() > 0;
        o.envelope = {sourceNext(), sourceNext(), sourceNext(), sourceNext()};
        o.data = sourcePointers[size_t(i)].load();
        size_t g = size_t(Param::o1gmodel) + size_t(i) * 15;
        auto granNext = [&] { return raw[g++]->load(); };
        o.gran.enabled = granNext() > 0;
        o.gran.attack = granNext() * .001f;
        o.gran.hold = granNext() * .001f;
        o.gran.grainSpacing = granNext();
        o.gran.waveSpacing = granNext();
        o.gran.pan = granNext();
        o.gran.depth = granNext();
        o.gran.frequency = granNext();
        o.gran.randomness = granNext();
        o.gran.transients = int(granNext());
        o.gran.transientHold = granNext() * .001f;
        o.gran.freeze = granNext() > 0;
        o.gran.keyTo = int(granNext());
        o.gran.sync = granNext() > 0;
        o.gran.beats = granNext();
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
    p.pattern = get(Param::arppattern) > 0;
    p.latch = get(Param::arplatch) > 0;
    for (int i = 16; i < 32; ++i) {
        size_t at = size_t(Param::step17velocity) + size_t(i - 16) * 2;
        p.sequence[size_t(i)] = {0, int(raw[at]->load()), raw[at + 1]->load() > 0};
    }
    for (int i = 0; i < 32; ++i) {
        size_t at = size_t(Param::pat1note) + size_t(i) * 5;
        p.lanes[size_t(i)] = {int(raw[at]->load()), int(raw[at + 1]->load()),
                              int(raw[at + 2]->load()), int(raw[at + 3]->load()),
                              int(raw[at + 4]->load())};
    }
    for (int i = 0; i < 6; ++i)
        p.lengths[size_t(i)] = int(raw[size_t(Param::arpnotelen) + size_t(i)]->load());
    p.graphs.enabled = get(Param::arpgraphs) > 0;
    p.graphs.retrigger = get(Param::gretrigger) > 0;
    p.graphs.link = get(Param::glink) > 0;
    p.graphs.chordMode = get(Param::gchord) > 0;
    p.graphs.division = int(get(Param::gdivision));
    p.graphs.freeHz = get(Param::gfree);
    for (int lane = 0; lane < 8; ++lane) {
        p.graphs.lengths[size_t(lane)] = int(raw[size_t(Param::glen0) + size_t(lane) * 2]->load());
        p.graphs.divisors[size_t(lane)] = int(raw[size_t(Param::gdiv0) + size_t(lane) * 2]->load());
    }
    for (int i = 0; i < 7; ++i)
        p.graphs.degrees[size_t(i)] = raw[size_t(Param::gdegree0) + size_t(i)]->load() > 0;
    for (int i = 0; i < 12; ++i)
        p.graphs.constraints[size_t(i)] = int(raw[size_t(Param::gprevent0) + size_t(i)]->load());
    for (int i = 0; i < 32; ++i) {
        size_t at = size_t(Param::g1note) + size_t(i) * 10;
        auto next = [&] { return int(raw[at++]->load()); };
        auto &g = p.graphs.steps[size_t(i)];
        g.note = next();
        g.random = next();
        g.octave = next();
        g.pitch = next();
        g.velocity = next();
        g.gate = next();
        g.late = next();
        g.harmony = next();
        g.mute = next() > 0;
        g.reset = next() > 0;
    }
    p.clockAvailable = clockAvailable;
    p.transportPlaying = wasPlaying;
    p.ppq = hostPpq;
    p.positionValid = hostPpqValid;
    p.bpm = bpm;
    return p;
}
void LogisthProcessor::prepareToPlay(double sampleRate, int) {
    generatedMidi.ensureSize(131072);
    engine.prepare(sampleRate);
    effects.prepare(sampleRate);
    arp.prepare(sampleRate);
    reset();
}
void LogisthProcessor::reset() {
    engine.reset();
    scope.clear();
    chordMapper.reset();
    chordsWereEnabled = false;
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
    struct AssetReadScope {
        std::atomic<int> &count;
        AssetReadScope(std::atomic<int> &c) : count(c) {
            count.fetch_add(1);
        }
        ~AssetReadScope() {
            count.fetch_sub(1);
        }
    } assetRead(assetReaders);
    generatedMidi.clear();
    bool stop = false, seek = false;
    clockAvailable = false;
    hostPpqValid = false;
    if (auto *head = getPlayHead())
        if (auto position = head->getPosition()) {
            clockAvailable = true;
            if (auto ppq = position->getPpqPosition()) {
                hostPpq = *ppq;
                hostPpqValid = std::isfinite(hostPpq);
            }
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
            engine.render(l, r, count, p, driveMod.data(), phaserMod.data(), partScopeA.data(),
                          partScopeB.data());
            effects.process(l, r, count, driveMod.data(), phaserMod.data(), fx);
            for (int s = 0; s < count; ++s) {
                masterSmooth += smoothing * (masterTarget - masterSmooth);
                for (auto *ch : {l, r}) {
                    float x = get(Param::gmuteaudio) > 0 ? 0 : ch[s] * masterSmooth;
                    ch[s] = std::isfinite(x) ? x : 0;
                    maximum = jmax(maximum, std::abs(ch[s]));
                    const size_t channel = ch == l ? 0 : 1;
                    maxima[channel] = jmax(maxima[channel], std::abs(ch[s]));
                }
                scope.push(partScopeA[size_t(s)] * masterSmooth,
                           partScopeB[size_t(s)] * masterSmooth, l[s] + r[s]);
            }
            const auto &views = engine.playbackPictures();
            for (int i = 0; i < 4; ++i)
                playback[size_t(i)].write(views[size_t(i)]);
            rendered += count;
        }
    };
    const bool output = get(Param::arpout) > 0;
    const bool flushMidi = midiWasEnabled && !output;
    if (flushMidi)
        for (int ch = 1; ch <= 16; ++ch)
            generatedMidi.addEvent(MidiMessage::allNotesOff(ch), 0);
    midiWasEnabled = output;
    auto emit = [&](MidiEvent event) {
        if (output && event.time < n) {
            if (event.type == EventType::NoteOn)
                generatedMidi.addEvent(MidiMessage::noteOn(event.channel, event.key, event.value),
                                       event.time);
            else if (event.type == EventType::NoteOff)
                generatedMidi.addEvent(MidiMessage::noteOff(event.channel, event.key), event.time);
            else if (event.type == EventType::Controller)
                generatedMidi.addEvent(
                    MidiMessage::controllerEvent(event.channel, event.key, int(event.value)),
                    event.time);
            else if (event.type == EventType::PitchWheel)
                generatedMidi.addEvent(
                    MidiMessage::pitchWheel(event.channel,
                                            jlimit(0, 16383, int(event.value * 8192 + 8192))),
                    event.time);
            else if (event.type == EventType::Aftertouch)
                generatedMidi.addEvent(
                    event.key < 0
                        ? MidiMessage::channelPressureChange(event.channel, int(event.value * 127))
                        : MidiMessage::aftertouchChange(event.channel, event.key,
                                                        int(event.value * 127)),
                    event.time);
        }
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
    const bool chordEnabled = get(Param::chordon) > 0;
    if (stop || seek || chordsWereEnabled != chordEnabled) {
        arp.stop(0, emit);
        chordMapper.reset();
    }
    chordsWereEnabled = chordEnabled;
    int changes = uiChanges.exchange(0);
    if (changes & 4) {
        arp.stop(0, emit);
        chordMapper.reset();
    }
    auto mapped = [&](MidiEvent event) { arp.input(event, ap, emit); };
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
        } else {
            if (output && m.getRawDataSize() <= 4096)
                generatedMidi.addEvent(m, offset);
            continue;
        }
        if (event.type == EventType::PitchWheel)
            pitchPosition = event.value;
        if (event.type == EventType::Controller && event.key == 1)
            wheelPosition = event.value / 127.f;
        const int learn = learnTarget.load();
        if (learn >= 0 && (event.type == EventType::NoteOn || event.type == EventType::NoteOff)) {
            if (event.type == EventType::NoteOn)
                learned[size_t(event.key / 32)].fetch_or(uint32_t(1) << (event.key % 32));
            continue;
        }
        chordMapper.input(event, chordEnabled, *chordPointer.load(), mapped);
    }
    arp.advance(scheduled, n, ap, emit);
    renderTo(n);
    scope.commit();
    for (int i = 0; i < 8; ++i)
        graphPlayheads[size_t(i)] = arp.graphHead(i);
    arpPlayhead = arp.playhead();
    midi.clear();
    if (output || flushMidi)
        midi.swapWith(generatedMidi);
    peak = maximum;
    channelPeaks[0] = maxima[0];
    channelPeaks[1] = maxima[1];
    voices = engine.activeVoices();
}
void LogisthProcessor::getStateInformation(MemoryBlock &dest) {
    auto tree = state.copyState();
    tree.setProperty("schema", 4, nullptr);
    tree.setProperty("presetName", currentPresetName(), nullptr);
    saveAssets(tree);
    copyXmlToBinary(*tree.createXml(), dest);
}
void LogisthProcessor::setStateInformation(const void *data, int size) {
    applyState(data, size);
}
bool LogisthProcessor::applyState(const void *data, int size) {
    if (size <= 0 || size > 64 * 1024 * 1024)
        return false;
    auto xml = getXmlFromBinary(data, size);
    if (!xml)
        return false;
    auto restore = [&](const ValueTree &source) -> bool {
        ValueTree safe("LogisthState");
        safe.setProperty("schema", 4, nullptr);
        if (source.hasProperty("uiScope"))
            safe.setProperty("uiScope", jlimit(0, 2, int(source["uiScope"])), nullptr);
        if (source.hasProperty("uiDark"))
            safe.setProperty("uiDark", int(source["uiDark"]) != 0, nullptr);
        for (const auto &spec : parameterSpecs) {
            float value = (String(spec.id) == "arppattern" || String(spec.id) == "arpgraphs" ||
                           String(spec.id).endsWith("gmodel"))
                              ? 0
                              : spec.initial;
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
        if (!restoreAssets(source))
            return false;
        state.replaceState(safe);
        {
            const ScopedLock lock(assetLock);
            presetName = source.getProperty("presetName", "").toString().substring(0, 128);
            activePresetFile = File{};
        }
        ++presetRevision;
        uiChanges.fetch_or(4);
        return true;
    };
    if (xml->hasTagName(state.state.getType())) {
        return restore(ValueTree::fromXml(*xml));
    }
    // Import the previous prototype's state without retaining its product identity.
    if (!xml->hasTagName("AsterionState"))
        return false;
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
    set("arppattern", 0);
    set("lfo1dest1", 15);
    set("lfo1amt1", oldValue("lfo1depth", 0) / 12);
    set("lfo2dest1", 6);
    set("lfo2amt1", oldValue("lfo2depth", 0) / 4.8f);
    set("diston", oldValue("drive", 0) > 0 ? 1 : 0);
    set("choruson", oldValue("chorus", 0) > 0 ? 1 : 0);
    set("delayon", oldValue("delay", 0) > 0 ? 1 : 0);
    return restore(migrated);
}
void LogisthProcessor::loadPreset(int index) {
    for (auto *parameter : getParameters()) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
        parameter->endChangeGesture();
    }
    {
        const ScopedLock lock(assetLock);
        presetName.clear();
        activePresetFile = File{};
        patterns = {};
        currentPattern = 0;
    }
    for (int i = 0; i < 4; ++i)
        clearSource(i);
    auto defaultChords = std::make_shared<ChordMap>();
    for (int key = 0; key < 128; ++key)
        defaultChords->set(key, 1);
    publishChords(defaultChords);
    ++presetRevision;
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
        set("arppattern", 0);
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
    if (index >= 6 && index <= 9) {
        clearSource(0);
        set("o2level", 0);
        set("p1cutoff", 8500);
        set("env1amt1", 0);
        if (index == 6) {
            set("o1engine", 1);
            set("o1position", .55f);
            set("o1voices", 3);
            set("lfo1dest1", 28);
            set("lfo1amt1", 2);
            set("lfo1free", 1);
            set("lfo1rate", .2f);
        }
        if (index == 7) {
            set("o1engine", 2);
            set("o1loop", 0);
            set("p1ampr", .8f);
        }
        if (index == 8) {
            set("o1gmodel", 0);
            set("o1engine", 3);
            set("o1position", .2f);
            set("o1size", 140);
            set("o1density", 28);
            set("o1spray", .5f);
            set("o1scan", .15f);
            set("p1ampa", .5f);
            set("p1ampr", 1.8f);
            set("reverb", .5f);
        }
        if (index == 9) {
            set("o1engine", 1);
            set("o1position", .75f);
            set("arpon", 1);
            set("arppattern", 1);
            set("chordon", 1);
            set("p1amps", .15f);
            set("p1ampd", .15f);
            set("delayon", 1);
            set("delay", .25f);
        }
    }
    uiChanges.fetch_or(4);
}
AudioProcessorEditor *LogisthProcessor::createEditor() {
    return new LogisthEditor(*this);
}
AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new LogisthProcessor();
}
