#pragma once
#include "Arpeggiator.h"
#include "Chords.h"
#include "Effects.h"
#include "Engine.h"
#include "Parameters.h"
#include <juce_audio_utils/juce_audio_utils.h>
namespace logisth {
struct ThemeDraft;
}
class LogisthProcessor : public juce::AudioProcessor {
  public:
    LogisthProcessor();
    // GUI-only draft, deliberately excluded from sound presets and DAW audio state.
    std::shared_ptr<logisth::ThemeDraft> appearanceDraft;
    ~LogisthProcessor() override;
    juce::Result loadSource(int, const juce::File &, int frameSize = 0);
    void loadSourceAsync(int, const juce::File &, int frameSize = 0,
                         bool switchClassicToSampler = false);
    void clearSource(int);
    std::shared_ptr<const logisth::SourceData> sourceData(int) const;
    juce::String sourceStatus() const;
    std::shared_ptr<const logisth::ChordMap> chordMap() const;
    void setChord(int, int);
    void setChordNote(int, int, int);
    void learnChord(int);
    void finishLearn();
    int learning() const {
        return learnTarget.load();
    }
    void storePattern(int);
    void recallPattern(int);
    void randomisePattern();
    void serviceAssets();
    std::atomic<int> arpPlayhead{-1};
    logisth::ScopeBuffer scope;
    std::array<logisth::PlaybackTelemetry, 4> playback;
    void copyPattern(int, int);
    void setPatternName(int, const juce::String &);
    juce::String patternName(int) const;
    juce::File presetDirectory() const;
    void setPresetDirectory(const juce::File &);
    juce::Result savePresetFile(const juce::File &);
    juce::Result loadPresetFile(const juce::File &);
    void savePresetAsync(const juce::File &);
    void loadPresetAsync(const juce::File &);
    juce::String currentPresetName() const;
    juce::String presetComment() const;
    void setPresetComment(const juce::String &);
    std::atomic<int> commentRevision{0};
    juce::File currentPresetFile() const;
    int activePattern() const {
        return currentPattern.load();
    }
    std::atomic<int> presetRevision{0};
    bool applyState(const void *, int);
    const juce::String getName() const override {
        return "logisth";
    }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout &l) const override {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
               l.getMainInputChannelSet().isDisabled();
    }
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override {
        return true;
    }
    bool acceptsMidi() const override {
        return true;
    }
    bool producesMidi() const override {
        return true;
    }
    bool isMidiEffect() const override {
        return false;
    }
    double getTailLengthSeconds() const override {
        return 60;
    }
    int getNumPrograms() override {
        return 1;
    }
    int getCurrentProgram() override {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {
        return "Default";
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    void loadPreset(int);
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;
    void setUiWheel(bool pitch, float value) {
        if (pitch) {
            pitchPosition = value;
            uiChanges.fetch_or(1);
        } else {
            wheelPosition = value;
            uiChanges.fetch_or(2);
        }
    }
    std::atomic<float> pitchPosition{0}, wheelPosition{0};
    std::atomic<float> peak{0};
    std::array<std::atomic<float>, 2> channelPeaks{};
    std::atomic<int> voices{0};

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(logisth::Param id) const {
        return raw[size_t(id)]->load();
    }
    logisth::Params snapshot() const;
    logisth::EffectsParams effectsSnapshot() const;
    logisth::ArpParams arpSnapshot() const;
    std::array<std::atomic<float> *, logisth::parameterCount> raw{};
    bool publishSource(int, std::shared_ptr<const logisth::SourceData>);
    void publishChords(std::shared_ptr<const logisth::ChordMap>);
    void saveAssets(juce::ValueTree &);
    bool restoreAssets(const juce::ValueTree &);
    mutable juce::CriticalSection assetLock;
    std::array<std::shared_ptr<const logisth::SourceData>, 4> sourceOwners;
    std::array<std::atomic<const logisth::SourceData *>, 4> sourcePointers{};
    std::vector<std::shared_ptr<const logisth::SourceData>> retiredSources;
    std::shared_ptr<const logisth::ChordMap> chordOwner;
    std::vector<std::shared_ptr<const logisth::ChordMap>> retiredChords;
    std::atomic<const logisth::ChordMap *> chordPointer{nullptr};
    std::atomic<int> assetReaders{0}, learnTarget{-1};
    std::array<std::atomic<uint32_t>, 4> learned{};
    juce::String assetStatus;
    std::array<juce::ValueTree, 12> patterns;
    juce::File userPresetFolder, activePresetFile;
    juce::String presetName, comment;
    std::atomic<int> currentPattern{0};
    juce::ThreadPool importWorkers{1};
    logisth::ChordMapper chordMapper;
    bool chordsWereEnabled = false, midiWasEnabled = false;
    juce::MidiBuffer generatedMidi;
    logisth::Engine engine;
    logisth::Effects effects;
    logisth::Arpeggiator arp;
    std::array<float, 512> driveMod{}, phaserMod{}, partScopeA{}, partScopeB{};
    std::atomic<int> uiChanges{0};
    float masterSmooth = 0;
    double bpm = 120;
    bool wasPlaying = false, clockAvailable = false;
    double hostPpq = 0;
    bool hostPpqValid = false;
    juce::int64 expectedSample = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogisthProcessor)
};
