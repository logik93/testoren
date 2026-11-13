#include "PluginProcessor.h"
using namespace juce;
using namespace logisth;
File LogisthProcessor::presetDirectory() const {
    const ScopedLock lock(assetLock);
    return userPresetFolder == File{} ? File::getSpecialLocation(File::userApplicationDataDirectory)
                                            .getChildFile("logisth/Presets")
                                      : userPresetFolder;
}
void LogisthProcessor::setPresetDirectory(const File &folder) {
    const ScopedLock lock(assetLock);
    userPresetFolder = folder;
    ++presetRevision;
}
File LogisthProcessor::currentPresetFile() const {
    const ScopedLock lock(assetLock);
    return activePresetFile;
}
String LogisthProcessor::currentPresetName() const {
    const ScopedLock lock(assetLock);
    return presetName;
}
Result LogisthProcessor::savePresetFile(const File &file) try {
    if (!file.hasFileExtension("logisth"))
        return Result::fail("Use the .logisth preset extension");
    auto tree = state.copyState();
    tree.setProperty("schema", 4, nullptr);
    tree.setProperty("presetName", file.getFileNameWithoutExtension(), nullptr);
    saveAssets(tree);
    MemoryBlock data;
    copyXmlToBinary(*tree.createXml(), data);
    if (data.getSize() > stateMemoryLimit)
        return Result::fail("Preset exceeds the 1 GiB state limit");
    auto folder = file.getParentDirectory().createDirectory();
    if (folder.failed())
        return folder;
    TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithData(data.getData(), data.getSize()) ||
        !temporary.overwriteTargetFileWithTemporary())
        return Result::fail("Preset could not be saved");
    {
        const ScopedLock lock(assetLock);
        presetName = file.getFileNameWithoutExtension();
        activePresetFile = file;
    }
    ++presetRevision;
    return Result::ok();
} catch (const std::bad_alloc &) {
    return Result::fail("Not enough memory to save this preset.");
}
Result LogisthProcessor::loadPresetFile(const File &file) try {
    if (!file.existsAsFile() || file.getSize() <= 0 || file.getSize() > stateMemoryLimit)
        return Result::fail("Preset is missing or too large");
    MemoryBlock data;
    if (!file.loadFileAsData(data) || !applyState(data.getData(), int(data.getSize())))
        return Result::fail("Invalid or unsupported logisth preset; the current patch was kept");
    {
        const ScopedLock lock(assetLock);
        presetName = file.getFileNameWithoutExtension();
        activePresetFile = file;
    }
    ++presetRevision;
    return Result::ok();
} catch (const std::bad_alloc &) {
    return Result::fail("Not enough memory to load this preset.");
}
void LogisthProcessor::savePresetAsync(const File &file) {
    {
        const ScopedLock lock(assetLock);
        assetStatus = "Saving preset...";
    }
    importWorkers.addJob([this, file] {
        auto result = savePresetFile(file);
        const ScopedLock lock(assetLock);
        assetStatus = result.wasOk() ? "Saved " + file.getFileName() : result.getErrorMessage();
    });
}
void LogisthProcessor::loadPresetAsync(const File &file) {
    {
        const ScopedLock lock(assetLock);
        assetStatus = "Loading preset...";
    }
    importWorkers.addJob([this, file] {
        auto result = loadPresetFile(file);
        const ScopedLock lock(assetLock);
        assetStatus = result.wasOk() ? "Loaded " + file.getFileName() : result.getErrorMessage();
    });
}
void LogisthProcessor::setPatternName(int slot, const String &name) {
    if (slot < 0 || slot >= 12)
        return;
    const ScopedLock lock(assetLock);
    if (!patterns[size_t(slot)].isValid())
        patterns[size_t(slot)] = ValueTree("PATTERN");
    patterns[size_t(slot)].setProperty("slot", slot, nullptr);
    patterns[size_t(slot)].setProperty("name", name.substring(0, 64), nullptr);
}
String LogisthProcessor::patternName(int slot) const {
    if (slot < 0 || slot >= 12)
        return {};
    const ScopedLock lock(assetLock);
    return patterns[size_t(slot)]
        .getProperty("name", String::charToString(juce_wchar('A' + slot)))
        .toString();
}
void LogisthProcessor::copyPattern(int from, int to) {
    if (from < 0 || from >= 12 || to < 0 || to >= 12)
        return;
    if (from == currentPattern)
        storePattern(from);
    {
        const ScopedLock lock(assetLock);
        patterns[size_t(to)] = patterns[size_t(from)].createCopy();
        if (patterns[size_t(to)].isValid())
            patterns[size_t(to)].setProperty("slot", to, nullptr);
    }
    if (to == currentPattern) {
        currentPattern = -1;
        recallPattern(to);
    }
}
