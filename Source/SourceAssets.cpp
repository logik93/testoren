#include "PluginProcessor.h"
using namespace juce;
using namespace logisth;
LogisthProcessor::~LogisthProcessor() {
    importWorkers.removeAllJobs(true, -1);
}
void LogisthProcessor::serviceAssets() {
    const ScopedLock lock(assetLock);
    if (assetReaders.load() == 0) {
        retiredSources.clear();
        retiredChords.clear();
    }
}
bool LogisthProcessor::publishSource(int index, std::shared_ptr<const SourceData> data) {
    if (index < 0 || index >= 4 || !data)
        return false;
    const ScopedLock lock(assetLock);
    uint64_t bytes = data == SourceData::factory() ? 0 : data->samples[0].size() * 8ull;
    for (int i = 0; i < 4; ++i)
        if (i != index && sourceOwners[size_t(i)] &&
            sourceOwners[size_t(i)] != SourceData::factory())
            bytes += sourceOwners[size_t(i)]->samples[0].size() * 8ull;
    if (bytes > sampleMemoryBudget)
        return false;
    if (sourceOwners[size_t(index)])
        retiredSources.push_back(sourceOwners[size_t(index)]);
    sourceOwners[size_t(index)] = std::move(data);
    sourcePointers[size_t(index)].store(sourceOwners[size_t(index)].get());
    if (assetReaders.load() == 0)
        retiredSources.clear();
    return true;
}
std::shared_ptr<const SourceData> LogisthProcessor::sourceData(int index) const {
    const ScopedLock lock(assetLock);
    return sourceOwners[size_t(jlimit(0, 3, index))];
}
void LogisthProcessor::clearSource(int index) {
    publishSource(index, SourceData::factory());
}
String LogisthProcessor::sourceStatus() const {
    const ScopedLock lock(assetLock);
    return assetStatus;
}
Result LogisthProcessor::loadSource(int index, const File &file, int frameSize) try {
    if (index < 0 || index >= 4)
        return Result::fail("Invalid source slot");
    if (frameSize != 256 && frameSize != 512 && frameSize != 1024 && frameSize != 2048)
        return Result::fail("Unsupported frame size");
    AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
        return Result::fail("Cannot decode this audio file. Use WAV, AIFF or FLAC.");
    if (!std::isfinite(reader->sampleRate) || reader->sampleRate < 8000 ||
        reader->sampleRate > 96000 || reader->lengthInSamples < 2 || reader->numChannels < 1)
        return Result::fail("Use an audio file at 8-96 kHz.");
    uint64_t otherBytes = 0;
    {
        const ScopedLock lock(assetLock);
        for (int i = 0; i < 4; ++i)
            if (i != index && sourceOwners[size_t(i)] != SourceData::factory())
                otherBytes += sourceOwners[size_t(i)]->samples[0].size() * 8ull;
    }
    if (!fitsSampleMemory(uint64_t(reader->lengthInSamples), otherBytes))
        return Result::fail("The four sources exceed the 512 MiB decoded-audio memory budget.");
    int n = int(reader->lengthInSamples);
    auto data = std::make_shared<SourceData>();
    data->sampleRate = reader->sampleRate;
    data->name = file.getFileNameWithoutExtension().toStdString();
    for (auto &channel : data->samples)
        channel.resize(size_t(n));
    float *channels[] = {data->samples[0].data(), data->samples[1].data()};
    AudioBuffer<float> buffer(channels, 2, n);
    if (!reader->read(&buffer, 0, n, 0, true, true))
        return Result::fail("Audio file could not be read");
    if (reader->numChannels == 1)
        std::copy(data->samples[0].begin(), data->samples[0].end(), data->samples[1].begin());
    for (auto &channel : data->samples)
        for (auto &sample : channel) {
            if (!std::isfinite(sample))
                return Result::fail("Audio contains non-finite samples");
            sample = jlimit(-8.f, 8.f, sample);
        }
    data->buildTables(frameSize);
    data->analyseTransients();
    int cueCount =
        jlimit(0, 4096, reader->metadataValues.getValue("NumCuePoints", "0").getIntValue());
    for (int i = 0; i < cueCount; ++i) {
        int64 offset =
            reader->metadataValues.getValue("Cue" + String(i) + "Offset", "-1").getLargeIntValue();
        if (offset >= 0 && offset < n)
            data->regions.push_back(offset);
    }
    std::sort(data->regions.begin(), data->regions.end());
    data->regions.erase(std::unique(data->regions.begin(), data->regions.end()),
                        data->regions.end());
    if (!publishSource(index, data))
        return Result::fail("The four sources exceed the 512 MiB decoded-audio memory budget.");
    return Result::ok();
} catch (const std::bad_alloc &) {
    return Result::fail("Not enough memory to load this sample; the previous source was kept.");
}
void LogisthProcessor::loadSourceAsync(int index, const File &file, int frameSize,
                                       bool switchClassicToSampler) {
    {
        const ScopedLock lock(assetLock);
        assetStatus = "Loading " + file.getFileName() + "...";
    }
    importWorkers.addJob([this, index, file, frameSize, switchClassicToSampler] {
        auto result = loadSource(index, file, frameSize);
        if (result.wasOk() && switchClassicToSampler) {
            auto id = "o" + String(index + 1) + "engine";
            if (state.getRawParameterValue(id)->load() == 0) {
                auto *parameter = state.getParameter(id);
                parameter->setValueNotifyingHost(parameter->convertTo0to1(2));
            }
        }
        const ScopedLock lock(assetLock);
        assetStatus = result.wasOk() ? "Loaded " + file.getFileName() : result.getErrorMessage();
    });
}
std::shared_ptr<const ChordMap> LogisthProcessor::chordMap() const {
    const ScopedLock lock(assetLock);
    return chordOwner;
}
void LogisthProcessor::publishChords(std::shared_ptr<const ChordMap> map) {
    const ScopedLock lock(assetLock);
    if (chordOwner)
        retiredChords.push_back(chordOwner);
    chordOwner = std::move(map);
    chordPointer.store(chordOwner.get());
    if (assetReaders.load() == 0)
        retiredChords.clear();
}
void LogisthProcessor::setChord(int trigger, int type) {
    if (trigger < 0 || trigger > 127)
        return;
    auto map = std::make_shared<ChordMap>(*chordMap());
    map->set(trigger, type);
    publishChords(map);
}
void LogisthProcessor::setChordNote(int trigger, int voice, int note) {
    if (trigger < 0 || trigger > 127 || voice < 0 || voice > 7)
        return;
    auto map = std::make_shared<ChordMap>(*chordMap());
    auto &c = map->keys[size_t(trigger)];
    if (note < 0) {
        if (voice < c.count) {
            for (int i = voice; i < c.count - 1; ++i)
                c.notes[size_t(i)] = c.notes[size_t(i + 1)];
            --c.count;
        }
    } else {
        note = jlimit(0, 127, note);
        for (int i = 0; i < c.count; ++i)
            if (c.notes[size_t(i)] == note && i != voice)
                return;
        if (voice > c.count)
            return;
        c.notes[size_t(voice)] = note;
        c.count = std::max(c.count, voice + 1);
        std::sort(c.notes.begin(), c.notes.begin() + c.count);
    }
    publishChords(map);
}
void LogisthProcessor::learnChord(int trigger) {
    for (auto &bits : learned)
        bits = 0;
    learnTarget = jlimit(0, 127, trigger);
    uiChanges.fetch_or(4);
}
void LogisthProcessor::finishLearn() {
    int trigger = learnTarget.exchange(-1);
    if (trigger < 0)
        return;
    auto map = std::make_shared<ChordMap>(*chordMap());
    ChordMap::Chord c;
    for (int note = 0; note < 128 && c.count < 8; ++note)
        if (learned[size_t(note / 32)].load() & (uint32_t(1) << (note % 32)))
            c.notes[size_t(c.count++)] = note;
    if (c.count > 0) {
        map->keys[size_t(trigger)] = c;
        publishChords(map);
    }
    uiChanges.fetch_or(4);
}
namespace {
bool patternParameter(const String &id) {
    return (id.startsWith("arp") && id != "arpon" && id != "arpout") || id.startsWith("step") ||
           id.startsWith("pat") ||
           (id.startsWith("g") && !id.startsWith("gprevent") && id != "gmuteaudio");
}
} // namespace
void LogisthProcessor::storePattern(int slot) {
    if (slot < 0 || slot >= 12)
        return;
    ValueTree tree("PATTERN");
    tree.setProperty("slot", slot, nullptr);
    tree.setProperty("name", patternName(slot), nullptr);
    for (auto &spec : parameterSpecs)
        if (patternParameter(spec.id)) {
            ValueTree value("P");
            value.setProperty("id", spec.id, nullptr);
            value.setProperty("value", state.getRawParameterValue(spec.id)->load(), nullptr);
            tree.addChild(value, -1, nullptr);
        }
    const ScopedLock lock(assetLock);
    patterns[size_t(slot)] = tree;
}
void LogisthProcessor::recallPattern(int slot) {
    if (slot < 0 || slot >= 12 || slot == currentPattern)
        return;
    if (currentPattern >= 0)
        storePattern(currentPattern);
    ValueTree tree;
    {
        const ScopedLock lock(assetLock);
        tree = patterns[size_t(slot)].createCopy();
    }
    if (!tree.isValid() || tree.getNumChildren() == 0) {
        tree = ValueTree("PATTERN");
        tree.setProperty("slot", slot, nullptr);
        for (const auto &spec : parameterSpecs)
            if (patternParameter(spec.id)) {
                ValueTree value("P");
                value.setProperty("id", spec.id, nullptr);
                value.setProperty("value", spec.initial, nullptr);
                tree.addChild(value, -1, nullptr);
            }
    }
    currentPattern = slot;
    for (auto v : tree)
        if (patternParameter(v["id"].toString()))
            if (auto *parameter = state.getParameter(v["id"].toString())) {
                float value = float(v["value"]);
                if (!std::isfinite(value))
                    continue;
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
                parameter->endChangeGesture();
            }
}
void LogisthProcessor::randomisePattern() {
    auto set = [&](const String &id, float value) {
        auto *p = state.getParameter(id);
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(value));
        p->endChangeGesture();
    };
    auto &r = Random::getSystemRandom();
    for (int i = 1; i <= 32; ++i) {
        String b = "pat" + String(i);
        set(b + "note", float(r.nextInt(4)));
        set(b + "oct", float(r.nextInt(3) - 1));
        set(b + "gate", float(35 + r.nextInt(56)));
        set(b + "repeat", r.nextInt(8) == 0 ? 2.f : 1.f);
        set(b + "chance", 100);
        set("step" + String(i) + "velocity", float(70 + r.nextInt(58)));
        set("step" + String(i) + "hold", 0);
    }
}
void LogisthProcessor::saveAssets(ValueTree &tree) {
    std::array<std::shared_ptr<const SourceData>, 4> owners;
    std::shared_ptr<const ChordMap> chordsSnapshot;
    std::array<ValueTree, 12> patternSnapshot;
    {
        const ScopedLock lock(assetLock);
        owners = sourceOwners;
        chordsSnapshot = chordOwner;
        for (int i = 0; i < 12; ++i)
            patternSnapshot[size_t(i)] = patterns[size_t(i)].createCopy();
    }
    tree.setProperty("currentPattern", currentPattern.load(), nullptr);
    for (int slot = 0; slot < 4; ++slot) {
        const auto &data = owners[size_t(slot)];
        if (data == SourceData::factory())
            continue;
        MemoryOutputStream compressed;
        {
            GZIPCompressorOutputStream zip(compressed);
            zip.writeDouble(data->sampleRate);
            zip.writeInt(int(data->samples[0].size()));
            zip.writeInt(data->frameSize);
            zip.writeString(data->name);
            for (const auto &channel : data->samples) {
#if JUCE_BIG_ENDIAN
                for (float sample : channel)
                    zip.writeFloat(sample);
#else
                zip.write(channel.data(), channel.size() * sizeof(float));
#endif
            }
        }
        ValueTree node("SOURCE");
        node.setProperty("slot", slot, nullptr);
        String regions;
        for (auto point : data->regions)
            regions += String(point) + " ";
        node.setProperty("regions", regions.trim(), nullptr);
        node.setProperty("audio", var(compressed.getMemoryBlock()), nullptr);
        tree.addChild(node, -1, nullptr);
    }
    ValueTree chords("CHORDMAP");
    for (int key = 0; key < 128; ++key) {
        const auto &chord = chordsSnapshot->keys[size_t(key)];
        if (!chord.count)
            continue;
        ValueTree node("CHORD");
        node.setProperty("key", key, nullptr);
        String notes;
        for (int i = 0; i < chord.count; ++i)
            notes += String(chord.notes[size_t(i)]) + " ";
        node.setProperty("notes", notes.trim(), nullptr);
        chords.addChild(node, -1, nullptr);
    }
    tree.addChild(chords, -1, nullptr);
    for (auto &pattern : patternSnapshot)
        if (pattern.isValid())
            tree.addChild(pattern.createCopy(), -1, nullptr);
}
bool LogisthProcessor::restoreAssets(const ValueTree &tree) {
    std::array<std::shared_ptr<const SourceData>, 4> sources;
    sources.fill(SourceData::factory());
    auto map = std::make_shared<ChordMap>();
    for (int key = 0; key < 128; ++key)
        map->set(key, 1);
    std::array<ValueTree, 12> restoredPatterns;
    std::array<bool, 4> seen{};
    uint64_t totalDecoded = 0;
    for (auto node : tree) {
        if (node.hasType("SOURCE")) {
            int slot = int(node.getProperty("slot", -1));
            const auto *blob = node["audio"].getBinaryData();
            if (slot < 0 || slot > 3 || seen[size_t(slot)] || !blob ||
                blob->getSize() > sourceBlobLimit)
                return false;
            seen[size_t(slot)] = true;
            MemoryInputStream input(*blob, false);
            GZIPDecompressorInputStream zip(input);
            double sr = zip.readDouble();
            int n = zip.readInt(), frameSize = zip.readInt();
            std::array<char, 4096> nameBytes{};
            size_t nameLength = 0;
            for (; nameLength < nameBytes.size(); ++nameLength) {
                if (zip.read(&nameBytes[nameLength], 1) != 1)
                    return false;
                if (nameBytes[nameLength] == 0)
                    break;
            }
            if (nameLength == nameBytes.size())
                return false;
            String name = String::fromUTF8(nameBytes.data(), int(nameLength));
            if (!std::isfinite(sr) || sr < 8000 || sr > 96000 || n < 2 ||
                !fitsSampleMemory(uint64_t(n), totalDecoded) || name.length() > 512 ||
                (frameSize != 256 && frameSize != 512 && frameSize != 1024 && frameSize != 2048))
                return false;
            totalDecoded += uint64_t(n) * 8;
            auto data = std::make_shared<SourceData>();
            data->sampleRate = sr;
            data->name = name.toStdString();
            for (auto &channel : data->samples) {
                channel.resize(size_t(n));
                auto *bytes = reinterpret_cast<char *>(channel.data());
                size_t remaining = channel.size() * sizeof(float);
                while (remaining > 0) {
                    int read = zip.read(bytes, int(std::min<size_t>(remaining, 65536)));
                    if (read <= 0)
                        return false;
                    bytes += read;
                    remaining -= size_t(read);
                }
#if JUCE_BIG_ENDIAN
                for (auto &value : channel) {
                    uint32 bits;
                    std::memcpy(&bits, &value, sizeof(bits));
                    bits = ByteOrder::swap(bits);
                    std::memcpy(&value, &bits, sizeof(bits));
                }
#endif
                for (float value : channel)
                    if (!std::isfinite(value) || std::abs(value) > 8)
                        return false;
            }
            char extra;
            if (zip.read(&extra, 1) != 0)
                return false;
            data->buildTables(frameSize);
            data->analyseTransients();
            auto markers = StringArray::fromTokens(node["regions"].toString(), " ", "");
            if (markers.size() > 4096)
                return false;
            for (const auto &marker : markers) {
                auto at = marker.getLargeIntValue();
                if (!marker.containsOnly("0123456789") || at < 0 || at >= n)
                    return false;
                data->regions.push_back(at);
            }
            std::sort(data->regions.begin(), data->regions.end());
            data->regions.erase(std::unique(data->regions.begin(), data->regions.end()),
                                data->regions.end());
            sources[size_t(slot)] = data;
        } else if (node.hasType("CHORDMAP")) {
            *map = ChordMap{};
            std::array<bool, 128> seenKeys{};
            for (auto entry : node) {
                int key = int(entry.getProperty("key", -1));
                auto tokens = StringArray::fromTokens(entry["notes"].toString(), " ", "");
                if (key < 0 || key > 127 || tokens.size() > 8 || seenKeys[size_t(key)])
                    return false;
                seenKeys[size_t(key)] = true;
                auto &chord = map->keys[size_t(key)];
                for (const auto &token : tokens) {
                    int note = token.getIntValue();
                    if (note < 0 || note > 127 || token.containsOnly("0123456789") == false)
                        return false;
                    if (std::find(chord.notes.begin(), chord.notes.begin() + chord.count, note) ==
                        chord.notes.begin() + chord.count)
                        chord.notes[size_t(chord.count++)] = note;
                }
            }
        } else if (node.hasType("PATTERN")) {
            int slot = int(node.getProperty("slot", -1));
            if (slot < 0 || slot > 11)
                return false;
            restoredPatterns[size_t(slot)] = node.createCopy();
        }
    }
    {
        const ScopedLock lock(assetLock);
        retiredSources.reserve(retiredSources.size() + 4);
        retiredChords.reserve(retiredChords.size() + 1);
        for (int slot = 0; slot < 4; ++slot) {
            retiredSources.push_back(sourceOwners[size_t(slot)]);
            sourceOwners[size_t(slot)] = std::move(sources[size_t(slot)]);
            sourcePointers[size_t(slot)].store(sourceOwners[size_t(slot)].get());
        }
        publishChords(map);
        if (assetReaders.load() == 0)
            retiredSources.clear();
        patterns = std::move(restoredPatterns);
        currentPattern = jlimit(0, 11, int(tree.getProperty("currentPattern", 0)));
    }
    return true;
}
